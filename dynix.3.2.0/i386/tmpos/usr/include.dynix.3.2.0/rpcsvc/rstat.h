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

/* $Header: rstat.h 1.6 87/07/22 $ */

/*	@(#)rstat.h 1.1 86/02/05 SMI */
/* @(#)rstat.h	2.1 86/04/14 NFSSRC */

/* 
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef DST_NONE
#include <sys/time.h>
#endif

#ifndef DK_NDRIVE
#include <sys/dk.h>
#ifdef	sequent
#define	DK_NDRIVE 4			/* no longer exists on sequent */
#endif
#endif

#ifndef	CPUSTATES
#include <sys/vmmeter.h>
#endif

#define RSTATPROG 100001
#define RSTATVERS_ORIG 1
#define RSTATVERS_SWTCH 2
#define RSTATVERS_TIME  3
#define RSTATVERS 3
#define RSTATPROC_STATS 1
#define RSTATPROC_HAVEDISK 2

struct stats {				/* version 1 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
};

struct statsswtch {				/* version 2 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	unsigned v_swtch;
	long avenrun[3];
	struct timeval boottime;
};
struct statstime {				/* version 3 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	unsigned v_swtch;
	long avenrun[3];
	struct timeval boottime;
	struct timeval curtime;
};

int xdr_stats();
int xdr_statsswtch();
int xdr_statstime();
int havedisk();
