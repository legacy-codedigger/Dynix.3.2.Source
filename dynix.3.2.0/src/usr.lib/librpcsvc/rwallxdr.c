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
static char rcsid[] = "$Header: rwallxdr.c 1.1 86/12/12 $";
#endif

#ifndef lint
/* @(#)rwallxdr.c	2.2 86/08/14 NFSSRC */
static  char sccsid[] = "@(#)rwallxdr.c 1.1 86/02/05 Copyr 1985 Sun Micro";
#endif

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/rwall.h>

rwall(host, msg)
	char *host;
	char *msg;
{
	return (callrpc(host, WALLPROG, WALLVERS, WALLPROC_WALL,
	    xdr_wrapstring, &msg,  xdr_void, NULL));
}
