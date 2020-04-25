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

/* $Header: dispose.c 1.1 89/03/12 $ */
#include	"h00vars.h"

DISPOSE(var, size)
register char **var;	/* pointer to pointer being deallocated */
register long size;	/* sizeof(object-to-deallocate) */
{
	if (*var == 0 || *var + size > _maxptr || *var < _minptr) {
		ERROR("Pointer value out of legal range\n", 0);
		/*NOTREACHED*/
	}
	free(*var);
	if (*var == _minptr)
		_minptr += size;
	if (*var + size == _maxptr)
		_maxptr -= size;
	*var = (char *)(0);
}
