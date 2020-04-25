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
 * $Header: vmparam.h 2.14 1991/06/03 20:49:09 $
 *
 * vmparam.h
 *	Various VM related machine-dependent constants for Intel 80386.
 */

/* $Log: vmparam.h,v $
 *
 *
 */

/*
 * Kernel address space layout.
 *
 * Kernel maps all physical memory 1-1, all useful IO space 1-1 (including
 * processor local resources).  See machine/hwparam.h for description of
 * IO space.
 *
 * User mapping consumes 1Gig of space, starting at 1Gig.  User segment
 * registers declare full 4Gig space, to allow FPA mapping to be in the
 * last 4Meg of the top of the 4Gig space (Intel standard).  Note that
 * the user segments wrap-around as a result, placing the FPA mapping
 * in the top 4Meg of the 1st Gig of address space.
 *
 * Various other utility maps (self-map page-table, processor-local stuff)
 * are arranged to top-out just below FPA mapping space.
 *
 * Space in-between is used for dynamically mapped kernel data structures
 * (buf-cache, Usrptmap[], etc).
 */

/*
 * VA_USER is the virtual address in the shared user+kernel address
 * space where user virtual space starts (for USER_SPACE bytes).
 * Use of constant segmentation descriptors allows the user code to
 * think it starts at virtual 0.
 *
 * The FPA is mapped just before user virtual space.  A process not using
 * FPA stores a zero in the level-1 slot; one using FPA points to shared
 * level-2 mapping page for the FPA (same physical address in each processor).
 *
 * Processor local data (VA_PLOCAL) and self-reference page-table (VA_PT)
 * are mapped just below FPA address space.
 *
 * Uarea is mapped RO into last page of user process address space
 * for faster context switch (VA_UAREA).
 *
 * Since per-processor Uarea is mapped using same L2 page as processor
 * local data (see struct priv_map), VA_PLOCAL+4Meg is constant virtual
 * address for top of this stack (VA_PRIVSTACK).
 *
 *	*** IF CHANGE MAPPING OF U-AREA, CHANGE machine/plocal.h ALSO ***
 *
 * L1IDX(VA_PLOCAL)	== index of context-switch invariant.
 * L1IDX(VA_PT)		== level-1 index to place self-ref map to whole PT.
 * L1IDX(VA_USER)	== index where user level-1 PT entries start.
 * L1IDX(VA_UAREA)	== place to map private Uarea in private kl1pt.
 * L1IDX(VA_FPA)	== level-1 PT index for user FPA mapping.
 */

#define KSOFF		(0)
#define USOFF		(VA_USER)

#define	FPA_SPACE	(1 * NPTEPG * NBPG)		/* 1 L1 page for this */

#define	VA_USER		(1*1024*1024*1024)		/* 1 Gig */
#define	USER_SPACE	(1*1024*1024*1024)		/* user space (bytes) */

#define	VA_FPA		(VA_USER - FPA_SPACE)		/* mapped FPA */

#define	VA_PT		(VA_FPA - KL1PT_PAGES * NPTEPG * NBPG)
#define	VA_PLOCAL	(VA_PT - PLMAP_PAGES * NPTEPG * NBPG)

#define	VA_PRIVSTACK	VA_PT
#define	VA_UBLOCK	(VA_USER + USER_SPACE - UPAGES * NBPG)
#define	VA_UAREA	(VA_USER + USER_SPACE - UPAGES * NBPG)

/*
 * Invariant virtual addresses, only valid in process mapping a
 * given page-table.
 *
 * VA_L1PT	== vaddr of 1st pte of self-mapped level-1 PT.
 * VA_FPA_PTE	== vaddr of level-1 pte that maps FPA mapping page.
 *
 * Level-1 pte that maps "l." (VA_PLOCAL) is preserved across context switch
 * by using l.plocal_pte.
 */

#define	VA_L1PT		VADDR(L1IDX(VA_PT), L1IDX(VA_PT))
#define	VA_FPA_PTE	(VA_L1PT + L1IDX(VA_FPA) * sizeof(struct pte))

/*
 * MAX_KER_VADDR gives the limit of normal kernel virtual space.
 * Mostly this limits number of allocated buffer headers.
 * Must be <= minimum of user virtual address range, IO mapping, utility
 * maps, etc.
 */

#define	MAX_KER_VADDR	VA_PLOCAL

/*
 * Relevant HW oriented addresses.
 */

#define	VA_SLIC		PHYS_SLIC		/* IO is 1-1 map */

/*
 * Address space sizing constants.
 *
 * Actual user allocatable space is limited to 256Meg for now (until
 * page the page-tables or whatever).  Thus max size user process still
 * has hole between data and stack.
 *
 * Allow "full" user space for each "segment".
 * Since the sum doesn't wrap around a 32-bit integer, this is sufficient.
 * All sizes in HW pages (4096-bytes for i386).
 *
 * Note that user level-1 page table is subset of shared kernel+user
 * level-1 page table.  Thus, KL1PT includes both kernel and user page-table.
 *
 * User page table is allocated concatenated with per-process KL1PT.
 */

#define	MAXADDR		(256 * 1024 * 1024)	/* allocatable virtual space (bytes) */

#define	MAXTSIZ		(MAXADDR/NBPG)		/* max text size (clicks) */
#define	MAXDSIZ		(MAXADDR/NBPG)		/* max data size (clicks) */
#define	MAXSSIZ		(MAXADDR/NBPG)		/* max stack size (clicks) */

#define	KL1PT_PAGES	1			/* # HW pages for KL1PT */
#define	KL1PT_BYTES	(KL1PT_PAGES*NBPG)	/* # bytes for KL1PT */

#define	UL1PT_PAGES	KL1PT_PAGES		/* ns32000 backwards compat */

/*
 * MAXUL2PTES is max number possible pages mapped by user page table.
 * MAXSZPT is max useful user PT, in mapping pages.
 */

#define	MAXUL2PTES	(MAXADDR / NBPG)	/* max # pte's in UL2PT */
#define	MAXSZPT		(MAXUL2PTES / NPTEPG)	/* max user PT pages */

/*
 * SZ_FULL_PT is size (bytes) of maximum size page-taable.
 */

#define	SZ_FULL_PT	(KL1PT_PAGES*NPTEPG*NBPG)

/*
 * LOWPAGES	# HW pages at start of text to provide zero at virtual zero
 *		or invalid page at zero. Text addresses are relocated to
 *		account for these magic pages at the start of text space.
 * HIGHPAGES	# HW pages at top of user address space reserved for
 *		system use.  In this case, mapped Uarea.
 * USRSTACK	is the top (end) of the user stack.
 *
 * NOTE: If change mapping of Uarea, must also change in plocal.h.
 */

#define	LOWPAGES	1			/* for 0@0 or inval@0 */
#define	HIGHPAGES	UPAGES			/* for Uarea mapped at top */
#define	USRSTACK	(USER_SPACE-HIGHPAGES*NBPG)

#define USRSTACKADJ	0			/* no a22 bug on 80386 */

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * DISKRPM is used to estimate the number of paging i/o operations
 * (per second) which one can expect from a single disk controller.
 */
#define	DISKRPM		60

/*
 * Klustering constants.  Klustering is the gathering
 * of pages together for pagein/pageout, while clustering
 * is the treatment of hardware page size as though it were
 * larger than it really is.
 *
 * KLMAX gives maximum cluster size in CLSIZE page (cluster-page)
 * units.  Note that ctod(KLMAX*CLSIZE) must be <= DMMIN in dmap.h, and
 * must divide DMMIN.
 */

#define	KLMAX	(dtoc(DMMIN)/CLSIZE)

/*
 * Swapping thresholds (see vm_sched.c).
 * Strategy:
 *	desfree is 400k bytes, but at most 1/8 of memory and at least 2%.
 *	minfree is 100k bytes, but at most 1/2 of desfree.
 *	dirtyhigh is 200k bytes, but at most 1/8 of memory.
 *	dirtylow is 100k bytes, but at most 1/2 of dirtyhigh.
 *	maxdirty is twice dirtyhigh.
 * The corresponding variables are run-time tunable in struct vmtune.
 */
#ifdef STHREE
#define	DESFREE		(200 * 1024)
#else
#define	DESFREE		(400 * 1024)
#endif
#define	DESFREEFRACT	8
#define	MINDESFREEPERCENT	2
#define	MINFREE		(100 * 1024)
#define	MINFREEFRACT	2

#define	DIRTYHIGH	(200 * 1024)
#define	DIRTYHIGHFRACT	8
#define	DIRTYLOW	(100 * 1024)
#define	DIRTYLOWFRACT	2

#define	MAXDIRTYMULT	2

/*
 * Believed thresholds for which interleaved swapping area is desirable.
 */
#define	LOTSOFMEM	4		/* megabytes */
#define	LOTSOFCPUS	4		/* 4 processor system */

/*
 * Floating-scale for scaled integer arithmetic in preference to
 * floating arithmetic.
 */
#define	FSCALE	1000

extern	int	Usrptsize;		/* # pte's for Usrptmap[] */
