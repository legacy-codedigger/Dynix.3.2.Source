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

#ifndef	lint
static	char	rcsid[] = "$Header: conf_net.c 2.22 1991/09/26 21:08:26 $";
#endif

/*
 *	Network binary config variables.
 */

/* $Log: conf_net.c,v $
 *
 *
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/if_ether.h"

/*
 * checksum option flags
 * Initializing any of the following flags to 1 indicates that 
 * checksumming is to be peformed by that protocol on that data
 * path (e.g. udpcksumi == UDP will/will not checksum input data packets).
 */

int	ipcksumo = 1;	/* generate checksum on output */
int	ipcksumi = 1;	/* check checksum on input  */

int	tcpcksumi = 1;	/* check checksums on input */
int	tcpcksumo = 1;	/* generate checksums on output */

int	udpcksumi = 1;	/* check udp checksum on input */
#ifdef NFS
int	udpcksumo = 0;	/* no udp checksum on output (for performance) */
#else
int	udpcksumo = 1;	/* udp checksum on output (for safety) */
#endif NFS

/*
 * Initial mbuf and cluster sizes.  Used by ../sys/uipc_mbuf.c.
 */

int	mbinitbufs = 64*1024/CLBYTES;	/* pages of 128 byte mbufs */
int	mbinitcl = 200;			/* number of MCLBYTES mclusters */

/*
 *	max interface queue lengths - currently == 200 entries
 */

#ifndef IFQ_MAXLEN
#define IFQ_MAXLEN	200
#endif

int	ifqmaxlen =	 IFQ_MAXLEN;
int	raw_ifq_maxlen = IFQ_MAXLEN;
int	ipqmaxlen =	 IFQ_MAXLEN;

/*
 *	tcp config variables
 *	used by ../netinet/tcp_usrreq.c (externed in tcp_var.h)
 */

int	tcp_sendspace = 1024*4;	/* size of socket buffer window */
int	tcp_recvspace = 1024*4;

/*
 *	udp config variables
 *	used by ../netinet/udp_usrreq.c (externed in udp_var.h)
 */

int	udp_sendspace = 16384;	/* size of largest udp */
int	udp_recvspace = 32767;	/* size of recv buffer space */

/*
 * buffer space for raw sockets.
 */

int rawsndq = RAWSNDQ;
int rawrcvq = RAWRCVQ;

/*
 * used by promiscuous monitor to distinquish loopback packets
 */

u_char loopbackfakeaddr[6] = {0xfe, 0xed, 0xfa, 0xce, 0xbe, 0xef};

/*
 *	used by ../sys/uipc_socket.c
 */

int	somaxcon = SOMAXCONN;	/* max number of connections listen
				 * can queue (currently 5) */
/*
 *	used by ../sys/uipc_usrreq.c
 */

int	unp_sendspace = 1024*2;
int	unp_recvspace = 1024*2;

/*
 *	used by ../netinet/tcp_timer.c
 */

int	tcpnodelack = 0;

#ifdef	notdef
/*
 * Dynix 3 used to use the following backoff values.  In the interest
 * of being a good network citizen, it has been replaced with the
 * 4.3 tahoe values, which aren't as agressive.  This may lead, however, to
 * longer connection timeouts.  Its included here for backward compatablitity
 */
int	tcp_backoff[TCP_MAXRXTSHIFT+1] =
{ 1, 2, 4, 6, 8, 10, 15, 20, 30, 30, 30, 30, 30 };	
#endif

int	tcp_backoff[TCP_MAXRXTSHIFT+1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };
/*
 * If a connection is idle (no segments received) for TCPTV_KEEP_INIT 
 * amount of time, but not yet established, then we drop the connection.
 * Once the connection is established, if the connection is idle for 
 * TCPTV_KEEP_IDLE time (and keepalives have been enabled on the socket),
 * we begin to send keepalives on the connection, and continue sending
 * them every TCPTV_KEEPINTVL seconds for at most TCPTV_KEEPCNT times.
 * If, despite the keepalive segments we cannot elicit a response 
 * from a peer in TCPT_MAXIDLE amount of time probing, then we drop
 * the connection.
 *
 * These defines are from netinet/tcp_timer.h and should be modified with
 * care!
 */

int	tcp_keepinit = TCPTV_KEEP_INIT;
int	tcp_keepidle = TCPTV_KEEP_IDLE;
int	tcp_keepintvl = TCPTV_KEEPINTVL;
int	tcp_keepcnt = TCPTV_KEEPCNT;

/*
 * Time To Live (hop count) for Tcp/Udp/Icmp segments
 */
int	tcp_ttl = 50;
int	udp_ttl = 30;
int	ip_ttl = MAXTTL;
/*
 *	used by ../netinet/ip_input.c
 */

#ifndef	IPFORWARDING
#define	IPFORWARDING	1
#endif
#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif

int	ipprintfs = 0;
int	ipforwarding = IPFORWARDING;
int	ipsendredirects = IPSENDREDIRECTS;

#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif

int subnetsarelocal = SUBNETSARELOCAL;

/*
 * if INBROADCAST == 0 then use 0 as default IP broadcast address
 * otherwise use all 1's
 */

int INBROADCAST = 1;

/*
 * if in_connectzero == 1, a connect on an AF_INET socket to the address
 * INADDR_ANY (i.e. 0) will successfully connect the socket to the 
 * primary interface address (i.e. the first interface address assigned
 * with ifconfig(8c)).  This behaviour is correctly handled by
 * applications linked with 3.1 or later resolver libraries.
 * If in_connectzero == 0, the attempt will fail with
 * EADDRNOTAVAIL.  This is what is expected from the 3.0.17
 * resolver libraries if your system is not running a nameserver.
 * If you're not running named and your application hangs and eventually
 * fails whenever it attempts to lookup a hostname, set this to zero.
 */

int in_connectzero = 1;

/*
 *	used by ../netinet/ip_output.c
 */

short	ip_allowbroadcast = 1;	/* allows user programs to use broadcast */

#ifndef	GATEWAY
#define RTHASHSIZ 8
#define	GATEWAY	0	/* do not send ICMP_UNREACH over one in_interface */
#else
#define RTHASHSIZ 64	/* gateways like more routes cached */
#endif

int	ipgateway = GATEWAY;
struct	mbuf *rthost[RTHASHSIZ];
struct	mbuf *rtnet[RTHASHSIZ];
int rthashsize = RTHASHSIZ;	/* used by netstat */

/*
 * APPLETALK config
 */

int kin_atalk_arp = 0;		/* Kinetics Appletalk Arp using net 0 IP addrs */
int ddpqmaxlen = IFQ_MAXLEN;
int ddp_sendspace = 2048;
int ddp_recvspace = 2048;

/*
 * ARP table size
 */

#ifndef	ARPTAB_BSIZ	
#define	ARPTAB_BSIZ	9		/* bucket size */
#endif
#ifndef	ARPTAB_NB	
#define	ARPTAB_NB	19		/* number of buckets */
#endif

int	arptab_bsiz = ARPTAB_BSIZ;
int	arptab_nb = ARPTAB_NB;
struct	arptab arptab[ARPTAB_BSIZ * ARPTAB_NB];
int	arptab_size = ARPTAB_BSIZ * ARPTAB_NB;

#ifndef	NUM_LOIF
#define	NUM_LOIF 1
#endif

int	num_loif = NUM_LOIF;
struct	ifnet loif[NUM_LOIF];

/*
 * Unix domain sockets are used to implement ordinary pipes in dynix.
 * The following define can be used to tune the default buffer size
 * used for these pipes.
 */
#ifndef UIPC_PIPESIZE
#define	UIPC_PIPESIZE 4096
#endif

int	uipc_pipesize = UIPC_PIPESIZE;
