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

/* $Header: drawbox.c 2.0 86/01/28 $
 *
 * Draw a box around a window.  The lower left corner of the box is at (r, c).
 * Color is 1 for drawing a box, 0 for erasing.
 * The box is nrow by ncol.
 */

#include "2648.h"

drawbox(r, c, color, nrow, ncol)
int r, c, color, nrow, ncol;
{
	if (color)
		setset();
	else
		setclear();
	move(c, r);
	draw(c+ncol-1, r);
	draw(c+ncol-1, r+nrow-1);
	draw(c, r+nrow-1);
	draw(c, r);
}
