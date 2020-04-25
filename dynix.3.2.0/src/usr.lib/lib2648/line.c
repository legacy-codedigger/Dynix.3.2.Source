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

/* $Header: line.c 2.0 86/01/28 $
 *
 * line: draw a line from point 1 to point 2.
 */

#include "2648.h"

line(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
#ifdef TRACE
	if (trace)
		fprintf(trace, "line((%d, %d), (%d, %d)),", x1, y1, x2, y2);
#endif
	if (x1==_penx && y1==_peny) {
		/*
		 * Get around a bug in the HP terminal where one point
		 * lines don't get drawn more than once.
		 */
		move(x1, y1+1);
		sync();
	}
	move(x1, y1);
	draw(x2, y2);
}
