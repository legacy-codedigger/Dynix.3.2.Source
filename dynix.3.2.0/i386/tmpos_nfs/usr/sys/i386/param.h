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
 * $Header: param.h 2.11 90/11/08 $
 *
 * param.h
 *	Machine dependent constants for Intel 80386.
 */

/* $Log:	param.h,v $
 */

#define	NBPG	4096		/* bytes/page */
#define	PGOFSET	(NBPG-1)	/* byte offset into page */
#define	PGSHIFT	12		/* LOG2(NBPG) */

#define	CLSIZE		1	/* 4K "logical" pages */
#define	CLSIZELOG2	0

#define	SSIZE		1	/* initial stack size/NBPG */
#define	SINCR		1	/* increment of stack/NBPG */

#define	NUMMAP		12	/* # mmap's per process */

#define	UPAGES		1	/* pages of u-area */

#define	UPTMAPMULT	3	/* nproc mult for uptmap[] # entries */
				/* to compensate for fragmentation caused by */
				/* multiple allocations per process */

#define	MAXNUMCPU	30	/* max supported # processors.  Used in */
				/* various statistics, most of */
				/* kernel is independent of this */

#define	MAXNUMSEC	6	/* max supported # SEC controllers. */
				/* Used in autoconf, etc. especially for */
				/* handling ambiguous wildcards. */
				/* Most of kernel is independent of this. */
#define MAXNUMSSM	6	/* max supported # SSM controllers. */
				/* Used in autoconf, etc. especially for */
				/* handling ambiguous wildcards. */
				/* Most of kernel is independent of this. */

#define SSM_WDT_TIMEOUT	4	/* Watchdog timer timeout value in seconds */
				/* Watchdog timer reset every 1/2 second */

#define WDT_TIMEOUT	10	/* Watchdog timer timeout value in seconds */
				/* Watchdog timer reset every 2 second */

#define	MEMINTVL	(60*10)	/* Memory ecc error polling interval */
				/* Check for errors every 10 minutes */

/*
 * Some macros for units conversion
 */

/* Core clicks (4096 bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (4096 bytes) to disk blocks */
#define	ctod(x)	((x)*(NBPG/DEV_BSIZE))
#define	dtoc(x)	((x)/(NBPG/DEV_BSIZE))
#define	dtob(x)	((x)*DEV_BSIZE)

/* clicks to bytes */
#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	(((int)(x)+NBPG-1)>>PGSHIFT)

/*
 * Macro to decode processor status word.
 */

#define	USERMODE(csreg) (((csreg) & RPL_MASK) != KERNEL_RPL)

#define	DELAY(n)	{ register int N = calc_delay((unsigned int)n); while (--N > 0); }
