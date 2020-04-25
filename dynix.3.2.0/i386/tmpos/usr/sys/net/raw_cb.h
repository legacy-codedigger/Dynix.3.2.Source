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
#ifndef _NET_RAW_CB_INCLUDED
#define _NET_RAW_CB_INCLUDED

/*
 * $Header: raw_cb.h 2.2 90/05/25 $
 */

/* $Log:	raw_cb.h,v $
 */

/*
 * Raw protocol interface control block.  Used
 * to tie a socket to the generic raw interface.
 */

struct rawcb {
	struct	rawcb *rcb_next;	/* doubly linked list */
	struct	rawcb *rcb_prev;
	struct	socket *rcb_socket;	/* back pointer to socket */
	struct	sockaddr rcb_faddr;	/* destination address */
	struct	sockaddr rcb_laddr;	/* socket's address */
	struct	sockproto rcb_proto;	/* protocol family, protocol */
	caddr_t	rcb_pcb;		/* protocol specific stuff */
	struct	mbuf *rcb_options;	/* protocol specific options */
	struct	route rcb_route;	/* routing information */
	short	rcb_flags;
					/* added for DYNIX */
	struct	rawcb *rcb_head;	/* for raw_attach mutex semantics */
	struct	socket_peer rcb_sop;	/* socket_peer lock and refcnt */
	struct	socket_peer * rcb_sopp;	/* pcb pointer to socket_peer */
};

/*
 * Since we can't interpret canonical addresses,
 * we mark an address present in the flags field.
 */

#define	RAW_LADDR	01
#define	RAW_FADDR	02
#define	RAW_DONTROUTE	04		/* no routing, default */

#define	sotorawcb(so)		((struct rawcb *)(so)->so_pcb)

/*
 * Nominal space allocated to a raw socket.
 */

#define	RAWSNDQ		2048
#define	RAWRCVQ		2048

/*
 * Format of raw interface header prepended by
 * raw_input after call from protocol specific
 * input routine.
 */

struct raw_header {
	struct	sockproto raw_proto;	/* format of packet */
	struct	sockaddr raw_dst;	/* dst address for rawintr */
	struct	sockaddr raw_src;	/* src address for sbappendaddr */
};

/*
 * uses raw_lock in head rawcb structure to mutex list
 * list is headed by rawcb - CAUTION, cut/edited from INP_ in in_pcb.h
 * edited because rawcb is not a struct inpcb, but similar - also no
 * general purpose pcb list routines exist for rawcb list so
 * passing list as argument is not appropriate.
 */

#define RCB_LOCK() \
		p_lock(&((&rawcb)->rcb_sopp->sop_lock), SPLNET);

#define RCB_UNLOCK(splevel) \
		(void) v_lock(&((&rawcb)->rcb_sopp->sop_lock), splevel);

struct sockaddr_rawE {
	u_short rawE_family;
	u_short rawE_type;
	u_short rawE_unit;
	char	rawE_fill[10];
};

#ifdef KERNEL
extern	struct rawcb rawcb;			/* head of list */
extern	struct ifnet rawif;			/* raw net i/f */
extern	int	raw_ifq_maxlen;
extern	int	rawsndq;
extern	int	rawrcvq;
#endif

#endif	/* _NET_RAW_CB_INCLUDED */
