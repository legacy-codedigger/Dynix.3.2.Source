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
#ifndef _NETINET_IP_VAR_INCLUDED
#define _NETINET_IP_VAR_INCLUDED

/*
 * $Header: ip_var.h 2.9 1991/04/30 17:17:30 $
 */

/* $Log: ip_var.h,v $
 *
 */

/*
 * Overlay for ip header used by other protocols (tcp, udp).
 */
struct ipovly {
	caddr_t	ih_next, ih_prev;	/* for protocol sequence q's */
	u_char	ih_x1;			/* (unused) */
	u_char	ih_pr;			/* protocol */
	short	ih_len;			/* protocol length */
	struct	in_addr ih_src;		/* source internet address */
	struct	in_addr ih_dst;		/* destination internet address */
};

/*
 * Ip reassembly queue structure.  Each fragment
 * being reassembled is attached to one of these structures.
 * They are timed out after ipq_ttl drops to 0, and may also
 * be reclaimed if memory becomes tight.
 */

struct ipq {
	struct	ipq *next,*prev;	/* to other reass headers */
	u_char	ipq_ttl;		/* time for reass q to live */
	u_char	ipq_p;			/* protocol of this fragment */
	u_short	ipq_id;			/* sequence id for reassembly */
	struct	ipasfrag *ipq_next,*ipq_prev;
					/* to ip headers of fragments */
	struct	in_addr ipq_src,ipq_dst;
	long	ipq_time;
};

/*
 * Ip header, when holding a fragment.
 *
 * Note: ipf_next must be at same offset as ipq_next above
 */

struct	ipasfrag {
#if defined (vax) || defined (ns32000) || defined(i386)
	u_char	ip_hl:4,
		ip_v:4;
#endif
	u_char	ipf_mff;		/* copied from (ip_off&IP_MF) */
	short	ip_len;
	u_short	ip_id;
	short	ip_off;
	u_char	ip_ttl;
	u_char	ip_p;
	u_short	ip_sum;
	struct	ipasfrag *ipf_next;	/* next fragment */
	struct	ipasfrag *ipf_prev;	/* previous fragment */
};

/*
 * Structure stored in mbuf in inpcb.ip_options
 * and passed to ip_output when ip options are in use.
 * The actual length of the options (including ipopt_dst)
 * is in m_len.
 */

#define MAX_IPOPTLEN	40

struct ipoption {
	struct	in_addr ipopt_dst;	/* first-hop dst if source routed */
	char	ipopt_list[MAX_IPOPTLEN];	/* options proper */
};

struct	ipstat {
	long	ips_total;		/* total packets received */
	long	ips_badsum;		/* checksum bad */
	long	ips_tooshort;		/* packet too short */
	long	ips_toosmall;		/* not enough data */
	long	ips_badhlen;		/* ip header length < data size */
	long	ips_badlen;		/* ip length < ip header length */
	long	ips_fragments;		/* fragments received */
	long	ips_fragdropped;	/* frags dropped (dups, out of space) */
	long	ips_fragtimeout;	/* fragments timed out */
	long	ips_forward;		/* packets forwarded */
	long	ips_cantforward;	/* packets rcvd for unreachable dest */
	long	ips_redirectsent;	/* packets forwarded on same net */
	long	ips_unknownproto;	/* unkown protocols in ip_p */
	long	ips_outreq;		/* output request */
	long	ips_discards;		/* interface dropped */
	long	ips_noroute;		/* rtalloc failed on output */
	long	ips_reassok;		/* fragments successfully reassembled */
	long	ips_fragok;		/* datagrams which did get fragmented */
	long	ips_fragfail;		/* fragments which couldn't created */
	long	ips_newfrags;		/* fragments created */
};

#ifdef KERNEL

/*
 * flags passed to ip_output as last parameter
 */

#define	IP_FORWARDING		0x1		/* most of ip header exists */
#define	IP_ROUTETOIF		SO_DONTROUTE	/* bypass route tables */
#define	IP_ALLOWBROADCAST	SO_BROADCAST	/* can send broadcast packets */

/*
 * kernel globals defined in ip_input
 */

extern	struct	ipstat	ipstat;
extern	struct	ipq	ipq;			/* ip reass. queue */
extern  int	ipqmaxlen;
extern	u_short	ip_id;				/* ip packet ctr, for ids */
extern  int	ipcksumi;
extern  int	ipcksumo;
extern  int	ipprintfs;
extern  int	ipforwarding;
extern  int	ipsendredirects;

struct	mbuf *ip_srcroute();

/*
 * ip_id fetch-and-add.
 */
#define	GET_IP_ID(id)	{ \
		GATESPL(s); \
		P_GATE(G_NETISR, s); \
		id = htons(ip_id); \
		++ip_id; \
		V_GATE(G_NETISR, s); \
}
#endif	KERNEL
#endif	/* _NETINET_IP_VAR_INCLUDED */
