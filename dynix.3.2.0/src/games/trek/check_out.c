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

/* $Header: check_out.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)check_out.c	4.1	(Berkeley)	3/23/83";
#endif not lint

# include	"trek.h"

/*
**  CHECK IF A DEVICE IS OUT
**
**	The indicated device is checked to see if it is disabled.  If
**	it is, an attempt is made to use the starbase device.  If both
**	of these fails, it returns non-zero (device is REALLY out),
**	otherwise it returns zero (I can get to it somehow).
**
**	It prints appropriate messages too.
*/

check_out(device)
int	device;
{
	register int	dev;

	dev = device;

	/* check for device ok */
	if (!damaged(dev))
		return (0);

	/* report it as being dead */
	out(dev);

	/* but if we are docked, we can go ahead anyhow */
	if (Ship.cond != DOCKED)
		return (1);
	printf("  Using starbase %s\n", Device[dev].name);
	return (0);
}
