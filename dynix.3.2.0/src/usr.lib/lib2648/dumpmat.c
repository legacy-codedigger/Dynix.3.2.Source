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

/* $Header: dumpmat.c 2.0 86/01/28 $ */

#include "bit.h"

#ifdef TRACE
/*
 * dumpmat: debugging dumpmat of a window or other bit matrix.
 * msg is a handy label, m is the matrix, rows, cols is the size of the matrix.
 */
dumpmat(msg, m, rows, cols)
char *msg;
bitmat m;
int rows, cols;
{
	register int r, c;
	int r1, r2, c1, c2;

	if (trace == NULL)
		return;
	fprintf(trace, "\ndumpmat %s, m=%x, rows=%d, cols=%d\n", msg, m, rows, cols);
	minmax(m, rows, cols, &r1, &c1, &r2, &c2);
	fprintf(trace, "r1=%d, r2=%d, c1=%d, c2=%d\n", r1, r2, c1, c2);
	for (r=r1; r<=r2; r++) {
		fprintf(trace, "%2d ", r);
		for (c=c1; c<=c2; c++)
			fprintf(trace, "%c", mat(m, rows, cols, r, c, 5) ? 'X' : '.');
		fprintf(trace, "\n");
	}
	fprintf(trace, "\n");
}
#endif
