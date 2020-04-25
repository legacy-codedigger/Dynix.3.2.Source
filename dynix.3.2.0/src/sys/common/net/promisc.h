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
#ifndef _NET_PROMISC_INCLUDED
#define _NET_PROMISC_INCLUDED

/*
 * $Header: promisc.h 2.8 90/12/13 $
 * promisc.h 
 *	promiscuous queue handler struct definitions
 */

/* $Log:	promisc.h,v $
 */

/*
 * wraparound buffer area definition -----------------------------
 */

#define	WNUMBUFS	20		/* start with 20 2K buffers */
#define WBUFSZ		1550		/* max size of ether packet */

struct wbuf {
	char	wbuf[WBUFSZ];
	int	wbuf_len;
	caddr_t	wbuf_ifnet;
	n_time	wbuf_time;
	short	wbuf_unit;
};

struct e_packet {
	u_char	ether_dhost[6];
	u_char	ether_shost[6];
	u_short	ether_type;
	/* 
	 * IP header
	 */
	u_short	ip_tos;			/* type of service */
	short	ip_len;			/* total length */
	u_short	ip_id;			/* identification */
	short	ip_off;			/* fragment offset field */
	u_char  ip_ttl;			/* time to live */
	u_char  ip_p;			/* and protocol fields */
	u_short ip_sum;			/* ip checksum (20 bytes of ip hdr) */
	struct	in_addr ip_src,ip_dst;	/* source and dest address */
	/* 
	 * UDP header
	 */
	u_short	uh_sport;		/* source port */
	u_short	uh_dport;		/* destination port */
	short	uh_ulen;		/* udp length */
	u_short	uh_sum;			/* udp checksum */
};

struct eentry {
	char * ename;
	u_char eaddr[6];
};

#define TRAPEES 5
#define TNAMLEN 12

struct promiscstat {
	struct wbuf * promisc_wbuf_begin;	/* start of wbufs */
	struct wbuf * promisc_wbuf_end;		/* end of wbufs */
	struct wbuf *promisc_wbufp;		/* current wbuf pointer */
	int promisc_packets;	/* total promiscuous packets */
	int promisc_station;	/* packets addressed to this station */
	int promisc_bcast;	/* total broadcast packets */
	int promisc_mcast;	/* total broadcast packets */
	int promisc_rwho;	/* total rwho packets */
	int promisc_arp;	/* total ARP packets */
	int promisc_unk_broad;	/* total unrecognized broadcasts */
	int promisc_ip;		/* total IP packets */
	int promisc_udp;	/* total udp data grams */
	int promisc_tcp;	/* total tcp packets */
	int promisc_icmp;	/* total icmp packets */
	int promisc_trailers;	/* total trailer packets */
	int promisc_xns;	/* total xns type packets */
	int promisc_at;		/* total appletalk type packets */
	int promisc_sequentE;	/* total packets with Sequent addr's */
	int promisc_loopback;	/* total packets through loopback */

	int promisc_bytes;	/* total int number of bytes */
	int promisc_beelions;	/* keep separate #billions */

	int promisc_unk;	/* total unrecognized packets */

	int promisc_kept;	/* packets kept (filtered) */
	int promisc_reject;	/* packets rejected */
	int promisc_bytes_kept;	/* total bytes filtered */

	int promisc_crc;	/* total h/w detected crc's */
	int promisc_align;	/* total h/w detected alignment errs */
	int promisc_lost;	/* total h/w detected lost packets */
	int promisc_runt;	/* total h/w detected runts */
	int promisc_toobig;	/* total h/w detected too big packets */

	short promisc_bkeep;	/* broadcast keeper flag */
	short promisc_keepall;	/* all packet keeper flag */
	short promisc_keeparp;	/* arp packet keeper flag */
	short promisc_keeptrail;/* trail packet keeper flag */
	short promisc_keeptrap;	/* trap packet keeper flag */
	short promisc_keepbogus;/* bogus packet keeper flag */
	short promisc_keepxns;	/* keep xns packets */
	short promisc_keepat;	/* keep appletalk packets */

	int buck[6];		/* packet length distribution */
	int promisc_trapx;
	int promisc_trapping;
	struct eentry promisc_trapees[TRAPEES];
};

#define PROMISC_XMIT	0
#define PROMISC_RCVD	1
#define PROMISC_LOOP	2

struct promiscif {
	caddr_t promiscif_ifnet;
	short	promiscif_flag;
};

#define XNS_TYPE	0x600	/* type field for PCI packets (XNS) */
#define EADDRLEN	6	/* length of Ether address */
#define RWHOPORT	513	/* rwho's well-known port */

#define PROMISC_DEV	15	/* promiscuous pseudo device # */

#ifdef KERNEL
extern dev_t promiscdev;
extern short promiscon;
extern short promiscrace;
extern u_char loopbackfakeaddr[];
extern struct	ifqueue	promiscq;	/* promiscuous packet input queue */
#endif KERNEL

#endif	/* _NET_PROMISC_INCLUDED */
