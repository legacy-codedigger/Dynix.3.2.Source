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

/* $Header: gets.c 2.0 86/01/28 $ */
/* @(#)gets.c	4.1 (Berkeley) 12/21/80 */
#include	<stdio.h>

char *
gets(s)
char *s;
{
	register c;
	register char *cs;

	cs = s;
	while ((c = getchar()) != '\n' && c >= 0)
		*cs++ = c;
	if (c<0 && cs==s)
		return(NULL);
	*cs++ = '\0';
	return(s);
}
