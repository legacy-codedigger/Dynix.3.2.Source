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

#ifndef	lint
static char rcsid[] = "$Header: uulog.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)uulog.c	5.4 (Berkeley) 6/23/85";
#endif

#include "uucp.h"

#ifndef	SYSBUF
static char SYSBUF[BUFSIZ];
#endif

main(argc, argv)
char *argv[];
{
	FILE *plogf;
	char *sys, *user;

	char buf[BUFSIZ], u[64], s[64];

	setbuf(stdout, SYSBUF);
	strcpy(Progname, "uulog");
	sys = user = NULL;


	while (argc>1 && argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 's':
			sys = &argv[1][2];
			if (*sys == NULL && argc > 2 && argv[2][0] != '-') {
				sys = &argv[2][0];
				argv++;
				argc--;
			}
			if (strlen(sys) > MAXBASENAME)
				sys[MAXBASENAME] = '\0';
			if (versys(&sys) != SUCCESS){
				fprintf(stderr,"uulog: unknown system %s\n", sys);
				sys = NULL;
			}
			break;
		case 'u':
			user = &argv[1][2];
			if (*user == NULL && argc > 2 && argv[2][0] != '-') {
				user = &argv[2][0];
				argv++;
				argc--;
			}
			break;
		default:
			printf("unknown flag %s\n", argv[1]); break;
		}
		--argc;  argv++;
	}


	if (user == NULL && sys == NULL) {
		fprintf(stderr, "usage: uulog [-u user] [-s sys]\n");
		exit(1);
	}

#ifdef LOGBYSITE
	if (chdir(SPOOL) < 0) {
		perror(SPOOL);
		exit(1);
	}
	/* this program is really obsolete, this is a rude backward compat */
	if (user) {
		sprintf(buf, "exec cat LOG/uu*/* | egrep '^%s '", user);
		system(buf);
	}
	if (sys) {
		sprintf(buf,"exec cat LOG/uu*/%s", sys);
		system(buf);
	}
#else !LOGBYSITE
	plogf = fopen(LOGFILE, "r");
	ASSERT(plogf != NULL, "CAN NOT OPEN", LOGFILE, 0);
	while (fgets(buf, BUFSIZ, plogf) != NULL) {
		sscanf(buf, "%s%s", u, s);
		if (user != NULL && !prefix(user, u))
			continue;
		if (sys != NULL && !prefix(sys, s))
			continue;
		fputs(buf, stdout);
		fflush(stdout);
	}
#endif !LOGBYSITE
	exit(0);
}

cleanup(code)
int code;
{
	exit(code);
}
