/* $Copyright:	$
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

/* $Header: fdopen.c 2.2 91/03/14 $ */
/* @(#)fdopen.c	4.4 (Berkeley) 9/12/83 */
/*
 * Unix routine to do an "fopen" on file descriptor
 * The mode has to be repeated because you can't query its
 * status
 */

#include	<stdio.h>
#include	<errno.h>
FILE *_findiop();

FILE *
fdopen(fd, mode)
register char *mode;
{
	extern int errno;
	register FILE *iop;

	if ((unsigned)fd >= getdtablesize())
		return (NULL);
	iop = _findiop();
	if (iop == NULL) {
		return(NULL);
	}
	iop->_cnt = 0;
	iop->_file = fd;
	if (*mode != 'r') {
		iop->_flag |= _IOWRT;
		if (*mode == 'a')
			lseek(fd, 0L, 2);
	} else
		iop->_flag |= _IOREAD;
	if (mode[1] == '+') {
		iop->_flag &= ~(_IOREAD|_IOWRT);
		iop->_flag |= _IORW;
	}
	return(iop);
}
