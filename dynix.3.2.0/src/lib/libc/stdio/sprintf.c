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

/* $Header: sprintf.c 2.0 86/01/28 $ */
/* @(#)sprintf.c	4.1 (Berkeley) 12/21/80 */
#include	<stdio.h>

char *sprintf(str, fmt, args)
char *str, *fmt;
{
	struct _iobuf _strbuf;

	_strbuf._flag = _IOWRT+_IOSTRG;
	_strbuf._ptr = str;
	_strbuf._cnt = 32767;
	_doprnt(fmt, &args, &_strbuf);
	putc('\0', &_strbuf);
	return(str);
}
