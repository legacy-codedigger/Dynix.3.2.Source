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

#ifndef	lint
static char rcsid[] = "$Header: sysacct.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)sysacct.c	5.3 (Berkeley) 6/23/85";
#endif

#include <sys/types.h>

/*LINTLIBRARY*/

/*
 *	output accounting info
 */

/*ARGSUSED*/
sysacct(bytes, time)
time_t time;
long bytes;
{
	return;
}
