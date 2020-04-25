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

/* $Header: mat.c 2.0 86/01/28 $
 *
 * mat: retrieve the value in m[r, c].
 * rows and cols are the size of the matrix in all these routines.
 */

#include "bit.h"

int
mat(m, rows, cols, r, c)
register bitmat m;
register int c;
int rows, cols, r;
{
	register int thisbyte;

	thisbyte = m[r*((cols+7)>>3) + (c>>3)] & 0xff;
	thisbyte &= 0x80 >> (c&7);
	return (thisbyte != 0);
}
