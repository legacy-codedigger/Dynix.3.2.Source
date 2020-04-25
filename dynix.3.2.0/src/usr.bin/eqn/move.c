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
static char rcsid[] = "$Header: move.c 2.0 86/01/28 $";
#endif

# include "e.h"
# include "e.def"

move(dir, amt, p) int dir, amt, p; {
	int a;

	yyval = p;
#ifndef NEQN
	a = VERT( (EFFPS(ps) * 6 * amt) / 100);
#else NEQN
	a = VERT( (amt+49)/50 );	/* nearest number of half-lines */
#endif NEQN
	printf(".ds %d ", yyval);
	if( dir == FWD || dir == BACK )	/* fwd, back */
		printf("\\h'%s%du'\\*(%d\n", (dir==BACK) ? "-" : "", a, p);
	else if (dir == UP)
		printf("\\v'-%du'\\*(%d\\v'%du'\n", a, p, a);
	else if (dir == DOWN)
		printf("\\v'%du'\\*(%d\\v'-%du'\n", a, p, a);
	if(dbg)printf(".\tmove %d dir %d amt %d; h=%d b=%d\n", 
		p, dir, a, eht[yyval], ebase[yyval]);
}
