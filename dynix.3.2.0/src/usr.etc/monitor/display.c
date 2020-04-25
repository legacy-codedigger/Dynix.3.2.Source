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
static char rcsid[] = "$Header: display.c 2.3 91/03/29 $";
#endif

/*
 * Display routines for monitor
 *
 * Updates fields on the screen according to data read from the kernel
 * data structures
 */

#include <sys/time.h>
#include <sys/vm.h>
#include <sys/dk.h>
#include <curses.h>
#include "monitor.h"
#include "sslib.h"

extern int p_cols, flip, terse;
extern char *percent[], *hashes[];
extern int p_bar_len[], tot_bar_len;
extern int p_lines, n_text;
extern unsigned pt_user, pt_sys, pt_total;
extern struct sstat scr_text[];
extern WINDOW *bar_win, *stat_win, *on_win;

display(sc, rate, procutil, avenrun)
	struct ss_cfg *sc;
	struct vmmeter *rate;
	unsigned *procutil;
	double *avenrun;
{
	register int i, u, s, len, set;
	struct sstat *sp;
	char buf[80];
	int proc, c, off;
	unsigned total_ticks;

	total_ticks = 0;
	for (i = 0; i < CPUSTATES; i++)
		total_ticks += rate->v_time[i];

	set = 0;
	len = p_bar_len[p_cols-1];
	if (p_cols == 1)
		c = 4;
	else
		c = 0;
	/*
	 * The per processor stuff has already been converted to a percent.
	 */
	for (proc = 0; proc < sc->Nengine; procutil += CPUSTATES) {
		s = ((double)len) * procutil[CP_SYS] / 100. + 0.5;
		u = ((double)len) *
			(procutil[CP_USER] + procutil[CP_NICE]) / 100. + 0.5;

		i = 0;
		while (s-- > 0)
			buf[i++] = '=';
		while (u-- > 0)
			buf[i++] = '-';
		while (i < len)
			buf[i++] = ' ';

		buf[len] = 0;
		mvwaddstr(bar_win, proc-(p_lines*set)+1,
			c+set*(COL_SPACING+len+2)+(COL_SPACING+1), buf);
		if ((++proc) % p_lines == 0)
			set++;
	}

	pt_user = rate->v_time[CP_USER] + rate->v_time[CP_NICE];
	pt_sys = rate->v_time[CP_SYS];
	pt_total = pt_sys + pt_user;
	s = ((double)tot_bar_len) * pt_sys / total_ticks + 0.5;
	u = ((double)tot_bar_len) * pt_user / total_ticks + 0.5;

	i = 0;
	while (s-- > 0)
		buf[i++] = '=';
	while (u-- > 0)
		buf[i++] = '-';
	while (i < tot_bar_len)
		buf[i++] = ' ';
	buf[tot_bar_len] = 0;
	mvwaddstr(bar_win, p_lines + 6, 9, buf);

	off = (terse) ? TERSE_TEXT_LENGTH : VERBOSE_TEXT_LENGTH;
	for (sp = scr_text; sp < &scr_text[n_text]; sp++) {
	    if (sp->displayed) {
		sprintf(buf, sp->fmt, *(sp->datav));
		mvwaddstr(stat_win, sp->text_pt.x, sp->text_pt.y + off, buf);
	    }
	}

	/*
	 * display load averages
	 */
	wmove(bar_win, p_lines+4, 51);
	wprintw(bar_win, "%6.2f%6.2f%6.2f", avenrun[0], avenrun[1], avenrun[2]);

	wmove(on_win, LINES-1, COLS-1);
	wrefresh(on_win);
}
