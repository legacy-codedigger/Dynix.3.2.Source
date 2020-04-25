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

/* $Header: getch.c 2.0 86/01/28 $ */

# include	"curses.ext"

/*
 * This routine reads in a character from the window.
 */
wgetch(win)
reg WINDOW	*win; {

	reg bool	weset = FALSE;
	reg char	inp;

	if (!win->_scroll && (win->_flags&_FULLWIN)
	    && win->_curx == win->_maxx - 1 && win->_cury == win->_maxy - 1)
		return ERR;
# ifdef DEBUG
	fprintf(outf, "WGETCH: _echoit = %c, _rawmode = %c\n", _echoit ? 'T' : 'F', _rawmode ? 'T' : 'F');
# endif
	if (_echoit && !_rawmode) {
		raw();
		weset++;
	}
	inp = getchar();
# ifdef DEBUG
	fprintf(outf,"WGETCH got '%s'\n",unctrl(inp));
# endif
	if (_echoit) {
		mvwaddch(curscr, win->_cury, win->_curx, inp);
		waddch(win, inp);
	}
	if (weset)
		noraw();
	return inp;
}
