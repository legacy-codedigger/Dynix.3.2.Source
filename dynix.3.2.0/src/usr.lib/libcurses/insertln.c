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

/* $Header: insertln.c 2.0 86/01/28 $ */

# include	"curses.ext"

/*
 * This routine performs an insert-line on the window, leaving
 * (_cury,_curx) unchanged.
 */
winsertln(win)
reg WINDOW	*win; {

	reg char	*temp;
	reg int		y;
	reg char	*end;

	temp = win->_y[win->_maxy-1];
	win->_firstch[win->_cury] = 0;
	win->_lastch[win->_cury] = win->_maxx - 1;
	for (y = win->_maxy - 1; y > win->_cury; --y) {
		win->_y[y] = win->_y[y-1];
		win->_firstch[y] = 0;
		win->_lastch[y] = win->_maxx - 1;
	}
	for (end = &temp[win->_maxx]; temp < end; )
		*temp++ = ' ';
	win->_y[win->_cury] = temp - win->_maxx;
	if (win->_cury == LINES - 1 && win->_y[LINES-1][COLS-1] != ' ')
		if (win->_scroll) {
			wrefresh(win);
			scroll(win);
			win->_cury--;
		}
		else
			return ERR;
	return OK;
}
