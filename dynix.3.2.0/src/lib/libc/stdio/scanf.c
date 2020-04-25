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

/* $Header: scanf.c 2.0 86/01/28 $ */
/* @(#)scanf.c	4.2 (Berkeley) 10/5/82 */
#include	<stdio.h>

scanf(fmt, args)
char *fmt;
{
	return(_doscan(stdin, fmt, &args));
}

fscanf(iop, fmt, args)
FILE *iop;
char *fmt;
{
	return(_doscan(iop, fmt, &args));
}

sscanf(str, fmt, args)
register char *str;
char *fmt;
{
	FILE _strbuf;

	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;
	return(_doscan(&_strbuf, fmt, &args));
}
