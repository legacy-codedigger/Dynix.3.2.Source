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

/*
 * $Header: svc_dup.h 1.1 87/05/05 $
 *
 * svc_dup.h
 *	Server duplicate request cache.
 */

/* $Log:	svc_dup.h,v $
 */

#ifdef	KERNEL
/*
 * The dup cacheing routines provide a cache of non-failure
 * transaction id's. Rpc service routines can use this to detect
 * retransmissions and re-send a non-failure response.
 */
struct dupreq {
	u_long		dr_xid;
	struct sockaddr_in dr_addr;
	u_long		dr_proc;
	u_long		dr_vers;
	u_long		dr_prog;
	struct dupreq	*dr_next;
	struct dupreq	*dr_chain;
};
#endif	KERNEL
