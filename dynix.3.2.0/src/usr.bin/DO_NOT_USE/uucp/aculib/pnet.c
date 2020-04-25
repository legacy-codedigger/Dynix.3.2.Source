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
static char rcsid[] = "$Header: pnet.c 1.2 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)pnet.c	4.2 (Berkeley) 6/23/85";
#endif

#include "../condevs.h"
#ifdef PNET

/***
 *	pnetopn(flds)
 *
 *	call remote machine via Purdue network
 *	use dial string as host name, speed as socket number
 *	- Steve Bellovin
 */
pnetopn(flds)
char *flds[];
{
	int fd;
	int socket;
	register char *cp;

	fd = pnetfile();
	DEBUG(4, "pnet fd - %d\n", fd);
	if (fd < 0) {
		logent("AVAILABLE DEVICE", "NO");
		return CF_NODEV;
	}
	socket = 0;
	for (cp = flds[F_CLASS]; *cp; cp++)
		socket = 10*socket + (*cp - '0');
	DEBUG(4, "socket - %d\n", socket);
	if (setjmp(Sjbuf)) {
		DEBUG(4, "pnet timeout  - %s\n", flds[F_PHONE]);
		return CF_DIAL;
	}
	signal(SIGALRM, alarmtr);
	DEBUG(4, "host - %s\n", flds[F_PHONE]);
	alarm(15);
	if (pnetscon(fd, flds[F_PHONE], socket) < 0) {
		DEBUG(4, "pnet connect failed - %s\n", flds[F_PHONE]);
		alarm(0);
		return CF_DIAL;
	}
	alarm(0);
	return fd;
}
#endif	PNET
