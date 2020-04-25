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

/* $Header: bitcopy.c 2.0 86/01/28 $
 *
 * Copy from msrc to mdest.
 * This is done as it is because it would be much slower to do it
 * a bit at a time.
 */

#include "bit.h"

bitcopy(mdest, msrc, rows, cols)
bitmat mdest, msrc;
int rows, cols;
{
	register int size = ((cols + 7) >> 3) * rows;
	register char *p, *q;

	for (p = &mdest[size], q = &msrc[size]; p>=mdest; )
		*--p = *--q;
}
