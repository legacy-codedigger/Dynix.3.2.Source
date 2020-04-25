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

/* $Header: emptyrow.c 2.0 86/01/28 $
 *
 * emptyrow: returns true if row r of m is all zeros.
 *
 * Note that we assume the garbage at the end of the
 * row is all zeros.
 */

#include "bit.h"

emptyrow(m, rows, cols, r)
bitmat m;
int rows, cols, r;
{
	char *top, *bot;

	bot = &m[r*((cols+7)>>3)];
	top = bot + ((cols-1) >> 3);
	while (bot <= top)
		if (*bot++)
			return(0);
	return (1);
}
