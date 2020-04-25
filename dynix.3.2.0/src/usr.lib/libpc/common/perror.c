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

/* $Header: perror.c 1.1 89/03/12 $ */
#include	<stdio.h>
#include	<signal.h>

/*
 * Routine PERROR is called from the runtime library when a runtime
 * I/O error occurs. Its arguments are a pointer to an error message and 
 * the name of the offending file.
 */
long
PERROR(msg, fname)
char	*msg, *fname;
{
	PFLUSH();
	fputc('\n',stderr);
	fputs(msg, stderr);
	perror(fname);
	kill(getpid(), SIGTRAP);
	/*NOTREACHED*/
	return 0;
}
