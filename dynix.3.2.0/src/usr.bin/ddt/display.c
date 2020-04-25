/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */


/*
 * display.c: version 1.22 of 8/30/83
 * 
 */
# ifndef lint
static char *rcsid = "$Header: display.c 2.0 86/01/28 $";
# endif

/* display routines, does all fancy output */

/*
 * host defines
 */

#include <ctype.h>

/* 
 * target defines
 */

#include <a.out.h>
#include <signal.h>
#include <sys/param.h>
#include "fpu.h"
#include "main.h"
#include "parse.h"
#include "sym.h"
#include "display.h"

int	acontext = 0;	/* last size(b,w,d) of a displayed object */
int	firsthex;
int	ilen;
char	*ibuf = 0;
extern char *dis32000();

nprint(num,radix)
unsigned int num,radix;
{
	unsigned int i;
	char cout;
	if ((i = num/radix) != 0) nprint(i,radix);
	num %= radix;
	if (num >= 10) {
	    cout = 'a' + (num - 10);
	    if (firsthex) printf("0");
	}
	else cout = num + '0';
	printf("%c",cout);
	firsthex = FALSE;
}

snprint(num)
int	num;
{
	firsthex = TRUE;
	if ((num < 0) && ((tempmodes.outradix == 10) || (num > -100))) {
		printf("-");
		num =  -num;
	}
	nprint(num,tempmodes.outradix);
}

typeout(num,format)
int num;
char *format;
{
	int off;
	char *symatch;

	if ((*format != '*') && (*format != '\0'))
		printf("%c",*format);
	format++;
	lasttypeout = num;		/* save value for tab */
	if ((tempmodes.relative) && (num >= matchbase)) {
		symatch = lookbyval(num, &off);
		if ((symatch != NULL) && (off <= matchequal)) {
			printf("%s", symatch);
			if (off) {
				printf("+");
				snprint(off);
			}
		} else
			snprint(num);
	} else 
		snprint(num);
	if ((*format != '*') && (*format != '\0'))
		printf("%c",*format);
	format++;
	if ((*format != '*') && (*format != '\0'))
		printf("%c",*format);

}

autoi(addrat)
	int addrat;
{
	if (tempmodes.outputmode == INSTRUCT)
		return(TRUE);
	if (tempmodes.outputmode != NUMERIC)
		return(FALSE);
	if (tempmodes.automat) {
		if (addrat < ddtheader.a_text &&
		    addrat > (int) ((struct exec *) N_ADDRADJ(ddtheader))->a_brtoentry)
			return(TRUE);
	}
	return(FALSE);
}

display(address)
int address;
{
	char	achar;
	char	*s;
	int	contentat, delta, instat, instlen;
	union {
		int w;
		float f;
	} word;
	union {
		struct{
			int w1, w2;
		} ww;
		double d;
	} dbl;

	acontext = tempmodes.context;
	if ((address >= 0) && autoi(address)) {
		instat = address;
		if ((s = dis32000(instat, &instlen)) != (char *)0) {
			printf("\t%s", s);
			contentat = getmem(address);
			acontext = instlen;
			return(contentat);
		}
	}
	if (address < 0) {
		contentat = getreg(address);
	} else {
		contentat = getmem(address);
	}
	if (tempmodes.outputmode == ABSNUMERIC) {
		snprint(contentat);
	} else if (tempmodes.outputmode == STRING) {
		delta = 1;
		while (TRUE) {
			achar = getbyte(address+delta-1);
			if (achar == '\0') break;
			if ((achar >= ' ') && (achar <='~')) printf("%c",achar);
			else printf("/%x",achar);
			delta++;
		}
		acontext = delta;
		printf("\r\n");
	} else if (tempmodes.outputmode == CHARACTER) {
		acontext = 1;
		achar = contentat;
		if ((achar >= ' ') && (achar <= '~')) printf("%c",achar);
		else printf("/%x",achar);
	} else if (tempmodes.outputmode == FLOATING) {
		word.w = contentat;
		printf("%g", word.f);
	} else if (tempmodes.outputmode == DOUBLE) {
		dbl.ww.w1 = contentat;
		dbl.ww.w2 = (address < 0) ?
			getreg( address-1 ) :
			getmem( address+4 );	/* XXX WORDSIZE */
		printf("%.13g", dbl.d);
	} else typeout(contentat,"  ");
	return(contentat);
}

ibackup(addrat)
{
	int newaddr,off;
	char *symatch;
	int instlen = 0;

	if (autoi(addrat)) {
		newaddr = addrat - 1;
		symatch = lookbyval(newaddr, &off);
		if (symatch != NULL) {
		    newaddr -= off;
		    while ((newaddr + instlen) < addrat) {
			newaddr += instlen;
			if (dis32000(newaddr, &instlen) != (char *)0) {
			} else return(addrat);
		    }
		    return(newaddr);
		}
		

	} 
	return(addrat);
}

char *regstr(regnum)
int regnum;
{
char *preg = '\0';
	if (regnum >= 0) {
		printf("can't print this register- bug\r\n");
	} else {
		if (regnum > SPSR) {
			preg = reg2tab[0 - (regnum - SR0)].regname;
		} else if (regnum >= SMOD) {
			preg = reg3tab[0 - (regnum - SPSR)].regname;
		} else if (regnum == FSR) {
			preg = reg3tab[2].regname;
		} else if ((regnum >= F7) && (regnum <= F0)) {
			preg = reg2tab[11 + (F0 - regnum)].regname;
		} else {
		    printf("can't print this register- bug\r\n");
		}
	}
	return(preg);
}

tracestack(fpat, pcfrom, fplast, doargs)
	int fpat, pcfrom, fplast, doargs;
{
	int pcreturn, fpnext, off;
	char *symatch;
	int instreturn, args, arg, i = 0;

	symatch = lookbyval(pcfrom, &off);
	if (symatch == NULL)
		return;
	if (strcmp(symatch, "__sigcode") == 0 && fplast != 0) /* signal context */
		pcfrom = sigcontext(fplast, &symatch, &off);
	pcreturn = getdouble(fpat + 4);
	printf("%s", symatch);
	if (off) {
		printf("+");
		snprint(off);
	}
	printf("(");
	if (doargs) {
		instreturn = getdouble(pcreturn);
		if ((instreturn & 0xffff) == 0xa57c) {  /* ADJSPB instr */
			args = - ((instreturn >> 16) | 0xffffff00);
			while (args > 0) {
				arg = getdouble(fpat + 8 + i);
				typeout(arg,"***");
				i += 4;
				args -= 4;
				if (args > 0)
					printf(",");

			}
		}
	}
	printf(")\r\n");
	fpnext = getdouble(fpat);
	if (fpcheck(fpnext))
		tracestack(fpnext, pcreturn, fpat, doargs);
}

skipinst()
{
int instlen, thepc;
	thepc = getreg(SPC);
	if (dis32000(thepc, &instlen) != (char *)0) {
		setreg(SPC,(thepc + instlen));
	} else {
		printf("\r\ncan not skip this instruction\r\n");
	}
	showspot();
}

/*
 * called by the disassembler to get a symbolic name for an address.
 */
char *
addrname(addr)
	unsigned long addr;
{
	int offset;
	char *name;
	static char buf[256];
	int len;

	name = lookbyval(addr, &offset);
	if ((name != NULL) && (offset <= matchequal)) {
		lasttypeout = addr;		/* save value for tab */
		len = strlen(name) % (sizeof(buf) - (offset ? 13 : 0));
		strncpy(buf, name, len);
		if (offset) {
			buf[len] = '+';
			sprintf(&buf[len+1], "0x%x", offset); /* XXX snprint! */
		} else
			buf[len] = '\0';
		return(buf);
	} else
		return((char *) 0);
}


sigcontext(fp, name_p, offset_p)
	int fp;
	char **name_p;
	int *offset_p;
{
	unsigned int newpc;
	struct sigcontext *scp;

	printf("%s", *name_p);
	if (*offset_p) {
		printf("+");
		snprint(*offset_p);
	}
	printf("\r\n");
	scp = (struct sigcontext *)getdouble(fp + 16);
	newpc = getdouble((int) &scp->sc_pc);
	*name_p = lookbyval(newpc, offset_p);
#ifdef DEBUG
	printf("\r\nscp = 0x%x, newpc at 0x%x = 0x%x, name = 0x%x\r\n",
		scp, (int) &scp->sc_pc, newpc, *name_p);
#endif
	return(newpc);
}
