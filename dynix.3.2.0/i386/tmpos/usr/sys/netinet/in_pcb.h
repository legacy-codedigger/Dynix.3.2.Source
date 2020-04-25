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
#ifndef _NETINET_IN_PCB_INCLUDED
#define _NETINET_IN_PCB_INCLUDED

/*
 * $Header: in_pcb.h 2.4 91/03/11 $
 */

/* $Log:	in_pcb.h,v $
 */

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct inpcb {
	struct	inpcb *inp_next,*inp_prev;
					/* pointers to other pcb's */
	struct	inpcb *inp_head;	/* pointer back to chain of inpcb's
					   for this protocol */
	struct	in_addr inp_faddr;	/* foreign host table entry */
	u_short	inp_fport;		/* foreign port */
	struct	in_addr inp_laddr;	/* local host table entry */
	u_short	inp_lport;		/* local port */
	struct	socket *inp_socket;	/* back pointer to socket */
	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	struct	route inp_route;	/* placeholder for routing entry */
	struct	mbuf *inp_options;	/* IP options */
	struct	socket_peer inp_sop;	/* this holds lock and refcnt */
	struct	socket_peer *inp_sopp;	/* pcb pointer to socket_peer */
};

#define	INPLOOKUP_WILDCARD	1
#define	INPLOOKUP_SETLOCAL	2

#define	INP_HASHSZ		31
#define	INP_HASH(port)		(port % INP_HASHSZ)

#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)
#define	inp_cache	inp_head	/* pointer to last used inpcb */

struct	inhcomm {		/* things all hash chains need in common*/
	lock_t	inh_lock;	/* mutex */
	u_short	inh_port;	/* next port to allocate */
	struct	inpcb *inh_root;/* base of hash heads */
};

#define inptoinhcomm(inp)	((struct inhcomm *)(inp)->inp_ppcb)

#define	INPCOMM_LOCK(inp)	p_lock(&(inptoinhcomm(inp)->inh_lock), SPLNET)
#define	INPCOMM_UNLOCK(inp, s)	v_lock(&(inptoinhcomm(inp)->inh_lock), s)

/*
 * uses inp_lock in head inpcb structure to mutex list.
 * list can be headed with tcb or udb.
 */

#define	G_PCB	G_IFNET		/* temporary gate assignment */

#define INP_LOCK(inp) \
		p_lock(&((inp)->inp_sopp->sop_lock), SPLNET)
#define INP_UNLOCK(inp, splevel) \
		(void) v_lock(&((inp)->inp_sopp->sop_lock), splevel)

#ifdef KERNEL
struct	inpcb *in_pcblookup();
struct	inpcb *in_pcblookup_c();
#ifdef removed
struct	inpcb *lin_pcblookup();
#endif removed
#endif
#endif	/* _NETINET_IN_PCB_INCLUDED */
