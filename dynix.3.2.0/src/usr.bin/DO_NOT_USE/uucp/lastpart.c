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

#ifndef	lint
static char rcsid[] = "$Header: lastpart.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)lastpart.c	5.4 (Berkeley) 6/20/85";
#endif

#include "uucp.h"

/*LINTLIBRARY*/

/*
 *	find last part of file name
 *
 *	return - pointer to last part
 */

char *
lastpart(file)
register char *file;
{
	register char *c;

	c = rindex(file, '/');
	if (c++)
		return c;
	else
		return file;
}
