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
#ifndef _NETIF_IF_SE_INCLUDED
#define _NETIF_IF_SE_INCLUDED

/*
 * $Header: if_se.h 2.7 90/05/25 $
 */

/* $Log:	if_se.h,v $
 */

/*
 * se_counts - statistics taken from the interface.
 */
struct se_counts {
	u_long	ec_rx_ovfl;
	u_long	ec_rx_crc;
	u_long	ec_rx_dribbles;
	u_long	ec_rx_short;
	u_long	ec_rx_good;
	u_long	ec_tx_unfl;
	u_long	ec_tx_coll;
	u_long	ec_tx_16xcoll;
	u_long	ec_tx_good;
};
/*
 * SEC-related types.
 *
 * Due to the SCSI-related interface to the Ether firmware,
 * it is helps to hide the interface from the driver.
 * Some day some of this should be made available for the other SEC
 * drivers but time does not permit that now.
 */

/*
 * sec_pq describes the complete state of a device program
 * queue for the SCSI/Ether controller.
 * For unknown reasons, the size of the queue is not included
 * in the standard SCSI/Ether interface.
 */

struct sec_pq {
	struct sec_progq	*sq_progq;
	u_short			sq_size;
};


/*
 * This type records the state of a queue of sec_iat structures.
 * These are used for both input and output of Ether packets.
 */

struct sec_iatq {
	struct sec_iat	*iq_iats;	/* ring of iats itself */
	u_short		iq_size;	/* number of entries in the array */
	u_short		iq_head;	/* index of next available iat */
};

struct sec_mbufq {
	struct mbuf	**mq_mbufp;
	struct mbuf	**mq_head;
	struct mbuf	**mq_tail;
	struct mbuf	**mq_end;
};

#define	EBUF_SZ		3*512	/* allocate 1.5K for ether buffers */

struct se_xbuf {
	struct se_xbuf*	se_xnext;
	u_char		se_xdata[EBUF_SZ - sizeof(struct se_xbuf*)];
};

/*
 * Ethernet software state per interface(one for each controller).
 * It contains 3 segments: details about the controller;
 * details about the input side; and details about the output side.
 * Locks exist on each of the separate segments, and should
 * be used to lock as little about the state as is necessary.
 *
 * It is assumed throughout that both input and output will be
 * locked whenever any of the controller details are changed.
 *
 * Locking rules to avoid deadlock are as follows:
 *	- Lock the output data before the controller data.
 *	- Lock the interrupt lock (output only) after the data.
 */

struct	se_state {
						/* Describing the controller: */
	struct arpcom		ss_arp;		/* Ethernet common */
	lock_t			ss_lock;	/* mutex lock */
	struct se_counts	ss_sum;		/* statistics summary */
	int			ss_scan_int;	/* stat scan interval */
	u_short			ss_ether_flags;	/* SETMODE flags */
	u_char			ss_slic;	/* slic address of SEC */
	u_char			ss_bin;		/* bin to intr SEC with */
	u_char			ss_alive:1,	/* controller alive? */
				ss_initted:1,	/* filled in? */
				ss_init_called:1;
						/* Describing the input half: */
	lock_t			is_lock;	/* interrupt lock */
	struct sec_cib		*is_cib;	/* input cib */
	int			is_status;	/* cib's status var */
	struct sec_pq		is_reqq;	/* request queue */
	struct sec_pq		is_doneq;	/* done queue */
	struct sec_iatq		is_iatq;	/* queue of iats */
	struct sec_mbufq	is_mbufq;	/* corresponding mbuf list */
	u_char			is_initted:1;	/* filled in? */
						/* Describing the output half */
	lock_t			os_lock;	/* mutex lock */
	struct sec_cib		*os_cib;	/* output cib */
	struct se_xbuf		*os_xfree;	/* free list of xmit bufs */
	int			os_status;	/* cib's status var */
	struct sec_pq		os_reqq;	/* request queue */
	struct sec_pq		os_doneq;	/* done queue */
	struct sec_iatq		os_iatq;	/* iats for output */
	struct sec_mbufq	os_mbufq;	/* corresponding mbuf list */
	struct sec_gmode	os_gmode;	/* temp for SINST_GETMODE */
	int			os_npending;	/* number of pending writes */
	u_char			os_initted:1,	/* filled in */
				os_intr:1;	/* in xmit intr */
	long			ss_pad;		/* 4-byte pad for bitfields */
};


#ifdef KERNEL

#ifdef	ns32000			/* no SLIC gates in SGS */
extern gate_t se_gate;		/* gate for locks */
#endif	ns32000

extern int se_watch_interval;	/* seconds between stats collection */
extern int se_write_iats;	/* number of IATs for Ether writes */
extern int se_bin;		/* bin number to interrupt SEC on mIntr */
extern short se_mtu;		/* max transfer unit on se interface */

#ifdef DEBUG
extern int se_ibug, se_obug;	/* debug flags */
#endif DEBUG
#endif KERNEL
#endif	/* _NETIF_IF_SE_INCLUDED */
