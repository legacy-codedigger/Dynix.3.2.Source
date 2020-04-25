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
static char rcsid[] = "$Header: 0.extr.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"
struct lablist	{long labelt;  struct lablist *nxtlab; };
struct lablist *endlab, *errlab, *reflab, *linelabs, *newlab;

int nameline;			/* line number of function/subroutine st., if any */
int stflag;		/* determines whether at beginning or middle of block of straight line code */



int   nlabs, lswnum, swptr, flag,
	 counter, p1, p3, begline, endline, r1,r2, endcom;
long begchar, endchar, comchar;


char *pred, *inc, *prerw, *postrw, *exp, *stcode;

#define maxdo	20	/* max nesting of do loops */
long dostack[maxdo];		/* labels of do nodes */
int doloc[maxdo];		/* loc of do node */
int doptr;


struct list *FMTLST;		/* list of FMTVX's generated */
struct list *ENTLST;		/* list of STLNVX nodes corresponding to entry statements */
long rtnbeg;	/* number of chars up to beginning of current routine */
