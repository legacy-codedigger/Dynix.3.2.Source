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

/* $Header: dispmsg.c 2.0 86/01/28 $
 *
 * display a message, str, starting at (x, y).
 */

#include "2648.h"

dispmsg(str, x, y, maxlen)
char *str;
int x, y;
{
	int oldx, oldy;
	int oldcuron;
	int oldquiet;
	extern int QUIET;

	oldx = _curx; oldy = _cury;
	oldcuron = _cursoron;
	zoomout();
	areaclear(y, x, y+8, x+6*maxlen);
	setset();
	curon();
	movecurs(x, y);
	texton();
	oldquiet = QUIET;
	QUIET = 0;
	outstr(str);
	if (oldquiet)
		outstr("\r\n");
	QUIET = oldquiet;
	textoff();
	movecurs(oldx, oldy);
	if (oldcuron == 0)
		curoff();
}
