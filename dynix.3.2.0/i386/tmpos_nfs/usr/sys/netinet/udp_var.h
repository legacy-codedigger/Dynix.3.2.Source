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
#ifndef _NETINET_UDP_VAR_INCLUDED
#define _NETINET_UDP_VAR_INCLUDED

/*
 * $Header: udp_var.h 2.5 1991/04/30 17:17:44 $
 */

/* $Log: udp_var.h,v $
 *
 */

/*
 * UDP kernel structures and variables.
 */
struct	udpiphdr {
	struct 	ipovly ui_i;		/* overlaid ip structure */
	struct	udphdr ui_u;		/* udp header */
};
#define	ui_next		ui_i.ih_next
#define	ui_prev		ui_i.ih_prev
#define	ui_x1		ui_i.ih_x1
#define	ui_pr		ui_i.ih_pr
#define	ui_len		ui_i.ih_len
#define	ui_src		ui_i.ih_src
#define	ui_dst		ui_i.ih_dst
#define	ui_sport	ui_u.uh_sport
#define	ui_dport	ui_u.uh_dport
#define	ui_ulen		ui_u.uh_ulen
#define	ui_sum		ui_u.uh_sum

struct	udpstat {
	int	udps_hdrops;
	int	udps_badsum;
	int	udps_badlen;
	int	udps_fullsock;
	int	udps_delivers;
	int	udps_noport;
	int	udps_sends;
};

#ifdef KERNEL
extern	struct	inpcb udb[];
extern	struct	udpstat udpstat;
extern	int	udp_sendspace;
extern	int	udp_recvspace;
extern	int	udpcksumi;
extern	int	udpcksumo;
extern	int	udp_ttl;
#endif
#endif	/* _NETINET_UDP_VAR_INCLUDED */
