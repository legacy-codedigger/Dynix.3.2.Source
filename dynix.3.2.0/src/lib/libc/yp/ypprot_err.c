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
static char rcsid[] = "$Header: ypprot_err.c 1.1 86/12/11 $";
#endif

#ifndef lint
/* @(#)ypprot_err.c	2.1 86/04/14 NFSSRC */
static  char sccsid[] = "@(#)ypprot_err.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

/*
 * Maps a yp protocol error code (as defined in
 * yp_prot.h) to a yp client interface error code (as defined in
 * ypclnt.h).
 */
int
ypprot_err(yp_protocol_error)
	unsigned int yp_protocol_error;
{
	int reason;

	switch (yp_protocol_error) {
	case YP_TRUE: 
		reason = 0;
		break;
 	case YP_NOMORE: 
		reason = YPERR_NOMORE;
		break;
 	case YP_NOMAP: 
		reason = YPERR_MAP;
		break;
 	case YP_NODOM: 
		reason = YPERR_DOMAIN;
		break;
 	case YP_NOKEY: 
		reason = YPERR_KEY;
		break;
 	case YP_BADARGS:
		reason = YPERR_BADARGS;
		break;
 	case YP_BADDB:
		reason = YPERR_BADDB;
		break;
 	case YP_VERS:
		reason = YPERR_VERS;
		break;
	default:
		reason = YPERR_YPERR;
		break;
	}
	
  	return(reason);
}
