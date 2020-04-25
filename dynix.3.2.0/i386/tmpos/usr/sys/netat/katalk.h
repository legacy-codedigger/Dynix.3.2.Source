
/*
 * $Copyright:	$
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

#ifndef _NETAT_KATALK_INCLUDED
#define _NETAT_KATALK_INCLUDED
/*
 *  DYNIX AppleTalk Kernel Definitions
 *  (c) 1985, 1986 Kinetics, Inc.
 */

/*
 * $Header: katalk.h 1.8 90/05/25 $
 */

/*
 * $Log:	katalk.h,v $
 */

/*
 * ddp protocol statistics counters - should be in netstat
 */

struct	ddpstat {
	int	ddps_hdrops;	/* length too short */
	int	ddps_badlen;	/* wrong length, too long */
	int	ddps_ddpintr;	/* calls to ddpintr */
	int	ddps_ddppckts;	/* times thru ddpintr loop */
	int	ddps_nullifp;	/* no interface! */
	int	ddps_rtmppckts;	/* rtmp packets rcvd */
	int	ddps_shortddps;	/* short ddp pckts rcvd */
	int	ddps_wrongnet;	/* rcvd pckt not for this net */
	int	ddps_forward;	/* pckts forwarded */
	int	ddps_noddpcb;	/* undeliverable: no ddpcb */
	int	ddps_sockerr;	/* socket wouldn't accept packet */
	int	ddps_bad;	/* bad exits from ddpintr */
};

#ifdef KERNEL
struct ifnet *atroute();

/*
 * these could be done with bitfields...
 */

#define atgetlength(x)	(ntohs((u_short)((struct ddp *)(x))->d_length) & 0x3FF)

/*
 * this sets hop count to 0, too
 */

#define	atsetlength(x, l) {(((struct ddp *)(x))->d_length)=htons((u_short) l);}

#define atgethops(x)	((ntohs((u_short)((struct ddp *)(x))->d_length) >> 10) \
				  & 0xF)

#define	atbumphops(x)		{(((struct ddp *)(x))->d_length) = \
				 (((atgethops(x) + 1) << 10) + atgetlength(x));}		
/*
 * lucky for us, if_host[1] is not used by any other networking code
 */

#define ifpgetnet(ifp)		(((struct a_addr *)(&ifp->if_host[1]))->at_Net)
#define ifpsetnet(ifp, net)	{((struct a_addr *)(&ifp->if_host[1]))->at_Net\
				  = net;}
#define ifpgetnode(ifp)		(((struct a_addr *)(&ifp->if_host[1]))->at_Node)
#define ifpsetnode(ifp, node)	{((struct a_addr *)(&ifp->if_host[1]))->at_Node\
				  = node;}
#define ifpgetbridge(ifp)	(((struct a_addr *)(&ifp->if_host[1]))-> \
				  at_Abridge)
#define ifpsetbridge(ifp, br)	{((struct a_addr *)(&ifp->if_host[1]))-> \
				   at_Abridge = br;}
/*
 * Structure atcb for Appletalk protocol implementation.
 * Here are stored pointers to local and foreign addresses,
 * local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */

struct atcb {
	struct	atcb *at_next,*at_prev; /* pointers to other pcb's */
#ifdef sequent
	struct	atcb *at_head;		/* used in deletion semantics */
#endif sequent
	struct	sockaddr_at at_faddr;	/* foreign address */
	struct	sockaddr_at at_laddr;	/* local address */
	short	atcb_flags;
	short	atcb_ptype;
	struct	socket *at_socket;	/* back pointer to socket */
	caddr_t	at_ppcb;		/* pointer to per-protocol pcb */
#ifdef sequent
	struct	socket_peer at_sop;	/* this holds the socket_peer
						lock and refcnt */
	struct socket_peer * at_sopp;	/* pcb pointer to socket_peer */
#endif sequent
};
#define	at_fport	at_faddr.at_sno
#define	at_lport	at_laddr.at_sno
#define	at_fnet		at_faddr.at_net
#define	at_lnet		at_laddr.at_net
#define	at_fnode	at_faddr.at_node
#define	at_lnode	at_laddr.at_node

#define	sotoatcb(so)	((struct atcb *)(so)->so_pcb)

struct	ifqueue	ddpintq;		/* ddp packet input queue */
struct	atcb *at_pcblookup();

#ifdef sequent
struct	atcb *lat_pcblookup();
#endif sequent

struct	ddpstat ddpstat;		/* protocol statistics */
#define	AT_FADDR 			1
#define	AT_IGNADDR			2
#define ATPORT_RESERVED			128
#define	ATPLOOKUP_WILDCARD		1
#define	ATMAXHOPS			15

#define	NETISR_DDP		9

#ifdef sequent
#define ATP_LOCK(atp) \
		p_lock(&((atp)->at_sopp->sop_lock), SPLNET);
#define ATP_UNLOCK(atp, splevel) \
		(void) v_lock(&((atp)->at_sopp->sop_lock), splevel);

extern struct ifqueue ddpintq;

extern	int ddpqmaxlen;
extern	int ddp_sendspace;
extern	int ddp_recvspace;

#endif sequent

#endif	KERNEL
#endif	/* _NETAT_KATALK_INCLUDED */
