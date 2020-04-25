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
static char rcsid[] = "$Header: biod.c 1.2 87/05/26 $";
#endif

#ifndef lint
static  char sccsid[] = "@(#)biod.c 1.1 85/05/30 Copyr 1983 Sun Micro";
#endif

#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

int badsys();

/*
 * This is the NFS asynchronous block I/O daemon
 */

main(argc, argv)
	int argc;
	char *argv[];
{
	extern int errno;
	int pid;
	int count;

	if (argc > 2) {
		usage(argv[0]);
	}

	if (argc == 2) {
		count = atoi(argv[1]);
		if (count < 0) {
			usage(argv[0]);
		}
	} else {
		count = 1;
	}

	signal(SIGSYS, badsys);
	{ int tt = open("/dev/tty", O_RDWR);
		if (tt > 0) {
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
	}
	while (count--) {
		pid = fork();
		if (pid == 0) {
			async_daemon();		/* Should never return */
			fprintf(stderr, "%s: async_daemon ", argv[0]);
			perror("");
			exit(1);
		}
		if (pid < 0) {
			fprintf(stderr, "%s: cannot fork", argv[0]);
			perror("");
			exit(1);
		}
	}
}

usage(name)
	char	*name;
{

	fprintf(stderr, "usage: %s [<count>]\n", name);
	exit(1);
}

#define COMPLAINT "biod: bad system call, NFS needs to be defined in the kernel\n"

badsys()
{
	int fd;

	if ((fd = open("/dev/console", O_WRONLY)) < 0)
		exit(1);

	write(fd, COMPLAINT, sizeof(COMPLAINT));
	exit(1);
}
