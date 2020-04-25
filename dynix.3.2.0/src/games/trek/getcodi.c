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

/* $Header: getcodi.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)getcodi.c	4.2	(Berkeley)	5/27/83";
#endif not lint

# include	"getpar.h"

/*
**  get course and distance
**
**	The user is asked for a course and distance.  This is used by
**	move, impulse, and some of the computer functions.
**
**	The return value is zero for success, one for an invalid input
**	(meaning to drop the request).
*/

getcodi(co, di)
int	*co;
double	*di;
{

	*co = getintpar("Course");

	/* course must be in the interval [0, 360] */
	if (*co < 0 || *co > 360)
		return (1);
	*di = getfltpar("Distance");

	/* distance must be in the interval [0, 15] */
	if (*di <= 0.0 || *di > 15.0)
		return (1);

	/* good return */
	return (0);
}
