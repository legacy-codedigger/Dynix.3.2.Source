/*
 * $Copyright: $
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
#ifndef _NETIF_PCI_INCLUDED
#define _NETIF_PCI_INCLUDED

/*
 * $Header: pci.h 2.3 1991/05/13 18:37:29 $
 */
 
/*
 * $Log: pci.h,v $
 *
 */

#define PCI_PRI		(PZERO+8)
#define PCI_MISC_UNIT	0		/* default input unit */

#define PCI_BASE	0x400		/* Base of ether types used */
#define PCI_TYPE	0x0600		/* ether_type for EtherSeries */

#define SZ_EHEAD	sizeof(struct ether_header)

struct pci_unit {
	sema_t		pcu_read;	/* Read ready semaphore */
	struct mbuf	*pcu_chain;	/* Chain of pending packets */
	lock_t		pcu_lock;	/* Unit lock */
	short		pcu_state;	/* Unit state */
	short		pcu_pending;	/* Number of pending packets */
	short		pcu_maxpend;	/* Max number of pending packets */
};

/* pcu_state states */
#define PCI_OPEN	1
#define PCI_CLOSED	0


struct pci_state {
	struct ifnet	*pci_ifp;		/* Pointer to sender */
	u_char		pci_ether_addr[6];	/* My Ethernet address */
	struct pci_unit	*pci_units;		/* Pointer to units */
	short		pci_nunits;		/* Number of units */
	struct pci_stats {
		u_long	pci_dropped;		/* Number of packets we lost */
		u_long	pci_nreceived;		/* Total packets received */
		u_long	pci_write_fails;	/* Writes with ENOBUFS */
	}		pci_stats;
};

#define EIOCGETA	_IOR(P, 1, u_char[6])
#define EIOCFLSH	_IOR(P, 2, int)

#ifdef KERNEL
#ifdef	ns32000				/* no SLIC gates in SGS */
extern gate_t pci_gate;			/* Gate used for locks*/
#endif	ns32000
extern int pci_debug;			/* debug level */
extern int pci_qsizes[];		/* Size of queue for each unit */
extern int pci_nqsizes;			/* number of entries in pci_qsizes */
extern	int (* pciattach_fctn)();
extern	int (* pcirint_fctn)();
#endif KERNEL
#endif	/* _NETIF_PCI_INCLUDED */
