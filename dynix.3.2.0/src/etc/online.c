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
static char rcsid[] = "$Header: online.c 2.2 87/04/12 $";
#endif lint

/*
 * online.c -- bring processors online and offline
 *
 * Usage:
 *	online [-a] [-v] [processor-number ...]
 *	online -N count number of online procs, print the number
 *	online -S give list of procs online
 *	offline [-a] [-v] [processor-number ...]
 *
 * Flags:
 *	-a	bring all processors online or all but one offline.
 *	-v 	talk about what's happening.
 *
 * 'online' brings processors online.  if it is invoked as 'offline'
 * it brings processors offline.
 *
 * $Log:	online.c,v $
 */

#include <stdio.h>
#include <errno.h>
#include <sys/tmp_ctl.h>

#define	TRUE	1
#define FALSE	0

int offline = FALSE;		/* true if invoked as "offline" */
int verbose_flag = FALSE;	/* true if invoked with -v */
int all_flag = FALSE;		/* true if invoked with -a */
int N_flag = FALSE;		/* true if invoked with -N */
int S_flag = FALSE;		/* true if invoked with -S */
unsigned int n_engine;		/* number of processors in the system */
char *myname;			/* name program is invoked as */

char *rindex();
unsigned int tmp_ctl();
extern int errno;

main(argc, argv)
int argc;
char **argv;
{
	register int i, non;
	/*
	 * invoked as "offline"?
	 */

	myname = rindex(argv[0], '/');
	if (myname)
		myname++;
	else
		myname = argv[0];
	if (!strcmp(myname, "offline")) 
		offline = 1;
	argv++;
	argc--;

	/*
	 * find out how many processors are in the system
	 */

	n_engine = tmp_ctl(TMP_NENG, 0);

	/*
	 * first, scan command line for flags and gross errors.
	 */

	if (get_flags(argc, argv) == -1) {
		fprintf(stderr,
			"Usage: %s [-a] [-v] [processor-number ...]\n       online -N\n       online -S\n", myname);
		exit(1);
	}
	if( N_flag ) {
		for (non = i = 0; i < n_engine; i++)
			if (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE)
				non++;
		printf("%d\n", non);
		exit(0);
	}
	if( S_flag ) {
		for (i = 0; i < n_engine; i++)
			if (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE)
				printf(" %d", i);
		printf("\n");
		exit(0);
	}

	/*
	 * go through the command line and on/off line all the
	 * explicitly specified processors. 
	 */

	while( argc ) {
		if (**argv != '-')
			on_or_off( atoi(*argv) );
		argv++;
		argc--;
	}

	/*
	 * if -a flag, online or offline them all.
	 */

	if (all_flag)
		all();

	show();

	exit(0);
}

/*
 * get_flags looks at the flags and makes sure anything that isn't
 * a flag is a decimal integer.
 */

get_flags(argc, argv)
int argc;
char **argv;
{
	char *cp;
	int n = argc;

	while( argc ) {
		cp = *argv;
		if (*cp == '-') {
			while (*++cp != '\0') {
				switch( *cp ) {

				case 'a':
					all_flag = TRUE;
					break;

				case 'v':
					verbose_flag = TRUE;
					break;
				
				case 'N':
					if( offline || ( n != 1 ) ) {
						fprintf(stderr, "%s: bad use of -N flag\n", myname);
						return( -1);
					}
					N_flag = TRUE;
					break;

				case 'S':
					if( offline || ( n != 1 ) ) {
						fprintf(stderr, "%s: bad use of -S flag\n", myname);
						return( -1);
					}
					S_flag = TRUE;
					break;

				default:
					fprintf(stderr, "%s: Unknown option: -%c\n", myname, *cp);
					return( -1 );

				}
			}
		} else {
			/*
			 * make sure anything not starting with a '-'
			 * is a decimal integer
			 */
			for (cp = *argv; *cp != '\0'; cp++)
				if (*cp < '0' || *cp > '9') {
					fprintf(stderr, "%s: `%s' is not a processor number\n", myname, *argv);
				return( -1 );
			}
		}
		argv++;
		argc--;
	}
	return( 0 );
}

/*
 * all brings all the processors online or offline
 */

all()
{
	unsigned int first;	/* first online processor */
	unsigned int i;

	if (offline) {
		/*
		 * don't offline first online processor
		 */
		for (first = 0; first < n_engine; first++)
			if (tmp_ctl(TMP_QUERY, first) == TMP_ENG_ONLINE)
				break;
		if (first >= n_engine) {			/* sanity */
			fprintf(stderr, "%s: no processors online!\n", myname);
			exit(2);
		}
		/*
		 * offline the rest
		 */
		for (i = first+1; i < n_engine; i++) {
			if (tmp_ctl(TMP_QUERY, i)==TMP_ENG_ONLINE || verbose_flag)
				on_or_off( i );
		}
	} else {
		/*
		 * online all processors
		 */
		for (i = 0; i < n_engine; i++) {
			if (tmp_ctl(TMP_QUERY,i)==TMP_ENG_OFFLINE || verbose_flag)
				on_or_off( i );
		}
	}
}

/*
 * on_or_off brings a particular processor online or offline.
 */

on_or_off( engine )
unsigned int engine;
{
	unsigned int status;

	if (verbose_flag) {
		printf("processor %d: ", engine);
		fflush(stdout);
	}

	/*
	 * change the state of the processor
	 */

	status = tmp_ctl(offline ? TMP_OFFLINE : TMP_ONLINE, engine);

	if (status) {
		if ( !verbose_flag )
			fprintf(stderr, "processor %d: ", engine);
		switch( errno ) {
		case ENXIO:
			fprintf(stderr, "not that many processors in system\n");
			break;
		case EINVAL:
			fprintf(stderr, "already %sline\n", offline ? "off" : "on");
			break;
		case ENODEV:
			fprintf(stderr, "not configured\n");
			break;
		case EINTR:
			fprintf(stderr, "system call interrupted\n");
			break;
		case EPERM:
			fprintf(stderr, "must be super user\n");
			break;
		case EBUSY:
			fprintf(stderr, "last processor or has process bound to it\n");
			break;
		default:
			fprintf(stderr, "tmp_ctl: unknown return code %d\n", errno);
		}
	} else if (verbose_flag)
		printf("%s\n", offline ? "offline" : "online");
}

/*
 * show the state of each processor in the system
 */

show()
{
	register int i, flg;
	register rflg;

	printf(" online:");
	rflg = -1;
	for (i = 0; i <= n_engine; i++) {
		if( tmp_ctl(TMP_QUERY,i) == TMP_ENG_ONLINE ) {
			if( rflg == -1 )
				rflg = i;
		} else {
			if( rflg != -1 )
				if( rflg == i-1 )
					printf(" %d", rflg);
				else
					printf(" %d-%d", rflg, i-1);
			rflg = -1;
		}
	}
	printf("\n");

	rflg = -1;
	for (flg = i = 0; i <= n_engine; i++) {
		if( tmp_ctl(TMP_QUERY,i) == TMP_ENG_OFFLINE ) {
			if( !flg++ )
				printf("offline:");
			if( rflg == -1 )
				rflg = i;
		} else {
			if( rflg != -1 )
				if( rflg == i-1 )
					printf(" %d", rflg);
				else
					printf(" %d-%d", rflg, i-1);
			rflg = -1;
		}
	}
	if( flg )
		printf("\n");
}
