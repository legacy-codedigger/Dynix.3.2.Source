/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.

/* $Header: mmap.h 1.2 1991/08/01 16:18:15 $ */

/* $Log: mmap.h,v $
 *
 *
 */

#define	mbuf_map(x, t)		(t) ((u_int) x + (u_int) mdiff)
#define	mbuf_unmap(x, t)	(t) ((u_int) x - (u_int) mdiff)

extern	int	arptab_size;
extern	struct	arptab *arptab;
extern	struct  icmpstat *icmpstat;
extern	int	tcp_backoff[];
extern	struct	tcpstat *tcpstat;
extern	struct	ipstat *ipstat;
extern	struct	udpstat *udpstat;
extern	struct	inpcb *tcb;
extern	struct	inpcb *udb;
extern	struct	inpcb *tcb_addrs[];
extern	struct	inpcb *udb_addrs[];
extern	struct	mbuf *mlo, *mhi;
extern	off_t	mdiff;	
extern	off_t	ifnet;
extern	int	*ipforwarding;
extern	struct	ifqueue	*ipintrq;
extern	struct	mbuf **rthost;
extern	struct	mbuf **rtnet;
extern	int	rthashsize;
extern	int	*ip_ttl;

extern	int	km;
extern	int	ctl_sock;
extern	char	*unix_file;
extern	char	*kmem_file;

/*
 * 3.1 compatability
 */
#ifndef	INP_HASHSZ
#define	INP_HASHSZ 1
#endif
