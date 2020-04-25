/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: fcntl.h 1.1 1991/04/25 18:21:46 $ */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fcntl.h	5.2 (Berkeley) 1/8/86
 */


#ifndef F_DUPFD

/* command values for fcntl(2) */
#define	F_DUPFD		0		/* duplicate file descriptor */
#define	F_GETFD		1		/* get file descriptor flags */
#define	F_SETFD		2		/* set file descriptor flags */
#define	F_GETFL		3		/* get file status flags */
#define	F_SETFL		4		/* set file status flags */
#ifndef _POSIX_SOURCE
#define	F_GETOWN	5		/* get SIGIO/SIGURG proc/pgrp */
#define F_SETOWN	6		/* set SIGIO/SIGURG proc/pgrp */
#endif
#define	F_GETLK		7		/* get record locking information */
#define	F_SETLK		8		/* set record locking information */
#define	F_SETLKW	9		/* F_SETLK; wait if blocked */

/* file descriptor flags (F_GETFD, F_SETFD) */
#define	FD_CLOEXEC	1		/* close-on-exec flag */

/* record locking flags (F_GETLK, F_SETLK, F_SETLKW) */
#define	F_RDLCK		1		/* shared or read lock */
#define	F_UNLCK		3		/* unlock */
#define	F_WRLCK		2		/* exclusive or write lock */

#ifndef _POSIX_SOURCE
/* lock operations for flock(2) */
#define	LOCK_SH		0x01		/* shared file lock */
#define	LOCK_EX		0x02		/* exclusive file lock */
#define	LOCK_NB		0x04		/* don't block when locking */
#define	LOCK_UN		0x08		/* unlock file */
#endif

/* file status flags */
#define	O_RDONLY	00000		/* open for reading only */
#define	O_WRONLY	00001		/* open for writing only */
#define	O_RDWR		00002		/* open for reading and writing */
#define	O_NONBLOCK	00004		/* no delay */
#ifndef _POSIX_SOURCE
#define	O_NDELAY	O_NONBLOCK
#define	FNDELAY		O_NONBLOCK
#endif

#define	O_APPEND	00010		/* set append mode */
#ifndef _POSIX_SOURCE
#define	FAPPEND		O_APPEND
#endif

#if !defined(_POSIX_SOURCE) && defined(KERNEL)
#define	O_MARK		00020		/* mark during gc() */
#define	O_DEFER		00040		/* defer for next gc pass */
#endif

#ifndef _POSIX_SOURCE
#define	O_ASYNC	 	000100		/* signal pgrp when data ready */
#define	FASYNC		O_ASYNC
#endif
#define	O_SHLOCK	00200		/* shared file lock present */
#define	O_EXLOCK	00400		/* exclusive file lock present */

#define	O_CREAT		01000		/* create if nonexistant */
#define	O_TRUNC		02000		/* truncate to zero length */
#define	O_EXCL		04000		/* error on create if file exists */
#define	O_SYNC		010000		/* synchronous write option */



/* file segment locking set data type - information passed to system by user */
struct flock {
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;		/* len = 0 means until end of file */
	int	l_pid;
};

#endif /* !F_DUPFD */
