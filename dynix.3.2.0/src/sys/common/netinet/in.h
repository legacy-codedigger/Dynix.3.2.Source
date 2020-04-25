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
#ifndef _NETINET_IN_INCLUDED
#define _NETINET_IN_INCLUDED

/*
 * $Header: in.h 2.7 90/05/25 $
 */

/* $Log:	in.h,v $
 */

/*
 * Constants and structures defined by the internet system,
 * Per RFC 1010, May 1987.
 */

/*
 * Protocols
 */

#define	IPPROTO_IP		0		/* dummy for IP */
#define	IPPROTO_ICMP		1		/* control message protocol */
#define	IPPROTO_GGP		3		/* gateway^2 (deprecated) */
#define	IPPROTO_TCP		6		/* tcp */
#define	IPPROTO_EGP		8		/* exterior gateway protocol */
#define	IPPROTO_PUP		12		/* pup */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_IDP		22		/* xns idp */
#define	IPPROTO_ND		77		/* UNOFFICIAL net disk proto */

#define	IPPROTO_RAW		255		/* raw IP packet */
#define	IPPROTO_MAX		256

/*
 * Port/socket numbers: network standard functions
 */
#define	IPPORT_ECHO		7
#define	IPPORT_DISCARD		9
#define	IPPORT_SYSTAT		11
#define	IPPORT_DAYTIME		13
#define	IPPORT_NETSTAT		15
#define	IPPORT_FTP		21
#define	IPPORT_TELNET		23
#define	IPPORT_SMTP		25
#define	IPPORT_TIMESERVER	37
#define	IPPORT_NAMESERVER	42
#define	IPPORT_WHOIS		43
#define	IPPORT_MTP		57

/*
 * Port/socket numbers: host specific functions
 */
#define	IPPORT_TFTP		69
#define	IPPORT_RJE		77
#define	IPPORT_FINGER		79
#define	IPPORT_TTYLINK		87
#define	IPPORT_SUPDUP		95

/*
 * UNIX TCP sockets
 */
#define	IPPORT_EXECSERVER	512
#define	IPPORT_LOGINSERVER	513
#define	IPPORT_CMDSERVER	514
#define	IPPORT_EFSSERVER	520

/*
 * UNIX UDP sockets
 */
#define	IPPORT_BIFFUDP		512
#define	IPPORT_WHOSERVER	513
#define	IPPORT_ROUTESERVER	520	/* 520+1 also used */

/*
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 * Ports > IPPORT_USERRESERVED are reserved
 * for servers, not necessarily privileged.
 */
#define	IPPORT_RESERVED		1024
#define	IPPORT_USERRESERVED	5000

/*
 * Link numbers
 */

#define	IMPLINK_IP		155
#define	IMPLINK_LOWEXPER	156
#define	IMPLINK_HIGHEXPER	158

/*
 * Internet address (old style... should be updated)
 */

struct in_addr {
	union {
		struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { u_short s_w1,s_w2; } S_un_w;
		u_long S_addr;
	} S_un;
#define	s_addr	S_un.S_addr	/* can be used for most tcp & ip code */
#define	s_host	S_un.S_un_b.s_b2	/* host on imp */
#define	s_net	S_un.S_un_b.s_b1	/* network */
#define	s_imp	S_un.S_un_w.s_w2	/* imp */
#define	s_impno	S_un.S_un_b.s_b4	/* imp # */
#define	s_lh	S_un.S_un_b.s_b3	/* logical host */
};

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */

#define	IN_CLASSA(i)		((((long)(i))&0x80000000)==0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		((((long)(i))&0xc0000000)==0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		((((long)(i))&0xc0000000)==0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((long)(i) & 0xe0000000) == 0xe0000000)
#define	IN_BADCLASS(i)		(((long)(i) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY		(u_long)0x00000000
#define	INADDR_BROADCAST	(u_long)0xffffffff	/* must be masked */
#ifndef KERNEL
#define	INADDR_NONE		0xffffffff		/* -1 return */
#endif

#define IN_LOOPBACKNET	127

/*
 * Socket address, internet style.
 */

struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

/*
 * Options for use with [gs]etsockopt at the IP level.
 */

#define	IP_OPTIONS	1		/* set/get IP per-packet options */

#if !defined(vax) && !defined(ns32000) && !defined(i386)

/*
 * Macros for number representation conversion.
 *			A.K.A. byte swap macros
 *	The 32K and vax have to do byte swap - others do not
 *		these are the do not macros
 */

#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)
#endif

#ifdef KERNEL
#ifdef	i386				/* expand the byte swap macros inline */
#ifndef	lint				/* lint can't handle asm fcts yet */

/*
 * Byte swapping routines used by the network - although they are
 *	generally applicable if byte swapping is an issue
 */

/*
 * netorder = htonl(hostorder)
 *
 * Rotates words then bytes in words
 */

asm htonl(hostorder)
{
%mem hostorder;
	movl	hostorder, %eax	/* 3!2!1!0 */
	xchgb	%ah, %al	/* 3!2!0!1 */
	roll	$16, %eax	/* 0!1!3!2 */
	xchgb	%ah, %al	/* 0!1!2!3 */
}

/*
 * netorder = htons(hostorder)
 *
 * Rotates two lower bytes and clears upper two
 */

asm htons(hostorder)
{
%con	hostorder;
/PEEPOFF
	xorl	%eax, %eax
/PEEPON
	movw	hostorder, %ax
	xchgb	%ah, %al
%mem	hostorder;
	movzwl	hostorder, %eax
	xchgb	%ah, %al
}

/*
 * hostorder = ntohl(netorder)
 *
 * Rotates words then bytes in words
 */

asm ntohl(netorder)
{
%mem netorder;
	movl	netorder, %eax	/* 3!2!1!0 */
	xchgb	%ah, %al	/* 3!2!0!1 */
	roll	$16, %eax	/* 0!1!3!2 */
	xchgb	%ah, %al	/* 0!1!2!3 */
}

/*
 * hostorder = ntohs(netorder)
 *
 * Rotates two lower bytes and clears upper two
 */

asm ntohs(netorder)
{
%con	netorder;
/PEEPOFF
	xorl	%eax, %eax
/PEEPON
	movw	netorder, %ax
	xchgb	%ah, %al
%mem	netorder;
	movzwl	netorder, %eax
	xchgb	%ah, %al
}

#endif	lint
#endif	i386

extern	struct domain inetdomain;
extern	struct protosw inetsw[];

struct	in_addr in_makeaddr();
u_long	in_netof(), in_lnaof();
#endif
#endif	/* _NETINET_IN_INCLUDED */
