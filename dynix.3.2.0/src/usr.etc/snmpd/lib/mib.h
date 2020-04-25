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

#ident	"$Header: mib.h 1.1 1991/07/31 00:05:57 $"

/*
 * mib.h
 *   Definitions for the variables as defined in the MIB
 */

/* $Log: mib.h,v $
 *
 */

/***********************************************************
	Copyright 1988, 1989 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
#define SETTBLSIZE	2	/* size of the table that is built for setting
				   variables. This is also used as the increment 
				   the table is enlarged by if the table is found 
				   to be too small */
#define MAX_IF		20	/* max number of interfaces */
#define MAX_IF_NAME	16	/* length of interface name */

#define	DESC_MAX	255	/* any description length */
#define	OBJ_MAX		16	/* object identifier length */
#define	ADDR_MAX	11	/* address length */
#define IPADDRESS_SIZE	4	/* ip address size (4 bytes) */

#define ATTBLSIZE	6	/* length of obj instance for address trans table */
#define IPADDRTBLSIZE	4	/* length of obj instance for ip address table */
#define IPROUTETBLSIZE 	4 	/* length of obj instance for ip route table */
#define IPNETTOMEDIATBLSIZE 5 	/* length of obj instance for ip net-to-media table */
#define TCPCONNSIZE	10	/* length of obj instance for tcp connection table */
#define UDPTBLSIZE	5	/* length of obj instance for udp table */

/* caching times (in seconds) */
#define IF_EXPTIME 10
#define AT_EXPTIME 10
#define ICMP_EXPTIME 10
#define TCP_EXPTIME 10
#define UDP_EXPTIME 10
#define IP_EXPTIME 10
#define IPADDR_EXPTIME 10
#define IPROUTE_EXPTIME 10
#define IPNETTOMEDIA_EXPTIME 10


struct	mib_system_struct {
    char    sysDescr[DESC_MAX];   /* textual description */
    int     DescrLen;
    u_char  sysObjectID[OBJ_MAX];/* OBJECT IDENTIFIER of system */
    int	    ObjIDLen;	    /* length of sysObjectID */
    u_long  sysUpTime;	    /* Uptime in 100/s of a second */    
    char    sysContact[DESC_MAX];/* who manages this node */
    int     ContactLen;
    char    sysName[DESC_MAX];   /* fully qualified domain name */
    char    sysLocation[DESC_MAX];/* physical location */
    int     LocationLen;
    u_long  sysServices;    /* set of offered services */
} Mib_system;

struct mib_interface_struct {
    long    ifNumber;	    /* number of interfaces */
};

struct ifTable {
    char fname[64];	/* file name e.g. /dev/enet/se0 */
    char ifname[16];	/* device name from netd e.g. se0 */
    u_long ifExpTime;
    struct mib_ifEntry_struct *mib_ifEntry;
};

struct mib_ifEntry_struct {
    long    ifIndex;	    /* index of this interface	*/
    char    ifDescr[32];    /* english description of interface	*/
    long    ifType;	    /* network type of device	*/
    long    ifMtu;	    /* size of largest packet in bytes	*/
    u_long  ifSpeed;	    /* bandwidth in bits/sec	*/
    u_char  ifPhysAddress[ADDR_MAX];	/* interface's address */
    u_char  PhysAddrLen;    /* length of physAddr */
    long    ifOperStatus;   /* current operational status */
    u_long  ifInOctets;	    /* number of octets received on interface */
    u_long  ifInUcastPkts;  /* number of unicast packets delivered */
    u_long  ifInNUcastPkts; /* number of broadcasts or multicasts */
    u_long  ifInDiscards;   /* number of packets discarded with no error */
    u_long  ifInErrors;	    /* number of packets containing errors */
    u_long  ifInUnknownProtos;	/* number of packets with unknown protocol */
    u_long  ifOutOctets;    /* number of octets transmitted */
    u_long  ifOutUcastPkts; /* number of unicast packets sent */
    u_long  ifOutNUcastPkts;/* number of broadcast or multicast pkts */
    u_long  ifOutDiscards;  /* number of packets discarded with no error */
    u_long  ifOutErrors;    /* number of pkts discarded with an error */
    u_long  ifOutQLen;	    /* number of packets in output queue */
    u_char  ifSpecific[OBJ_MAX]; /* specific mib definitions for this media */
    u_char  SpecLen;        /* length of ifSpecific */
    long    ifAdminStatus;  /* desired state of interface */
    u_long  ifLastChange;   /* value of sysUpTime when current state entered */
};

struct mib_atEntry_struct {
    long    atIfIndex;	    /* interface on which this entry maps */
    u_char  atPhysAddress[ADDR_MAX]; /* physical address of destination */
    u_char  PhysAddrLen; /* length of atPhysAddress */
    u_long  atNetAddress;   /* IP address of physical address */
    oid	    objid[ATTBLSIZE];	/* object id extension for each entry */
};

struct mib_ip_struct {
    long    ipForwarding;   /* 1 if gateway, 2 if host */
    long    ipDefaultTTL;   /* default TTL for pkts originating here */
    u_long  ipInReceives;   /* no. of IP packets received from interfaces */
    u_long  ipInHdrErrors;  /* number of pkts discarded due to header errors */
    u_long  ipInAddrErrors; /* no. of pkts discarded due to bad address */
    u_long  ipForwDatagrams;/* number pf pkts forwarded through this entity */
    u_long  ipInUnknownProtos;/* no. of local-addressed pkts w/unknown proto */
    u_long  ipInDiscards;   /* number of error-free packets discarded */
    u_long  ipInDelivers;   /* number of datagrams delivered to upper level */
    u_long  ipOutRequests;  /* number of IP datagrams originating locally */
    u_long  ipOutDiscards;  /* number of error-free output IP pkts discarded */
    u_long  ipOutNoRoutes;  /* number of IP pkts discarded due to no route */
    long    ipReasmTimeout; /* seconds fragment is held awaiting reassembly */
    u_long  ipReasmReqds;   /* no. of fragments needing reassembly (here) */
    u_long  ipReasmOKs;	    /* number of fragments reassembled */
    u_long  ipReasmFails;   /* number of failures in IP reassembly */
    u_long  ipFragOKs;	    /* number of datagrams fragmented here */
    u_long  ipFragFails;    /* no. pkts unable to be fragmented here */
    u_long  ipFragCreates;  /* number of IP fragments created here */
};

struct mib_ipAddrEntry_struct {
    u_long  ipAdEntAddr;    /* IP address of this entry */
    long    ipAdEntIfIndex; /* IF for this entry */
    u_long  ipAdEntNetMask; /* subnet mask of this entry */
    long    ipAdEntBcastAddr;/* read the MIB for this one */
    long    ipAdEntReasmMaxSize; /* largest datagram which can be reassembled */
    char    if_name[MAX_IF_NAME];	/* ascii name of device */
    oid	    objid[IPADDRTBLSIZE];	/* object id extension for each entry */
};

struct mib_ipRouteEntry_struct {
    u_long  ipRouteDest;    /* destination IP addr for this route */
    long    ipRouteIfIndex; /* index of local IF for this route */
    long    ipRouteMetric1; /* Primary routing metric */
    long    ipRouteMetric2; /* Alternate routing metric */
    long    ipRouteMetric3; /* Alternate routing metric */
    long    ipRouteMetric4; /* Alternate routing metric */
    u_long  ipRouteNextHop; /* IP addr of next hop */
    long    ipRouteType;    /* Type of this route */
    long    ipRouteProto;   /* How this route was learned */
    long    ipRouteAge;	    /* No. of seconds since updating this route */
    u_long  ipRouteMask;    /* netmask associated with this IP address */
    oid	    objid[IPROUTETBLSIZE];	/* object id extension for each entry */
};

struct mib_ipNetToMediaEntry_struct {
    long    ipNetToMediaIfIndex;	/* IF for this entry */
    u_char  ipNetToMediaPhysAddress[ADDR_MAX];	/* physical address */
    u_char  PhysAddrLen;	/* length of ipNetToMediaPhysAddress */
    u_long  ipNetToMediaNetAddress;	/* IP address for this phys addr */
    u_long  ipNetToMediaType;		/* type of mapping */
    oid	    objid[IPNETTOMEDIATBLSIZE];	/* object id extension for each entry */
};

struct mib_icmp_struct {
    u_long  icmpInMsgs;	    /* Total of ICMP msgs received */
    u_long  icmpInErrors;   /* Total of ICMP msgs received with errors */
    u_long  icmpInDestUnreachs;
    u_long  icmpInTimeExcds;
    u_long  icmpInParmProbs;
    u_long  icmpInSrcQuenchs;
    u_long  icmpInRedirects;
    u_long  icmpInEchos;
    u_long  icmpInEchoReps;
    u_long  icmpInTimestamps;
    u_long  icmpInTimestampReps;
    u_long  icmpInAddrMasks;
    u_long  icmpInAddrMaskReps;
    u_long  icmpOutMsgs;
    u_long  icmpOutErrors;
    u_long  icmpOutDestUnreachs;
    u_long  icmpOutTimeExcds;
    u_long  icmpOutParmProbs;
    u_long  icmpOutSrcQuenchs;
    u_long  icmpOutRedirects;
    u_long  icmpOutEchos;
    u_long  icmpOutEchoReps;
    u_long  icmpOutTimestamps;
    u_long  icmpOutTimestampReps;
    u_long  icmpOutAddrMasks;
    u_long  icmpOutAddrMaskReps;
};

struct	mib_tcp_struct {
    long    tcpRtoAlgorithm;	/* retransmission timeout algorithm */
    long    tcpRtoMin;		/* minimum retransmission timeout (mS) */
    long    tcpRtoMax;		/* maximum retransmission timeout (mS) */ 
    long    tcpMaxConn;		/* maximum tcp connections possible */
    u_long  tcpActiveOpens;	/* number of SYN-SENT -> CLOSED transitions */
    u_long  tcpPassiveOpens;	/* number of SYN-RCVD -> LISTEN transitions */
    u_long  tcpAttemptFails;/*(SYN-SENT,SYN-RCVD)->CLOSED or SYN-RCVD->LISTEN*/
    u_long  tcpEstabResets;	/* (ESTABLISHED,CLOSE-WAIT) -> CLOSED */
    u_long  tcpCurrEstab;	/* number in ESTABLISHED or CLOSE-WAIT state */
    u_long  tcpInSegs;		/* number of segments received */
    u_long  tcpOutSegs;		/* number of segments sent */
    u_long  tcpRetransSegs;	/* number of retransmitted segments */
    u_long  tcpInErrs;		/* number of segments recieved in error */
    u_long  tcpOutRsts;		/* Number of segments sent with RST */
};

struct mib_tcpConnEntry_struct {
    long    tcpConnState;	/* State of this connection */
    u_long  tcpConnLocalAddress;/* local IP address for this connection */
    long    tcpConnLocalPort;	/* local port for this connection */
    u_long  tcpConnRemAddress;	/* remote IP address for this connection */
    long    tcpConnRemPort;	/* remote port for this connection */
    oid	    objid[TCPCONNSIZE];	/* object id extension for each entry */
};

struct mib_udp_struct {
    u_long  udpInDatagrams; /* No. of UDP datagrams delivered to users */
    u_long  udpNoPorts;	    /* No. of UDP datagrams to port with no listener */
    u_long  udpInErrors;    /* No. of UDP datagrams unable to be delivered */
    u_long  udpOutDatagrams;/* No. of UDP datagrams sent from this entity */
};

struct mib_udpEntry_struct {
    u_long  udpLocalAddress; /* IP address of udp listener */
    long    udpLocalPort;    /* local port of this listener */
    oid	    objid[UDPTBLSIZE];	/* object id extension for each entry */
};

struct	mib_egp_struct {
    u_long  egpInMsgs;	/* No. of EGP msgs received without error */
    u_long  egpInErrors;/* No. of EGP msgs received with error */
    u_long  egpOutMsgs;	/* No. of EGP msgs sent */
    u_long  egpOutErrors;/* No. of (outgoing) EGP msgs dropped due to error */
};

struct	mib_egpNeighEntry_struct {
    long    egpNeighState;  /* local EGP state with this entry's neighbor */
    u_long  egpNeighAddr;   /* IP address of this entry's neighbor */
    long    egpNeighAs;		/* The autonomous system of this peer */
    u_long  egpNeighInMsgs;	/* number of messages recieved without error */
    u_long  egpNeighInErrs;	/* number of messages recieved with error */
    u_long  egpNeighOutMsgs;	/* number of generated messages */
    u_long  egpNeighOutErrs;	/* number of generated messages not sent */
    u_long  egpNeighInErrMsgs;	/* number of error messages recieved */
    u_long  egpNeighOutErrMsgs;	/* number of error messages sent */
    u_long  egpNeighStateUps;	/* number of transitions to the UP state*/
    u_long  egpNeighStateDowns;	/* number of transitions to the DOWN state */
    long    egpNeighIntervalHello;/* interval between HELLO retransmissions */
    long    egpNeighIntervalPoll;/* interval between POLL retransmissions */
    long    egpNeighMode;	/* the polling activity of this entity */
    long    egpNeighEventTrigger;/* trigger operator initiated events */
};

struct	mib_trans_struct {
    u_long  transInMsgs;	/* No. of TRANS msgs received without error */
    u_long  transInErrors;/* No. of TRANS msgs received with error */
    u_long  transOutMsgs;	/* No. of TRANS msgs sent */
    u_long  transOutErrors;/* No. of (outgoing) msgs dropped due to error */
};

struct	mib_snmp_struct {
  u_long  snmpInPkts;
  u_long  snmpOutPkts;
  u_long  snmpInBadVersions;
  u_long  snmpInBadCommunityNames;
  u_long  snmpInBadCommunityUses;
  u_long  snmpInASNParseErrs;
  u_long  snmpInBadTypes;
  u_long  snmpInTooBigs;
  u_long  snmpInNoSuchNames;
  u_long  snmpInBadValues;
  u_long  snmpInReadOnlys;
  u_long  snmpInGenErrs;
  u_long  snmpInTotalReqVars;
  u_long  snmpInTotalSetVars;
  u_long  snmpInGetRequests;
  u_long  snmpInGetNexts;
  u_long  snmpInSetRequests;
  u_long  snmpInGetResponses;
  u_long  snmpInTraps;
  u_long  snmpOutTooBigs;
  u_long  snmpOutNoSuchNames;
  u_long  snmpOutBadValues;
  u_long  snmpOutReadOnlys;
  u_long  snmpOutGenErrs;
  u_long  snmpOutGetRequests;
  u_long  snmpOutGetNexts;
  u_long  snmpOutSetRequests;
  u_long  snmpOutGetResponses;
  u_long  snmpOutTraps;
  long    snmpEnableAuthTraps;
} Mib_snmp;

#define MIB 1, 3, 6, 1, 2, 1

#define MIB_IFTYPE_OTHER		    1
#define MIB_IFTYPE_REGULAR1822		    2
#define MIB_IFTYPE_HDH1822		    3
#define MIB_IFTYPE_DDNX25		    4
#define MIB_IFTYPE_RFC877X25		    5
#define MIB_IFTYPE_ETHERNETCSMACD	    6
#define MIB_IFTYPE_ISO88023CSMACD	    7
#define MIB_IFTYPE_ISO88024TOKENBUS	    8
#define MIB_IFTYPE_ISO88025TOKENRING	    9
#define MIB_IFTYPE_ISO88026MAN		    10
#define MIB_IFTYPE_STARLAN		    11
#define MIB_IFTYPE_PROTEON10MBIT	    12
#define MIB_IFTYPE_PROTEON80MBIT	    13
#define MIB_IFTYPE_HYPERCHANNEL		    14
#define MIB_IFTYPE_FDDI			    15
#define MIB_IFTYPE_LAPB			    16
#define MIB_IFTYPE_SDLC			    17
#define MIB_IFTYPE_T1CARRIER		    18
#define MIB_IFTYPE_CEPT			    19
#define MIB_IFTYPE_BASICISDN		    20
#define MIB_IFTYPE_PRIMARYISDN		    21
#define MIB_IFTYPE_PROPPOINTTOPOINTSERIAL   22
#define MIB_IFTYPE_PPP			    23
#define MIB_IFTYPE_SOFTWARELOOPBACK	    24
#define MIB_IFTYPE_EON			    25
#define MIB_IFTYPE_ETHERNET_3MBIT	    26
#define MIB_IFTYPE_NSIP			    27
#define MIB_IFTYPE_SLIP			    28

#define MIB_IFSTATUS_UP		1
#define MIB_IFSTATUS_DOWN	2
#define MIB_IFSTATUS_TESTING	3

#define MIB_FORWARD_GATEWAY	1
#define MIB_FORWARD_HOST	2

#define MIB_IPROUTETYPE_OTHER	1
#define MIB_IPROUTETYPE_INVALID	2
#define MIB_IPROUTETYPE_DIRECT	3
#define MIB_IPROUTETYPE_REMOTE	4

#define MIB_IPROUTEPROTO_OTHER	    1
#define MIB_IPROUTEPROTO_LOCAL	    2
#define MIB_IPROUTEPROTO_NETMGMT    3
#define MIB_IPROUTEPROTO_ICMP	    4
#define MIB_IPROUTEPROTO_EGP	    5
#define MIB_IPROUTEPROTO_GGP	    6
#define MIB_IPROUTEPROTO_HELLO	    7
#define MIB_IPROUTEPROTO_RIP	    8
#define MIB_IPROUTEPROTO_ISIS	    9
#define MIB_IPROUTEPROTO_ESIS	    10
#define MIB_IPROUTEPROTO_CISCOIGRP  11
#define MIB_IPROUTEPROTO_BBNSPFIGP  12
#define MIB_IPROUTEPROTO_OIGP	    13

#define	MIB_IPAT_OTHER   1
#define	MIB_IPAT_INVALID 2
#define	MIB_IPAT_DYNAMIC 3
#define	MIB_IPAT_STATIC  4

#define MIB_TCPRTOALG_OTHER	1
#define MIB_TCPRTOALG_CONSTANT	2
#define MIB_TCPRTOALG_RSRE	3
#define MIB_TCPRTOALG_VANJ	4

#define MIB_TCPCONNSTATE_CLOSED		1
#define MIB_TCPCONNSTATE_LISTEN		2
#define MIB_TCPCONNSTATE_SYNSENT	3
#define MIB_TCPCONNSTATE_SYNRECEIVED	4
#define MIB_TCPCONNSTATE_ESTABLISHED	5
#define MIB_TCPCONNSTATE_FINWAIT1	6
#define MIB_TCPCONNSTATE_FINWAIT2	7
#define MIB_TCPCONNSTATE_CLOSEWAIT	8
#define MIB_TCPCONNSTATE_LASTACK	9
#define MIB_TCPCONNSTATE_CLOSING	10
#define MIB_TCPCONNSTATE_TIMEWAIT	11

#define MIB_EGPNEIGHSTATE_IDLE		1
#define MIB_EGPNEIGHSTATE_AQUISITION	2
#define MIB_EGPNEIGHSTATE_DOWN		3
#define MIB_EGPNEIGHSTATE_UP		4
#define MIB_EGPNEIGHSTATE_CEASE		5

#define	MIB_IPNETOTMEDIATTYPE_DYNAMIC	3
#define	MIB_IPNETOTMEDIATTYPE_STATIC	4

/*
 * definitions for talking to the dynix kernel mib driver
 */

struct mib_req {
	char *	mib_buf;	/* buffer containing data */
	int	mib_blen;	/* buffer length */
};

#define	MIB_SYSTEM		1	/* system group */

#define MIB_INTERFACES		2	/* interfaces group */
#define	 MIB_INENUM		0	/* number of interfaces */
#define	 MIB_INENTRY		1	/* interface entry */

#define MIB_AT			3	/* address table group */
#define	 MIB_ATENUM		0	/* number of at entries */
#define	 MIB_ATENTRY		1	/* address table entry */

#define MIB_IP			4	/* IP group */
#define	 MIB_IPSTATS		0	/* IP statistics */
#define	 MIB_IPENUM		1	/* number of IP table entries */
#define	 MIB_IPENTRY		2	/* IP address entry */
#define	 MIB_IPRENUM		3	/* number of IP routine table entries */
#define	 MIB_IPRENTRY		4	/* IP routing table entry */

#define MIB_ICMP		5	/* ICMP group */

#define MIB_TCP			6	/* TCP group */
#define	 MIB_TCPSTATS		0	/* TCP statistics */
#define	 MIB_TCPENUM		1	/* number of TCP table entries */
#define	 MIB_TCPENTRY		2	/* TCP connection table entry */

#define MIB_UDP			7	/* UDP group */

#define MIB_EGP			8	/* EGP group - no support */
#define	MIB_CMOT		9	/* CMOT group - no support */
#define	MIB_TRANSMISSION	10	/* Transmission group - no support */
#define	MIB_SNMP		11	/* SNMP group - no support */

