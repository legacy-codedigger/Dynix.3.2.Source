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
 */
#ifndef _NET_IF_INCLUDED
#define _NET_IF_INCLUDED

#undef	PROMISCUOUS	/* UNDO experimental promiscuous kernel */
#define PROMISCUOUS	/* EXPERIMENTAL kernel */

/*
 * $Header: if.h 2.5 1991/04/30 23:52:30 $
 */

/* $Log: if.h,v $
 *
 */

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three parameters:
 *	(*ifp->if_output)(ifp, m, dst)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */

/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 *
 * EVENTUALLY PURGE if_net AND if_host FROM STRUCTURE
 */

struct ifnet {
	char	*if_name;		/* name, e.g. ``se'' */
	short	if_unit;		/* sub-unit for lower level driver */
	short	if_mtu;			/* maximum transmission unit */
	short	if_flags;		/* up/down, broadcast, etc. */
	short	if_timer;		/* time 'til if_watchdog called */
	int	if_metric;		/* routing metric (external only) */
	int	if_host[2];		/* local net host number XXX */
	struct	ifaddr *if_addrlist;	/* linked list of addresses per if */
	struct	ifqueue {
		struct	mbuf *ifq_head;
		struct	mbuf *ifq_tail;
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
		lock_t	ifq_lock;	/* struct ifnet lock */
		short	ifq_busy;	/* queue currently being processed */
	} if_snd;			/* output queue */
/* procedure handles */
	int	(*if_init)();		/* init routine */
	int	(*if_output)();		/* output routine */
	int	(*if_ioctl)();		/* ioctl routine */
	int	(*if_reset)();		/* bus reset routine */
	int	(*if_watchdog)();	/* timer routine */
/* generic interface statistics */
	int	if_ibytes;		/* bytes received on interface */
	int	if_ipackets;		/* unicast packets received */
	int	if_inunicast;		/* non-unicast packets received */
	int	if_idiscards;		/* packets discarded */
	int	if_ierrors;		/* input errors on interface */
	int	if_obytes;		/* bytes sent on interface */
	int	if_opackets;		/* unicast packets sent */
	int	if_onunicast;		/* non-unicast packets sent */
	int	if_odiscards;		/* packets discarded */
	int	if_oerrors;		/* output errors on interface */
	int	if_collisions;		/* collisions on csma interfaces */
/* end statistics */
	struct	ifnet *if_next;
};

#define	IFF_UP		0x1		/* interface is up */
#define	IFF_BROADCAST	0x2		/* broadcast address valid */
#define	IFF_DEBUG	0x4		/* turn on debugging */
#define	IFF_LOOPBACK	0x8		/* is a loopback net */
#define	IFF_ROUTE	0x8		/* routing entry installed */
#define	IFF_POINTOPOINT	0x10		/* interface is point-to-point link */
#define	IFF_NOTRAILERS	0x20		/* avoid use of trailers */
#define	IFF_RUNNING	0x40		/* resources allocated */
#define	IFF_NOARP	0x80		/* no address resolution protocol */
#define	IFF_PROMISC	0x100		/* receive all packets */
#define	IFF_ALLMULTI	0x200		/* receive all multicast packets */

#define	IFF_CANTCHANGE	(IFF_BROADCAST | IFF_POINTOPOINT | IFF_RUNNING)

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros.  IF_LOCK/IF_UNLOCK are used for mutex
 */

#define	IF_LOCK(ifq) \
		p_lock(&(ifq)->ifq_lock, SPLIMP);

#define	IF_UNLOCK(ifq, splevel) \
		v_lock(&(ifq)->ifq_lock, (splevel));

#define	IF_QFULL(ifq) \
	((ifq)->ifq_len >= (ifq)->ifq_maxlen)

#define	IF_DROP(ifq) \
	{ \
		((ifq)->ifq_drops++); \
	}

#define	IF_ENQUEUE(ifq, m) { \
	(m)->m_act = 0; \
	if ((ifq)->ifq_tail == (struct mbuf *) NULL) \
		(ifq)->ifq_head = (m); \
	else \
		(ifq)->ifq_tail->m_act = (m); \
	(ifq)->ifq_tail = (m); \
	(ifq)->ifq_len++; \
}

#define	IF_PREPEND(ifq, m) { \
	(m)->m_act = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == (struct mbuf *) NULL) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}

/*
 * Packets destined for level-1 protocol input routines
 * have a pointer to the receiving interface prepended to the data.
 * IF_DEQUEUEIF extracts and returns this pointer when dequeueing the packet.
 * IF_ADJ should be used otherwise to adjust for its presence.
 */

#define	IF_ADJ(m) { \
	(m)->m_off += sizeof(struct ifnet *); \
	(m)->m_len -= sizeof(struct ifnet *); \
	if ((m)->m_len == 0) { \
		struct mbuf *n; \
		MFREE((m), n); \
		(m) = n; \
	} \
}
#define	IF_DEQUEUEIF(ifq, m, ifp) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
		(ifp) = *(mtod((m), struct ifnet **)); \
		IF_ADJ(m); \
	}else{ \
		(ifq)->ifq_busy = 0; \
	} \
}

#define	IF_DEQUEUE(ifq, m) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = (struct mbuf *) NULL; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
	}else{ \
		(ifq)->ifq_busy = 0; \
	} \
}

#define	IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */

struct ifaddr {
	struct	sockaddr ifa_addr;	/* address of interface */
	union {
		struct	sockaddr ifu_broadaddr;
		struct	sockaddr ifu_dstaddr;
	} ifa_ifu;
#define	ifa_broadaddr	ifa_ifu.ifu_broadaddr	/* broadcast address */
#define	ifa_dstaddr	ifa_ifu.ifu_dstaddr	/* other end of p-to-p link */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	struct	ifaddr *ifa_next;	/* next address for interface */
};


/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */

struct	ifreq {
#define	IFNAMSIZ	16
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		caddr_t	ifru_data;
		short	ifru_mtu;
		struct	ifru_promisc {
			short	ifru_wbsize;
			short	ifru_prflag;
		}ifru_promisc;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
#define	ifr_mtu		ifr_ifru.ifru_mtu	/* mtu */
#define ifr_wbsize	ifr_ifru.ifru_promisc.ifru_wbsize /* promiscuous */
#define ifr_prflag	ifr_ifru.ifru_promisc.ifru_prflag /* promiscuous */
};

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */

struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

/*
 * structure to allow customed ethernet clients
 */

struct custom_client {
	u_short custom_type;
	u_short custom_count;
	dev_t	custom_devno;
}; 

#ifdef KERNEL

#include "../net/if_arp.h"

#ifdef INET
extern	struct	ifqueue	ipintrq;	/* ip packet input queue */
#endif

#ifdef PROMISCUOUS
extern	struct	ifqueue	promiscq;	/* promiscuous input queue */
#endif PROMISCUOUS

extern	struct	ifnet *ifnet;		/* head of ifnet list */
struct	ifaddr *ifa_ifwithaddr(), *ifa_ifwithnet(), *ifa_ifwithdstaddr();
#endif
#endif	/* _NET_IF_INCLUDED */
