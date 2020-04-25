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

#if !defined(lint)
static char rcsid[] = "$Id: main.c,v 1.1 88/09/02 11:44:19 ksb Exp $";
#endif lint

/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"

/*
 * This program is described in detail in the "PXP 1.0 Implementation Notes"
 *
 * The structure of pxp is very similar to that of the translator pi.
 * The major new pieces here are a set of profile data maintenance
 * routines in the file pmon.c and a set of pretty printing utility
 * routines in the file pp.c.
 * The semantic routines of pi have been rewritten to do a simple
 * reformatting tree walk, the parsing and scanning remains
 * the same.
 *
 * This version does not place more than one statement per line and
 * is not very intelligent about folding long lines, with only
 * an ad hoc way of folding case label list and enumerated type
 * declarations being implemented.
 */

char	usagestr[] =
	"pxp [ -acdefjntuw_ ] [ -23456789 ] [ -z [ name ... ] ] name.p";
char	*howfile =	"/usr/lib/how_pxp";
char	*stdoutn =	"Standard output";

int	unit =	4;

FILE	*ibuf;
extern	char errout;

/*
 * Main program for pxp.
 * Process options, then call yymain
 * to do all the real work.
 */
FILE *ibp;
main(argc, argv)
	int argc;
	char **argv;
{
	register char *cp;
	register int c;

	if (argv[0][0] == 'a')
		howfile += 9;
	argc--, argv++;
	if (argc == 0) {
		execl("/bin/cat", "cat", howfile, 0);
		goto usage;
	}
	while (argc > 0) {
		cp = argv[0];
		if (*cp++ != '-')
			break;
		while (c = *cp++) switch (c) {
#if defined(DEBUG)
			case 'T':
				typetest++;
				continue;
			case 'A':
				testtrace++;
			case 'F':
				fulltrace++;
			case 'E':
				errtrace++;
				continue;
			case 'C':
				yycosts();
				pexit(NOSTART);
			case 'U':
				yyunique++;
				continue;
#endif
			case 'a':
				all++;
				continue;
			case 'c':
				core++;
				continue;
			case 'd':
				nodecl++;
				continue;
			case 'e':
				noinclude = -1;
				continue;
			case 'f':
				full++;
				continue;
			case 'j':
				justify++;
				continue;
			case 'l':
			case 'n':
				togopt(c);
				continue;
			case 'o':
				onefile++;
				continue;
			case 's':
				stripcomm++;
				continue;
			case 't':
				table++;
				continue;
			case 'u':
			case 'w':
				togopt(c);
				continue;
			case 'z':
				profile++;
				pflist = argv + 1;
				pflstc = 0;
				while (argc > 1) {
					if (dotted(argv[1], 'p'))
						break;
					pflstc++, argc--, argv++;
				}
				if (pflstc == 0)
					togopt(c);
				else
					nojunk++;
				continue;
			case '_':
				underline++;
				continue;
#if defined(RMOTHERS)
			case 'O':
				rmothers++;
				continue;
#endif RMOTHERS
			default:
				if (c >= '2' && c <= '9') {
					unit = c - '0';
					continue;
				}
usage:
				Perror("Usage", usagestr);
				exit(1);
		}
		argc--, argv++;
	}
	if (core && !profile && !table)
		profile++;
	if (argc == 0 || argc > 2)
		goto usage;
	if (profile || table) {
		noinclude = 0;
		if (argc == 2) {
			argc--;
			getit(argv[1]);
		} else
			getit(core ? "core" : "pmon.out");
	} else
		noinclude++;
	if (argc != 1)
		goto usage;
	firstname = filename = argv[0];
	if (dotted(filename, 'i')) {
		if (profile || table)
			goto usage;
		noinclude = 1;
		bracket++;
	} else if (!dotted(filename, 'p')) {
		Perror(filename, "Name must end in '.p'");
		exit(1);
	}
	if ((ibuf = fopen(filename, "r")) == NULL)
		perror(filename), pexit(NOSTART);
	ibp = ibuf;
	if (onefile) {
		int onintr();

		cp = (stdoutn = "/tmp/pxp00000") + 13;
		signal(2, onintr);
		for (c = getpid(); c; c /= 10)
			*--cp |= (c % 10);
		if (freopen(stdoutn, "w", stdout) == NULL)
bad:
			perror(stdoutn), exit(1);
	} else {
		extern char _sobuf[BUFSIZ];
		setbuf(stdout, _sobuf);
	}
	if (profile || opt('l')) {
		opt('n')++;
		yysetfile(filename);
		opt('n')--;
	} else
		lastname = filename;
	errout = 2;
	yymain();
	/* No return */
}

/*
 * Put a header on a top of a page
 */
header()
{
	extern char version[];
	static char reenter;
	extern int outcol;

	gettime(filename);
	if (reenter) {
		if (outcol)
			fputc('\n',stderr);
		fputc('\f',stderr);
	}
	reenter++;
	if (profile || table) {
		fprintf(stderr,"Berkeley Pascal PXP -- Version %s\n\n%s  %s\n\n",
			version, myctime(&tvec), filename);
		fprintf(stderr,"Profiled %s\n\n", myctime(&ptvec));
	}
}

char	ugh[] =	"Fatal error in pxp\n";
/*
 * Exit from the Pascal system.
 * We throw in an ungraceful termination
 * message if c > 1 indicating a severe
 * error such as running out of memory
 * or an internal inconsistency.
 */
pexit(c)
	int c;
{
	register char *cp;
	extern int outcol;

	if (stdoutn[0] == '/')
		unlink(stdoutn);
	if (outcol)
		fputc('\n',stderr);
	flush();
	if (c == DIED)
		write(2, ugh, sizeof ugh);
	exit(c);
}

onintr()
{

	pexit(DIED);
}

puthedr()
{

	yysetfile(filename);
}
