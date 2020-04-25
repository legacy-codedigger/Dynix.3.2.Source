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

#ifndef	lint
static	char	rcsid[] = "$Header: vm_text.c 2.11 90/06/03 $";
#endif

/*
 * vm_text.c
 *	Management of shared-text sements.
 *
 * Shared-texts are really a sub-case of mapped files and are implemented
 * here as such.  Some of the old interfaces are kept for convenience of
 * calling code (eg, xrele()).
 *
 * The implementation assumes VREG vnodes are being mapped, and the
 * implementation is file-system independent.  This can be extended to
 * file-system dependent mapping as necessary by deriving the map
 * allocator and mapping function from the vfs identified in the vnode,
 * and modifying xrele(), xflush(), xumount() accordingly.
 *
 * The text of an executable binary is mapped read only, not copy-on-ref.
 * Data is mapped read-write, copy-on-ref.
 * Invalid addresses at zero are implemented by zapping the pte's.
 * Zero address space at zero is implemented by an internal mapping function.
 * Any/all of these mappings may be changed by calls to mmap() or munmap().
 */

/* $Log:	vm_text.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/kernel.h"
#include "../h/vm.h"

#include "../machine/exec.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"

#define	MM_TEXT		0			/* text u_mmap[] index */
#define	MM_DATA		1			/* data u_mmap[] index */
#define	MM_ZERO		2			/* 0@0 u_mmap[] index */

static	struct	mmap	text_mmap;		/* prototype text mmap entry */
static	struct	mmap	data_mmap;		/* prototype data mmap entry */
static	struct	mmap	zero_mmap;		/* prototype 0@0 mmap entry */

static	struct	pte	zero_at_zero_ptes[LOWPAGES];	/* 0@0 pte's */

extern	struct	mapops	mmap_reg;		/* VREG mapping functions */

/*
 * zero_at_zero()
 *	Generic mmap function for zero @ zero pages.
 *
 * NOP, since only known locally, and only interesting operation is "dup"
 * which doesn't even return a value.  Not called for swapin/out since set
 * up as !paged above.
 */

static
zero_at_zero() {}

static	struct	mapops	zero_at_zero_ops = {
	zero_at_zero,		/* create a new map */
	zero_at_zero,		/* dup ref to map (fork) */
	zero_at_zero,		/* release reference to map */
	zero_at_zero,		/* swap out ref to map */
	zero_at_zero,		/* swap in ref to map */
	zero_at_zero,		/* get ref to page */
	zero_at_zero,		/* remove page ref */
	zero_at_zero,		/* drop reclaim link to page */
	zero_at_zero,		/* page-out page */
	zero_at_zero,		/* get information about the map */
	zero_at_zero,		/* import an error to the map */
};

/*
 * xinit()
 *	Init text things.
 *
 * Init 0@0 mapping pte's and prototype struct mmap's for xinitpt().
 *
 * Uses zeropg[] exists (see mfinit()).
 */

xinit()
{
	register int i;

	/*
	 * Init zero @ zero pte's.
	 */

	for (i = 0; i < LOWPAGES; i += CLSIZE) {
		copycl(&zero_at_zero_ptes[i], zeropg);
		*(int*)&zero_at_zero_ptes[i] |= MAPXTOPTE(MM_ZERO)|PG_R|PG_V|RO;
	}

	/*
	 * Init prototype text mmap entry.
	 * mm_text allows chgprot() to fuss with the pages in traced process.
	 */

	text_mmap.mm_off = 0;			/* 1st mapped offset */
	text_mmap.mm_1stpg = LOWPAGES;		/* 1st vpn in process */
	text_mmap.mm_ops = &mmap_reg;		/* mapping operations */
	text_mmap.mm_fdidx = -1;		/* no fd index mapped */
	text_mmap.mm_prot = PROT_READ|PROT_EXEC;/* "prot" bits */
	text_mmap.mm_paged = 1;			/* paged map */
	text_mmap.mm_noio = 0;			/* IO services allowed */
	text_mmap.mm_lastfd = 1;		/* last fd ref (no fd, really)*/
	text_mmap.mm_cor = 0;			/* don't copy-on-ref the pgs */
	text_mmap.mm_text = 1;			/* this is a "text" */

	/*
	 * Init prototype data mmap entry.
	 */

	data_mmap = text_mmap;			/* mostly same as text */
	data_mmap.mm_cor = 1;			/* do copy-on-ref the pgs */
	data_mmap.mm_text = 0;			/* this is not a "text" */

	/*
	 * Init prototype zero @ zero mmap entry.
	 * Note: no variable fields.
	 */

	zero_mmap.mm_off = 0;			/* 1st mapped offset */
	zero_mmap.mm_1stpg = 0;			/* 1st vpn in process */
	zero_mmap.mm_size = LOWPAGES;		/* size mapped  */
	zero_mmap.mm_pgcnt = LOWPAGES;		/* # pages mapped */
	zero_mmap.mm_ops = &zero_at_zero_ops;	/* mapping operations */
	zero_mmap.mm_handle = 0;		/* identifies mapped object */
	zero_mmap.mm_fdidx = -1;		/* no fd index mapped */
	zero_mmap.mm_prot = PROT_READ;		/* "prot" bits */
	zero_mmap.mm_paged = 0;			/* paged map */
	zero_mmap.mm_noio = 0;			/* IO services allowed */
	zero_mmap.mm_lastfd = 1;		/* last fd ref (no fd, really)*/
	zero_mmap.mm_cor = 0;			/* no copy-on-ref */
	zero_mmap.mm_text = 0;			/* this is not a "text" */
}

/*
 * xalloc()
 *	Get handles on a mapping for text and data.
 *
 * Called from getxfile() before process has released current resources.
 *
 * Returns 0 for success, else error.
 * Assumes argument vnode is locked.
 */

xalloc(vp, ts, ds, thandlep, dhandlep)
	struct	vnode	*vp;			/* vnode to map */
	register size_t ts;			/* text size (HW pages) */
	register size_t ds;			/* text+data size (HW pages) */
	u_long		*thandlep;		/* return text+data map handle*/
	u_long		*dhandlep;		/* return data map handle*/
{
	register int	val;

	/*
	 * Call to map both text + data first.
	 * This allows underlying mapper to optimize resource
	 * allocation, and speed up data-only map, below.
	 */

	val = (*mmap_reg.map_new)(vp, (u_long)0, ds - LOWPAGES,
						PROT_READ|PROT_EXEC, thandlep);
	if (val > 0)
		return(val);				/* error */
	ASSERT(val == MM_PAGED, "xalloc: map type");
	/*
	 *+ While the kernel was trying to map 
	 *+ the text segment of a process into memory,
	 *+ the underlying mapping function returned a map type 
	 *+ indicating that nonpaged memory was being used.
	 */

	/*
	 * Map data again, since will have two u_mmap[]'s for this.
	 * Assume all implementations succeed for subset map if
	 * current map already exists.
	 *
	 * Allow for no data (ds == ts).
	 */

	if (ds > ts) {
		val = (*mmap_reg.map_new)(vp, (u_long)ts - LOWPAGES, ds - ts,
						PROT_READ|PROT_EXEC, dhandlep);
	}
	ASSERT(val == MM_PAGED, "xalloc: skew");
	/*
	 *+ When the kernel was trying to map 
	 *+ the data segment of a process into memory,
	 *+ the underlying mapping function returned a map type 
	 *+ indicating that nonpaged memory was being used.
	 */

	return(0);			/* success! */
}

/*
 * xfree()
 *	Undo what xalloc() did.
 *
 * Error unwind in getxfile() -- for some reason couldn't complete.
 * Pass "lastfd" since no real fd associated with the mapping.
 *
 * This is *NOT* used in exit -- once mapping is set up for a process,
 * standard mapped-file handling does all the work.
 */

xfree(ts, ds, thandle, dhandle)
	size_t		ts;			/* text size (HW pages) */
	size_t		ds;			/* text+data size (HW pages) */
	u_long		thandle;		/* text+data map handle */
	u_long		dhandle;		/* data map handle */
{
	(*mmap_reg.map_unmap)(thandle, (u_long)0, ds - LOWPAGES,
					PROT_READ|PROT_EXEC|PROT_LASTFD);
	if (ds > ts) {			/* any data? */
		(*mmap_reg.map_unmap)(dhandle, (u_long)ts - LOWPAGES, ds-ts,
					PROT_READ|PROT_EXEC|PROT_LASTFD);
	}
}

/*
 * xinitpt()
 *	Set up "text" and "data" u_mmap[]'s and pte's in page-table.
 *
 * Init LOWPAGES ptes to read-only zero @ zero or totally invalid @ zero,
 * according to magic number.
 *
 * Essentially this is a special-case version of mmap().
 *
 * Called in getxfile() after map and process page table are allocated.
 *
 * Caller assures exclusive access to target pte's.
 */

xinitpt(ts, ds, thandle, dhandle, magic)
	size_t		ts;			/* text size (HW pages) */
	size_t		ds;			/* text+data size (HW pages) */
	u_long		thandle;		/* text+data map handle */
	u_long		dhandle;		/* data map handle */
	long		magic;			/* exec magic number */
{
	register struct	proc	*p = u.u_procp;
	register struct pte	*pte;
	register int i;

	ASSERT_DEBUG(u.u_mmapmax == u.u_mmap, "xinitpt: u_mmap skew");

	/*
	 * Fill out text u_mmap[] entry and text pte's.
	 */

	u.u_mmapmax++;					/* "alloc" u_mmap[0] */
	u.u_mmap[MM_TEXT] = text_mmap;			/* most of it */
	u.u_mmap[MM_TEXT].mm_size = ds - LOWPAGES;	/* size mapped  */
	u.u_mmap[MM_TEXT].mm_pgcnt = ts - LOWPAGES;	/* # pages mapped */
	u.u_mmap[MM_TEXT].mm_handle = thandle;		/* id's mapped object */

	ptefill(dvtopte(p, LOWPAGES), MAPXTOPTE(MM_TEXT)|RO, ts-LOWPAGES);

	/*
	 * Fill out data u_mmap[] entry and data pte's.
	 * No init'd data (ds==ts) sets mm_pgcnt=0 (this is rare).
	 */

	u.u_mmapmax++;					/* "alloc" u_mmap[1] */
	u.u_mmap[MM_DATA] = data_mmap;			/* most of it */
	u.u_mmap[MM_DATA].mm_off = ts-LOWPAGES;		/* 1st mapped offset */
	u.u_mmap[MM_DATA].mm_1stpg = ts;		/* 1st vpn in process */
	u.u_mmap[MM_DATA].mm_size = ds - ts;		/* size mapped  */
	u.u_mmap[MM_DATA].mm_pgcnt = ds - ts;		/* # pages mapped */
	u.u_mmap[MM_DATA].mm_handle = dhandle;		/* id's mapped object */
	ptefill(dvtopte(p, ts), MAPXTOPTE(MM_DATA)|RW, ds - ts);

	/*
	 * Initialize LOWPAGES as RO zeroes (ZMAGIC) or invalid (XMAGIC).
	 * For ZMAGIC set up internal mapping function.
	 */

	if (magic == ZMAGIC) {				/* zero @ zero */
		u.u_mmapmax++;				/* "alloc" u_mmap[2] */
		u.u_mmap[MM_ZERO] = zero_mmap;		/* constant mmap */
		for (i = 0, pte = dvtopte(p, 0); i < LOWPAGES; i++, pte++)
			*(int *) pte = *(int *) &zero_at_zero_ptes[i];
	} else						/* XMAGIC: inval@zero */
		ptefill(dvtopte(p, 0), PG_INVAL, LOWPAGES);
}

/*
 * xumount()
 *	Free all texts (sticky and tacky) that are from a given device.
 *
 * Used in umount() to make all refs to such vnodes go away.
 */

xumount(vfsp)
	struct vfs *vfsp;
{
	mmreg_umount(vfsp);
}

/*
 * xrele()
 *	Remove a shared text segment from the text table, if possible.
 *
 * Called with locked vnode.
 *
 * Returns true if text is released, else false.
 */

bool_t
xrele(vp)
	struct vnode *vp;
{
	return (mmreg_rele(vp));
}

/*
 * xflush()
 *	Try to delete a tacky/sticky text as a means of freeing some
 *	memory and/or vnode.
 *
 * Returns true if one is released, else fails.
 */

bool_t
xflush(dosticky)
	bool_t	dosticky;
{
	return (mmreg_flush(dosticky));
}
