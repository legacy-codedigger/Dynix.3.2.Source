/* @(#)$Copyright:	$
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

/* @(#)$Header: lock.c 1.4 87/05/28 $ */

#include <stdio.h>
#include <sys/file.h>
#include "host.h"

static char lock[50] = "/usr/spool/uucp/LCK/LCK..";

/*
 * attempt to lock the port, from "uucp" and "tip"
 */
setlock()
{
	register int fd;
	register char *p;

	char *rindex();

	if (p = rindex(portname, '/')) {
		strcat(lock, ++p);
		if ((fd = open(lock, O_WRONLY | O_CREAT | O_EXCL, 0440)) < 0) {
			printf("%s: port busy\n", myname);
			exit(1);
		}
		write(fd, myname, strlen(myname));
		close(fd);
	} else {
		printf("%s: invalid port: %s\n", myname, portname);
		exit(1);
	}
}

/*
 * get rid of the lock file
 */
unlock()
{
	unlink(lock);
}
