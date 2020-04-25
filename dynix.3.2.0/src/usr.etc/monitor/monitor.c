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

#ifndef lint
static char rcsid[] = "$Header: monitor.c 2.3 87/04/29 $";
#endif

/*
 * monitor.c
 *
 * This program will display on a dumb terminal various performance metrics
 */

#include <sys/tmp_ctl.h>

#include <sys/types.h>
#include <sys/vm.h>
#include <sys/time.h>
#include <sys/dk.h>
#include <sys/flock.h>
#include <stdio.h>
#include <curses.h>
#include "monitor.h"
#include "sslib.h"

extern char *malloc();

struct timeval timeout;

/*
 * For getopt
 */
extern char *optarg;
extern int optind;

/*
 * global argv[0] (for error messages)
 */
char *whoiam;

/*
 * Flags for options settable from command line
 */
int interval, processor, p_cols, flip, terse;
/*
 * Other Global parameters
 */
int p_lines;			/* # of lines of processor bars */
/*
 * Global statistic data structures
 * elements of sstat have hooks into these
 */
struct dk dk_sum;
struct ether_stat es_sum;
struct vmmeter rate;	/* rate over last interval */
struct vmtotal total;
struct proc_stat ps;
unsigned pt_user;		/* total user time */
unsigned pt_sys;		/* total sys time */
unsigned pt_total;		/* total time */
unsigned fsreadhit, fswritehit; /* File Sys hit ratios */
double avenrun[3];		/* Load averages */
unsigned avedirty;		/* average dirty memory in K */
unsigned deficit;		/* deficit in K */
struct flckinfo flckinfo;	/* Raw sampled file/record locking stats */
struct flock_stats fl_stats;	/* File/Record locking stats */
WINDOW *bar_win, *stat_win, *on_win;
char *exitmsg;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	struct vmmeter *sd[2];
	struct dk *dk[2];
	struct dk *dk_rate;
	struct ether_stat *es[2];
	struct ether_stat *ether_rate;
	struct vmmeter syst[2];	/* totals from per processor data */
	struct ss_cfg sc;
	int i;
	unsigned *procutil;
	int old, new;
	int c, err = 0;

	whoiam = argv[0];
	interval = 1;		/* default to 1 sec intervals */
	processor = 0;		/* default to affinity proc 0 */
	flip = 0;		/* default to no flip between screens */
	terse = 0;		/* default to verbose text */
	p_cols = 0;		/* # of cols of processors not forced */

	while ((c = getopt(argc, argv, "c:fi:p:t")) != EOF) {
		switch (c) {
		case 'c':
			p_cols = atoi(optarg);
			if (p_cols < 1 || p_cols > MAX_PROC_COLS) {
				fprintf(stderr, "%s: bad # of columns: %s\n",
					whoiam, optind);
				err++;
			}
			break;
		case 'f':
			flip++;
			break;
		case 'i':
			interval = atoi(optarg);
			if (interval < 0) {
				fprintf(stderr, "%s: invalid interval: %s\n",
					whoiam, optind);
				err++;
			}
			break;
		case 'p':
			processor = atoi(optarg);
			break;
		case 't':
			terse++;
			break;
		case '?':
			err++;
			break;
		}
	}
	if (err || optind < argc) {
		fprintf(stderr,
	   "Usage: %s  [-t] [-f] [-c columns] [-i interval] [-p processor]\n",
			whoiam);
		exit(1);
	}
	/*
	 * Attempt to bind to a processor (default is processor 0)
	 */
	(void) tmp_affinity(processor);
	/*
	 * initialize the screen and setup for data gathering
	 */
	get_sscfg(&sc);
	setup(&sc);
	procutil = (unsigned *) malloc(sc.Nengine * sizeof(unsigned) * CPUSTATES);
	dk_rate = (struct dk *) malloc(sc.dk_nxdrive * sizeof(struct dk));
	ether_rate = (struct ether_stat *) malloc(sc.nse_unit * sizeof(struct ether_stat));
	if (procutil == (unsigned *)NULL ||
	    dk_rate == (struct dk *)NULL ||
	    ether_rate == (struct ether_stat *)NULL) {
		exitmsg = "Memory allocation failure";
		die(1);
	}
	for (i = 0; i < 2; i++) {
		sd[i] = (struct vmmeter *)
				malloc(sc.Nengine * sizeof(struct vmmeter));
		dk[i] = (struct dk *)
				malloc(sc.dk_nxdrive * sizeof(struct dk));
		es[i] = (struct ether_stat *)
				malloc(sc.nse_unit * sizeof(struct ether_stat));
		if (sd[i] == (struct vmmeter *)NULL ||
		    dk[i] == (struct dk *)NULL ||
		    es[i] == (struct ether_stat *)NULL) {
			exitmsg = "Memory allocation failure";
			die(1);
		}
	}
	/* 
	 * Loop forever sampling and updating the screen
	 */

	old = 0;
	new = 1;
	ss_sample(&sc, sd[old], dk[old], es[old], &ps, &total, avenrun,
			  &avedirty, &deficit, &flckinfo);
	ss_sum(&sc, sd[old], &syst[old]);
	while (1) {
		if (interval)
			sleep((unsigned)interval);
		ss_sample(&sc, sd[new], dk[new], es[new], &ps, &total, avenrun,
			  &avedirty, &deficit, &flckinfo);
		ss_sum(&sc, sd[new], &syst[new]);
		munge(&sc, &syst[old], &syst[new], &rate,
			dk[old], dk[new], dk_rate, &dk_sum,
			es[old], es[new], ether_rate, &es_sum, &total,
			&flckinfo, &fl_stats);
		perprocrate(&sc, sd[old], sd[new], procutil);
		display(&sc, &rate, procutil, avenrun);
		chars_typed(&sc);
		old = new;
		new = !new;
	}
}

int
chars_typed(sc)
	struct ss_cfg *sc;
{
	int readfds;
	int c;

	/* see if anything has been typed at the terminal */
	readfds = 1;
	(void) select(1, &readfds, (int *)NULL, (int *)NULL, &timeout);

	if (readfds != 0) {
		c = getc(stdin);
		switch (c&0x7f) {
		case '\n':
		case '\r':
		case 'q':
			die(0);
		case 'f':
			wclear(on_win);
			wrefresh(on_win);
			if (on_win == bar_win)
				on_win = stat_win;
			else
				on_win = bar_win;
			draw_screen(sc);
			wrefresh(on_win);
			break;
		case '\014':
			wclear(on_win);
			wrefresh(on_win);
			draw_screen(sc);
			wrefresh(on_win);
			break;
		}
	}
}

die(status)
	int status;
{
	if (stdscr != (WINDOW *)NULL) {
		erase();
		refresh();
	}
	endwin();
	if (exitmsg != (char *)NULL)
		fprintf(stderr, "%s: %s\n", whoiam, exitmsg);
	exit(status);
}
