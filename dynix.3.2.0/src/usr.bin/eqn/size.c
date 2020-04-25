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
static char rcsid[] = "$Header: size.c 2.0 86/01/28 $";
#endif

# include "e.h"

setsize(p)	/* set size as found in p */
char *p;
{
	if (*p == '+')
		ps += atoi(p+1);
	else if (*p == '-')
		ps -= atoi(p+1);
	else
		ps = atoi(p);
	if(dbg)printf(".\tsetsize %s; ps = %d\n", p, ps);
}

size(p1, p2) int p1, p2; {
		/* old size in p1, new in ps */
	int effps, effp1;

	yyval = p2;
	if(dbg)printf(".\tb:sb: S%d <- \\s%d S%d \\s%d; b=%d, h=%d\n", 
		yyval, ps, p2, p1, ebase[yyval], eht[yyval]);
	effps = EFFPS(ps);
	effp1 = EFFPS(p1);
	printf(".ds %d \\s%d\\*(%d\\s%d\n", 
		yyval, effps, p2, effp1);
	ps = p1;
}

globsize() {
	char temp[20];

	getstr(temp, 20);
	if (temp[0] == '+')
		gsize += atoi(temp+1);
	else if (temp[0] == '-')
		gsize -= atoi(temp+1);
	else
		gsize = atoi(temp);
	yyval = eqnreg = 0;
	setps(gsize);
	ps = gsize;
	if (gsize >= 12)	/* sub and sup size change */
		deltaps = gsize / 4;
	else
		deltaps = gsize / 3;
}
