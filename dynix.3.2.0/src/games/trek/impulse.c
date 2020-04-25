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

/* $Header: impulse.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)impulse.c	4.3	(Berkeley)	5/27/83";
#endif not lint

# include	"trek.h"

/**
 **	move under impulse power
 **/

impulse()
{
	int			course;
	register int		power;
	double			dist, time;
	register int		percent;
	extern double		move();

	if (Ship.cond == DOCKED)
		return (printf("Scotty: Sorry captain, but we are still docked.\n"));
	if (damaged(IMPULSE))
		return (out(IMPULSE));
	if (getcodi(&course, &dist))
		return;
	power = 20 + 100 * dist;
	percent = 100 * power / Ship.energy + 0.5;
	if (percent >= 85)
	{
		printf("Scotty: That would consume %d%% of our remaining energy.\n",
			percent);
		if (!getynpar("Are you sure that is wise"))
			return;
		printf("Aye aye, sir\n");
	}
	time = dist / 0.095;
	percent = 100 * time / Now.time + 0.5;
	if (percent >= 85)
	{
		printf("Spock: That would take %d%% of our remaining time.\n",
			percent);
		if (!getynpar("Are you sure that is wise"))
			return;
		printf("(He's finally gone mad)\n");
	}
	Move.time = move(0, course, time, 0.095);
	Ship.energy -= 20 + 100 * Move.time * 0.095;
}
