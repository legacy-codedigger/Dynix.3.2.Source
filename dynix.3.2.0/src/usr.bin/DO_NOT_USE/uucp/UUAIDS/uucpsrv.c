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
static char rcsid[] = "$Header: uucpsrv.c 2.1 87/05/27 $";
#endif

/*
 * UNET (3Com) TCP-IP server for uucico.
 * uucico's UNET channel causes this server to be run at the remote end.
 * An argument, if present, is the local port number.
 * This server does a tcpopen(III) to establish the connection,
 * renames file descriptors 0,1, and 2 to be the UNET connection,
 * and then exec(II)s uucico.
 */

#include <stdio.h>
#include <UNET/unetio.h>
#include <UNET/tcp.h>

/* Default port of uucico server */
#define	DFLTPORT	33

main(argc, argv)
int argc;
char **argv;
{
	register int lport, fd;
	register FILE *fp;
	extern int errno;

	lport = DFLTPORT;
	if (argc >= 2)
		lport = atoi(argv[1]);
	if (lport <= 0 || lport > 255)
		lport = DFLTPORT;

	fd = tcpopen((char *)0, 0, lport, TO_PASSIVE, "rw");
	if (fd == -1) {
		perror("uucico server: tcpopen");
		exit(1);
	}
	close(0); close(1);
	dup(fd); dup(fd);
	execl("/usr/lib/uucp/uucico", "uucico", (char *)0);
	perror("uucico server: execl");
	exit(1);
}
