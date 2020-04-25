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

/* $Header: scroll.c 2.0 86/01/28 $ */

# include	"curses.ext"

/*
 * This routine scrolls the window up a line.
 */
scroll(win)
reg WINDOW	*win; {

	reg char	*sp;
	reg int		i;
	reg char	*temp;

	if (!win->_scroll)
		return ERR;
	temp = win->_y[0];
	for (i = 1; i < win->_maxy; i++)
		win->_y[i - 1] = win->_y[i];
	for (sp = temp; sp < &temp[win->_maxx]; )
		*sp++ = ' ';
	win->_y[win->_maxy - 1] = temp;
	win->_cury--;
	if (win == curscr) {
		putchar('\n');
		if (!NONL)
			win->_curx = 0;
# ifdef DEBUG
		fprintf(outf, "SCROLL: win == curscr\n");
# endif
	}
# ifdef DEBUG
	else
		fprintf(outf, "SCROLL: win [0%o] != curscr [0%o]\n",win,curscr);
# endif
	touchwin(win);
	return OK;
}
