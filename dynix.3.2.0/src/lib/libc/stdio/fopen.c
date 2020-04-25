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

/* $Header: fopen.c 2.2 91/03/13 $ */
/* @(#)fopen.c	4.3 (Berkeley) 9/12/83 */
#include	<stdio.h>
#include	<errno.h>
FILE *_findiop();

FILE *
fopen(file, mode)
char *file;
register char *mode;
{
	extern int errno;
	register f, rw;
	register FILE *iop,*newiop;

	if ((iop = _findiop()) == NULL) {
		return(NULL);
	}

	rw = mode[1] == '+';

	if (*mode=='w') {
		f = creat(file, 0666);
		if (rw && f>=0) {
			close(f);
			f = open(file, 2);
		}
	} else if (*mode=='a') {
		if ((f = open(file, rw? 2: 1)) < 0) {
			if (errno == ENOENT) {
				f = creat(file, 0666);
				if (rw && f>=0) {
					close(f);
					f = open(file, 2);
				}
			}
		}
		if (f >= 0)
			lseek(f, 0L, 2);
	} else if (*mode=='r') {
		f = open(file, rw? 2: 0);
	} else
		return(NULL);
	if (f < 0)
		return(NULL);
	iop->_cnt = 0;
	iop->_file = f;
	if (rw)
		iop->_flag |= _IORW;
	else if (*mode != 'r')
		iop->_flag |= _IOWRT;
	else
		iop->_flag |= _IOREAD;
	return(iop);
}
