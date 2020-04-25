
/* 
 * $Copyright:	$
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

/* 
 * $Header: lockf.c 2.0 86/01/28 $
 */

/*
 * $Log:	lockf.c,v $
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

extern int errno;

lockf(fildes, function, size)
long size;
int fildes, function;
{
	struct flock l;
	int rv;

	l.l_whence = 1;
	if (size < 0) {
		l.l_start = size;
		l.l_len = -size;
	} else {
		l.l_start = 0L;
		l.l_len = size;
	}
	switch (function) {
	case F_ULOCK:
		l.l_type = F_UNLCK;
		rv = fcntl(fildes, F_SETLK, &l);
		break;
	case F_LOCK:
		l.l_type = F_WRLCK;
		rv = fcntl(fildes, F_SETLKW, &l);
		break;
	case F_TLOCK:
		l.l_type = F_WRLCK;
		rv = fcntl(fildes, F_SETLK, &l);
		break;
	case F_TEST:
		l.l_type = F_WRLCK;
		rv = fcntl(fildes, F_GETLK, &l);
		if (rv != -1) {
			if (l.l_type == F_UNLCK)
				return (0);
			else {
				errno = EACCES;
				return (-1);
			}
		}
	default:
		errno = EINVAL;
		return (-1);
	}
	if (rv < 0) {
		switch(errno) {
		case EMFILE:
		case ENOSPC:
			/* A deadlock error is given if we run out of resources,
			 * in compliance with /usr/group standards.
			 */
			errno = EDEADLK;
			break;
		default:
			break;
		}
	}
	return (rv);
}


