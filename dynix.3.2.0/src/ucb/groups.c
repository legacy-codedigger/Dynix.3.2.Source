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
static char rcsid[] = "$Header: groups.c 2.0 86/01/28 $";
#endif

/*
 * groups
 */

#include <sys/param.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>

int	groups[NGROUPS];

main(argc, argv)
	int argc;
	char *argv[];
{
	int ngroups, i;
	char *sep = "";
	struct group *gr;

	if (argc > 1)
		showgroups(argv[1]);
	ngroups = getgroups(NGROUPS, groups);
	for (i = 0; i < ngroups; i++) {
		gr = getgrgid(groups[i]);
		if (gr == NULL)
			printf("%s%d", sep, groups[i]);
		else
			printf("%s%s", sep, gr->gr_name);
		sep = " ";
	}
	printf("\n");
	exit(0);
}

showgroups(user)
	register char *user;
{
	register struct group *gr;
	register struct passwd *pw;
	register char **cp;
	char *sep = "";

	if ((pw = getpwnam(user)) == NULL) {
		fprintf(stderr, "No such user\n");
		exit(1);
	}
	while (gr = getgrent()) {
		if (pw->pw_gid == gr->gr_gid) {
			printf("%s%s", sep, gr->gr_name);
			sep = " ";
			continue;
		}	
		for (cp = gr->gr_mem; cp && *cp; cp++)
			if (strcmp(*cp, user) == 0) {
				printf("%s%s", sep, gr->gr_name);
				sep = " ";
				break;
			}
	}
	printf("\n");
	exit(0);
}
