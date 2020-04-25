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
static char rcsid[] = "$Header: rquotaxdr.c 1.2 88/08/03 $";
#endif

#ifndef lint
/* @(#)rquotaxdr.c	2.2 86/08/14 NFSSRC */
static	char sccsid[] = "@(#)rquotaxdr.c 1.1 86/02/05 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <ufs/quota.h>
#include <rpcsvc/rquota.h>


bool_t
xdr_getquota_args(xdrs, gq_argsp)
	XDR *xdrs;
	struct getquota_args *gq_argsp;
{
	extern bool_t xdr_path();

	return (xdr_path(xdrs, &gq_argsp->gqa_pathp) &&
	    xdr_int(xdrs, &gq_argsp->gqa_uid));
}

struct xdr_discrim gqr_arms[2] = {
	{ (int)Q_OK, xdr_rquota },
	{ __dontcare__, NULL }
};

bool_t
xdr_getquota_rslt(xdrs, gq_rsltp)
	XDR *xdrs;
	struct getquota_rslt *gq_rsltp;
{

	return (xdr_union(xdrs,
	    &gq_rsltp->gqr_status, &gq_rsltp->gqr_rquota,
	    gqr_arms, xdr_void));
}

bool_t
xdr_rquota(xdrs, rqp)
	XDR *xdrs;
	struct rquota *rqp;
{

	return (xdr_int(xdrs, &rqp->rq_bsize) &&
	    xdr_bool(xdrs, &rqp->rq_active) &&
	    xdr_u_long(xdrs, &rqp->rq_bhardlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_bsoftlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_curblocks) &&
	    xdr_u_long(xdrs, &rqp->rq_fhardlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_fsoftlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_curfiles) &&
	    xdr_u_long(xdrs, &rqp->rq_btimeleft) &&
	    xdr_u_long(xdrs, &rqp->rq_ftimeleft) );
}
