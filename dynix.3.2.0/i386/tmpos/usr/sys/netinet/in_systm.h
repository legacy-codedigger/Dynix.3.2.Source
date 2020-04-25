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
#ifndef _NETINET_IN_SYSTM_INCLUDED
#define _NETINET_IN_SYSTM_INCLUDED

/*
 * $Header: in_systm.h 2.1 90/05/25 $
 */

/* $Log:	in_systm.h,v $
 */

/*
 * Miscellaneous internetwork
 * definitions for kernel.
 */

#ifndef LOCORE
/*
 * Network types.
 *
 * Internally the system keeps counters in the headers with the bytes
 * swapped so that VAX instructions will work on them.  It reverses
 * the bytes before transmission at each protocol level.  The n_ types
 * represent the types with the bytes in ``high-ender'' order.
 */
typedef u_short n_short;		/* short as received from the net */
typedef u_long	n_long;			/* long as received from the net */

typedef	u_long	n_time;			/* ms since 00:00 GMT, byte rev */
#endif

#ifndef LOCORE
#ifdef KERNEL
n_time	iptime();
#endif
#endif
#endif	/* _NETINET_IN_SYSTM_INCLUDED */
