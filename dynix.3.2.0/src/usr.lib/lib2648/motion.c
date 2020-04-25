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

/* $Header: motion.c 2.0 86/01/28 $
 *
 * Move the pen to x, y.  We assume we are already in ESCP mode.
 */

#include "2648.h"

motion(x, y)
{
	char lox, loy, hix, hiy;
	int delx, dely;

	delx = x-_penx; dely = y-_peny;
	if (-16 <= delx && delx <= 15 && -16 <= dely && dely <= 15) {
		/*
		 * Optimization: if within 15 in both directions, can use
		 * HP short incremental mode, only 3 bytes.
		 */
		outchar('j');
		outchar(32 + (delx & 31));
		outchar(32 + (dely & 31));
	} else {
		/*
		 * Otherwise must use binary absolute mode, 5 bytes.
		 * We never use ascii mode or binary incremental, since
		 * those both take many more bytes.
		 */
		outchar('i');
		outchar(32+ ((x>>5) & 31));
		outchar(32+ (x&31));
		outchar(32+ ((y>>5) & 31));
		outchar(32+ (y&31));
	}
	_penx = x;
	_peny = y;
}
