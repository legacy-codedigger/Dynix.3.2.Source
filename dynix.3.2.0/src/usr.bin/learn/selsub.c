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
static char rcsid[] = "$Header: selsub.c 2.1 1991/05/16 21:26:19 $";
#endif

#include "stdio.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "lrnref.h"

selsub(argc,argv)
char *argv[];
{
	char ans1[100];
	static char ans2[30];
	static char dirname[20];
	static char subname[20];
	struct stat statbuf;

	if (argc > 1 && argv[1][0] == '-') {
		direct = argv[1]+1;
		argc--;
		argv++;
	}
	if (chdir(direct) != 0) {
		perror(direct);
		fprintf(stderr, "Selsub:  couldn't cd to non-standard directory\n");
		exit(1);
	}
	sname = argc > 1 ? argv[1] : 0;
	if (argc > 2) {
		strcpy (level=ans2, argv[2]);
		if (strcmp(level, "-") == 0)	/* no lesson name is - */
			ask = 1;
		else
			again = 1;	/* treat as if "again lesson" */
	}
	else
		level = 0;
	if (argc > 3 )
		speed = atoi(argv[3]);
	if (!sname) {
		printf("These are the available courses -\n");
		list("Linfo");
		printf("If you want more information about the courses,\n");
		printf("or if you have never used 'learn' before,\n");
		printf("press RETURN; otherwise type the name of\n");
		printf("the course you want, followed by RETURN.\n");
		fflush(stdout);
		gets(sname=subname);
		if (sname[0] == '\0') {
			list("Xinfo");
			do {
				printf("\nWhich subject?  ");
				fflush(stdout);
				gets(sname=subname);
			} while (sname[0] == '\0');
		}
	}
	chknam(sname);
	stat(sname, &statbuf);
	total = statbuf.st_size / 16 - 2;	/* size/dirsize-(.+..) */
	if (!level) {
		printf("If you were in the middle of this subject\n");
		printf("and want to start where you left off, type\n");
		printf("the last lesson number the computer printed.\n");
		printf("If you don't know the number, type in a word\n");
		printf("you think might appear in the lesson you want,\n");
		printf("and I will look for the first lesson containing it.\n");
		printf("To start at the beginning, just hit RETURN.\n");
		fflush(stdout);
		gets(ans2);
		if (ans2[0]==0)
			strcpy(ans2,"0");
		level=ans2;
	}

	/* make new directory for user to play in */
	if (chdir("/tmp") != 0) {
		perror("/tmp");
		fprintf(stderr, "Selsub:  couldn't cd to public directory\n");
		exit(1);
	}
	(void) sprintf(dir=dirname, "pl%da", getpid());
	(void) sprintf(ans1, "mkdir %s", dir);
	system(ans1);
	if (chdir(dir) < 0) {
		perror(dir);
		fprintf(stderr, "Selsub:  couldn't make play directory with %s.\nBye.\n", ans1);
		exit(1);
	}
	/* after this point, we have a working directory. */
	/* have to call wrapup to clean up */
	(void) sprintf(ans1, "%s/%s/Init", direct, sname);
	if (access(ans1, 04)==0) {
		(void) sprintf(ans1, "%s/%s/Init %s", direct, sname, level);
		if (system(ans1) != 0) {
			printf("Leaving learn.\n");
			wrapup(1);
		}
	}
}

chknam(name)
char *name;
{
	if (access(name, 05) < 0) {
		printf("Sorry, there is no subject or lesson named %s.\nBye.\n", name);
		exit(1);
	}
}
