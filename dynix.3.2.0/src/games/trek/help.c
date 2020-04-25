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

/* $Header: help.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)help.c	4.2	(Berkeley)	5/9/83";
#endif not lint

# include	"trek.h"

/*
**  call starbase for help
**
**	First, the closest starbase is selected.  If there is a
**	a starbase in your own quadrant, you are in good shape.
**	This distance takes quadrant distances into account only.
**
**	A magic number is computed based on the distance which acts
**	as the probability that you will be rematerialized.  You
**	get three tries.
**
**	When it is determined that you should be able to be remater-
**	ialized (i.e., when the probability thing mentioned above
**	comes up positive), you are put into that quadrant (anywhere).
**	Then, we try to see if there is a spot adjacent to the star-
**	base.  If not, you can't be rematerialized!!!  Otherwise,
**	it drops you there.  It only tries five times to find a spot
**	to drop you.  After that, it's your problem.
*/

char	*Cntvect[3] =
{"first", "second", "third"};

help()
{
	register int		i;
	double			dist, x;
	register int		dx, dy;
	int			j, l;

	/* check to see if calling for help is reasonable ... */
	if (Ship.cond == DOCKED)
		return (printf("Uhura: But Captain, we're already docked\n"));

	/* or possible */
	if (damaged(SSRADIO))
		return (out(SSRADIO));
	if (Now.bases <= 0)
		return (printf("Uhura: I'm not getting any response from starbase\n"));

	/* tut tut, there goes the score */
	Game.helps += 1;

	/* find the closest base */
	dist = d1e50;
	if (Quad[Ship.quadx][Ship.quady].bases <= 0)
	{
		/* there isn't one in this quadrant */
		for (i = 0; i < Now.bases; i++)
		{
			/* compute distance */
			dx = Now.base[i].x - Ship.quadx;
			dy = Now.base[i].y - Ship.quady;
			x = dx * dx + dy * dy;
			x = sqrt(x);

			/* see if better than what we already have */
			if (x < dist)
			{
				dist = x;
				l = i;
			}
		}

		/* go to that quadrant */
		Ship.quadx = Now.base[l].x;
		Ship.quady = Now.base[l].y;
		initquad(1);
	}
	else
	{
		dist = 0.0;
	}

	/* dematerialize the Enterprise */
	Sect[Ship.sectx][Ship.secty] = EMPTY;
	printf("Starbase in %d,%d responds\n", Ship.quadx, Ship.quady);

	/* this next thing acts as a probability that it will work */
	x = pow(1.0 - pow(0.94, dist), 0.3333333);

	/* attempt to rematerialize */
	for (i = 0; i < 3; i++)
	{
		sleep(2);
		printf("%s attempt to rematerialize ", Cntvect[i]);
		if (franf() > x)
		{
			/* ok, that's good.  let's see if we can set her down */
			for (j = 0; j < 5; j++)
			{
				dx = Etc.starbase.x + ranf(3) - 1;
				if (dx < 0 || dx >= NSECTS)
					continue;
				dy = Etc.starbase.y + ranf(3) - 1;
				if (dy < 0 || dy >= NSECTS || Sect[dx][dy] != EMPTY)
					continue;
				break;
			}
			if (j < 5)
			{
				/* found an empty spot */
				printf("succeeds\n");
				Ship.sectx = dx;
				Ship.secty = dy;
				Sect[dx][dy] = Ship.ship;
				dock();
				compkldist(0);
				return;
			}
			/* the starbase must have been surrounded */
		}
		printf("fails\n");
	}

	/* one, two, three strikes, you're out */
	lose(L_NOHELP);
}