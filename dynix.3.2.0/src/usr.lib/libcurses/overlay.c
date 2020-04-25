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

/* $Header: overlay.c 2.0 86/01/28 $ */

# include	"curses.ext"
# include	<ctype.h>

# define	min(a,b)	(a < b ? a : b)
# define	max(a,b)	(a > b ? a : b)

/*
 * This routine writes win1 on win2 non-destructively.
 */
overlay(win1, win2)
reg WINDOW	*win1, *win2; {

	reg char	*sp, *end;
	reg int		x, y, endy, endx, starty, startx;

# ifdef DEBUG
	fprintf(outf, "OVERLAY(%0.2o, %0.2o);\n", win1, win2);
# endif
	starty = max(win1->_begy, win2->_begy) - win1->_begy;
	startx = max(win1->_begx, win2->_begx) - win1->_begx;
	endy = min(win1->_maxy, win2->_maxy) - win1->_begy - 1;
	endx = min(win1->_maxx, win2->_maxx) - win1->_begx - 1;
	for (y = starty; y < endy; y++) {
		end = &win1->_y[y][endx];
		x = startx + win1->_begx;
		for (sp = &win1->_y[y][startx]; sp <= end; sp++) {
			if (!isspace(*sp))
				mvwaddch(win2, y + win1->_begy, x, *sp);
			x++;
		}
	}
}
