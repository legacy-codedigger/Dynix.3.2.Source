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
static char rcsid[] = "$Header: lock.c 2.0 86/01/28 $";
#endif

/*
 * Lock a terminal up until the knowledgeable Joe returns.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sgtty.h>

struct	sgttyb tty, ntty;
char	s[BUFSIZ], s1[BUFSIZ];

main(argc, argv)
	char **argv;
{
	register int t;
	struct stat statb;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	if (argc > 0)
		argv[0] = 0;
	if (ioctl(0, TIOCGETP, &tty))
		exit(1);
	ntty = tty; ntty.sg_flags &= ~ECHO;
	ioctl(0, TIOCSETN, &ntty);
	printf("Key: ");
	fgets(s, sizeof s, stdin);
	printf("\nAgain: ");
	fgets(s1, sizeof s1, stdin);
	putchar('\n');
	if (strcmp(s1, s)) {
		putchar(07);
		stty(0, &tty);
		exit(1);
	}
	printf("LOCKED\n");
	s[0] = 0;
	for (;;) {
		fgets(s, sizeof s, stdin);
		if (strcmp(s1, s) == 0)
			break;
		putchar(07);
		if (ioctl(0, TIOCGETP, &ntty))
			exit(1);
	}
	ioctl(0, TIOCSETN, &tty);
}
