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
 * $Header: cmap.h 2.7 87/04/06 $
 *
 * cmap.h
 *	Core-map structure.
 *
 * One per allocatable page of memory, as determined at boot time.
 */

/* $Log:	cmap.h,v $
 */

/*
 * There is one struct cmap entry per page of "free space".
 * They are threaded into a clean and dirty free-list thru c_next and c_prev
 * (pointers are used to allow this to represent very large physical memories).
 */

struct	cmap	{
struct	cmap	*c_next,	/* next free list entry */
		*c_prev;	/* previous free list entry */
unsigned long	c_page;		/* virtual page number in segment */
unsigned short	c_refcnt,	/* page-table reference-count */
		c_iondx;	/* index of proc waiting for share pagein IO */
unsigned long	c_ndx;		/* index of owner proc or map (may be dev_t) */
unsigned char	c_type	: 2,	/* type: CSYS, CSTACK, CDATA, CMMAP */
		c_dirty	: 1,	/* page is dirty (eg, needs to be paged-out) */
		c_holdfod:1,	/* holds inode FOD page, c_blkno valid */
		c_pageout:1,	/* pageout daemon is writing the page */
		c_gone	: 1,	/* associated page has been released */
		c_intrans:1,	/* in-transit bit */
		c_iowait: 1;	/* c_iondx valid; there is a waiter */
struct	mapops	*c_mapf;	/* mapper functions for c_type==CMMAP */
unsigned int	c_blkno;	/* disk block this is a copy of */
};

/*
 * Values defined for c_type.
 */

#define	CSYS		0		/* none of below */
#define	CMMAP		1		/* belongs to mapped vnode file */
#define	CDATA		2		/* belongs to private data segment */
#define	CSTACK		3		/* belongs to private stack segment */

/*
 * c_dirty, c_pageout, and c_refcnt encode the state of the associated page
 * as follows:
 *
 *	c_pageout	c_dirty		c_refcnt
 * (0)	    0		   0		    0		on free-list
 * (1)	    0		   0		  > 0		ref'd in a Page-Table
 * (2)	    0		   1		    0		on dirty-list
 * (3)	    0		   1		  > 0		BAD STATE
 * (4)	    1		   0		    0		being cleaned
 * (5)	    1		   0		  > 0		reclaimed during pageout
 * (6)	    1		   1		    0		see below
 * (7)	    1		   1		  > 0		BAD STATE
 *
 * The c_gone bit implies the is no need for the page any further.  As
 * soon as c_refcnt == c_pageout == 0, the page is placed on the free-list
 * (at the head).
 *
 * State (6) is the result of a page being reclaimed while pageout IO is
 * occurring, being modified *again*, and released (eg, pushed out of
 * Rset).
 *
 * State (7) doesn't really exist, since the c_dirty bit gets copied to
 * the pte's of the referencing page-table; thus, this reduces to state
 * (5) with 'm'-bit on in referencing pte's.
 * 
 * c_pageout is a bit rather than a counter, since a c_pageout page is not
 * placed again on a free-list until the IO is complete.  Thus, pageout()
 * will *not* start another IO on it.  The page may be reclaimed by its
 * "owner" or released completely (c_gone) but not reallocated until IO is
 * complete.
 *
 * c_holdfod is a reclaimable page on the free-list the holds a RO copy of
 * a fill-on-demand page from a vnode.  This bit allows us to re-encode
 * the FOD pte's in a referencing page-table in the event the page is
 * reallocated first before being reclaimed.
 */

/*
 * Macros to insert a page into a free-list.
 */

#define	CM_INS_HEAD_CLEAN(c) { \
	(c)->c_prev = &cm_clean; \
	(c)->c_next = cm_clean.c_next; \
	cm_clean.c_next->c_prev = (c); \
	cm_clean.c_next = (c); \
	freemem += CLSIZE; \
}
#define	CM_INS_TAIL_CLEAN(c) { \
	(c)->c_next = &cm_clean; \
	(c)->c_prev = cm_clean.c_prev; \
	cm_clean.c_prev->c_next = (c); \
	cm_clean.c_prev = (c); \
	freemem += CLSIZE; \
}
#define	CM_INS_TAIL_DIRTY(c) { \
	(c)->c_next = &cm_dirty; \
	(c)->c_prev = cm_dirty.c_prev; \
	cm_dirty.c_prev->c_next = (c); \
	cm_dirty.c_prev = (c); \
	dirtymem += CLSIZE; \
}

/*
 * Macros to remove a page from a free-list, optionally keeping counts.
 */

#define	CM_UNLINK_FREE(c)	{ \
	(c)->c_prev->c_next = (c)->c_next; \
	(c)->c_next->c_prev = (c)->c_prev; \
}
#define	CM_UNLINK_DIRTY(c)	{ CM_UNLINK_FREE(c); dirtymem -= CLSIZE; }
#define	CM_UNLINK_CLEAN(c)	{ CM_UNLINK_FREE(c); freemem -= CLSIZE; }

/*
 * CM_UNLINK() unlinks a page frame from the free list (dirty or clean).
 * Used when the page being reclaimed from a free list.
 */

#define	CM_UNLINK(c) \
	if ((c)->c_dirty) { CM_UNLINK_DIRTY(c); } else { CM_UNLINK_CLEAN(c); }

/*
 * PTETOCMAP:	turn pte into cmap entry mapped by that pte.
 * CMAPTOPHYS:	turn cmap entry into physical address.
 */

#define	PTETOCMAP(pte)	(&cmap[ PTETOPHYS(pte) >> (PGSHIFT + CLSIZELOG2) ])
#define	CMAPTOPHYS(c)	(((c) - cmap) << (PGSHIFT + CLSIZELOG2))

#ifdef	KERNEL
extern	struct	cmap cm_clean;		/* heads clean free list */
extern	struct	cmap cm_dirty;		/* heads dirty free list */
extern	struct	cmap *cmap;		/* base of cmap table */
extern	struct	cmap *ecmap;		/* end of cmap table */
extern	int	ecmx;			/* initial free-list size (for `ps') */
extern	lock_t	mem_alloc;		/* locks access to free-list */
extern	sema_t	mem_wait;		/* wait here for memory */
#endif	KERNEL
