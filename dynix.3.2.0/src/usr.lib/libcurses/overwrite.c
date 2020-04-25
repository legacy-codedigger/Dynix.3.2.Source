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

/* $Header: overwrite.c 2.0 86/01/28 $ */

# include	"curses.ext"

# define	min(a,b)	(a < b ? a : b)

/*
 * This routine writes win1 on win2 destructively.
 */
overwrite(win1, win2)
reg WINDOW	*win1, *win2; {

	reg int		x, y, minx, miny, startx, starty;

# ifdef DEBUG
	fprintf(outf, "OVERWRITE(0%o, 0%o);\n", win1, win2);
# endif
	miny = min(win1->_maxy, win2->_maxy);
	minx = min(win1->_maxx, win2->_maxx);
# ifdef DEBUG
	fprintf(outf, "OVERWRITE:\tminx = %d,  miny = %d\n", minx, miny);
# endif
	starty = win1->_begy - win2->_begy;
	startx = win1->_begx - win2->_begx;
	if (startx < 0)
		startx = 0;
	for (y = 0; y < miny; y++)
		if (wmove(win2, y + starty, startx) != ERR)
			for (x = 0; x < minx; x++)
				waddch(win2, win1->_y[y][x]);
}
