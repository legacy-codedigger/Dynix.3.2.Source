
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
#ifndef _NET_IF_ARP_INCLUDED
#define _NET_IF_ARP_INCLUDED

/*
 * $Header: if_arp.h 1.3 90/09/02 $
 */

/* $Log:	if_arp.h,v $
 */

/*
 * Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  ARP packets are variable
 * in size; the arphdr structure defines the fixed-length portion.
 * Protocol type values are the same as those for 10 Mb/s Ethernet.
 * It is followed by the variable-sized fields ar_sha, arp_spa,
 * arp_tha and arp_tpa in that order, according to the lengths
 * specified.  Field names used correspond to RFC 826.
 */
struct	arphdr {
	u_short	ar_hrd;		/* format of hardware address */
#define ARPHRD_ETHER 	1	/* ethernet hardware address */
	u_short	ar_pro;		/* format of protocol address */
	u_char	ar_hln;		/* length of hardware address */
	u_char	ar_pln;		/* length of protocol address */
	u_short	ar_op;		/* one of: */
#define	ARPOP_REQUEST	1	/* request to resolve address */
#define	ARPOP_REPLY	2	/* response to previous request */

/*
 * The remaining fields are variable in size,
 * according to the sizes above.
 */
/*	u_char	ar_sha[6];	sender hardware address */
/*	u_char	ar_spa[4];	sender protocol address */
/*	u_char	ar_tha[6];	target hardware address */
/*	u_char	ar_tpa[4];	target protocol address */
};

/*
 * ARP ioctl request
 */

struct arpreq {
	struct	sockaddr arp_pa;		/* protocol address */
	struct	sockaddr arp_ha;		/* hardware address */
	int	arp_flags;			/* flags */
};
/*  arp_flags and at_flags field values */
#define	ATF_INUSE	0x01	/* entry in use */
#define ATF_COM		0x02	/* completed entry (enaddr valid) */
#define	ATF_PERM	0x04	/* permanent entry */
#define	ATF_PUBL	0x08	/* publish entry (respond for other host) */
#define	ATF_USETRAILERS	0x10	/* has requested trailers */

#endif	/* _NET_IF_ARP_INCLUDED */
