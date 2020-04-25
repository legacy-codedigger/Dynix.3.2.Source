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

/* $Header: newmat.c 2.0 86/01/28 $
 *
 * newmat: return a brand new bitmat with the proper size.
 * To get rid of it just call free.
 */

#include "bit.h"

bitmat
newmat(rows, cols)
int rows, cols;
{
	int size = ((cols + 7) >> 3) * rows;
	char *m;

#ifdef TRACE
	if (size <= 0 && trace) {
		fprintf(trace, "newmat: rows=%d, cols=%d\n", rows, cols);
		abort();
	}
	if (trace)
		fprintf(trace, "newmat: malloc(%d) =", size);
#endif
	m = (char *) malloc(size);
#ifdef TRACE
	if (trace)
		fprintf(trace, "%x\n", m);
#endif
	zermat(m, rows, cols);
	return (m);
}
