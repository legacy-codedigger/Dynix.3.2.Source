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

/* $Header: touchwin.c 2.0 86/01/28 $ */

# include	"curses.ext"

/*
 * make it look like the whole window has been changed.
 */
touchwin(win)
reg WINDOW	*win;
{
	reg WINDOW	*wp;

	do_touch(win);
	for (wp = win->_nextp; wp != win; wp = wp->_nextp)
		do_touch(wp);
}

/*
 * do_touch:
 *	Touch the window
 */
static
do_touch(win)
reg WINDOW	*win; {

	reg int		y, maxy, maxx;

	maxy = win->_maxy;
	maxx = win->_maxx - 1;
	for (y = 0; y < maxy; y++) {
		win->_firstch[y] = 0;
		win->_lastch[y] = maxx;
	}
}
