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

/* $Header: strout.c 2.0 86/01/28 $ */
/* @(#)strout.c	4.1 (Berkeley) 12/21/80 */
#include	<stdio.h>

_strout(count, string, adjust, file, fillch)
register char *string;
register count;
int adjust;
register struct _iobuf *file;
{
	while (adjust < 0) {
		if (*string=='-' && fillch=='0') {
			putc(*string++, file);
			count--;
		}
		putc(fillch, file);
		adjust++;
	}
	while (--count>=0)
		putc(*string++, file);
	while (adjust) {
		putc(fillch, file);
		adjust--;
	}
}
