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

/* $Header: move.c 2.0 86/01/28 $
 *
 * move to (x, y).  Both the _pen and cursor are supposed to be moved.
 * We really just remember it for later, in case we move again.
 */

#include "2648.h"

move(x, y)
{
#ifdef TRACE
	if (trace)
		fprintf(trace, "\tmove(%d, %d), ", x, y);
#endif
	_supx = x;
	_supy = y;
}
