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

/* $Header: damaged.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)damaged.c	4.1	(Berkeley)	3/23/83";
#endif not lint

# include	"trek.h"

/*  DAMAGED -- check for device damaged
**
**	This is a boolean function which returns non-zero if the
**	specified device is broken.  It does this by checking the
**	event list for a "device fix" action on that device.
*/

damaged(dev)
int	dev;
{
	register int		d;
	register struct event	*e;
	register int		i;

	d = dev;

	for (i = 0; i < MAXEVENTS; i++)
	{
		e = &Event[i];
		if (e->evcode != E_FIXDV)
			continue;
		if (e->systemname == d)
			return (1);
	}

	/* device fix not in event list -- device must not be broken */
	return (0);
}
