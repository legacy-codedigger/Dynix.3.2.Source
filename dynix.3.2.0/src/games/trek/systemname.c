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

/* $Header: systemname.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)systemname.c	4.2	(Berkeley)	5/27/83";
#endif not lint

# include	"trek.h"

/*
**  RETRIEVE THE STARSYSTEM NAME
**
**	Very straightforward, this routine just gets the starsystem
**	name.  It returns zero if none in the specified quadrant
**	(which, by the way, is passed it).
**
**	This routine knows all about such things as distressed
**	starsystems, etc.
*/

char *systemname(q1)
struct quad	*q1;
{
	register struct quad	*q;
	register int		i;

	q = q1;

	i = q->qsystemname;
	if (i & Q_DISTRESSED)
		i = Event[i & Q_SYSTEM].systemname;

	i &= Q_SYSTEM;
	if (i == 0)
		return (0);
	return (Systemname[i]);
}
