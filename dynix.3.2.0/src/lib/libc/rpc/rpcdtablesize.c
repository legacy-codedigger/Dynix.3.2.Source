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

#ifndef	lint
static char rcsid[] = "$Header: rpcdtablesize.c 1.1 91/01/10 $";
#endif

/* NFSSRC @(#)rpcdtablesize.c	1.3 88/02/08 */
#ifndef lint
static char sccsid[] = "@(#)rpcdtablesize.c	1.2 88/07/27 Copyr 1988 Sun Micro";
#endif


/*
 * Cache the result of getdtablesize(), so we don't have to do an
 * expensive system call every time.
 */
_rpc_dtablesize()
{
	static int size;
	
	if (size == 0) {
		size = getdtablesize();
	}
	return (size);
}
