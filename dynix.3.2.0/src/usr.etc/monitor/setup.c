/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef lint
static char rcsid[] = "$Header: setup.c 2.3 91/03/29 $";
#endif

/*
 * setup.c
 *
 * Initialize the screen
 */

#include <signal.h>
#include <curses.h>
#include "monitor.h"
#include "sslib.h"

extern char *whoiam;
extern int p_cols, flip, terse;
extern char *percent[], *hashes[];
extern struct sstat scr_text[];
extern int p_bar_len[], tot_bar_len;
extern int p_lines, n_text;

extern WINDOW *bar_win, *stat_win, *on_win;

extern char *exitmsg;
extern int die();

struct sigvec vec = {
	die, 0, 0
};

char hostname[256];

setup(sc)
	struct ss_cfg *sc;
{
	/*
	 * Some curses initialization stuff
	 */
	sigvec(SIGHUP, &vec, (struct sigvec *)NULL);
	sigvec(SIGINT, &vec, (struct sigvec *)NULL);
	sigvec(SIGQUIT, &vec, (struct sigvec *)NULL);
	if (initscr() == (WINDOW *)NULL) {
		exitmsg = "Memory allocation failure";
		die(1);
	}
	crmode();
	noecho();
	nonl();
	/*
	 * If two screens (flip set via -f)
	 * then set up separate windows for the bar charts and the stats
	 */
	bar_win = newwin(0,0,0,0);
	if (bar_win == (WINDOW *)NULL) {
		exitmsg = "Memory allocation failure";
		die(1);
	}
	on_win = bar_win;
	if (flip) {
		stat_win = newwin(0,0,0,0);
		if (stat_win == (WINDOW *)NULL) {
			exitmsg = "Memory allocation failure";
			die(1);
		}
	} else
		stat_win = bar_win;
	/*
	 * Some calculations/checking so can adapt to # of processors
	 */
	if (p_cols == 0) {
		p_cols = sc->Nengine / MAX_PROC_LINES;
		if (sc->Nengine % MAX_PROC_LINES != 0 && p_cols < MAX_PROC_COLS)
			p_cols++;
	}

	p_lines = sc->Nengine / p_cols;
	if (sc->Nengine % p_cols != 0)
		p_lines++;
	if (p_lines > LINES) {
		fprintf(stderr, "%s: Can't display %d processors with %d columns\n",
			whoiam, sc->Nengine, p_cols);
		die(1);
	}
	/*
	 * Get the machine (host) name
	 */
	gethostname(hostname, 255);
	/*
	 * Select other data to be displayed, based on room left
	 * and any guidance provided by the user.
	 */
	select_data(p_lines + 8);
	/*
	 * Initialize the screen
	 */
	draw_screen(sc);
	wrefresh(on_win);
}

select_data(last_proc_line)
	int last_proc_line;
{
	int st_line, n_scols, spacing;
	int ncol;
	int x, y;
	int isroom = TRUE;
	struct sstat *s;

	if (flip) {
		st_line = 0;
	} else {
		st_line = last_proc_line + 2;
	}
	if (terse) {
		n_scols = 5;
		spacing = 16;
	} else {
		n_scols = 3;
		spacing = 26;
	}
	x = st_line;
	y = 0;
	ncol = 0;
	for (s = scr_text; s < &scr_text[n_text]; s++) {
		if (s->displayed && isroom) {
			s->text_pt.x = x;
			s->text_pt.y = y;
			if (++x >= LINES) {
				x = st_line;
				if (++ncol >= n_scols) {
					isroom = FALSE;
				} else {
					y += spacing;
				}
			}
		} else {
			s->displayed = 0;
		}
	}
}

draw_screen(sc)
	struct ss_cfg *sc;
{
	register int i, n, j, c;
	register int pci = p_cols - 1;	/* adjusted p_cols to be index */
	struct sstat *s;
	int set;
	char buffer[80];

	n = strlen(hashes[pci]) + COL_SPACING;
	if (pci == 0)
		c = 4;
	else
		c = 0;
	for (i = 0; i < p_cols; i++) {
		mvwaddstr(bar_win, 0, c+COL_SPACING+(i*n), hashes[pci]);
		mvwaddstr(bar_win, 0, c+i*n+1, "P#");
		mvwaddstr(bar_win, p_lines+1, c+COL_SPACING+(i*n), hashes[pci]);
		mvwaddstr(bar_win, p_lines+2, c+COL_SPACING+(i*n), percent[pci]);
	}

	/*
	 * The processor numbers and vertical portions of the boxes
	 */
	set = 0;
	for (i = 0; i < p_cols*p_lines; i++) {
		j = p_lines*set - 1;
		if (i < sc->Nengine) {
			sprintf(buffer, "%3d", i);
			/* whole calc used to be i-(p_lines*set)+2 */
			mvwaddstr(bar_win, i-j, c+set*n, buffer);
		}
		mvwaddch(bar_win, i-j, c+set*n+COL_SPACING, '|');
		mvwaddch(bar_win, i-j, c+(set+1)*n-1, '|');
		if ((i+1) % p_lines == 0)
			set++;
	}
	/*
	 * Total lines
	 */
	mvwaddstr(bar_win, p_lines+8, 8, percent[0]);
	mvwaddstr(bar_win, p_lines+5, 8, hashes[0]);
	mvwaddstr(bar_win, p_lines+7, 8, hashes[0]);
	mvwaddstr(bar_win, p_lines+6, 0, " Total  |");
	mvwaddch(bar_win, p_lines+6, strlen(hashes[0])+7, '|');

	mvwaddstr(bar_win, p_lines+5, strlen(hashes[0])+10, "= sys");
	mvwaddstr(bar_win, p_lines+6, strlen(hashes[0])+10, "- user");

	/*
	 * Activity monitor text
	 */
	for (s = scr_text; s < &scr_text[n_text]; s++) {
		if (s->displayed)
			mvwaddstr(stat_win, s->text_pt.x, s->text_pt.y,
				(terse) ? s->terse : s->verbose);
	}
	/*
	 * machine name, load average, etc on line 0
	 */
	mvwaddstr(bar_win, p_lines + 4, 9, "machine:");
	wmove(bar_win, p_lines + 4, 18);

	wprintw(bar_win, "%-22.22s", hostname);
	mvwaddstr(bar_win, p_lines + 4, 42, "load avg:");
}
