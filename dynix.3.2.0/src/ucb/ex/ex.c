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

/*  ADA support used by permission of Verdix Corporation
 *  Given Sept 25, 1985 by Steve Zeigler
 *  Author: Ben Priest
 */

#ifndef lint
static char *rcsid = "$Header: ex.c 2.7 90/06/12 $";
#endif

/* Copyright (c) 1981 Regents of the University of California */
#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include <fcntl.h>

#ifdef TRACE
char	tttrace[]	= { '/','d','e','v','/','t','t','y','x','x',0 };
#endif

/*
 * The code for ex is divided as follows:
 *
 * ex.c			Entry point and routines handling interrupt, hangup
 *			signals; initialization code.
 *
 * ex_addr.c		Address parsing routines for command mode decoding.
 *			Routines to set and check address ranges on commands.
 *
 * ex_cmds.c		Command mode command decoding.
 *
 * ex_cmds2.c		Subroutines for command decoding and processing of
 *			file names in the argument list.  Routines to print
 *			messages and reset state when errors occur.
 *
 * ex_cmdsub.c		Subroutines which implement command mode functions
 *			such as append, delete, join.
 *
 * ex_data.c		Initialization of options.
 *
 * ex_get.c		Command mode input routines.
 *
 * ex_io.c		General input/output processing: file i/o, unix
 *			escapes, filtering, source commands, preserving
 *			and recovering.
 *
 * ex_put.c		Terminal driving and optimizing routines for low-level
 *			output (cursor-positioning); output line formatting
 *			routines.
 *
 * ex_re.c		Global commands, substitute, regular expression
 *			compilation and execution.
 *
 * ex_set.c		The set command.
 *
 * ex_subr.c		Loads of miscellaneous subroutines.
 *
 * ex_temp.c		Editor buffer routines for main buffer and also
 *			for named buffers (Q registers if you will.)
 *
 * ex_tty.c		Terminal dependent initializations from termcap
 *			data base, grabbing of tty modes (at beginning
 *			and after escapes).
 *
 * ex_unix.c		Routines for the ! command and its variations.
 *
 * ex_v*.c		Visual/open mode routines... see ex_v.c for a
 *			guide to the overall organization.
 */

/*
 * Main procedure.  Process arguments and then
 * transfer control to the main command processing loop
 * in the routine commands.  We are entered as either "ex", "edit", "vi"
 * or "view" and the distinction is made here.  Actually, we are "vi" if
 * there is a 'v' in our name, "view" is there is a 'w', and "edit" if
 * there is a 'd' in our name.  For edit we just diddle options;
 * for vi we actually force an early visual command.
 */
main(ac, av)
	register int ac;
	register char *av[];
{
#ifndef VMUNIX
	char *erpath = EXSTRINGS;
#endif
	register char *cp;
	register int c;
	bool recov = 0;
	bool ivis;
	bool itag = 0;
	bool fast = 0;
	extern int onemt();
#ifdef TRACE
	register char *tracef;
#endif

	/*
	 * Immediately grab the tty modes so that we wont
	 * get messed up if an interrupt comes in quickly.
	 */
	gTTY(1);
#ifndef USG3TTY
	normf = tty.sg_flags;
#else
	normf = tty;
#endif
	ppid = getpid();
	/*
	 * Defend against d's, v's, w's, and a's in directories of
	 * path leading to our true name.
	 */
	av[0] = tailpath(av[0]);

	/*
	** Figure out how we were invoked: ex, edit, vi, view, a.vi
	*/
	ivis = any('v', av[0]);	/* "vi" */
	if (any('a', av[0]))	/* "a.vi" */
		value(ADA) = 1;
	if (any('w', av[0]))	/* "view" */
		value(READONLY) = 1;
	if (any('d', av[0])) {	/* "edit" */
		value(OPEN) = 0;
		value(REPORT) = 1;
		value(MAGIC) = 0;
	}

#ifndef VMUNIX
	/*
	 * For debugging take files out of . if name is a.out.
	 */
	if (av[0][0] == 'a')
		erpath = tailpath(erpath);
#endif
	/*
	 * Open the error message file.
	 */
	draino();
#ifndef VMUNIX
	erfile = open(erpath+4, 0);
	if (erfile < 0) {
		erfile = open(erpath, 0);
	}
#endif
	pstop();

	/*
	 * Initialize interrupt handling.
	 */
	oldhup = signal(SIGHUP, SIG_IGN);
	if (oldhup == SIG_DFL)
		signal(SIGHUP, onhup);
	oldquit = signal(SIGQUIT, SIG_IGN);
	ruptible = signal(SIGINT, SIG_IGN) == SIG_DFL;
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		signal(SIGTERM, onhup);
	if (signal(SIGEMT, SIG_IGN) == SIG_DFL)
		signal(SIGEMT, onemt);
	/*
	 * Handle signals dealing with
	 * exceeding file size limits
	 * and cpu time limits
	 */
#ifdef SIGXFSZ
	signal(SIGXFSZ, onxfsz);
#endif
#ifdef SIGXCPU
	signal(SIGXCPU, onxcpu);
#endif

	/*
	 * Process flag arguments.
	 */
	ac--, av++;
	while (ac && av[0][0] == '-') {
		c = av[0][1];
		if (c && av[0][2] != '\0' && c != 'w' && c != 'T') {
			char buf[256];

			sprintf(buf, "Illegal text '%s' after option %c\n",
					&av[0][2], c);
			write(2, buf, sizeof(buf));
			exit(1);
		}
		if (c == 0) {
			hush = 1;
			value(AUTOPRINT) = 0;
			fast++;
		} else switch (c) {

		case 'R':
			value(READONLY) = 1;
			break;

#ifdef TRACE
		case 'T':
			if (av[0][2] == 0)
				tracef = "trace";
			else {
				tracef = tttrace;
				tracef[8] = av[0][2];
				if (tracef[8])
					tracef[9] = av[0][3];
				else
					tracef[9] = 0;
			}
			trace = fopen(tracef, "w");
#define tracbuf NULL
			if (trace == NULL)
				printf("Trace create error\n");
			else
				setbuf(trace, tracbuf);
			break;

#endif

#ifdef LISPCODE
		case 'l':
			value(LISP) = 1;
			value(SHOWMATCH) = 1;
			break;
#endif

		case 'r':
			recov++;
			break;

		case 't':
			if (ac > 1 && av[1][0] != '-') {
				ac--, av++;
				itag = 1;
				/* BUG: should check for too long tag. */
				CP(lasttag, av[0]);
			}
			break;

		case 'v':
			ivis = 1;
			break;

		case 'w':
			defwind = 0;
			if (av[0][2] == 0) defwind = 3;
			else for (cp = &av[0][2]; isdigit(*cp); cp++)
				defwind = 10*defwind + *cp - '0';
			break;

#ifdef CRYPT
		case 'x':
			/* -x: encrypted mode */
			xflag = 1;
			break;
#endif

		default:
			smerror("Unknown option %s\n", av[0]);
			break;
		}
		ac--, av++;
	}

	/*
	 * Initialize end of core pointers.
	 * Normally we avoid breaking back to fendcore after each
	 * file since this can be expensive (much core-core copying).
	 * If your system can scatter load processes you could do
	 * this as ed does, saving a little core, but it will probably
	 * not often make much difference.
	 */
	fendcore = (line *) sbrk(0);
	endcore = fendcore - 2;

#ifdef SIGTSTP
	if (!hush && signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, onsusp), dosusp++;
#endif

	if (ac && av[0][0] == '+') {
		firstpat = &av[0][1];
		ac--, av++;
	}

#ifdef CRYPT
	if(xflag){
		key = getpass(KEYPROMPT);
		kflag = crinit(key, perm);
	}
#endif

	/*
	 * If we are doing a recover and no filename
	 * was given, then execute an exrecover command with
	 * the -r option to type out the list of saved file names.
	 * Otherwise set the remembered file name to the first argument
	 * file name so the "recover" initial command will find it.
	 */
	if (recov) {
		if (ac == 0) {
			ppid = 0;
			setrupt();
			execl(EXRECOVER, "exrecover", "-r", 0);
			filioerr(EXRECOVER);
			exit(1);
		}
		CP(savedfile, *av++), ac--;
	}

	/*
	 * Initialize the argument list.
	 */
	argv0 = av;
	argc0 = ac;
	args0 = av[0];
	erewind();

	/*
	 * Initialize a temporary file (buffer) and
	 * set up terminal environment.  Read user startup commands.
	 */
	if (setexit() == 0) {
		int	oldmask;

		oldmask = sigblock(sigmask(SIGTSTP));
		setrupt();
		intty = isatty(0);
		value(PROMPT) = intty;
		if (cp = getenv("SHELL"))
			CP(shell, cp);
		if (fast || !intty)
			setterm("dumb");
		else {
			gettmode();
			if ((cp = getenv("TERM")) != 0 && *cp)
				setterm(cp);
		}
		(void)sigsetmask(oldmask);
	}
	if (setexit() == 0 && !fast && intty) {
		if ((globp = getenv("EXINIT")) && *globp)
			commands(1,1);
		else {
			globp = 0;
			if ((cp = getenv("HOME")) != 0 && *cp) {
				(void) strcat(strcpy(genbuf, cp), "/.exrc");
				if (iownit(genbuf))
					source(genbuf, 1);
			}
		}
		/*
		 * Allow local .exrc too.  This loses if . is $HOME,
		 * but nobody should notice unless they do stupid things
		 * like putting a version command in .exrc.  Besides,
		 * they should be using EXINIT, not .exrc, right?
		 */
		 if (iownit(".exrc"))
			source(".exrc", 1);
	}

	init();	/* moved after prev 2 chunks to fix directory option */

	/*
	** Read ada.lib into ADAPATH (brp)
	*/
	ada_path();

	/*
	 * Initial processing.  Handle tag, recover, and file argument
	 * implied next commands.  If going in as 'vi', then don't do
	 * anything, just set initev so we will do it later (from within
	 * visual).
	 */
	if (setexit() == 0) {
		if (recov)
			globp = "recover";
		else if (itag)
			globp = ivis ? "tag" : "tag|p";
		else if (argc)
			globp = "next";
		if (ivis)
			initev = globp;
		else if (globp) {
			inglobal = 1;
			commands(1, 1);
			inglobal = 0;
		}
	}

	/*
	 * Vi command... go into visual.
	 * Strange... everything in vi usually happens
	 * before we ever "start".
	 */
	if (ivis) {
		/*
		 * Don't have to be upward compatible with stupidity
		 * of starting editing at line $.
		 */
		if (dol > zero)
			dot = one;
		globp = "visual";
		if (setexit() == 0)
			commands(1, 1);
	}

	/*
	 * Clear out trash in state accumulated by startup,
	 * and then do the main command loop for a normal edit.
	 * If you quit out of a 'vi' command by doing Q or ^\,
	 * you also fall through to here.
	 */
	seenprompt = 1;
	ungetchar(0);
	globp = 0;
	initev = 0;
	setlastchar('\n');
	setexit();
	commands(0, 0);
	cleanup(1);
	exit(0);
}

/*
** Reads adapath in from ada.lib and sets the atags path as well
** But only if adapath not set already in .exrc
*/
ada_path()
{
	register int i;
	register int project;
	register char *ps;
	register char *aps;
	char buf;

	/* ada path already set in .exrc */
	if (*(svalue(ADAPATH)) != '\0') 
		return(0);

	if ((project = open("ada.lib", O_RDONLY)) == -1) 
		return(0);

	/* skip over first line */
	while (((i = read(project, &buf, 1)) == 1) && (buf != '\n'))
		;
	if (i == -1) {
		(void)close(project);
		return(1);
	}
	/* skip junk in next line */
	while (((i = read(project, &buf, 1)) == 1) && (buf != ' ')) {
		if (buf == '\n') {
			*(svalue(ADAPATH)) = '\0';
			(void)close(project);
			return(0);
		}
	}
	if (i == -1) {
		(void)close(project);
		return(1);
	}
	ps = svalue(ADAPATH);
	aps = svalue(ATAGS);
	if (*aps == '\0') {
		strcpy(aps, "atags ");
		aps += 6;
		while ((read(project, ps, 1) == 1) && (*ps != '\n')) {
			*aps = *ps;
			if (aps+7 - svalue(ATAGS) >= ONMSZ) {
				/*
				 * behavior consistent with other
				 * string variable overflows: zero
				 * out the overflowed strings.
				 */
				*svalue(ATAGS) = '\0';
				*svalue(ADAPATH) = '\0';
				merror("ada.lib path too long\n");
				(void)close(project);
				return(1);
			}		
			if((*aps == ' ') || (*aps == '\t')) {
				*aps = '\0';
				strcat(aps, "/atags ");
				aps += 7;
			} else 
				aps++;
			ps++;
		}
		*aps = '\0';
		strcat(aps, "/atags");
	} else {
		while ((read(project, ps, 1) == 1) && (*ps != '\n')) {
			if (ps+1 - svalue(ADAPATH) >= ONMSZ) {
				*svalue(ADAPATH) = '\0';
				merror("ada.lib path too long\n");
				(void)close(project);
				return(1);
			}		
			ps++;
		}
	}
	*ps = '\0';
	(void)close(project);
	return(0);
}

/*
 * Initialization, before editing a new file.
 * Main thing here is to get a new buffer (in fileinit),
 * rest is peripheral state resetting.
 */
init()
{
	register int i;

	fileinit();
	dot = zero = truedol = unddol = dol = fendcore;
	one = zero+1;
	undkind = UNDNONE;
	chng = 0;
	edited = 0;
	for (i = 0; i <= 'z'-'a'+1; i++)
		names[i] = 1;
	anymarks = 0;
#ifdef CRYPT
        if(xflag) {
                xtflag = 1;
                makekey(key, tperm);
        }
#endif
}

/*
 * Return last component of unix path name p.
 */
char *
tailpath(p)
register char *p;
{
	register char *r;

	for (r=p; *p; p++)
		if (*p == '/')
			r = p+1;
	return(r);
}

/*
 * Check ownership of file.  Return nonzero if it exists and is owned by the
 * user or the option sourceany is used
 */
iownit(file)
char *file;
{
	struct stat sb;

	if (stat(file, &sb) == 0 && (value(SOURCEANY) || sb.st_uid == getuid()))
		return(1);
	else
		return(0);
}
