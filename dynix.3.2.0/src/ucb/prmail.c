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
static char rcsid[] = "$Header: prmail.c 2.0 86/01/28 $";
#endif

#include <pwd.h>
/*
 * prmail
 */
struct	passwd *getpwuid();
char	*getenv();

main(argc, argv)
	int argc;
	char **argv;
{
	register struct passwd *pp;

	--argc, ++argv;
	if (chdir("/usr/spool/mail") < 0) {
		perror("/usr/spool/mail");
		exit(1);
	}
	if (argc == 0) {
		char *user = getenv("USER");
		if (user == 0) {
			pp = getpwuid(getuid());
			if (pp == 0) {
				printf("Who are you?\n");
				exit(1);
			}
			user = pp->pw_name;
		}
		prmail(user, 0);
	} else
		while (--argc >= 0)
			prmail(*argv++, 1);
	exit(0);
}

#include <sys/types.h>
#include <sys/stat.h>

prmail(user, other)
	char *user;
{
	struct stat stb;
	char cmdbuf[256];

	if (stat(user, &stb) < 0) {
		printf("No mail for %s.\n", user);
		return;
	}
	if (access(user, 4) < 0) {
		printf("Mailbox for %s unreadable\n", user);
		return;
	}
	if (other)
		printf(">>> %s <<<\n", user);
	sprintf(cmdbuf, "more %s", user);
	system(cmdbuf);
	if (other)
		printf("-----\n\n");
}
