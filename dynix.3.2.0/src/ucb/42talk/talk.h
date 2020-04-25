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

/* $Header: talk.h 1.1 89/10/09 $ */

#include <curses.h>
#include <utmp.h>

#define	forever		for(;;)

#define	BUF_SIZE	512

FILE *popen();
int quit(), sleeper();

extern int sockt;
extern int curses_initialized;
extern int invitation_waiting;

extern char *current_state;
extern int current_line;

typedef struct xwin {
	WINDOW *x_win;
	int x_nlines;
	int x_ncols;
	int x_line;
	int x_col;
	char kill;
	char cerase;
	char werase;
} xwin_t;

extern xwin_t my_win;
extern xwin_t his_win;
extern WINDOW *line_win;
