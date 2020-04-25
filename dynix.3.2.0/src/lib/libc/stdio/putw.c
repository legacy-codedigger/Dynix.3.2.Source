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

/* $Header: putw.c 2.2 87/06/02 $ */
/* @(#)putw.c	4.1 (Berkeley) 12/21/80 */
#include <stdio.h>

putw(w, iop)
register FILE *iop;
{
	register char *p;
	register i;

	p = (char *)&w;
	for (i=sizeof(int); --i>=0;)
		putc(*p++, iop);
	return(ferror(iop));
}
