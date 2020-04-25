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

/* $Header: add_utmp.c 1.2 88/05/20 $ */

#include <sys/types.h>
#include <sys/file.h>
#include <utmp.h>
#include <stdio.h>

#define	UTMP_FILE "/etc/utmp"

/*
 * add_utmp
 *	Add utmp entry to utmp file
 *	Return 0 for success, -1 for error or usr limit reached
 */
add_utmp(slot, up)
	int	slot;
	struct	utmp *up;
{
	int	fd;

	if ((fd = open(UTMP_FILE, O_RDWR)) < 0) {
		perror(UTMP_FILE);
		return (-1);
	}
	flock(fd, LOCK_EX);
	if (getnumusers(fd, up) > getmaxusers()) {
		flock(fd, LOCK_UN);
		close(fd);
		fprintf(stderr, "Login limit reached.\nTry again later.\n");
		return(-1);
	}
	lseek(fd, (off_t)(slot*sizeof(struct utmp)), 0);
	write(fd, (char *)up, sizeof(struct utmp));
	flock(fd, LOCK_UN);
	close(fd);
	return (0);
}
