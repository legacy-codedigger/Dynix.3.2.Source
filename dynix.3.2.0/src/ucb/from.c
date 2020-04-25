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
static char rcsid[] = "$Header: from.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>

struct	passwd *getpwuid();

main(argc, argv)
int argc;
register char **argv;
{
	char lbuf[BUFSIZ];
	char lbuf2[BUFSIZ];
	register struct passwd *pp;
	int stashed = 0;
	register char *name;
	char *sender;
	char *getlogin();

	if (argc > 1 && *(argv[1]) == '-' && (*++argv)[1] == 's') {
		if (--argc <= 1) {
			fprintf (stderr, "Usage: from [-s sender] [user]\n");
			exit (1);
		}
		--argc;
		sender = *++argv;
		for (name = sender; *name; name++)
			if (isupper(*name))
				*name = tolower(*name);

	}
	else
		sender = NULL;
	if (chdir("/usr/spool/mail") < 0)
		exit(1);
	if (argc > 1)
		name = argv[1];
	else {
		name = getlogin ();
		if (name == NULL || strlen(name) == 0) {
			pp = getpwuid(getuid());
			if (pp == NULL) {
				fprintf(stderr, "Who are you?\n");
				exit(1);
			}
			name = pp->pw_name;
		}
	}
	if (freopen(name, "r", stdin) == NULL)
		exit(0);
	while(fgets(lbuf, sizeof lbuf, stdin) != NULL)
		if (lbuf[0] == '\n' && stashed) {
			stashed = 0;
			printf("%s", lbuf2);
		}
		else if (bufcmp(lbuf, "From ", 5) &&
		    (sender == NULL || match(&lbuf[4], sender))) {
			strcpy(lbuf2, lbuf);
			stashed = 1;
		}
	if (stashed)
		printf("%s", lbuf2);
	exit(0);
}

bufcmp (b1, b2, n)
register char *b1, *b2;
register int n;
{
	while (n-- > 0)
		if (*b1++ != *b2++)
			return (0);
	return (1);
}

match (line, str)
register char *line, *str;
{
	register char ch;

	while (*line == ' ' || *line == '\t')
		++line;
	if (*line == '\n')
		return (0);
	while (*str && *line != ' ' && *line != '\t' && *line != '\n') {
		ch = isupper(*line) ? tolower(*line) : *line;
		if (ch != *str++)
			return (0);
		line++;
	}
	return (*str == '\0');
}
