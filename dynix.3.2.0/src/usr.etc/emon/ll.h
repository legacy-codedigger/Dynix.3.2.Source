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

/* $Header: ll.h 2.3 86/08/13 $ */

/*
 * $Log:	ll.h,v $
 */

/*
 * Structure of an Ethernet header -- xmit format
 */

struct	il_xheader {
	u_char	ilx_dhost[6];		/* Destination Host */
	u_char	ilx_shost[6];		/* Source Host */
	u_short	ilx_type;		/* Type of packet */
};

/*
 * Structure of an Ethernet header -- receive format
 */
struct	il_rheader {
	u_char	ilr_status;		/* Frame Status */
	u_char	ilr_fill1;
	u_short	ilr_length;		/* Frame Length */
	u_char	ilr_dhost[6];		/* Destination Host */
	u_char	ilr_shost[6];		/* Source Host */
	u_short	ilr_type;		/* Type of packet */
};

/*
 * Structure of statistics record
 */
struct	il_stats {
	u_short	ils_fill1;
	u_short	ils_length;		/* Length (should be 62) */
	u_char	ils_addr[6];		/* Ethernet Address */
	u_short	ils_frames;		/* Number of Frames Received */
	u_short	ils_rfifo;		/* Number of Frames in Receive FIFO */
	u_short	ils_xmit;		/* Number of Frames Transmitted */
	u_short	ils_xcollis;		/* Number of Excess Collisions */
	u_short	ils_frag;		/* Number of Fragments Received */
	u_short	ils_lost;		/* Number of Times Frames Lost */
	u_short	ils_multi;		/* Number of Multicasts Accepted */
	u_short	ils_rmulti;		/* Number of Multicasts Rejected */
	u_short	ils_crc;		/* Number of CRC Errors */
	u_short	ils_align;		/* Number of Alignment Errors */
	u_short	ils_collis;		/* Number of Collisions */
	u_short	ils_owcollis;		/* Number of Out-of-window Collisions */
	u_short	ils_fill2[8];
	char	ils_module[8];		/* Module ID */
	char	ils_firmware[8];	/* Firmware ID */
};

/*
 * Structure of Collision Delay Time Record
 */
struct	il_collis {
	u_short	ilc_fill1;
	u_short	ilc_length;		/* Length (should be 0-32) */
	u_short	ilc_delay[16];		/* Delay Times */
};
/* 
 *  10 Mbit Ether xmit packet format
 */
struct il_xpacket {
	/* 
	 * Ethernet header
	 */
	short 	fill1;			/* MUST BE HERE BECAUSE OF ALIGNMENT */
	u_char	ether_dhost[6];		/* destination */
	u_char	ether_shost[6];		/* source */
	u_short	ether_type;		/* type of packet */
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
	/*
	 * User data
	 */
	u_char  il_data[1500];		/* possible real data */
};

/* 
 *  10 Mbit Ether receive packet format --- N.B. with UDP header
 */
struct il_rpacket {
	/* 
	 * Ethernet header
	 */
	u_char	ether_dhost[6];		/* destination */
	u_char	ether_shost[6];		/* source */
	u_short	ether_type;		/* type of packet */
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
	/*
	 * User data
	 */
	u_char  il_data[1500];		/* possible real data */

/* temporary mod to compile */

	u_char	ether_status;		/* Frame Status */
	u_char	ether_fill1;
	u_short	ether_length;		/* Frame Length */
};

/* 
 *  10 Mbit Ether receive packet format --- N.B. with TCP header
 */
struct iltcp {
	/* 
	 * Ethernet header
	 */
	short 	fill1;			/* MUST BE HERE BECAUSE OF ALIGNMENT */
	u_char	ether_status;		/* Frame Status */
	u_char	ether_fill1;
	u_short	ether_length;		/* Frame Length */
	u_char	ether_dhost[6];		/* destination */
	u_char	ether_shost[6];		/* source */
	u_short	ether_type;		/* type of packet */
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
	 * TCP header
	 */
	u_short	th_sport;		/* source port */
	u_short	th_dport;		/* destination port */
	tcp_seq	th_seq;			/* sequence number */
	tcp_seq	th_ack;			/* acknowledgement number */
#if defined (vax) || defined (ns32000) || defined(i386)
	u_char	th_x2:4,		/* (unused) */
		th_off:4;		/* data offset */
#endif
	u_char	th_flags;
	u_short	th_win;			/* window */
	u_short	th_sum;			/* checksum */
	u_short	th_urp;			/* urgent pointer */
	 /*
	 * User data
	 */
	u_char  il_data[1500];		/* possible real data */
};
