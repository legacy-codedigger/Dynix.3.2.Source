/* $Copyright: $
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
static char	rcsid[] = "$Header: universe.c 2.4 1991/05/16 20:49:36 $";
#endif

/*
 * Universe command
 *   Called ucb, att, and universe
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/universe.h>
#include <sys/dir.h>
#include <sys/file.h>

#define ERROR 1
#define OK    0

extern char	*rindex();

main(argc, argv, envp)
int	argc;
char	*argv[], *envp[];
{
	char	**pathpp, **shellpp, **p;
	char	*attpathp, *ucbpathp, *ucbshellp, *attshellp, *homep, *myname;
	int	u;

	argv[argc] = 0;
	myname = rindex(argv[0], '/');
	if ( myname != NULL )
		++myname;
	else
		myname = argv[0];

	u = universe(U_GET);
	if ( !strcmp("universe", myname) ) {
		switch ( u ) {
		case -1:	
			perror("universe"); 
			exit(ERROR);
		case U_UCB:	
			printf("ucb\n"); 
			exit(OK);
		case U_ATT:	
			printf("att\n"); 
			exit(OK);
		default:	
			printf("%s: universe: bad return value, %d\n",
			    myname, u);
			exit(ERROR);
		}
	}

	attpathp = ucbpathp = attshellp = ucbshellp = homep = (char *)NULL;
	pathpp = shellpp = (char **)NULL;
	for (p = envp; *p != NULL; p++) {
		if ( !strncmp(*p, "UCBPATH=",  8) ) 
			ucbpathp = *p; 
		else if ( !strncmp(*p, "ATTPATH=",  8) ) 
			attpathp = *p; 
		else if ( !strncmp(*p, "UCBSHELL=", 8) ) 
			ucbshellp = *p; 
		else if ( !strncmp(*p, "ATTSHELL=", 8) ) 
			attshellp = *p; 
		else if ( !strncmp(*p, "PATH=",     5) ) 
			pathpp = p; 
		else if ( !strncmp(*p, "SHELL=",    5) ) 
			shellpp = p;
		else if ( !strncmp(*p, "HOME=",    4) ) 
			homep = *p;
	}

	if ( !strcmp("ucb", myname) ) {
		u = U_UCB;
		if ( pathpp && ucbpathp ) 
			*pathpp =  ucbpathp + 3;
		if ( shellpp && ucbshellp ) 
			*shellpp = ucbshellp + 3;
	} else if ( !strcmp("att", myname) ) {
		u = U_ATT;
		if ( pathpp && attpathp ) 
			*pathpp =  attpathp + 3;
		if ( shellpp && attshellp ) 
			*shellpp = attshellp + 3;
		if ( access("/usr/att", F_OK) < 0 ) {
			fprintf(stderr, 
"att: Error, System V application environment is not installed.\n");
			exit(ERROR);
		}
	} else
		fprintf(stderr, "universe: unknown universe '%s'\n", myname);

	if ( universe(u) < 0 ) {
		perror("universe");
		exit(ERROR);
	}
	if ( argc == 1 ) {
		char	*execname = "/bin/sh", *pgmname, buf[MAXNAMLEN+2];
		if ( shellpp )
			execname = *shellpp + 6;
		pgmname = rindex(execname, '/');
		if ( pgmname != NULL )
			pgmname++;
		else
			pgmname = execname;
		(void) sprintf(buf, "-%s", pgmname);
		argv[0] = buf;
		if( homep ) {
			if( chdir(homep+5) < 0 )
				fprintf(stderr, "%s: couldn't chdir to '%s' ($HOME)\n",
						myname, homep);
		}
		execvp(execname, argv);
		pgmname = (u == U_UCB) ? "UCB" : "ATT";
		fprintf(stderr, 
		    "%s: can't find '%s' ($%sSHELL) in the %s universe, trying '/bin/sh'\n",
		    myname, execname, pgmname, pgmname);
		execl("/bin/sh", "-sh", 0);
		fprintf(stderr, "%s: can't find '/bin/sh'\n", myname);
	} else {
		if (strlen(argv[1]) > 512) {
			fprintf(stderr, "%s: Line too long\n", myname);
			exit(0);
		}
		execvp(argv[1], &argv[1]);
		perror(argv[1]);
	}
	exit(ERROR);
}
