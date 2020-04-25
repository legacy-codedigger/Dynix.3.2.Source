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

/* $Header: setmat.c 2.0 86/01/28 $
 *
 * setmat: set the value in m[r, c] to nval.
 */

#include "bit.h"

setmat(m, rows, cols, r, c, nval)
bitmat m;
int rows, cols, r, c, nval;
{
	register int offset, thisbit;

	if (r<0 || c<0 || r>=rows || c>=cols) {
#ifdef TRACE
		if (trace)
			fprintf(trace, "setmat range error: (%d, %d) <- %d in a (%d, %d) matrix %x\n", r, c, nval, rows, cols, m);
#endif

		return;
	}
	offset = r*((cols+7)>>3) + (c>>3);
	thisbit = 0x80 >> (c&7);
	if (nval)
		m[offset] |= thisbit;
	else
		m[offset] &= ~thisbit;
}
