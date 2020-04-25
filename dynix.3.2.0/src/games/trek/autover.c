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

/* $Header: autover.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)autover.c	4.2	(Berkeley)	5/27/83";
#endif not lint

# include	"trek.h"

/*
**  Automatic Override
**
**	If we should be so unlucky as to be caught in a quadrant
**	with a supernova in it, this routine is called.  It is
**	called from checkcond().
**
**	It sets you to a random warp (guaranteed to be over 6.0)
**	and starts sending you off "somewhere" (whereever that is).
**
**	Please note that it is VERY important that you reset your
**	warp speed after the automatic override is called.  The new
**	warp factor does not stay in effect for just this routine.
**
**	This routine will never try to send you more than sqrt(2)
**	quadrants, since that is all that is needed.
*/

autover()
{
	double			dist;
	register int		course;

	printf("RED ALERT:  The %s is in a supernova quadrant\n", Ship.shipname);
	printf("***  Emergency override attempts to hurl %s to safety\n", Ship.shipname);
	/* let's get our ass out of here */
	Ship.warp = 6.0 + 2.0 * franf();
	Ship.warp2 = Ship.warp * Ship.warp;
	Ship.warp3 = Ship.warp2 * Ship.warp;
	dist = 0.75 * Ship.energy / (Ship.warp3 * (Ship.shldup + 1));
	if (dist > 1.4142)
		dist = 1.4142;
	course = ranf(360);
	Etc.nkling = -1;
	Ship.cond = RED;
	warp(-1, course, dist);
	attack(0);
}
