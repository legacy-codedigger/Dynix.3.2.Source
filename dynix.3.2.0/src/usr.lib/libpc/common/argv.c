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

/* $Header: argv.c 1.1 89/03/12 $ */
#include "h00vars.h"

ARGV(subscript, var, size)
long subscript;	/* subscript into argv */
register char *var;		/* pointer to pascal char array */
register long size;		/* sizeof(var) */
{
	register char	*cp;

	if ((unsigned)subscript >= _argc) {
		ERROR("Argument to argv of %D is out of range\n", subscript);
		/*NOTREACHED*/
		return;
	}
	cp = _argv[subscript];
	do	{
		*var++ = *cp++;
	} while (--size && *cp);
	while (size--) {
		*var++ = ' ';
	}
}
