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

/* $Header: minmax.c 2.0 86/01/28 $
 *
 * minmax: find the 4 edges of the glyph within a window.
 */

#include "bit.h"

minmax(g, nrow, ncol, rmin, cmin, rmax, cmax)
bitmat g;
int nrow, ncol;
int *rmin, *cmin, *rmax, *cmax;
{
	register int i, j;
	register int r1, r2, c1, c2;
	int ar1, ar2, ac1, ac2;

	aminmax(g, nrow, ncol, &ar1, &ac1, &ar2, &ac2);
#ifdef TRACE
	if (trace)
		fprintf(trace, "aminmax returns %d, %d, %d, %d\n", ar1, ac1, ar2, ac2);
#endif
	r1 = nrow; c1 = ncol; r2 = c2 = 0;
	for (i=ar1; i<=ar2; i++)
		for (j=ac1; j<=ac2; j++)
			if (mat(g, nrow, ncol, i, j, 8)) {
				r1 = min(r1, i);
				r2 = max(r2, i);
				c1 = min(c1, j);
				c2 = max(c2, j);
			}
	if (r2 < r1) {
		/* empty glyph! */
		r1 = c1 = r2 = c2 = 1;
	}
	*rmin = r1; *rmax = r2;
	*cmin = c1; *cmax = c2;
}