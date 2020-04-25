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

#ifndef lint
static char rcsid[] = "$Header: n10.c 2.0 86/01/28 $";
#endif

#include "tdef.h"
#include <sgtty.h>
#include <stdio.h>
extern
#include "d.h"
extern
#include "v.h"
extern
#include "tw.h"
/*
nroff10.c

Device interfaces
*/

extern int lss;
extern char obuf[];
extern char *obufp;
extern int xfont;
extern int esc;
extern int lead;
extern int oline[];
extern int *olinep;
extern int ulfont;
extern int esct;
extern int sps;
extern int ics;
extern int ttysave;
extern struct sgttyb ttys;
extern char termtab[];
extern int ptid;
extern int waitf;
extern int pipeflg;
extern int eqflg;
extern int hflg;
extern int tabtab[];
extern int ascii;
extern int xxx;
int dtab;
int bdmode;
int plotmode;

ptinit(){
	register i, j;
	register char **p;
	char *q, *tp;
	int x[8];
	char **ip;
	FILE *f;
	char lbuf[BUFSIZ];
#define  TERMBUFSIZE	(8*1024)
	static char termbuff[TERMBUFSIZE];
	extern char *setbrk(), *strcpy(), *sscan();

	strcat(termtab, ".ascii");
	/*
	 * Parse ascii terminal driver files
	 */
	if((f=fopen(termtab, "r")) == NULL)  {
		prstr("Cannot open ");
		prstr(termtab);
		prstr("\n");
		exit(-1);
	}

	i = 0;
	ip = (char **)&t.bset;
	tp = termbuff;
	while (fgets(lbuf, (sizeof lbuf)-1, f) != NULL) {
		if (i < 9)  {
			/*
			 * Careful here because nroff has its own
			 * atoi() routine! (and it works differently)
			 */
			ip[i] = (char *)atol(lbuf);
		} else	if (i >= 9 && i < (9+14+256-32)) {
			q = sscan(lbuf);
			if (q == NULL || *q == NULL) {
				ip[i] = "\000";
			} else {
				ip[i] = strcpy(tp, q);
				tp += strlen(q)+1;
				if (tp >= &termbuff[TERMBUFSIZE]) {
				    prstr("Terminal driver table overflow\n");
				    exit(-1);
				}
			}
		} else {
			fprintf(stderr, "%s: bad line, %s\n", termtab, lbuf);
			exit(-1);
		}
		i++;
	}
	fclose(f);
	t.zzz = 0;	/* sigh.. */

#ifdef	notdef
		printf("%d	/* bset */\n", t.bset);
		printf("%d	/* breset */\n", t.breset);
		printf("%d	/* Hor */\n", t.Hor);
		printf("%d	/* Vert */\n", t.Vert);
		printf("%d	/* Newline */\n", t.Newline);
		printf("%d	/* Char */\n", t.Char);
		printf("%d	/* Em */\n", t.Em);
		printf("%d	/* Halfline */\n", t.Halfline);
		printf("%d	/* Adj */\n", t.Adj);

		printf("\"%s\"	/*twinit*/\n", t.twinit);
		printf("\"%s\"	/*twrest*/\n", t.twrest);
		printf("\"%s\"	/*twnl*/\n", t.twnl);
		printf("\"%s\"	/*hlr*/\n", t.hlr);
		printf("\"%s\"	/*hlf*/\n", t.hlf);
		printf("\"%s\"	/*flr*/\n", t.flr);
		printf("\"%s\"	/*bdon*/\n", t.bdon);
		printf("\"%s\"	/*bdoff*/\n", t.bdoff);
		printf("\"%s\"	/*ploton*/\n", t.ploton);
		printf("\"%s\"	/*plotoff*/\n", t.plotoff);
		printf("\"%s\"	/*up*/\n", t.up);
		printf("\"%s\"	/*down*/\n", t.down);
		printf("\"%s\"	/*right*/\n", t.right);
		printf("\"%s\"	/*left*/\n", t.left);

		for (i=0; i < (256-32); i++) {
			printf("\"%s\"	/*codetab[%d]*/\n", t.codetab[i], i);
		}
		printf("%d	/*zzz*/\n", t.zzz);
	exit(0);
#endif	notdef

	sps = EM;
	ics = EM*2;
	dtab = 8 * t.Em;
	for(i=0; i<16; i++)tabtab[i] = dtab * (i+1);
	if(eqflg)t.Adj = t.Hor;
}


/*
 * Sscan:  Parses strings according to page 181 of K and R.
 *
 * Accepted strings start with ``"'' and end with ``"''.
 * Allowed escapes are:
 *	newline		NL	\n
 *	horizontal tab	HT	\t
 *	backspace	BS	\b
 *	carriage return	CR	\r
 *	form feed	FF	\f
 *	backslash	\	\\
 *	single quote	'	\'
 *	double quote	"	\"
 *	bit pattern	ddd	\ddd 	(0, 1, or 2 OCTAL digits)
 *	\ IGNORED	\*	where * is not one of the above (or newline)
 *	
 */

#include <ctype.h>
#define	DQUOTE	'"'		/* Double Quote */

char *
sscan(s)
	char *s;
{
	register char *str, *output, *dp;
	register n, c;

	output = str = s;
	while (str && *str && *str != DQUOTE)
		++str;
	if (str == 0 || *str != DQUOTE) {
		fprintf(stderr, "missing first double quote in term file\n");
		exit (-1);
	}
	++str;
	while (*str && (c = *str++) != DQUOTE) {
		switch (c) {
		default:
			*output++ = c;
			continue;

		case '\\':	/* seen backslash */
			dp = "n\nt\tb\br\rf\f\\\\";
			c = *str++;
	nextc:
			if (*dp++ == c) {
				*output++ = *dp;
				continue;
			}
			++dp;
			if (*dp)
				goto nextc;

			if (isdigit(c)) {
				c -= '0', n = 3;
				while (--n && isdigit(*str)) {
					c <<= 3, c |= *str++ - '0';
				}
			}
			*output++ = c;
			continue;
		}
	}
	if (c == DQUOTE) {
		*output = NULL;	/* null terminate string */
		return(s);
	}
	fprintf(stderr, "missing second double quote in term file\n");
	exit (-1);
}

twdone(){
	obufp = obuf;
	oputs(t.twrest);
	flusho();
	if(pipeflg){
		close(ptid);
		wait(&waitf);
	}
	if(ttysave != -1) {
		ttys.sg_flags = ttysave;
		stty(1, &ttys);
	}
}
ptout(i)
int i;
{
	*olinep++ = i;
	if(olinep >= &oline[LNSIZE])olinep--;
	if((i&CMASK) != '\n')return;
	olinep--;
	lead += dip->blss + lss - t.Newline;
	dip->blss = 0;
	esct = esc = 0;
	if(olinep>oline){
		move();
		ptout1();
		oputs(t.twnl);
	}else{
		lead += t.Newline;
		move();
	}
	lead += dip->alss;
	dip->alss = 0;
	olinep = oline;
}
ptout1()
{
	register i, k;
	register char *codep;
	extern char *plot();
	int *q, w, j, phyw;

	for(q=oline; q<olinep; q++){
	if((i = *q) & MOT){
		j = i & ~MOTV;
		if(i & NMOT)j = -j;
		if(i & VMOT)lead += j;
		else esc += j;
		continue;
	}
	if((k = (i & CMASK)) <= 040){
		switch(k){
			case ' ': /*space*/
				esc += t.Char;
				break;
		}
		continue;
	}
	codep = t.codetab[k-32];
	w = t.Char * (*codep++ & 0177);
	phyw = w;
	if(i&ZBIT)w = 0;
	if(*codep && (esc || lead))move();
	esct += w;
	if(i&074000)xfont = (i>>9) & 03;
	if(*t.bdon & 0377){
		if(!bdmode && (xfont == 2)){
			oputs(t.bdon);
			bdmode++;
		}
		if(bdmode && (xfont != 2)){
			oputs(t.bdoff);
			bdmode = 0;
		}
	}
	if(xfont == ulfont){
		for(k=w/t.Char;k>0;k--)oput('_');
		for(k=w/t.Char;k>0;k--)oput('\b');
	}
	while(*codep != 0){
		if(*codep & 0200){
			codep = plot(codep);
			oputs(t.plotoff);
			oput(' ');
		}else{
			if(plotmode)oputs(t.plotoff);
			*obufp++ = *codep++;
			if(obufp == (obuf + OBUFSZ + ascii - 1))flusho();
/*			oput(*codep++);*/
		}
	}
	if(!w)for(k=phyw/t.Char;k>0;k--)oput('\b');
	}
}
char *plot(x)
char *x;
{
	register int i;
	register char *j, *k;

	if(!plotmode)oputs(t.ploton);
	k = x;
	if((*k & 0377) == 0200)k++;
	for(; *k; k++){
		if(*k & 0200){
			if(*k & 0100){
				if(*k & 040)j = t.up; else j = t.down;
			}else{
				if(*k & 040)j = t.left; else j = t.right;
			}
			if(!(i = *k & 037))return(++k);
			while(i--)oputs(j);
		}else oput(*k);
	}
	return(k);
}
move(){
	register k;
	register char *i, *j;
	char *p, *q;
	int iesct, dt;

	iesct = esct;
	if(esct += esc)i = "\0"; else i = "\n\0";
	j = t.hlf;
	p = t.right;
	q = t.down;
	if(lead){
		if(lead < 0){
			lead = -lead;
			i = t.flr;
		/*	if(!esct)i = t.flr; else i = "\0";*/
			j = t.hlr;
			q = t.up;
		}
		if(*i & 0377){
			k = lead/t.Newline;
			lead = lead%t.Newline;
			while(k--)oputs(i);
		}
		if(*j & 0377){
			k = lead/t.Halfline;
			lead = lead%t.Halfline;
			while(k--)oputs(j);
		}
		else { /* no half-line forward, not at line begining */
			k = lead/t.Newline;
			lead = lead%t.Newline;
			if (k>0) esc=esct;
			i = "\n";
			while (k--) oputs(i);
		}
	}
	if(esc){
		if(esc < 0){
			esc = -esc;
			j = "\b";
			p = t.left;
		}else{
			j = " ";
			if(hflg)while((dt = dtab - (iesct%dtab)) <= esc){
				if(dt%t.Em)break;
				oput(TAB);
				esc -= dt;
				iesct += dt;
			}
		}
		k = esc/t.Em;
		esc = esc%t.Em;
		while(k--)oputs(j);
	}
	if((*t.ploton & 0377) && (esc || lead)){
		if(!plotmode)oputs(t.ploton);
		esc /= t.Hor;
		lead /= t.Vert;
		while(esc--)oputs(p);
		while(lead--)oputs(q);
		oputs(t.plotoff);
	}
	esc = lead = 0;
}
ptlead(){move();}
dostop(){
	char junk;

	flusho();
	read(2,&junk,1);
}
