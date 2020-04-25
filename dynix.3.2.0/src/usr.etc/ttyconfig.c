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

/*
** Usage: ttyconfig ttyname 
**	  [ -normal | -special ] [ -carrier | -nocarrier ] [-q]
**   configure Systech MTI-1650 tty lines for dual use using auto-dial modems
*/

#ifndef lint
static char rcsid[] = "$Header: ttyconfig.c 2.3 91/03/25 $";
#endif

#include <stdio.h>
#include <sys/param.h>
#include <nlist.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/file.h>
#define	KERNEL
#include <sys/ioctl.h>
#include <sys/tty.h>
#undef	KERNEL
#include <sys/clist.h>
#include <mbad/st.h>

#define	NORMAL	  1
#define SPECIAL	  2
#define	CARRIER	  3
#define	NOCARRIER 4
#define eq(a,b)	  ((a) && (b) && strcmp((a),(b)) == 0)
#define	SMAJOR	  7		/* Systech major device number */

char	*kernel = "/dynix";
char	*memory	= "/dev/kmem";
int	dynix, kmem;
int	cmask;
char	softc;
int 	mode, sense;
struct	stat sbuf;

long	lseek();
int	timeout();

struct	nlist nl[] = {
	{ "_stinfo" },
#define	X_STINFO	0
	{ "_nst" },
#define X_ST_MAX	1
	{ "" },
};

usage() { 

	fprintf(stderr, 
	"usage: ttyconfig ttyname [-q] [-normal|-special] [-carrier|-nocarrier]\n");
	exit(1);
}

main(argc, argv)
	char **argv;
{
	char	*ttyname;
	int	quiet = 0;
	
	(void) dup2(1, 2);	/* force stderr to stdout */
	argv[argc] = NULL;

	/*
	 * Add a quiet switch (-q) while maintaining the original algorithm
	 * and assumptions.  Enforce new usage to be cmd tty [-q]...
	 * This means that argc must be -- in order to maintain the
	 * argc assumptions (2 or 4 are only legal argc).
	 * After determining ttyname, ++ argv to skip the -q.
	 */

	if (strcmp(argv[2], "-q") == 0) {
		quiet++;
		argc--;
	}

	if (argc != 2 && argc != 4)
		usage();

	ttyname = argv[1];

	/*
	 * skip over -q if appropriate
	 */

	if (quiet)
		argv++;

	checktty(ttyname);

	if (argc == 4) {
		if (eq(argv[2], "-normal"))
			mode = NORMAL;
		else if (eq(argv[2], "-special"))
			mode = SPECIAL;
		else
			fprintf(stderr, 
	"you must specify one of \"-normal\" or \"-special\"\n"),
				exit(1);
		
		if (eq(argv[3], "-carrier"))
			sense = CARRIER;
		else if (eq(argv[3], "-nocarrier"))
			sense = NOCARRIER;
		else
			fprintf(stderr, 
	"you must specify one of \"-carrier\" or \"-nocarrier\"\n"), 
				exit(1);
	}

	if (mode == NORMAL && sense == NOCARRIER)
	    fatal("\"-normal\" and \"-nocarrier\" are incompatible options");

	fixtty(ttyname);

	/*
	 * check for !quiet mode
	 */

	if(!quiet) showtty(ttyname);

	exit(0);
}

/*
 * Verify tty is a Systech MTI-1650 line.
 */
checktty(t)
	char *t;
{

	if (stat(t, &sbuf) < 0) {
		perror(t);
		exit(1);
	}
	if ((sbuf.st_mode & S_IFMT) != S_IFCHR)
		fprintf(stderr, "%s: not a character device\n",t), exit(1);
	if (major(sbuf.st_rdev) != SMAJOR)
		fprintf(stderr, 
		"%s: (major #%d) is not a Systech MTI-1650 line (expected major #%d)\n",
			t, major(sbuf.st_rdev), SMAJOR), exit(1);
}

/*
 * Fix given tty to a given state.
 * Uses /dev/kmem to scribble on tty structure.
 */
fixtty(ttyname)
	char *ttyname;
{
	struct stinfo *stinfo;
	int nst;
	int dev, board, unit, n;
	struct tty *tp;
	short nopens;
	int size;
	
	dev = sbuf.st_rdev;
	board = STBOARD(dev);
	unit = STLINE(dev);

	kmem = open(memory, (mode && sense) ? O_RDWR : O_RDONLY);
	if (kmem < 0)
		perror(memory), exit(1);
	dynix = open(kernel, O_RDONLY);
	if (dynix < 0)
		perror(kernel), exit(1);

	nlist(kernel, nl);
	if (nl[X_STINFO].n_value == 0) 
		fatal("nlist for Systech MTI-1650 'stinfo' failed!");
	if (nl[X_ST_MAX].n_value == 0)
		fatal("nlist for Systech MTI-1650 'nst' failed!");

	if (lseek(kmem, (long)nl[X_STINFO].n_value, 0) < 0)
		fatal("lseek for Systech MTI-1650 tty info failed");
	if (read(kmem, &stinfo, sizeof (stinfo)) != sizeof (stinfo))
		fatal("read of Systech MTI-1650 tty info failed");
	if (lseek(kmem, (long)nl[X_ST_MAX].n_value, 0) < 0)
		fatal("lseek for Systech MTI-1650 max board failed");
	if (read(kmem, &nst, sizeof (nst)) != sizeof (nst))
		fatal("read of Systech MTI-1650 max board failed");

	if (board >= nst)
		fatal("board for tty not configured into system");

	if (lseek(kmem, (long) &((struct stinfo **)stinfo)[board], 0) < 0)
		fatal("lseek for Systech MTI-1650 tty info failed");
	if (read(kmem, &stinfo, sizeof (stinfo)) != sizeof (stinfo))
		fatal("read of Systech MTI-1650 tty info failed");

	if (stinfo == 0)
		fatal("Systech MTI-1650 controller configured but not available");

	if (lseek(kmem, (long) &(stinfo->st_size), 0) < 0)
		fatal("lseek for Systech MTI-1650 tty size failed");
	if (read(kmem, &size, sizeof (size)) != sizeof (size))
		fatal("read of Systech MTI-1650 tty size failed");

	if (size == 0)
		fatal("number of lines is zero!");
	if (unit >= size)
		fatal("not enough lines on controller");

	tp = &stinfo->st_tty[unit];

	if (lseek(kmem, (long) &(tp->t_nopen), 0) < 0)
		fatal("lseek for Systech MTI-1650 tty pointer failed");
	if (read(kmem, &nopens, sizeof (nopens)) != sizeof (nopens))
		fatal("read of Systech MTI-1650 tty pointer failed");

	if (mode && sense && nopens != 0)
		fprintf(stderr, 
			"ttyconfig: %s has %d open(s) outstanding, none allowed\n",
			ttyname, nopens), exit(1);

	if (lseek(kmem, (long) &(tp->t_cmask), 0) < 0)
		fatal("lseek for cmask failed");
	if (read(kmem, &cmask, sizeof (tp->t_cmask)) != sizeof (tp->t_cmask))
		fatal("read of t_cmask failed");
	if (lseek(kmem, (long) &(tp->t_softcarr), 0) < 0)
		fatal("lseek for t_softcarr failed");
	if (read(kmem, &softc, sizeof (softc)) != sizeof (softc))
		fatal("read of t_softcarr failed");

	if (mode && sense) { 	/* specified, so change them */
		/* sanity */
		if (cmask != DCD && cmask != DSR)
			fprintf(stderr, 
				"ttyconfig: cmask? (=%#x)\n", cmask), exit(1);
		if (softc != 0 && softc != SOFT_CARR)
			fprintf(stderr, 
				"ttyconfig: softc? (=%#x)\n", softc), exit(1);

		cmask = (mode == NORMAL) ? DCD : DSR;
		softc = (sense == CARRIER) ? 0 : SOFT_CARR;

		if (lseek(kmem, (long) &(tp->t_cmask), 0) < 0)
			fatal("lseek for cmask failed");
		if (write(kmem, &cmask, sizeof (cmask)) != sizeof (cmask))
			fatal("write of t_cmask failed");
		if (lseek(kmem, (long) &(tp->t_softcarr), 0) < 0)
			fatal("lseek for t_softcarr failed");
		if (write(kmem, &softc, sizeof (softc)) != sizeof (softc))
			fatal("write of t_softcarr failed");

		if (sense == NOCARRIER) {
			/* 
			 * Setting soft carrier so we must open line at
			 * least once to set ensure RTS set (looped back to CD),
			 * and take HUP signal
			 */
			signal(SIGHUP, SIG_IGN);
			signal(SIGALRM, timeout);
			alarm(5);
			(void) close( open(ttyname, O_RDONLY) );
			alarm(0);
		}
	}
}

int 
timeout()
{

	fprintf(stderr, "ttyconfig: timeout on open, check wiring.\n");
	exit(1);
}

/*
 * Display current setup for tty
 */
showtty(s)
	char *s;
{

	printf( "%s: configured for %s wiring, %s\n", s,
		(cmask == DCD) ? "normal" : "special",
		(softc == 0) ? "no soft carrier flag" : 
			       "soft carrier flag set" );
}

/*
 * Fatal error
 */
fatal(s)
	char *s;
{

	fprintf(stderr, "ttyconfig: %s\n", s);
	exit(1);
}
