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
static char rcsid[] = "$Header: chgrp.c 2.1 89/06/26 $";
#endif

/*
 * chgrp gid file ...
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>

struct	group *gr, *getgrnam(), *getgrgid();
struct	passwd *getpwuid(), *pwd;
struct	stat stbuf;
int	gid, uid;
int	status;
int	fflag;
/* VARARGS */
int	fprintf();

main(argc, argv)
	int argc;
	char *argv[];
{
	register c, i;

	argc--, argv++;
	if (argc > 0 && strcmp(argv[0], "-f") == 0) {
		fflag++;
		argv++, argc--;
	}
	if (argc < 2) {
		printf("usage: chgrp [-f] gid file ...\n");
		exit(2);
	}
	uid = getuid();
	if (isnumber(argv[0])) {
		gid = atoi(argv[0]);
		gr = getgrgid(gid);
		if (uid && gr == NULL) {
			printf("%s: unknown group\n", argv[0]);
			exit(2);
		}
	} else {
		gr = getgrnam(argv[0]);
		if (gr == NULL) {
			printf("%s: unknown group\n", argv[0]);
			exit(2);
		}
		gid = gr->gr_gid;
	}
	pwd = getpwuid(uid);
	if (pwd == NULL) {
		fprintf(stderr, "Who are you?\n");
		exit(2);
	}
	if (uid && pwd->pw_gid != gid) {
		for (i=0; gr->gr_mem[i]; i++)
			if (!(strcmp(pwd->pw_name, gr->gr_mem[i])))
				goto ok;
		if (fflag)
			exit(0);
		fprintf(stderr, "You are not a member of the %s group.\n",
		    argv[0]);
		exit(2);
	}
ok:
	for (c = 1; c < argc; c++) {
		if (lstat(argv[c], &stbuf)) {
			perror(argv[c]);
			continue;
		}
		if (uid && uid != stbuf.st_uid) {
			if (fflag)
				continue;
			fprintf(stderr, "You are not the owner of %s\n",
			    argv[c]);
			status = 1;
			continue;
		}
		if (chown(argv[c], -1, gid) && !fflag)
			perror(argv[c]);
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register int c;

	while (c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}
