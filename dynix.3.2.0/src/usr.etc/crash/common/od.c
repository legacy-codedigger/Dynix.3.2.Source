/* $Header: od.c 2.6 1991/07/26 18:33:54 $ */

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.

#ifndef lint
static char rcsid[] = "$Header: od.c 2.6 1991/07/26 18:33:54 $";
#endif

/*
 * $Log: od.c,v $
 *
 *
 *
 */

#include "crash.h"
#include <a.out.h>

int readv(), readp();
static struct sdb od_sdb;
int (*Read)() = readp;

Dv()
{
	Od( readv );
}

Dp()
{
	Od( readp );
}

Od(RD)
int  (*RD)();
{
	char *arg;
	int addr, count;

	Read=RD;
	if((arg = token()) == NULL) {
		printf("symbol expected");
		return;
	}
	addr = atoi(arg);
	if ( err_atoi ) {
Oderr:
		printf("'%s', %s?\n", arg, err_atoi);
		tok = 0;
		return;
	}
	od_sdb = dot.v_sdb;
	if((arg = token()) == NULL) {
		count = 1;
#ifdef BSD
		if (desc(&dot.v_sdb.sdb_desc) == PCCTM_PTR)
#else
		if (desc(&dot.v_sdb.sdb_desc) == DT_PTR)
#endif
			arg = "X";
		else
			switch(dot.v_sdb.sdb_size) {
		  	    case 1: arg = "b"; break;
		  	    case 2: arg = "x"; break;
		  	    default: 
		  	    case 4: arg = "X"; break;
			}
	} else {
		count = atoi(arg);
		if ( err_atoi )
			goto Oderr;
		if((arg = token()) == NULL)
			arg = "X";
	}
	prod(addr, count, arg, RD);
}

prod(addr, units, style, RD)
	unsigned	addr;
	int	units;
	char	*style;
	int (*RD)();
{
	register  int  i, j, col;
	char lbuf [ 128 ];
	register  struct  prmode  *pp;
	int	word;
	unsigned short	sword;
	char	ch;
	extern	struct	prmode	prm[];
	char	*type;
#ifdef BSD
	struct	symbol	*s;
	extern	struct symbol *lookup();
#else
	struct	sdb	*s;
#endif

	for(pp = prm; pp->pr_sw != 0; pp++) {
		if(strcmp(pp->pr_name, style) == 0)
			break;
	}
	switch(pp->pr_sw) {
	case STRUCT:
		if ((tok != 0) && ((type = token()) == NULL)) {
#ifdef BSD
			s =  od_sdb.sdb_sym;
#else
			s = &od_sdb;
#endif
			for(i = 0; i < units; i++) {
				if (pr_symbol(addr, units, s, RD) == 0) {
					error("no such structure '%s'\n", type);
					return;
				}
				addr += od_sdb.sdb_size;
			}
			return;
		} else {
			style = type;
			/* fall through to... */
		}

	case NULL:
		if (style[1]) {
#ifdef BSD
			if ((s = lookup(style)) == 0) {
				error("%s is not a known structure\n", style);
				return;
			} 
			j = size(s);
#else
			if (!search_stag(style))
				error("%s is not a structure\n", style);
			od_sdb = sdbinfo;
			s = &od_sdb;
			j = od_sdb.sdb_size;
#endif
			for(i = 0; i < units; i++) {
				if (pr_symbol(addr, units, s, RD) == 0)
					return;
				addr += j;
			}
			return;
		}
		/* fall through to...  */
	default:
		printf("invalid mode '%s'\n", style);
		return;

	case OCT2:
	case DEC2:
	case HEX2:
		for(i = 0; i < units; i++) {
			if(i % 8 == 0) {
				if(i != 0)
					printf("\n");
				printf("(2)%06#x: ",  addr);
			}
			if(RD(addr, &sword, sizeof sword) != sizeof sword) {
				printf("  read error\n");
				break;
			}
			addr += sizeof sword;
			printf(pp->pr_sw == OCT2 ? " %7o" :
				pp->pr_sw == HEX2 ? "  %4x" : "  %5u", sword);
		}
		break;

	case OCT4:
	case DEC4:
	case HEX4:
		for(i = 0; i < units; i++) {
			if(i % 4 == 0) {
				if(i != 0)
					printf("\n");
				printf("(4)%06#x: ",  addr);
			}
			if(RD(addr, &word, sizeof word) != sizeof word) {
				printf("  read error");
				break;
			}
			addr += sizeof word;
			printf(pp->pr_sw == OCT4 ? " %12lo" :
				pp->pr_sw == HEX4 ? "  %8x" : "  %10u", word);
		}
		break;

	case CHAR:
	case BYTE:
		for(i = 0; i < units; i++) {
			if(i % (pp->pr_sw == CHAR ? 16 : 8) == 0) {
				if(i != 0)
					printf("\n");
				printf("(1)%06#x: ",  addr);
			}
			if(RD(addr, &ch, sizeof (char)) != sizeof (char)) {
				printf("  read printf");
				break;
			}
			addr += sizeof (char);
			if(pp->pr_sw == CHAR)
				printf("%c", ch);
			else
				printf(" %4x", ch & 0377);
		}
		break;
	case STRING:
		col = 0;
		for (i = 0; i < units; i++) {
			if (i) printf("\n");
			sprintf(lbuf, "%06x: ", addr);
			col = strlen(lbuf);
			printf("%s",  lbuf);
			for (j=0; ; j++) {
				if(RD(addr, &ch, sizeof (char)) != sizeof (char)) 
					break;
				addr += sizeof (char);
				strcpy(lbuf, nice_char(ch));
				col += strlen(lbuf);
				printf("%s", lbuf);
				if (ch == '\0')
					break;
				if (col >= 75) {
					printf("\\\n");
					sprintf(lbuf, "%06x: ", addr);
					col += strlen(lbuf);
					printf("%s",  lbuf);
					col = j = 0;
				}
			}
		}
		break;
	}
	printf("\n");
}
