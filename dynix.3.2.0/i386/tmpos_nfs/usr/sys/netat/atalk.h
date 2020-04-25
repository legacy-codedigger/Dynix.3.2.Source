
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
#ifndef _NETAT_ATALK_INCLUDED
#define _NETAT_ATALK_INCLUDED
/*
 * copyright (c) 1985, 1986. Kinetics, Inc.
 *
 * atalk.h :  AppleTalk/DDP protocol definitions.
 */

/*
 * $Header: atalk.h 1.4 90/05/25 $
 */

/*
 * $Log:	atalk.h,v $
 */

/*
 * NOTE: assumes, in unions for overloading, below, that 2 chars fit into
 * a short exactly.
 */

#ifndef AF_APPLETALK
#define AF_APPLETALK	16		/* this is defined in 4.3! */
#define	PF_APPLETALK	AF_APPLETALK
#endif

#ifndef ETHERPUP_ATALKTYPE
#define	ETHERPUP_ATALKTYPE	0x809B
#endif

#ifdef	USG
#define	u_char	unsigned char
#define	u_short	unsigned short
#define	u_long	unsigned long
#endif	USG

struct lap {
	u_char	dst;		/* destination node */
	u_char	src;		/* source node */
	u_char	type;		/* lap-type (short ddp, long ddp, etc. */
	u_char	dummy;		/* long-allign lap */
};

struct a_addr {
	union {
		struct {
			u_short	Net;
			u_char	Node;
			u_char	Abridge;
		} at_chrs;
		long	Addr;
	} at_un;
};
#define at_Net		at_un.at_chrs.Net
#define	at_Node		at_un.at_chrs.Node
#define	at_Abridge	at_un.at_chrs.Abridge
#define	at_adr		at_un.Addr

#define sockaddr_at sad_at
struct sockaddr_at {
	short		at_family;		/* AF_APPLETALK */
	struct a_addr	at_addr;		/* 4 bytes */
	u_char		at_sno;			/* ddp socket number */
	u_char		at_ptype;		/* ddp protocol type */
	char		at_dummy[8];		/* pad to 14 addr bytes */
};
#define	at_net		at_addr.at_Net
#define at_node		at_addr.at_Node
#define at_abridge	at_addr.at_Abridge

struct ddp {
	u_short		d_length;	/* length (+ hopcount) */
	u_short		d_cksum;	/* checksum */
	u_short		d_dnet;		/* dest net */
	u_short		d_snet;		/* source net */
	u_char		d_dnode;	/* dest node */
	u_char		d_snode;	/* source node */
	u_char		d_dsno;		/* destination socket */
	u_char 		d_ssno;		/* source socket */
	u_char		d_type;		/* ddp protocol type */
	u_char		d_dummy[3];	/* long-allign */
};

struct ddpshort {
	u_short		d_length;	/* length */
	u_char		D_dsno;		/* destination socket */
	u_char		D_ssno;		/* source socket */
	u_char		D_type;	       	/* ddp protocol type */
	u_char		D_dummy[3];	/* long-allign */
};

/* ...routing... */

/*
 * rtmp header
 */

struct rtmp {
	u_short		r_net;		/* sender's net */
	u_char		r_idlen;	/* length (bits) of sender's id */
	u_char		r_id;		/* start of id */
};

/*
 * rtmp routing tuple
 */

struct r_tuple {
	u_short		rt_net;		/* net */
	u_char		rt_dist;	/* distance */
};
#define R_TUPLE_SIZE 3

/* ...name-binding... */

/*
 * an field in the name
 */

struct nbp_field {
	u_char			nb_nlength;
	char			nb_string[32];
};

/*
 * an entity name
 */

struct nbp_name {
	struct nbp_field	nb_object;
	struct nbp_field	nb_type;
	struct nbp_field	nb_zone;
};

/*
 * An nbp datagram consists of an nbp_header and 1 or more nbp_tuples
 */

/*
 * nbp header
 */

struct nbp_header {
	u_char			nb_cntrl;	/* conrol and tuple count */
	u_char			nb_id;		/* request id */
};

/*
 * nb_control values
 */

#define	BrRq		1
#define	LkUp		2
#define	LkUp_Reply	3

/*
 * name-binding tuple.  Used in requests, replies, and a node's names table
 */

struct nbp_theader {
	u_short			nb_netnum;
	u_char			nb_node;
	u_char			nb_socket;
	u_char			nb_enum;
};
#define nbpTupleHdrSize		5	/* sizeof nbp_tuple - sizeof nbp_name */

struct nbp_tuple {
	char		tuple[nbpTupleHdrSize + sizeof(struct nbp_name)];
};

/*
 * find start of nbp_name given start of nbp_tuple
 */

#define nbpNameLoc(x)	((struct nbp_name *)(((char *)(x))+nbpTupleHdrSize))
#define nbpnxtfield(p)	{(p) = (struct nbp_field *)(((char *)(p)) + \
			(p)->nb_nlength +1);}

/*
 * NBP error codes
 */

#define	nbpBuffOvr	-1024	/* buffer overflow in LookupName */
#define	nbpNoConfirm	-1025	/* name not confirmed on ConfirmName */
#define	nbpConfDiff	-1026	/* name confirmed at different socket */
#define	nbpDuplicate	-1027	/* duplicate name already exists */
#define	nbpNotFound	-1028	/* name not found on remove */
#define	nbpNISErr	-1029	/* error trying to open the NIS */
/* added... */
#define nbpNoBufs	-5	/* could not find local buffer space for nbp */

#define	NAMES_TABLE	"/etc/appletalk/names_table"
#define	MYNODE		"/etc/appletalk/mynode"
#define A_BRIDGE	"/etc/appletalk/a_bridge"
#define NBP_PID		"/etc/appletalk/nbp_pid"
#define	NBP_LOCK	"/etc/appletalk/nbp_lock"
#define NODES_LOCK	"/etc/appletalk/nodes_lock"
#define	NBPD_PATH	"/etc/appletalk/nbpd"
#define AT_LOG		"/etc/appletalk/at_log"

/* pap */

/*
 * the part of pap header in the user-bytes part of atp
 */

struct pap_ubytes {
	u_char	ConnID;
	u_char	PAPType;
	u_char	eofField;
	u_char	dummy;
};

/*
 * the part of the pap header in the atp data part of atp
 */

struct pap_xhdr {
	u_char	papSock;	/* responding socket in OpenConn() rqst/rply */
	u_char	papFlowQuant;	/* flow quantum in OpenConn() rqst/rply */
	u_short papWR;		/* wait time (open rqst) or result (open rply)*/
#define	papWaittime papWR
#define	papResult papWR
};

/* pap types */
#define	OpenConn	1
#define	OpenConnReply	2
#define	SendData	3
#define	Data		4
#define	Tickle		5
#define	CloseConn	6
#define	CloseConnReply	7
#define	SendStatus	8
#define	StatusReply	9
/* xtra... */
#define papBusy		10

/*
 * pap state in an atpsoc or atpcb
 */

#define	papConnid	user1
#define	papQuantum	user2
#define papEOFflag	user3
#define	papSentEOF	1
#define	papGotEOF	2
#define papTime		user4
#define papStart	user1cb

/* misc */

#define papTimedOut	-2
#define papBufTooBig	-7
#define papUnexpected	-8
#define papNotSupport	-9
#define PAPREAD_TIMO	15
#define PAPMAXTIME	20 /* time wherein pap should get something from peer */

/*
 * echo protocol
 */

#define	EchoReq		1
#define	EchoRep		2

#define LT_SHORTDDP	1	/* lap-type is short ddp */
#define LT_DDP		2	/* lap-type is regular ddp */
#define	LT_IP		22	/* lap-type for ip */
#define	LT_ARP		23	/* lap-type for arp */
#define ddpTypeRTMP	1	/* ddp-type is rtmp protocol */
#define	ddpTypeNBP	2	/* ddp-type is nbp protocol */
#define	ddpTypeATP	3	/* ddp-type is atp */
#define	ddpTypeEP	4	/* ddp-type is echo protocol */
#define ddpTypeRTMPReq	5	/* ddp-type is rtmp request */
#define ddpTypeZIP	6	/* ddp-type is zip */
#define	ddpTypeData	10	/* ddp-type is plain data */

#define WKS_EP		4	/* echo protocol socket */
#define WKS_NBP		2	/* names information socket */
#define WKS_RTMP	1	/* rtmp socket */

/*
 * "sizeof" yields incorrect results on some machines
 */

#define	SIZEOFLAP	3
#define	SIZEOFDDPSHORT	5
#define	SIZEOFDDP	13
#define	DDPMAXSIZE	587
#define	NFPDEVS		16

/*
 * Someday in the distant future when there might possibly be > 50 appletalk
 * servers available to a UNIX host, someone could increase N_NAMES or
 * make the nbp_remove() process disk-based.
 */

#define N_NAMES	50	/* number of appletalk names known to this host */

#define ELOCKED 500	/* a file was locked */

#ifdef	KERNEL
#include "../h/ioctl.h"
#else
#include <sys/ioctl.h>
#endif	/* KERNEL */
#define	SIOIGNADDR	_IO(a, 1)	/* for AppleTalk connect() call */

/*
 * Following definitions from (c) Stanford Univ. SUMEX project. C-language
 * version of atalk.h
 */

/*
 * atalk error codes
 */

#define	noErr		0
#define	ddpSktErr	-91	/* error in socket number */
#define	ddpLenErr	-92	/* data length too big */
#define	noBridgeErr	-93	/* no net bridge for non-local send */
#define	lapProtErr	-94	/* error in attach/detach */
#define	excessCollsns	-95	/* excessive collisions on write */
#define	portInUse	-97	/* driver Open error */
#define	portNotCf	-98	/* driver Open error */

/*
 * misc structures
 */

typedef struct {		/* BDS, buffer data structure */
	short	buffSize;
	char	*buffPtr;
	short	dataSize;
	long	userData;
} BDS;

/*
 * UDP-ish stuff
 */

#define	ddpMaxWKS	0x7F
#define	ddpWKS		128	/* boundary of DDP well known sockets */
#define	ddpWKSUnix	768	/* start of WKS range on UNIX */
#define	ddpNWKSUnix	16384	/* start of non-WKS .. */
#define	userBytes userData
#endif	/* _NETAT_ATALK_INCLUDED */
