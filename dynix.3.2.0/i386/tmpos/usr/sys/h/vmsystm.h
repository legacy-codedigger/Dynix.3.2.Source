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

/*
 * $Header: vmsystm.h 2.3 1991/05/29 17:34:12 $
 *
 * vmsystm.h
 *	Miscellaneous virtual memory subsystem variables and structures.
 */

/* $Log: vmsystm.h,v $
 *
 */

/*
 * VM System parameters; tunable.
 *
 * Where appropriate, the units (HW pages or clusters) are noted.
 * PFF rate units (PFFlow, PFFhigh) may change if faults/sec isn't
 * enough resolution.
 */

struct	vm_tune	{
	long	vt_minRS;	/* min # clusters for Rset */
	long	vt_maxRS;	/* max # clusters for Rset; limited by maxRS */
	long	vt_RSexecslop;	/* # HW pages slop in exec */
	long	vt_RSexecmult;	/* Rset size multipler */
	long	vt_RSexecdiv;	/* Rset size divider */
	int	vt_dirtylow;	/* dirty-list low-water mark (HW pages) */
	int	vt_dirtyhigh;	/* dirty-list high-water mark (HW pages) */
	int	vt_klout_look;	/* pageout kluster look-ahead (clusters) */
	long	vt_PFFvtime;	/* ticks between PFF adjusts; 0 ==> no PFF */
	long	vt_PFFdecr;	/* # clusters to decrement if PFF < PFFlow */
	long	vt_PFFlow;	/* low PFF rate, <= PFFhigh (faults/sec) */
	long	vt_PFFincr;	/* # clusters to add if PFF > PFFhigh */
	long	vt_PFFhigh;	/* high PFF rate (faults/sec) */
	long	vt_minfree;	/* free-list low bound for swap (HW pages) */
	long	vt_desfree;	/* free-list high bound for swap (HW pages) */
	long	vt_maxdirty;	/* max dirty-list before swap (HW pages) */
};

#ifdef	KERNEL
extern	struct	vm_tune	vmtune;	/* tuneable VM parameters */

extern	long	maxRS;		/* max # clusters possible for Rset */
extern	long	minRS;		/* min # clusters possible for Rset */
extern	long	maxRSslop;	/* slop for maxRS calculation */

extern	long	forkmap;	/* fork memory "resource" */
extern	long	forkmapmult;	/* boot-time multipler for init freemem */
extern	long	forkmapdiv;	/* boot-time divisor for init freemem */
extern	long	maxforkRS;	/* max # clusters when doing fork */
extern	lock_t	forkmap_mutex;	/* mutex's forkmap manipulation */
extern	sema_t	forkmap_wait;	/* wait here for formap space */

extern	int	freemem;	/* remaining blocks of free memory */
extern	int	dirtymem;	/* # clusters on dirty-list */
extern	sema_t	drain_dirty;	/* pageout daemon waits here for work to do */

extern	lock_t	swpq_mutex;	/* swapout-queue manipulation mutex */
extern	short	swpq_head;	/* swapout-queue head index */
extern	short	swpq_tail;	/* swapout-queue tail index */
extern	sema_t	swapout_sync;	/* swapper sync with swapouts */

extern	int	avefree;	/* moving average of remaining free blocks */
extern	int	avefree30;	/* 30 sec (avefree is 5 sec) moving average */
extern	int	avedirty;	/* moving average of size of dirty list */
extern	int	avedirty30;	/* 30 sec (avedirty is 5 sec) moving average */
extern	int	deficit;	/* estimate of needs of new swapped in procs */

/* writable copies of tunables */
extern	int	maxpgio;	/* max paging i/o per sec before start swaps */
extern	int	maxslp;		/* max sleep time before very swappable */
#endif	KERNEL

/*
 * For vm_ctl() system call.
 */

#define	VM_GETPARAM	0	/* return copy of vmtune */
#define	VM_SETPARAM	1	/* write into vmtune (root only) */
#define	VM_GETRS	2	/* return current Rset size */
#define	VM_SETRS	3	/* set current Rset size */
#define	VM_SWAPABLE	4	/* declare process swappable or not */
#define	VM_PFFABLE	5	/* declare process PFF-able or not */

/*
 * Swap kind accounting.
 */

struct	swptstat
{
	int	pteasy;		/* easy pt swaps */
	int	ptexpand;	/* pt expansion swaps */
};

#if	defined(KERNEL) && defined(PERFSTAT)
extern	struct	swptstat swptstat;
#endif
