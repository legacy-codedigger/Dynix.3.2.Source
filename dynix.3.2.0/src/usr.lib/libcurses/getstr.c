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

/* $Header: getstr.c 2.0 86/01/28 $ */

# include	"curses.ext"

/*
 * This routine gets a string starting at (_cury,_curx)
 */
wgetstr(win,str)
reg WINDOW	*win; 
reg char	*str; {

	while ((*str = wgetch(win)) != ERR && *str != '\n')
		str++;
	if (*str == ERR) {
		*str = '\0';
		return ERR;
	}
	*str = '\0';
	return OK;
}
