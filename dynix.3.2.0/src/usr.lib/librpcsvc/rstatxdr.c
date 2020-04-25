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
static char rcsid[] = "$Header: rstatxdr.c 1.1 86/12/12 $";
#endif

#ifndef lint
/* @(#)rstatxdr.c	2.2 86/08/14 NFSSRC */
static  char sccsid[] = "@(#)rstatxdr.c 1.1 86/02/05 Copyr 1984 Sun Micro";
#endif

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

rstat(host, statp)
	char *host;
	struct statstime *statp;
{
	return (callrpc(host, RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
	    xdr_void, 0, xdr_statstime, statp));
}

havedisk(host)
	char *host;
{
	long have;
	
	if (callrpc(host, RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_HAVEDISK,
	    xdr_void, 0, xdr_long,  &have) != 0)
		return (-1);
	else
		return (have);
}

xdr_stats(xdrs, statp)
	XDR *xdrs;
	struct stats *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	return (1);
}

xdr_statsswtch(xdrs, statp)		/* version 2 */
	XDR *xdrs;
	struct statsswtch *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	return (1);
}

xdr_statstime(xdrs, statp)		/* version 3 */
	XDR *xdrs;
	struct statstime *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	if (xdr_timeval(xdrs, &statp->curtime) == 0)
		return (0);
	return (1);
}

xdr_timeval(xdrs, tvp)
	XDR *xdrs;
	struct timeval *tvp;
{
	if (xdr_long(xdrs, &tvp->tv_sec) == 0)
		return (0);
	if (xdr_long(xdrs, &tvp->tv_usec) == 0)
		return (0);
	return (1);
}
