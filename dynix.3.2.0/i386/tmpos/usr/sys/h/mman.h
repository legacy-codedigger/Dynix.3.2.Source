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
 * $Header: mman.h 2.11 89/10/30 $
 *
 * mman.h
 *	Structures and definitions for memory mapping support.
 */

/* $Log:	mman.h,v $
 */

/*
 * mmap() system-call interface definitions.
 *
 * In the current implementation:
 *	PROT_WRITE ==> PROT_READ.
 *	PROT_EXEC insists on PROT_READ, and arranges caching of the map.
 */

#define	PROT_READ	0x4		/* read access */
#define	PROT_WRITE	0x2		/* write access */
#define	PROT_EXEC	0x1		/* executable access */

#define	PROT_RDWR	(PROT_READ|PROT_WRITE)
#define	PROT_BITS	(PROT_READ|PROT_WRITE|PROT_EXEC)

#define	PROT_LASTFD	0x8		/* internal state, orthog to above */

#define	MAP_SHARED	1		/* shared modifications */
#define	MAP_PRIVATE	2		/* private modifications */
#define	MAP_ZEROFILL	3		/* pages are zero-filled, private */

/*
 * Mapping operations structure -- defines a mapper.
 * There is a set of map operations per type of file (eg, VREG, VCHR).
 */

struct	mapops	{
	int	(*map_new)();		/* create a new map */
	int	(*map_dup)();		/* dup ref to map (fork) */
	int	(*map_unmap)();		/* release reference to map */
	int	(*map_swpout)();	/* swap out ref to map */
	int	(*map_swpin)();		/* swap in ref to map */
	int	(*map_refpg)();		/* get ref to page */
	int	(*map_derefpg)();	/* remove page ref */
	int	(*map_realloc)();	/* drop reclaim link to page */
	int	(*map_pgout)();		/* page-out page */
	int	(*map_stat)();		/* get info about the map */
	int	(*map_err)();		/* import an error to the map */
};

/*
 * Per-process array of struct mmap keeps track of current mmap's.
 * mm_pgcnt < mm_size if some pages are unmapped or re-mapped.
 * mm_pgcnt == 0 ==> entry is unused.
 *
 * mm_prot holds maximimum possible protection mapped by this entry; must
 * have enough bits to fit PROT_BITS.
 */

struct	mmap	{
	u_long		mm_off;		/* 1st file offset mapped (HW pages) */
	size_t		mm_1stpg;	/* 1st vpn in process */
	size_t		mm_size;	/* size mapped (HW pages) */
	size_t		mm_pgcnt;	/* # HW pages mapped */
	struct	mapops	*mm_ops;	/* mapper operations */
	u_long		mm_handle;	/* identifies mapped object */
	short		mm_fdidx;	/* fd index mapped */
	char		mm_prot;	/* "prot" bits mapped by this entry */
	u_char		mm_paged: 1,	/* paged? else phys */
			mm_noio:  1,	/* IO services prohibited? */
			mm_lastfd:1,	/* last fd ref is getting closed */
			mm_cor:   1,	/* copy-on-ref these pages */
			mm_text:  1,	/* is this a "text" map? */
				: 3;	/* reserved */
};

#ifdef	KERNEL
/*
 * Interface (mmap functions) definitions.
 *
 * These are used in character driver mapping functions (single function
 * based on older interface).
 */

#define	MM_MAP		0	/* verify, set up map:	(offset, size, prot) */
#define	MM_UNMAP	1	/* done with map:	(offset, size) */
#define	MM_SWPOUT	2	/* swap out ref to map:	(offset, size) */
#define	MM_SWPIN	3	/* swap in ref to map:	(offset, size) */
#define	MM_REFPG	4	/* add ref to page:	(offset) */
#define	MM_DEREFPG	5	/* remove page ref:	(offset) */
#define	MM_REALLOC	6	/* loose claim to page:	(page, ndx) */
#define	MM_PGOUT	7	/* page-out page:	(page, ndx) */

/*
 * MM_PHYS, MM_PAGED, MM_NPMEM are returned from MM_MAP calls,
 * encoded to avoid ambiguity with error return codes.
 *
 * System services are supported for MM_PAGED and MM_NPMEM mapped space,
 * disallowed for MM_PHYS (assumes non-support of general accesses).
 *
 * Note: mmap() assumes all are <= 0.
 */

#define	MM_PAGED	0		/* device will be page-mapped */
#define	MM_PHYS		-1		/* physically mapped, can't do IO */
#define	MM_NPMEM	-2		/* non-paged memory; IO is ok */

/*
 * Certain kinds of errors result in MM_REFPG requests failing.
 * These errors are encoded in bits that can't appear in page-aligned
 * physical addresses.  These errors may also be passed into the map_err()
 * function.
 */

#define	MM_STALETEXT	0x01		/* text was written on (NFS) */
#define	MM_IOERR	0x02		/* map had an IO error */
#define	MM_BADMAP	(MM_STALETEXT|MM_IOERR)

/*
 * Pageout interface structure.  Passed back to pageout() from a map_pgout().
 */

struct	pgout	{
	struct	pte	*po_pte;	/* 1st pte of set to write */
	daddr_t		po_blkno;	/* disk address of 1st page */
	int		po_cnt;		/* # (HW) pages involved */
	struct vnode	*po_devvp;	/* device to write to */
};

/*
 * Map status structure.  Used by swapout() to compute memory usage heuristics.
 */

struct	map_stat {
	int	ms_count;		/* current # references to the map */
	int	ms_ccount;		/* # of above actually in memory */
};

/*
 * PTETOMAPOFF	turn pte into offset in mapped object (assumes pte
 *		is data-segment pte for now)
 */

#define	PTETOMAPOFF(p, pte, um) \
		((um)->mm_off + (dptetov(p, pte) - (um)->mm_1stpg))

/*
 * machine/pte.h defines macros for manipulating map-state in pte's.
 */

/*
 * Mapped-file structure.  Used to represent mapped vnode.
 *
 * Use vnode sema to lock mfile table entry creation/deletion,
 * mf_swmutex to lock swap in/out.
 *
 * Multiple mfile's for a given vnode are doubly-linked thru mf_next,
 * mf_prev.  mf_vp->v_mapx is index of first mfile of set for the vnode
 * (sorted by increasing mf_pos, individual mfile's don't intersect).
 *
 * Seperate free list and vnode linked list to allow caching of unused
 * mfile's (ie, on free list but reclaimable; "tacky" text).  Cached mfile
 * entry is on free list with mf_count == 0, and mf_vp still referencing
 * the mapped vnode (!= NULL).  Truly free mfile entry has mf_vp == NULL.
 */

struct	mfile	{
	struct	mfile	*mf_next;	/* vnode list next entry */
	struct	mfile	*mf_prev;	/* vnode list prev entry */
	struct	mfile	*mf_nfree;	/* next free entry */
	struct	mfile	*mf_pfree;	/* previous free entry */
	struct	vnode	*mf_vp;		/* vnode mapped */
	char		mf_flag;	/* flags: see below */
	short		mf_count;	/* reference count */
	short		mf_ccount;	/* number of loaded references */
	size_t		mf_size;	/* size (HW pages; incl's null pte's) */
	size_t		mf_zsize;	/* contig non-zero pte's from mf_pos */
	u_long		mf_pos;		/* start position in file (HW pages) */
	struct	pte	*mf_pt;		/* page-table (0 ==> not-resident) */
	swblk_t		mf_ptdaddr;	/* disk address of page table */
	sema_t		mf_swmutex;	/* swap in/out mutex */
};

/*
 * Flag bits.
 */

#define	MF_PTDIRTY	0x01			/* dirty PT -- needs swap */
#define	MF_UNMAP_RACE	0x02			/* lost race with unmap */

/*
 * MFSZPT: size of page-table for mfile; mf_size is guaranteed a multiple of
 *	NPTEPG (mmreg_newmf()).
 */

#define	MFSZPT(mf)	((mf)->mf_size / NPTEPG)

/*
 * MF_LOCK(), MF_UNLOCK(): lock individual mfile.  Locks mf_count, mf_ccount,
 * and page-table (for swap in/out).
 *
 * To avoid deadlocks with swapper, process holding mfile locked is not
 * swappable.
 *
 * Should rename mf_swmutex to mf_mutex, since it's now more general.
 */

#define	MF_LOCK(mf)   { \
		++u.u_procp->p_noswap; \
		p_sema(&(mf)->mf_swmutex,PSWP); \
	}
#define	MF_UNLOCK(mf) { \
		v_sema(&(mf)->mf_swmutex); \
		--u.u_procp->p_noswap; \
	}
#define	MF_XFR_LOCK(mf, vp, s) { \
		++u.u_procp->p_noswap; \
		p_sema_v_lock(&(mf)->mf_swmutex, PSWP, &(vp)->v_mutex, (s)); \
	}
#define	MF_BLOCKED(mf)			blocked_sema(&(mf)->mf_swmutex)

/*
 * MFOFFTOPTE:	turns offset from mf_pos into pointer to relevant pte.
 * MFINTERSECT:	true iff mfile entry intersects [pos, pos+size).
 * MFOVERLAP:	size of intersection (in HW pages), assuming non-null.
 * MFIGNOREMOD:	true if modifications to pages can be ignored for this vnode.
 */

#define	MFOFFTOPTE(mf, off)	((mf)->mf_pt + (off))
#define	MFINTERSECT(mf, p, s)	\
		((mf)->mf_pos < (p)+(s) && (mf)->mf_pos + (mf)->mf_size > (p))
#define	MFOVERLAP(mf, p, s)	\
		imin((int)(((mf)->mf_pos + (mf)->mf_size) - (p)), (s))
#define	MFIGNOREMOD(lfd, vp)	\
		((lfd) && (vp)->v_count == 2 && ((vp)->v_flag & VNOLINKS))

/*
 * Macros to lock, unlock mfile free-list.
 */

#define	LOCK_MFILES(s)		s = p_lock(&mfile_list, SPLIMP)
#define	UNLOCK_MFILES(s)	v_lock(&mfile_list, (s))

/*
 * Macros to insert/delete mfile from free list.
 */

#define	MF_INS_HEAD_FREE(mf) { \
	(mf)->mf_pfree = &mfile_free; \
	(mf)->mf_nfree = mfile_free.mf_nfree; \
	mfile_free.mf_nfree->mf_pfree = (mf); \
	mfile_free.mf_nfree = (mf); \
}
#define	MF_INS_TAIL_FREE(mf) { \
	(mf)->mf_nfree = &mfile_free; \
	(mf)->mf_pfree = mfile_free.mf_pfree; \
	mfile_free.mf_pfree->mf_nfree = (mf); \
	mfile_free.mf_pfree = (mf); \
}
#define	MF_RM_FREE(mf) { \
	(mf)->mf_nfree->mf_pfree = (mf)->mf_pfree; \
	(mf)->mf_pfree->mf_nfree = (mf)->mf_nfree; \
}

extern	int		nmfile;			/* # of mfile's */
extern	struct	mfile	*mfile;			/* the array of mfile's */
extern	struct	mfile	mfile_free;		/* head of free list */
extern	lock_t		mfile_list;		/* lock list for alloc */
extern	struct	mapops	*vnode_mmap[];		/* v_type -> mapops */
#endif	KERNEL
