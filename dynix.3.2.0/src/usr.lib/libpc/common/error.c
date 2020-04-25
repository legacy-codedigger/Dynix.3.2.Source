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

/* $Header: error.c 1.1 89/03/12 $ */
#include	<stdio.h>
#include	<signal.h>

/*
 * Routine ERROR is called from the runtime library when a runtime
 * error occurs. Its arguments are a pointer to an error message and 
 * an error specific piece of data.
 */
long
ERROR(msg, d1, d2)
char	*msg;
long	d1, d2;
{
	PFLUSH();
	fputc('\n',stderr);
	fprintf(stderr, msg, d1, d2);
	kill(getpid(), SIGTRAP);
	return d1;
}
