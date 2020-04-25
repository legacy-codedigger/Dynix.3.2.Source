
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
#ifndef _NETAT_ATP_INCLUDED
#define _NETAT_ATP_INCLUDED

/*
 * $Header: atp.h 1.3 90/05/25 $
 */

/*
 * $Log:	atp.h,v $
 */


/*
 * ATP error codes
 */

#define	reqFailed	-1096	/* SendRequest failed: retrys exceeded */
#define	badATPSkt	-1099	/* bad ATP responding socket */
#define	noRelErr	-1101	/* no release received */
#define	noDataArea	-1104	/* no data area for MPP request */
#define	reqAborted	-1105	/* SendRequest aborted by RelTCB */
#define atpBadRsp	-3107	/* Bad response from ATPRequest */
#define atpLenErr	-3106	/* atp response msg too large */

/*
 * added...
 */

#define atpNoBufs	-3	/* could not get local buffer space */
#define atpUnkError	-4	/* unknown error */
#define atpBufTooSmall	-6	/* user buffer is too small */
#define atpTimedOut	-9	/* atp retry timer went to 0 */

struct ATP {			/* ATP */
	u_char	control;
	u_char	bitmap;
	u_short	transID;
	long	userBytes;
};

/*
 * misc.
 */

#define	atpSize		8	/* sizeof struct ATP */
#define	atpReqCode	0x40
#define	atpRspCode	0x80
#define	atpRelCode	0xC0
#define	atpXO		0x20
#define	atpEOM		0x10
#define	atpSTS		0x08
#define	atpSendChk	0x01
#define	atpTIDValid	0x02
#define	atpFlagMask	0x3F
#define	atpControlMask	0xF8
#define	atpMaxNum	8
#define	atpMaxData	578
#define	atpNData	512+16	/* normal amount of data */


/*
 * ATP packet buffer. 
 */

struct atpbuf {
	struct ATP atp;			/* ATP header */
	char	data[atpMaxData];
};


/*
 * A control-block for an atp transaction.
 */

struct atpcb {
	u_short	tid;		/* transaction ID */
	u_char	bitmap;		/* bitmap */
	short	nrespkts;	/* number of response pkts expected on a treq */
	short	nretries;	/* number of retries to attempt on a treq */
	short	timo;		/* timeout in sec. between retries */
	short	flags;		/* is this cb for a request or a response */
	BDS bds[atpMaxNum];	/* bds's for response data */
	short	resrcvd;	/* number of response buffers received */
	short	nressent;	/* number of times this response buf re-sent */
	long	nreqsent;	/* number of times this request buf re-sent */
	struct atpbuf req;	/* request buffer */
	int	reqlen;		/* length of request for retrans. */
	struct atpsoc *as;	/* back-pointer to atpsoc */
	struct sockaddr_at fsat;/* foreign address if this is a response */
	long	user1cb;	/* state user can maintain per atpcb */
	long	user2cb;
};

/*
 * flags
 */

#define ATPREQUEST	1
#define ATPRESPONSE	2
#define	ATPONCEONLY	4

#define MAXTRANS	atpMaxNum /* max concurrent transactions per atpsoc */
#define INF_RETRIES	255

/*
 * ATP socket.  This structure holds the state information for active
 * transactions per process.  On UNIX we normally use only one socket
 * for sending out things, per process.  The Mac uses several.  We must
 * take care to respond to the socket from which a request came from, but
 * nobody cares from which socket OUR datagrams come.  We could use several
 * sockets, too, and demultiplex to transactions based on socket (in
 * select()), but we have to check tid's anyway, and, it takes time to create
 * new sockets, etc.
 *
 * The "fsat" in atpsoc is a convenience for users of atp, like the variables
 * user1, user2, etc.  Some applications might be writtten to do transactions
 * with several fsat's, & if so, fine.  Most, eg, PAP, use only one foreign
 * address as the main corresponding socket to talk to.  So, keeping an
 * fsat per atpsoc seems reasonable.
 */

struct atpsoc {
	int	fd;		/* UNIX file descriptor (socket) */
	u_short	tid;		/* filled in by atpopen; inc'd for every req */

	/* control blocks both for responses AND requests */

	struct atpcb *atpCBs[MAXTRANS];
	struct sockaddr_at lsat; /* local sockaddr_at */

	/* state user can maintain per atpsoc */

	struct sockaddr_at fsat; /* foreign  sockaddr_at */
	long	user1;
	long	user2;
	long	user3;
	long	user4;
};

struct atpstats {		/* statistics counters */
	int	icount;		/* input packet count */
	int	ocount;		/* output packet count */
				/* input (receive) counters: */
	int	tooshort;	/* packet too short */
	int	badheader;	/* bad packet header fields */
	int	irelease;	/* 'release' count */
	int	irequest;	/* 'request' count */
	int	irexmit;	/* rexmit count */
	int	ibadseq;	/* bad TID sequence */
	int	ibadsrc;	/* bad src address */
};

extern struct atpstats atpstats;

#endif	/* _NETAT_ATP_INCLUDED */
