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

/* $Header: scanw.c 2.0 86/01/28 $
 *
 * scanw and friends
 */

# include	"curses.ext"

/*
 *	This routine implements a scanf on the standard screen.
 */
scanw(fmt, args)
char	*fmt;
int	args; {

	return _sscans(stdscr, fmt, &args);
}
/*
 *	This routine implements a scanf on the given window.
 */
wscanw(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	args; {

	return _sscans(win, fmt, &args);
}
/*
 *	This routine actually executes the scanf from the window.
 *
 *	This is really a modified version of "sscanf".  As such,
 * it assumes that sscanf interfaces with the other scanf functions
 * in a certain way.  If this is not how your system works, you
 * will have to modify this routine to use the interface that your
 * "sscanf" uses.
 */
_sscans(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	*args; {

	char	buf[100];
	FILE	junk;

	junk._flag = _IOREAD|_IOSTRG;
	junk._base = junk._ptr = buf;
	if (wgetstr(win, buf) == ERR)
		return ERR;
	junk._cnt = strlen(buf);
	return _doscan(&junk, fmt, args);
}
