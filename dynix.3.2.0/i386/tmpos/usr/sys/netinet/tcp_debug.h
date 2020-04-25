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
#ifndef _NETINET_TCP_DEBUG_INCLUDED
#define _NETINET_TCP_DEBUG_INCLUDED

/*
 * $Header: tcp_debug.h 2.2 90/05/25 $
 */

/* $Log:	tcp_debug.h,v $
 */

struct	tcp_debug {
	n_time	td_time;
	short	td_act;
	short	td_ostate;
	caddr_t	td_tcb;
	struct	tcpiphdr td_ti;
	short	td_req;
	struct	tcpcb td_cb;
};

#define	TA_INPUT 	0
#define	TA_OUTPUT	1
#define	TA_USER		2
#define	TA_RESPOND	3
#define	TA_DROP		4

#ifdef TANAMES
char	*tanames[] =
    { "input", "output", "user", "respond", "drop" };
#endif

#define	TCP_NDEBUG 100
extern	struct	tcp_debug tcp_debug[TCP_NDEBUG];
extern	int	tcp_debx;
#endif	/* _NETINET_TCP_DEBUG_INCLUDED */
