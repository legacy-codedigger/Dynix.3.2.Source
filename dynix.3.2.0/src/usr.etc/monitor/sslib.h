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

/*
 * $Header: sslib.h 2.1 87/04/10 $
 */

/*
 * sslib.h
 *
 * defines associated with the collection of data from the kernel
 */

struct ss_cfg {
	unsigned Nengine;	/* number of processors in system */
	unsigned nonline;	/* number of processors online */
	unsigned dk_nxdrive;	/* number of drives monitored */
	unsigned nse_unit;	/* number of scsi/ether units */
};

struct proc_stat {
	unsigned processes;
	unsigned running;
	unsigned runnable;
	unsigned fastwait;
	unsigned sleeping;
	unsigned swapped;
};

struct ether_stat {
	unsigned pktin;		/* number of packets recieved */
	unsigned pktout;	/* number of packets transmitted */
};

/*
 * File/record locking stats (derived from struct flckinfo)
 */

struct flock_stats {
	unsigned lck_ut;	/* percent locks utilized */
	unsigned fil_ut;	/* percent locks utilized */
};
