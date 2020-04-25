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
static char rcsid[] = "$Id: main.c,v 1.1 88/09/02 11:48:09 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree_ty.h"		/* must be included for yy.h */
#include "yy.h"
#include <signal.h>
#include "objfmt.h"
#include "config.h"

/*
 * This version of pi has been in use at Berkeley since May 1977
 * and is very stable. Please report any problems with the error
 * recovery to the second author at the address given in the file
 * READ_ME.  The second author takes full responsibility for any bugs
 * in the syntactic error recovery.
 */

char	piusage[]	= "pi [ -blnpstuw ] [ -i file ... ] name.p";

char	*usageis	= piusage;

#if defined(PC)

char	*pcname = "pc.pc0";
char	pcusage[]	= "pc [ options ] [ -o file ] [ -i file ... ] name.p";
FILE	*pcstream = NULL;

#endif PC
#if defined(PTREE)
    char	*pTreeName = "pi.pTree";
#endif PTREE

int	onintr();

extern	char *lastname;

FILE	*ibuf;

/*
 * these are made real variables
 * so they can be changed
 * if you are compiling on a smaller machine
 */
double	MAXINT	=  2147483647.;
double	MININT	= -2147483648.;

/*
 * Main program for pi.
 * Process options, then call yymain
 * to do all the real work.
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;
	register c;
	FILE *fopen();
	extern char *myctime();
	extern long lseek();
	int i;

	if (argv[0][0] == 'a')
		err_file += err_pathlen , how_file += how_pathlen;
	argv++, argc--;
	if (argc == 0) {
		i = fork();
		if (i == -1)
			goto usage;
		if (i == 0) {
			execl("/bin/cat", "cat", how_file, 0);
			goto usage;
		}
		while (wait(&i) != -1)
			continue;
		pexit(NOSTART);
	}
#if defined(PC)
	    opt( 'b' ) = 1;
	    opt( 'g' ) = 0;
	    opt( 't' ) = 0;
	    opt( 'p' ) = 0;
	    usageis = pcusage;
	    while ( argc > 0 ) {
		cp = argv[0];
		if ( *cp++ != '-' ) {
		    break;
		}
		c = *cp++;
		switch( c ) {
#if defined(DEBUG)
		    case 'k':
		    case 'r':
		    case 'y':
			    togopt(c);
			    break;
		    case 'K':
			    yycosts();
			    pexit(NOSTART);
		    case 'A':
			    testtrace = TRUE;
			    /* and fall through */
		    case 'F':
			    fulltrace = TRUE;
			    /* and fall through */
		    case 'E':
			    errtrace = TRUE;
			    opt('r')++;
			    break;
		    case 'U':
			    yyunique = FALSE;
			    break;
#endif
		    case 'b':
			    opt('b') = 2;
			    break;
		    case 'i':
			    pflist = argv + 1;
			    pflstc = 0;
			    while (argc > 1) {
				    if (dotted(argv[1], 'p'))
					    break;
				    pflstc++, argc--, argv++;
			    }
			    if (pflstc == 0)
				    goto usage;
			    break;
			/*
			 *	output file for the first pass
			 */
		    case 'o':
			    if ( argc < 2 ) {
				goto usage;
			    }
			    argv++;
			    argc--;
			    pcname = argv[0];
			    break;	
		    case 'C':
				/*
				 * since -t is an ld switch, use -C
				 * to turn on tests
				 */
			    togopt( 't' );
			    break;
		    case 'g':
				/*
				 *	sdb symbol table
				 */
			    togopt( 'g' );
			    break;
		    case 'l':
		    case 's':
		    case 'u':
		    case 'w':
			    togopt(c);
			    break;
		    case 'p':
				/*
				 *	-p on the command line means profile
				 */
			    profflag = TRUE;
			    break;
		    case 'z':
			    monflg = TRUE;
			    break;
		    case 'L':
			    togopt( 'L' );
			    break;
		    default:
usage:
			    Perror( "Usage", usageis);
			    pexit(NOSTART);
		}
		argc--;
		argv++;
	    }
#endif PC
	if (argc != 1)
		goto usage;
	efil = open ( err_file, 0 );
	if ( efil < 0 )
		perror(err_file), pexit(NOSTART);
	filename = argv[0];
	if (!dotted(filename, 'p')) {
		Perror(filename, "Name must end in '.p'");
		pexit(NOSTART);
	}
	close(0);
	if ( ( ibuf = fopen( filename , "r" ) ) == NULL )
		perror(filename), pexit(NOSTART);
	ibp = ibuf;
#if defined(PC)
	    if ( ( pcstream = fopen( pcname , "w" ) ) == NULL ) {
		perror( pcname );
		pexit( NOSTART );
	    }
	    stabsource( filename, TRUE );
#endif PC
#if defined(PTREE)
#define	MAXpPAGES	16
	    if ( ! pCreate( pTreeName , MAXpPAGES ) ) {
		perror( pTreeName );
		pexit( NOSTART );
	    }
#endif PTREE
	if ( signal( SIGINT , SIG_IGN ) != SIG_IGN )
		(void) signal( SIGINT , onintr );
	if (opt('l')) {
		opt('n')++;
		yysetfile(filename);
		opt('n')--;
	}
	yymain();
	/* No return */
}

pchr(c)
	char c;
{

	putc ( c , stdout );
}

#if defined(PC)
char	ugh[]	= "Fatal error in pc\n";
#endif
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

	if (opt('l') && c != DIED && c != NOSTART)
		while (getline() != -1)
			continue;
	yyflush();
	switch (c) {
		case DIED:
			write(2, ugh, sizeof ugh);
		case NOSTART:
		case ERRS:
#if defined(PC)
			    if ( pcstream != NULL ) {
				unlink( pcname );
			    }
#endif PC
			break;
		case AOK:
#if defined(PC)
			    puteof();
#endif PC
			break;
	}
	/*
	 *	this to gather statistics on programs being compiled
	 *	taken 20 june 79 	... peter
	 *
	 *  if (fork() == 0) {
	 *  	char *cp = "-0";
	 *  	cp[1] += c;
	 *  	execl("/usr/lib/gather", "gather", cp, filename, 0);
	 *  	exit(1);
	 *  }
	 */
#if defined(PTREE)
	    pFinish();
#endif
	exit(c);
}

onintr()
{

	(void) signal( SIGINT , SIG_IGN );
	pexit(NOSTART);
}

/*
 * Get an error message from the error message file
 */
geterr(seekpt, buf)
	int seekpt;
	char *buf;
{

	(void) lseek(efil, (long) seekpt, 0);
	if (read(efil, buf, 256) <= 0)
		perror(err_file), pexit(DIED);
}

header()
{
	extern char *version;
	static char anyheaders;

	gettime( filename );
	if (anyheaders && opt('n'))
		putc( '\f' , stdout );
	anyheaders++;
#if defined(PC)
	    printf("Berkeley Pascal PC -- Version %s\n\n%s  %s\n\n",
		    version, myctime((int *) (&tvec)), filename);
#endif PC
}
