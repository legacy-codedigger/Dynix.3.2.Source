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

#ifndef	lint
static	char	rcsid[] = "$Header: vm_drum.c 2.19 1991/07/09 00:11:08 $";
#endif

/*
 * vm_drum.c
 *	Manage swap space.
 */

/* $Log: vm_drum.c,v $
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/kernel.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"

struct dmapext *vsallocext();

int	dmmin = DMMIN;			/* min dmap chunk allocation size */
int	dmmax = DMMAX;			/* max dmap chunk allocation size */
int	dmmax_sw = DMMAX_SW;		/* max swap space chunk size */

struct	dmap	zdmap;			/* for clearing map structs */

int		maxdmap;		/* max size of swap alloc per segment */
static	int	dm_nemap;		/* number elements in dmapext array */
static	int	sizeof_dmapext;		/* size (bytes) of dmapext object */
static	long	sizeof_dmapext_disk;	/* size (sectors) of dmapext object */

/*
 * init_dmap()
 *	Initialize dmap related values.
 */

init_dmap()
{
	register int blk;
	register u_int vaddr;

	/* 
	 * Determine number of entries in dmapext array (dm_nemap),
	 * max swap size we can represent per segment (maxdmap),
	 * and size of dmapext object (sizeof_dmapext).
	 */

	for (blk = dmmin, vaddr = 0; vaddr < MAXADDR; ) {
		++dm_nemap;
		maxdmap += blk;
		vaddr += ctob(dtoc(blk));
		if (blk < dmmax)
			blk *= 2;
	}

#ifdef	i386
	/*
	 * Kludge to get sizeof_dmapext to not overflow into next page by
	 * only a few bytes -- assuming dmmin == 16k, dmmax == 256k, need
	 * to compensate for 6 longs (2 from struct dmapext and 4 from
	 * the short blocks).  In practice, this drops about 1.5Meg from
	 * the max size segment, and since stack consumes at least 4Meg
	 * of virtual space (due to virtual hole, data can't grow into
	 * top 4Meg of 256Meg), this is a don't care.
	 *
	 * Not an issue on ns32000, since 16Meg address space and 256k max
	 * chunk doesn't fill a HW page.
	 */
	dm_nemap -= 6;
	maxdmap -= 6 * dmmax;
#endif	i386

	sizeof_dmapext = sizeof(struct dmapext) + sizeof(swblk_t)*(dm_nemap-1);
	sizeof_dmapext_disk = ctod(clrnd(btoc(sizeof_dmapext)));
}

/*
 * vsalloc()
 *	Insure range of pages is covered by pieces of swap space.
 *
 * Returns true for success, false for fail.  If fail, undoes any allocation
 * done prior to failure.
 *
 * Assumes caller has exclusive access to dmap for changes;
 * can race with pageout (no harm).
 */

vsalloc(dmp, vsbase, npg)
	struct	dmap	*dmp;
	size_t		vsbase;			/* 1st page needing space */
	register size_t npg;			/* # pages needing space */
{
	register swblk_t *ip;
	register long	blk;
	struct	dmapext	*dme;
	long	blk0;
	swblk_t	*ip0;
	swblk_t	*ipmax;
	swblk_t	*mapmax;

	vsbase = ctod(vsbase);
	npg = ctod(npg);

	if (dme = dmp->dm_ext) {
		ip = dme->dme_map;
		mapmax = &ip[dm_nemap];
	} else {
		ip = dmp->dm_map;
		mapmax = &ip[NDMAP];
	}

	/*
	 * Find starting chunk.
	 */

	blk = dmmin;
	while (vsbase >= blk) {
		++ip;
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		else {				/* reached constant size blks */
			ip += (vsbase / blk);
			vsbase %= blk;
			break;
		}
	}
	ip0 = ip;
	blk0 = blk;
	npg += vsbase;				/* for start of block */

	/*
	 * Allocate space where needed.  Allocate extension if needed.
	 * Each allocated block number is stored negative to allow
	 * error unwind.  This has to be inverted before returning.
	 */

	while (npg != 0) {
		if (ip >= mapmax) {
			/*
			 * Allocate extension and copy current dmap to it.
			 * Write dmp->dm_ext only after it's all set up to
			 * avoid nasty races with pageout.
			 */
			ASSERT(dme == NULL, "vsalloc: dm_ext");
			/*
			 *+ When allocating swap space for a process, 
			 *+ the kernel overflowed the dmap array that holds
			 *+ swap addresses for this process, had to
			 *+ allocate a dmap extension object, and found that
			 *+ one had already been allocated.
			 */
			if ((dme = vsallocext()) == NULL)
				goto nospace;
			dme->dme_nent = NDMAP;
			bcopy((caddr_t) dmp->dm_map, (caddr_t) dme->dme_map,
						NDMAP * sizeof(swblk_t));
			dmp->dm_ext = dme;
			ip0 = &dme->dme_map[ip0 - dmp->dm_map];
			ip = &dme->dme_map[ip - dmp->dm_map];
			mapmax = &dme->dme_map[dm_nemap];
		}
		if (*ip == 0) {
			if ((*ip = -swapalloc(blk)) == 0)
				goto nospace;
		}
		++ip;
		if (blk > npg)
			break;
		npg -= blk;
		if (blk < dmmax)
			blk *= 2;
	}

	/*
	 * Got all requested space.  Un-negate block #'s to make them "real",
	 * and update # entries in extension.
	 */

	ipmax = ip;
	for (ip = ip0; ip < ipmax; ip++) {
		if (*ip < 0)
			*ip = -*ip;
	}
	if (dme && ip > &dme->dme_map[dme->dme_nent])
		dme->dme_nent = (ip - dme->dme_map);

	return (1);

	/*
	 * Didn't get space -- release any that we did get and return failure.
	 */
nospace:
	blk = blk0;
	ipmax = ip;
	for (ip = ip0; ip < ipmax; ip++) {
		if (*ip < 0) {
			swapfree(blk, -*ip);
			*ip = 0;
		}
		if (blk < dmmax)
			blk *= 2;
	}
#ifdef MFG_VSALLOC
	printf("vsalloc failed in %s\n", u.u_comm);
#endif MFG_VSALLOC
	return (0);
}

/*
 * vsfree()
 *	Deallocate any swap chunks totally covered by range of pages.
 */

vsfree(dmp, vsbase, npg)
	register struct dmap *dmp;
	register size_t vsbase;			/* 1st page not needing space */
	register size_t npg;			/* # pages not needing space */
{
	register swblk_t *ip;
	register long	blk;
	swblk_t	*mapmax;

	vsbase = ctod(vsbase);
	npg = ctod(npg);

	if (dmp->dm_ext) {
		ip = dmp->dm_ext->dme_map;
		mapmax = &ip[dmp->dm_ext->dme_nent];
	} else {
		ip = dmp->dm_map;
		mapmax = &ip[NDMAP];
	}

	/*
	 * Find starting chunk.
	 */

	blk = dmmin;
	while (vsbase >= blk && ip < mapmax) {
		++ip;
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		else {				/* reached constant size blks */
			/*
			 * Before advancing 'ip', check for possible pointer
			 * overflow that would otherwise satisfy the check:
			 *	while (ip < mapmax) ....
			 */
			if (vsbase / blk >= mapmax - ip)
				return;			/* no space to free */
			ip += (vsbase / blk);
			vsbase %= blk;
			break;
		}
	}

	/*
	 * If page not at start of chunk, adjust to start of next.
	 */

	if (vsbase) {
		npg -= (blk - vsbase);
		if (blk < dmmax)
			blk *= 2;
		++ip;
	}

	/*
	 * Free space as long as totally overlap and map exists.
	 * Caller has arranged that npg covers remainder of swap space
	 * if want to free it all beyond a given point.
	 */

	while (npg >= blk && ip < mapmax) {
		if (*ip) {
			swapfree(blk, *ip);
			*ip = 0;
		}
		npg -= blk;
		if (blk < dmmax)
			blk *= 2;
		++ip;
	}
}

/*
 * vsclone()
 *	Clone one dmap into another (for fork).
 *
 * Returns true for success, else false.
 */

vsclone(sdmp, tdmp)
	struct	dmap	*sdmp;				/* source dmap */
	struct	dmap	*tdmp;				/* target dmap */
{
	register swblk_t *sip;
	register swblk_t *tip;
	register swblk_t *lastip;
	register long	blk;

	if (sdmp->dm_ext) {
		if ((tdmp->dm_ext = vsallocext()) == NULL)
			return (0);
		tdmp->dm_ext->dme_nent = sdmp->dm_ext->dme_nent;
		sip = sdmp->dm_ext->dme_map;
		tip = tdmp->dm_ext->dme_map;
		lastip = &tdmp->dm_ext->dme_map[tdmp->dm_ext->dme_nent];
	} else {
		tdmp->dm_ext = NULL;
		sip = sdmp->dm_map;
		tip = tdmp->dm_map;
		lastip = &tdmp->dm_map[NDMAP];
	}

	blk = dmmin;
	for (; tip < lastip; sip++, tip++) {
		if (*sip) {
			if ((*tip = swapalloc(blk)) == 0) {
				while (++tip < lastip)
					*tip = 0;
				vsrele(tdmp);
				return (0);
			}
		} else
			*tip = 0;
		if (blk < dmmax)
			blk *= 2;
	}

	return (1);
}

/*
 * vsallocext()
 *	Allocate a dmap extension and swap space.
 *
 * Returns NULL if fail, else address of new struct dmapext.
 */

#ifdef	DEBUG
int	cnt_vsallocext;
int	cnt_vsrele;
int	cnt_vsswapout;
int	cnt_vsswapin;
#endif	DEBUG

static struct dmapext *
vsallocext()
{
	register struct dmapext *dme;
	register swblk_t dme_swap;

#ifdef	DEBUG
	++cnt_vsallocext;
#endif	DEBUG
	dme_swap = swapalloc(sizeof_dmapext_disk);
	if (dme_swap == 0)
		return (NULL);
	dme = (struct dmapext *) wmemall(sizeof_dmapext, 1);
	bzero((caddr_t) dme, (u_int) sizeof_dmapext);
	dme->dme_daddr = dme_swap;
	return (dme);
}

/*
 * vsrele()
 *	Release all resources associated with a dmap.
 */

vsrele(dmp)
	struct dmap *dmp;
{
	register struct dmapext *dme;

	vsfree(dmp, (size_t)0, (size_t)dtoc(maxdmap));
	if (dme = dmp->dm_ext) {		/* "vsdeallocext()" */
#ifdef	DEBUG
		++cnt_vsrele;
#endif	DEBUG
		dmp->dm_ext = 0;
		swapfree(sizeof_dmapext_disk, (long) dme->dme_daddr);
		wmemfree((caddr_t) dme, sizeof_dmapext);
	}
}

/*
 * vsswapout()
 *	If necessary, swap out swap-space representation (dmap).
 */

vsswapout(dmp)
	register struct dmap *dmp;
{
	register struct dmapext *dme;

	if (dme = dmp->dm_ext) {
#ifdef	DEBUG
		++cnt_vsswapout;
#endif	DEBUG
		swap(u.u_procp, dme->dme_daddr,
				&Usrptmap[btokmx((struct pte *)dme)],
				(int) dtob(sizeof_dmapext_disk),
				B_WRITE|B_UAREA, swapdev_vp);
		dmp->dm_daddr = dme->dme_daddr;
		wmemfree((caddr_t) dme, sizeof_dmapext);
	}
}

/*
 * vsswapin()
 *	If necessary, swap in swap-space representation (dmap).
 *
 * Returns 1 for success, 0 for fail.
 */

vsswapin(dmp)
	register struct dmap *dmp;
{
	register struct dmapext *dme;
	register swblk_t dme_swap;

	if ((dme_swap = dmp->dm_daddr) == 0)
		return (1);				/* nothing to do */
	if ((dme = (struct dmapext *)wmemall(sizeof_dmapext, 0)) == NULL)
		return (0);
#ifdef	DEBUG
	++cnt_vsswapin;
#endif	DEBUG
	swap(u.u_procp, dme_swap, &Usrptmap[btokmx((struct pte *)dme)],
			(int) dtob(sizeof_dmapext_disk),
			B_READ|B_UAREA, swapdev_vp);
	dmp->dm_ext = dme;
	ASSERT(dme->dme_daddr == dme_swap, "vsswapin: skew");
	/*
	 *+ The kernel discovered an inconsistency in the swap space
	 *+ representation maps.  When it swapped in a dmap extension object,
	 *+ the swap address the object was read from didn't match the
	 *+ address stored in the object itself.
	 */
	return (1);
}

/*
 * vsswap()
 *	Swap a segment of virtual memory to disk.
 *
 * Done by locating the contiguous dirty pte's (in Rset and on
 * dirty-list) and calling vschunk with each chunk.
 *
 * Algorithm is awkward, but handles the cases.
 *
 * Returns number of private pages freed from resident-set, and bumps
 * counters of # resident-set pages freed from each possible mapped object.
 */

size_t
vsswap(p, pte, type, vsbase, vscount, dmp, map_used)
	struct	proc	*p;
	register struct pte *pte;
	int		type;
	register int	vsbase;
	size_t		vscount;
	struct	dmap	*dmp;
	size_t		*map_used;
{
	register struct cmap *c;
	register int size = 0;
	register int skip = 0;
	size_t	priv_used = 0;
	int	incr;

	ASSERT((vscount % CLSIZE) == 0, "vsswap: vscount");
	/*
	 *+ The swapping function was asked to swap a segment of
	 *+ virtual memory to disk, but the number of pages to be
	 *+ written out was not a multiple of the page cluster size.
	 */

	incr = (type == CSTACK) ? -CLSIZE : CLSIZE;

	/*
	 * Look at each page and dispose of it appropriately.
	 * For contiguous sets of dirty pages, reclaim (if necessary)
	 * into process page-table (but not into Rset), then call
	 * vschunk() to write and free these pages.
	 */

	for (; vscount != 0; vscount -= CLSIZE, pte += incr) {

		/*
		 * A 'skipped' page may be end of a contiguous set
		 * of dirty pages.
		 */

		if (skip) {
			if (size) {
				vschunk(p, vsbase, size, type, dmp);
				vsbase += size;
				size = 0;
			}
			vsbase += skip;
			skip = 0;
		}

		/*
		 * FOD page is skipped.
 		 * PG_INVAL page is skipped (implicitly).
		 * Mapped pte is freed, no detach (==> no free u_mmap[]).
		 *	If we do this alot, is more efficient to expand
		 *	vmmapfree() in-line here.
		 * Already paged-out page is skipped.
		 * (order of tests is important!)
		 */

		if (pte->pg_fod) {
			ASSERT_DEBUG(*(int *)pte & PG_FZERO, "vsswap: !ZFOD");
			skip += CLSIZE;
			continue;
		}
		if (PTEMAPPED(*pte)) {
			size_t	map_freed = vmmapfree(p, pte, 0);
			p->p_rssize -= map_freed;
			map_used[PTETOMAPX(*pte)] += map_freed;
			skip += CLSIZE;
			continue;
		}
		if (PTEPF(*pte) == 0) {
			skip += CLSIZE;
			continue;
		}

		/*
		 * If valid and dirty, must write it (decr p_rssize here,
		 * although vschunk() will actually free it).
		 * Valid and non-dirty ==> just free (and skip).
		 */

		if (pte->pg_v) {
			if (anycl(pte, pg_m))
				size += CLSIZE;
			else {
				memfree(pte, CLSIZE, 1);
				skip += CLSIZE;
			}
			--p->p_rssize;
			++priv_used;
			continue;
		}

		/*
		 * Is reclaimable.  Lock memory list and check again.
		 * If still reclaimable...  if dirty, reclaim into
		 * page table ('alloc' so vschunk() can write and free);
		 * otherwise (not dirty) just set 'gone' (could move to
		 * front of free-list).
		 */

		LOCK_MEM;

		if (PTEPF(*pte) == 0) {			/* lost race! */
			UNLOCK_MEM;
			skip += CLSIZE;
			continue;
		}
		c = PTETOCMAP(*pte);

		ASSERT_DEBUG(c->c_refcnt == 0, "vsswap2");
		ASSERT_DEBUG(type == c->c_type, "vsswap3");
		ASSERT_DEBUG(!c->c_holdfod, "vsswap: holdfod");

		if (!c->c_dirty) {
			zap_pfnum(pte);
			c->c_gone = 1;
			skip += CLSIZE;
		} else {
			/*
			 * Can't have concurrent pageout since
			 * pageout is our caller.
			 */
			ASSERT_DEBUG(c->c_pageout == 0, "vsswap4");
			CM_UNLINK_DIRTY(c);
			++c->c_refcnt;
			c->c_dirty = 0;
			size += CLSIZE;
		}
		UNLOCK_MEM;
	}

	/*
	 * If any left, write & free 'em.
	 */

	if (size)
		vschunk(p, vsbase, size, type, dmp);

	return(priv_used);
}

/*
 * vschunk()
 *	Write a (virtually) contiguous chunk of a process segment
 *	to swap space.
 *
 * Caller insures pages are not reallocatable during this.
 */

static
vschunk(p, base, size, type, dmp)
	register struct proc *p;
	register int base;
	register int size;
	int	type;
	struct dmap *dmp;
{
	register struct pte *pte;
	struct dblock db;

	base = ctod(base);
	size = ctod(size);

	do {
		int result;
		/*
		 * Figure max contiguous blocks that can write.
		 */
		result = vstodb(base, size, dmp, &db, type == CSTACK);
		if (type == CSTACK)
			pte = sptopte(p, dtoc(base+db.db_size)-1);
		else
			pte = dptopte(p, dtoc(base));
		/*
		 * Write 'em.
		 */
		if (result == 0) {
			swap(p, db.db_base, pte, 
				(int)dtob(db.db_size), B_WRITE, swapdev_vp);
		}
		/*
		 * Free 'em.
		 */
		memfree(pte, (int)dtoc(db.db_size), 1);
		base += db.db_size;
		size -= db.db_size;
		/*
		 * result == -1 means that swap space is does not exits.
		 * This is only leagal if a shared mmap failed in pagein
		 * and was turned into a ZFOD. Handle by setting pte to 
		 * ZFOD, and just killing the process.
		 * Note: db_size from vstodb is always valid even
		 * in case of error.
		 */
		if (result != 0) {
			ptefill(pte, PG_ZFOD, (size_t)dtoc(db.db_size));
			swkill(p, "BAD SWAP DMAP");
		}
	} while (size != 0);
}

/*
 * vstodb()
 *	Given a base/size pair in virtual swap area,
 *	return a physical base/size pair which is the
 *	(largest) initial, physically contiguous block.
 *
 *	Return vaule of 0 means success, -1 means NULL
 *	dmap entry or unreasonable block number
 *
 * Called in this module and by pageout().
 *
 * A CLONE OF THIS PROCEDURE IS USED IN ps, gcore, w, AND (maybe)
 * OTHER UTILITIES.  IF IT CHANGES, CHECK THEIR USAGE.
 */

vstodb(vsbase, vssize, dmp, dbp, rev)
	register int	vsbase;
	register int	vssize;
	struct dmap	*dmp;
	register struct dblock *dbp;
	int		rev;
{
	register swblk_t *ip;
	register int blk;

	ASSERT_DEBUG(vsbase >= 0 && vssize >= 0, "vstodb");

	if (dmp->dm_ext)
		ip = dmp->dm_ext->dme_map;
	else
		ip = dmp->dm_map;

	blk = dmmin;
	while (vsbase >= blk) {
		ip++;
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		else {				/* reached constant size blks */
			ip += (vsbase / blk);
			vsbase %= blk;
			break;
		}
	}
	ASSERT_DEBUG(*ip && *ip + blk <= nswap, "vstodb: *ip");
	/*
	 *+ When getting the swap location for a piece of virtual
	 *+ memory, the kernel found an invalid swap block address 
	 *+ in the process's dmap structure.
	 */

	/*
	 * If we get an error condition, return -1 to caller who
	 * will take corrective action.
	 * Note: The value of db_size is valid in any event and
	 * will be used by vschunk, even if error occurs.
	 */
	dbp->db_size = imin(vssize, blk - vsbase);
	if (!*ip || !(*ip + blk <= nswap)) {
		printf("vstodb: NULL or unreasonable dmap entry = 0x%x\n", *ip);
		/*
		 *+ While trying to get the swap location for a piece of
		 *+ virtual memory, the kernel found an invalid or null dmap
		 *+ entry.
		 */
		return(-1);
	}

	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
	ASSERT_DEBUG((dbp->db_size&((NBPG/DEV_BSIZE)-1))==0, "vstodb: db_size");
	ASSERT_DEBUG((dbp->db_base&((NBPG/DEV_BSIZE)-1))==0, "vstodb: db_base");
	/* Return success */
	return(0);
}
