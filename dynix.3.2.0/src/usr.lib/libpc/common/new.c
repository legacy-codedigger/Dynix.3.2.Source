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

/* $Header: new.c 1.1 89/03/12 $ */
#include "h00vars.h"

NEW(var, size)
char **var;	/* pointer to item being deallocated */
long size;	/* sizeof struct pointed to by var */
{
	extern char *malloc();
	register char *memblk;

	memblk = malloc((int)size);
	if (memblk == 0) {
		ERROR("Ran out of memory\n", 0);
		/*NOTREACHED*/
		return;
	}
	*var = memblk;
	if (memblk < _minptr)
		_minptr = memblk;
	if (memblk + size > _maxptr)
		_maxptr = memblk + size;
}
