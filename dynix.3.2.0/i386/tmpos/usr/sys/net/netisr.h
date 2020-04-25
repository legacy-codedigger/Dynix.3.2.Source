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
#ifndef _NET_NETISR_INCLUDED
#define _NET_NETISR_INCLUDED

#undef	PROMISCUOUS	/* UNDO experimental promiscuous kernel */
#define PROMISCUOUS	/* DO experimental promiscuous kernel */

/*
 * $Header: netisr.h 2.3 90/05/25 $
 */

/* $Log:	netisr.h,v $
 */

/*
 * The networking code runs off software interrupts.
 *
 * The software interrupt level for the network is higher than the software
 * level for the clock (so you can enter the network in routines called
 * at timeout time).
 */

#define	setsoftnet()	{ \
	spl_t softl = splhi(); \
	sendsoft(l.eng->e_slicaddr, NETINTR); \
	(void) splx(softl); \
}

/*
 * Each ``pup-level-1'' input queue has a bit in a ``netisr'' status
 * word which is used to de-multiplex a single software
 * interrupt used for scheduling the network code to calls
 * on the lowest level routine of each protocol.
 */
#define	NETISR_RAW	0		/* same as AF_UNSPEC */

#ifdef	PROMISCUOUS
#define	NETISR_PROMISC	1		/* EXPERIMENTAL */
#endif	PROMISCUOUS

#define	NETISR_IP	2		/* same as AF_INET */
#define	NETISR_NS	6		/* same as AF_NS */
#define	NETISR_ND	7		/* network disk protocol */

#define	schednetisr(anisr)	{ \
	GATESPL(s); \
	P_GATE(G_NETISR, s); \
	netisr |= 1<<(anisr); \
	V_GATE(G_NETISR, s); \
	setsoftnet(); \
}

#ifndef LOCORE
#ifdef KERNEL
extern	int	netisr;		/* scheduling bits for network */
#endif
#endif

#endif	/* _NET_NETISR_INCLUDED */
