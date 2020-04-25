/* $Copyright:	$
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
static	char	rcsid[] = "$Header: vm_mem.c 2.17 91/03/31 $";
#endif

/*
 * vm_mem.c
 *	Memory procedures.
 *
 * NOTE: on a mono-processor, LOCK_MEM/UNLOCK_MEM need to splimp() to
 * insure no nesting via mbuf memory allocation.
 */

/* $Log:	vm_mem.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/map.h"
#include "../h/kernel.h"
#include "../h/conf.h"
#include "../h/vfs.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

/*
 * Variables mostly used in this module.
 */

lock_t	mem_alloc;			/* locks access to free-list */
sema_t	mem_wait;			/* place to wait for memory */

struct	cmap	cm_clean;		/* heads clean free list */
struct	cmap	cm_dirty;		/* heads dirty free list */

int	freemem = 0;			/* counts current free-pages */
int	dirtymem = 0;			/* # clusters in dirty-list */
int	ecmx = 0;			/* initial free list size (for `ps') */

/*
 * vmemall()
 *	Allocate memory, and always succeed.
 *
 * Assumes caller insures exclusive access to target pte's and target pte's
 * can't move (ie, via swap).
 */

vmemall(pte, size, p, type)
	register struct pte *pte;
	register int size;
	struct proc *p;
{
	register int m;

	ASSERT(size > 0 && size <= maxmem, "vmemall size");
	/*
	 *+ The kernel made an invalid memory allocation request
	 *+ where the size was not greater than 0 and less than
	 *+ the amount of memory available in the system.
	 */

	/*
	 * Loop, taking what memory we can, and waiting for what we can't
	 * yet take; all the time being careful to lock the free-list.
	 */

	while (size > 0) {

		LOCK_MEM;

		m = MIN(size, freemem);			/* maybe 0 */
		lmemall(pte, m, p, type);
		size -= m;
		pte += m;

		/*
		 * If that was all, then unlock memory and done.
		 * Else -- not enough:  Must wait for more memory.
		 * When somebody frees memory, we'll wakeup and try again.
		 */

		UNLOCK_MEM;
		if (size != 0) {
			WAIT_MEM;
		}
	}

	/*
	 * Always succeeds, but return success for callers which call either
	 * memall or vmemall depending on context.
	 */

	return (1);
}

/*
 * vmemfree()
 *	Release valid and reclaimable page frames belonging to the
 *	count pages starting at pte.
 *
 * Called to give up forever the requested pages.
 *
 * Called by expand() to give back pages when shrinking process,
 * during exit() when freeing VM, and during mmap() and obreak()
 * to release resources as appropriate.
 *
 * Assumes caller guaranteed source pte's are only refs to assoc pages.
 *
 * Returns # clusters actually freed.
 */

vmemfree(pte, count)
	register struct	pte *pte;
	register int count;
{
	register struct cmap *c;
	register int	pcnt;

	ASSERT((count % CLSIZE) == 0, "vmemfree: count");
	/*
	 *+ The kernel attempted to free an amount of memory that
	 *+ was not a multiple of the page cluster size.
	 */

	/*
	 * For each pte:
	 *	FOD ==> ignore.
	 *	PG_INVAL ==> ignore (implicit in algorithm).
	 *	Mapped ==> detach.
	 *	reclaimable ==> lock memory and if it's still reclaimable:
	 *			set c_gone (no longer need it).
	 *			if on dirty free-list, move to clean free list.
	 *			must zap_pfnum() in case swapout page-table.
	 *	valid ==> free it.
	 *
	 * Test for reclaimable is done while not holding memory lists locked;
	 * thus can race with lmemall() re-allocating the page.  No problem,
	 * are giving up page anyhow.
	 *
	 * Assumes only PTEMAPPED() pte's are in calling process (ie, u.)
	 */

	pcnt = 0;
	for (; count > 0; pte += CLSIZE, count -= CLSIZE) {
		if (pte->pg_fod)
			continue;
		if (PTEMAPPED(*pte)) {
			pcnt += vmmapfree(u.u_procp, pte, 1);
		} else if (pte->pg_v) {			/* valid */
			/*
			 * In-line expansion of memfree(pte, CLSIZE, 1).
			 */
			c = PTETOCMAP(*pte);
			ASSERT(c >= cmap && c < ecmap,"vmemfree: bad mem free");
			/*
			 *+ In freeing a page of memory, the kernel found that
			 *+ the physical address represented by the page table
			 *+ entry was invalid.
			 */
			ASSERT_DEBUG(c->c_refcnt == 1, "vmemfree: c_refcnt");
			ASSERT_DEBUG(!c->c_dirty, "vmemfree: detach c_dirty");
			LOCK_MEM;
			c->c_refcnt = 0;
			c->c_gone = 1;
			if (c->c_pageout == 0) {
				CM_INS_HEAD_CLEAN(c);
			}
			UNLOCK_MEM;
			zapcl(pte, PG_RSVD|PG_PFNUM|PG_V);
			++pcnt;
		} else if (PTEPF(*pte) != 0) {		/* reclaimable */
			LOCK_MEM;
			if (PTEPF(*pte) != 0) {		/* still? */
				c = PTETOCMAP(*pte);
				ASSERT_DEBUG(c->c_refcnt == 0,
						"vmemfree: refd reclaim");
				c->c_gone = 1;
				/*
				 * If reclaimable from dirty list,
				 * then put it on clean free-list if
				 * no IO in progress on it.
				 */
				if (c->c_dirty && c->c_pageout == 0) {
					CM_UNLINK_DIRTY(c);
					c->c_dirty = 0;
					CM_INS_HEAD_CLEAN(c);
				}
			}
			UNLOCK_MEM;
			zap_pfnum(pte);
		}
	}

	/*
	 * Just freed some; if anyone waiting, let'em try for it.
	 */

	if (blocked_sema(&mem_wait))		/* if someone waiting... */
		vall_sema(&mem_wait);		/* wake'em up */

	return (pcnt);
}

/*
 * vmmapfree()
 *	Free a mapped page given a pointer to referencing pte.
 *
 * If detach, decrement u_mmap[] page-count and really zap pte.
 * If this was last page in the u_mmap[] entry, free the entry.
 *
 * Could insist on valid pte and have caller zap invalid's; for now
 * keep it this way to keep decisions in one place.
 *
 * Only caller with p != u.u_procp is swap-out (vsswap()).
 *
 * Caller does NOT hold memory locked.
 *
 * Returns # clusters of "paged" pages freed (==> value is 0 or 1).
 */

vmmapfree(p, pte, detach)
	register struct proc *p;
	register struct pte *pte;
	bool_t		detach;
{
	register struct mmap *um;
	int		val = 0;

	ASSERT_DEBUG(PTEMAPPED(*pte), "vmmapfree: not mapped");

	/*
	 * If paged, de-ref pte if currently valid.
	 * If possible RW ref, pass in M-bit status (RO ref ==> ignore
	 * since can't write).
	 *
	 * If phys, must be valid (since don't page these).
	 * Swapper code (vsswap()) calls with arbitrary PTEMAPPED
	 * pte; thus can have !detach for phys-mapped pte.  If
	 * expand this in-line in vsswap() might be able to
	 * strengthen this.
	 *
	 * Pass knowledge of last fd reference to DEREFPG, since
	 * might avoid freeing dirty memory.
	 *
	 * Paged code assumes page is in data-segment.
	 */

	um = &p->p_uarea->u_mmap[PTETOMAPX(*pte)];

	if (um->mm_paged) {
		ASSERT_DEBUG(pte>=dptopte(p,0) && pte<=dptopte(p,p->p_dsize-1),
						"vmmapfree: non-data paged");
		if (pte->pg_v) {
			(*um->mm_ops->map_derefpg)
				(um->mm_handle,
				PTETOMAPOFF(p, pte, um),
				((um->mm_prot&PROT_WRITE)? anycl(pte,pg_m): 0),
				um->mm_lastfd);
			val = 1;
			zapcl(pte, ~(PG_PROT|PTE_MMAP_MASK));
		}
	} else {
		/*
		 * !detach ok, if called during swapout.
		 */
		ASSERT_DEBUG(pte->pg_v, "vmmapfree: phys");
	}

	/*
	 * If detaching, really zap pte and decrement u_mmap[] entry
	 * page count.  If hits zero, free the u_mmap[] entry.
	 *
	 * Leave UF_MAPPED bit on, since doesn't hurt and will get
	 * off when close.
	 *
	 * If relevant fd is getting closed and has only one reference,
	 * tell UNMAP procedure, since it can optimize the release in
	 * some cases.
	 */

	if (detach) {
		ASSERT_DEBUG(p == u.u_procp, "vmmapfree: non-self detach");
		ASSERT_DEBUG(um->mm_pgcnt >= CLSIZE, "vmmapfree: pgcnt");
		zapcl(pte, ~PG_PROT);			/* only keep PROT */
		u.u_mmapdel = 1;			/* deleted a page */
		if ((um->mm_pgcnt -= CLSIZE) == 0) {
			(*um->mm_ops->map_unmap)
				(um->mm_handle, um->mm_off, um->mm_size,
				um->mm_prot | (um->mm_lastfd ? PROT_LASTFD: 0));
			if (um->mm_noio)
				--u.u_pmapcnt;
			/*
			 * u_mmap[] holds index of referenced file-table entry.
			 * Must loose the reference, since mapping is now
			 * completely gone.  No need to worry about signals
			 * here, since mappable files won't block during
			 * close (they could, but none do).
			 */
			if (um->mm_fdidx >= 0) {
				deref_file(&file[um->mm_fdidx]);
			}
#ifdef	DEBUG
			/*
			 * Run thru PT and insure no more mapx's for this entry.
			 */
			for (pte = dptopte(p, 0); pte < dptopte(p, p->p_dsize); pte++) {
				if (pte->pg_fod)
					continue;
				ASSERT(	!PTEMAPPED(*pte)
				||	PTETOMAPX(*pte) != um-u.u_mmap,
						"vmmapfree: oops");
			}
#endif	DEBUG
		}
	}

	return(val);
}

/*
 * memall()
 *	Allocate memory, maybe not succeed.
 *
 * Locks free-list and allocates memory if there is enough.  Else,
 * allocates none.  Returns # pages allocated (0 ==> failure).
 * Basically a front-end to lmemall().
 *
 * Assumes caller insures exclusive access to target pte's.
 *
 * Could create version that locks per page, but most uses either call
 * with size==CLSIZE or need memory-lists locked for longer atomicity
 * (eg, mmreg_refpg()).  If access to memory-lists lock is still a problem,
 * can work on this.
 */

memall(pte, size, p, type)
	struct	pte	*pte;
	int		size;
	struct	proc	*p;
{
	LOCK_MEM;

	if (size <= freemem)
		lmemall(pte, size, p, type);
	else
		size = 0;

	UNLOCK_MEM;

	return(size);
}

/*
 * lmemall()
 *	Low-level memory allocator.
 *
 * The free list appears as a doubly linked list in the core map
 * with cm_clean serving as a header.
 *
 * Only fills out physical page base in pte.  Caller must fill out
 * valid, prot, etc fields.
 *
 * Assumes caller locked the free-list (hence "l" name) and locked
 * target pte's.  Also, caller verified enough memory in the free-list.
 *
 * Note: if run system low of memory (eg, below vmtune.vt_minfree), swapper
 * will be waking up soon anyhow, so no special action here.
 *
 * Mapf, page, and ndx arguments are only passed in for type CMMAP.
 * These are used by mapping functions to have enough information in the
 * cmap[] entry to identify the particular mapped page within the mapper.
 */

/*VARARGS4*/
lmemall(pte, size, p, type, mapf, page, ndx)
	register struct pte *pte;
	int	size;
	struct	proc *p;
	struct	mapops *mapf;
	u_long	page;
	u_long	ndx;
{
	register struct cmap	*c;
	register caddr_t	pf;
	register struct pte	*rpte;
	register struct proc	*rp;
	int	i;

	ASSERT_DEBUG(size % CLSIZE == 0, "lmemall: bad size");
	ASSERT_DEBUG(size <= freemem, "lmemall: no mem");

	while ((size -= CLSIZE) >= 0) {

		/*
		 * Yank 1st page off free-list.
		 */

		c = cm_clean.c_next;

		ASSERT(c >= cmap && c < ecmap, "lmemall: bad mem alloc");
		/*
		 *+ On allocating a page of memory, the kernel found that the
		 *+ cmap structure representing the page frame pointed to an
		 *+ invalid address.
		 */
		ASSERT_DEBUG(c->c_refcnt == 0 && c->c_pageout == 0, "lmemall1");
		ASSERT_DEBUG(c->c_intrans == 0 && c->c_iowait == 0, "lmemall2");

		CM_UNLINK_CLEAN(c);

		/*
		 * If not gone yet (ie, reclaimable), break reclaim-link.
		 */

		if (!c->c_gone) {
			switch (c->c_type) {

			case CDATA:
				rp = &proc[c->c_ndx];
				while (rp->p_flag & SNOVM)
					rp = rp->p_vflink;
				rpte = dptopte(rp, c->c_page);
				zap_pfnum(rpte);
				break;

			case CSTACK:
				rp = &proc[c->c_ndx];
				while (rp->p_flag & SNOVM)
					rp = rp->p_vflink;
				rpte = sptopte(rp, c->c_page);
				zap_pfnum(rpte);
				break;

			case CMMAP:
				(*(c->c_mapf->map_realloc))(c->c_ndx,c->c_page);
				break;

			default:
				panic("lmemall: c_type");
				/*
				 *+ The kernel found a reclaimable page on the
				 *+ free list that had an invalid page type.
				 */
				/*NOTREACHED*/
			}
			ASSERT_DEBUG(!c->c_holdfod, "lmemall: c_holdfod");
			++l.cnt.v_realloc;		/* count # reallocs */
		}

		/*
		 * Figure owner index and virtual-page-number.
		 * VPN used above to break reclaim links.
		 */

		switch (type) {
		case CSYS:
			c->c_ndx = p->p_ndx;
			break;

		case CDATA:
			c->c_page = ptetodp(p, pte);
			c->c_ndx = p->p_ndx;
			break;

		case CSTACK:
			c->c_page = ptetosp(p, pte);
			c->c_ndx = p->p_ndx;
			break;

		case CMMAP:
			c->c_mapf = mapf;
			c->c_page = page;
			c->c_ndx = ndx;
			break;

		default:
			panic("lmemall: type");
			/*
			 *+ The memory allocation function was asked to
			 *+ allocate a page of an invalid type.
			 */
			/*NOTREACHED*/
		}

		/*
		 * Fill out pte's and rest of cmap[] entry.
		 * Caller sets c_holdfod if appropriate.
		 * c_dirty is already zero since we took page from free-list.
		 */

		pf = (caddr_t) CMAPTOPHYS(c);
		for (i = 0; i < CLSIZE; i++) {
			*(int *)pte++ = PHYSTOPTE(pf);
			pf += NBPG;
		}
		c->c_refcnt = 1;
		c->c_holdfod = 0;
		c->c_gone = 0;
		c->c_type = type;
		ASSERT_DEBUG(c->c_dirty == 0, "lmemall: c_dirty");
		ASSERT_DEBUG(c->c_pageout == 0, "lmemall: c_pageout");
	}
}

/*
 * memfree()
 *	Low-level memory free'r.
 *
 * The page frames being returned are inserted to the head/tail of the
 * free list depending on whether there is any possible future use of them.
 *
 * Gooses mem_wait as appropriate.
 *
 * Caller does NOT hold memory lists locked.
 * Caller insures exclusive access to the pte's.
 * Caller insures none of the pte's are "mapped".
 *
 * Optimization: caller insures all ptes reference page with c_refcnt==1
 * (ie, really freeing pages).  This is sufficient for current mapped
 * file support, but might break with other possible mapping functions
 * (eg, hold a sema to insure exclusive access to pte's and use memfree()
 * rather than in-line expand).  See memfree1().
 */

memfree(pte, size, detach)
	register struct pte *pte;
	register int size;
	bool_t	detach;				/* true ==> make it gone */
{
	register struct cmap *c;

	ASSERT_DEBUG(size % CLSIZE == 0, "memfree: bad size");

	/*
	 * For each page involved, find the page based on the pte,
	 * and insert in appropriate free-list.  Handle case where
	 * pageout is cleaning the page.
	 *
	 * Zap valid, and page# field (ie, no reclaim) if detaching.
	 * If not detaching, but PT ref has 'm' bit, set c_dirty.
	 *
	 * Note: c_gone ==> c_holdfod ignored (see lmemall).
	 *
	 * Must clear c_refcnt while hold memory lists locked
	 * to avoid race with pageout() and freeing page twice.
	 */

	for (; size > 0; size -= CLSIZE, pte += CLSIZE) {

		ASSERT_DEBUG(!PTEMAPPED(*pte), "memfree: mapped pte");

		c = PTETOCMAP(*pte);
		ASSERT(c >= cmap && c < ecmap, "memfree: bad mem free");
		/*
		 *+ In freeing a page of memory, the kernel found that
		 *+ the physical address represented by the page table
		 *+ entry was invalid.
		 */
		ASSERT_DEBUG(c->c_refcnt == 1, "memfree: c_refcnt");

		if (detach) {					/* detaching */

			LOCK_MEM;
			c->c_refcnt = 0;
			c->c_gone = 1;
			ASSERT_DEBUG(!c->c_dirty, "memfree: detach c_dirty");
			if (c->c_pageout == 0) {
				CM_INS_HEAD_CLEAN(c);
			}
			UNLOCK_MEM;
			zapcl(pte, PG_RSVD|PG_PFNUM|PG_V);

		} else if (anycl(pte, pg_m)) {			/* dirty */

			LOCK_MEM;
			c->c_refcnt = 0;
			c->c_dirty = 1;
			if (c->c_pageout == 0) {
				CM_INS_TAIL_DIRTY(c);
			}
			UNLOCK_MEM;
#ifdef	MMU_MBUG
			zapcl(pte, PG_V);
#else
			zapcl(pte, PG_M|PG_V);
#endif	MMU_MBUG
			if (dirtymem >= vmtune.vt_dirtyhigh
			&&  !sema_avail(&drain_dirty))
				v_sema(&drain_dirty);

		} else {					/* clean */

			LOCK_MEM;
			ASSERT_DEBUG(!c->c_dirty, "memfree: clean c_dirty");
			c->c_refcnt = 0;
			if (c->c_pageout == 0) {
				CM_INS_TAIL_CLEAN(c);
			}
			UNLOCK_MEM;
			zapcl(pte, PG_V);

		}
	}

	/*
	 * Just freed some; if anyone waiting, let'em try for it.
	 */

	if (blocked_sema(&mem_wait))		/* if someone waiting... */
		vall_sema(&mem_wait);		/* wake'em up */
}

/*
 * memfree1()
 *	Free a single cluster, !detach, and allow c_refcnt > 1.
 *
 * Used by maped files to release pages while holding memory-lists locked
 * for other reasons.
 *
 * Basically the same thing as memfree() with different interface,
 * tuned for !detach and mapped-file needs.
 *
 * Assumes pte is NOT mapped, caller holds memory lists locked.
 */

memfree1(pte)
	register struct pte *pte;
{
	register struct cmap *c = PTETOCMAP(*pte);

	ASSERT_DEBUG(c >= cmap && c < ecmap, "memfree1: bad mem free");
	ASSERT_DEBUG(c->c_refcnt != 0, "memfree1: c_refcnt");

	/*
	 * Decrement ref-count.  If -> 0, put it on appropriate free-list.
	 */

	if (--c->c_refcnt == 0) {
		if (anycl(pte, pg_m)) {			/* dirty */
			c->c_dirty = 1;
			if (c->c_pageout == 0) {
				CM_INS_TAIL_DIRTY(c);
			}
#ifdef	MMU_MBUG
			zapcl(pte, PG_V);
#else
			zapcl(pte, PG_M|PG_V);
#endif	MMU_MBUG
			if (dirtymem >= vmtune.vt_dirtyhigh
			&&  !sema_avail(&drain_dirty))
				v_sema(&drain_dirty);
		} else {					/* clean */
			ASSERT_DEBUG(!c->c_dirty, "memfree1: clean c_dirty");
			if (c->c_pageout == 0) {
				CM_INS_TAIL_CLEAN(c);
			}
			zapcl(pte, PG_V);
		}
	}
}

/*
 * vredoFOD()
 *	Reconstruct a fill-on-demand pte from a cmap entry.
 *
 * Used in mmreg() when reallocating a reclaimable pre-paged FOD.
 *
 * Ok to not copy pte to others in cluster; pagein always looks at 1st.
 *
 * Caller holds memory locked.
 */

vredoFOD(c, pte)
	register struct cmap *c;
	struct pte *pte;
{
	ASSERT(!c->c_dirty, "vredoFOD: dirty");
	/*
	 *+ On reallocating a reclaimable fill-on-demand page,
	 *+ the cmap structure representing the page indicated
	 *+ that the page had been modified.
	 */

	*(int *) pte = BN_TO_PGBLKNO(c->c_blkno) | PG_FOD;
	c->c_holdfod = 0;			/* no longer holds FOD */
	++l.cnt.v_redofod;			/* count # rebuilt FOD ptes */
}

/*
 * wmemall()
 *	Allocate wired-down (non-paged) pages in kernel virtual memory.
 *
 * Called in smount() to get space to store cyl-group summary data.
 *
 * Note: could (maybe) use this in vgetpt()/etc, to simplify the code.
 */

caddr_t
wmemall(nbytes, must_succeed)
	int	nbytes;
	bool_t	must_succeed;
{
	register int	npg;
	register caddr_t va;
	register int	a;
	int	(*pmemall)() = (must_succeed ? vmemall : memall);

	npg = clrnd(btoc(nbytes));
	a = uptalloc((long)npg, must_succeed);
	if (a == 0)
		return(0);
	if ((*pmemall)(&Usrptmap[a], npg, &proc[0], CSYS) == 0) {
		uptfree((long)npg, (long)a);
		return(0);
	}
	/*
	 * adjust maxRS since we can no longer allocate this memory.
	 */

	maxRS -= npg;

	va = (caddr_t)kmxtob(a);
	vmaccess(&Usrptmap[a], va, npg);
	return(va);
}

/*
 * wmemfree()
 *	Free space allocated via wmemall().
 *
 * Called in unmount1() and error unwinds in smount().
 */

wmemfree(va, nbytes)
	caddr_t	va;
	int	nbytes;
{
	register int npg;
	register int a;

	a = btokmx((struct pte *)va);
	npg = clrnd(btoc(nbytes));
	(void) memfree(&Usrptmap[a], npg, 1);
	uptfree((long)npg, (long)a);
	/*
	 * adjust maxRS since we can now allocate this memory.
	 */

	maxRS += npg;
}

/*
 * meminit()
 *	Initialize core map.  Entire array has already been zeroed.
 *
 * Returns freemem value.
 *
 * Startup code allocates a struct cmap for each page of physical memory
 * including holes.  Thus, cmap[i] represents the physical page starting
 * at i*CLBYTES.
 *
 * cmap[] array has 3 basic parts:
 *
 *	1) part that represents physical addresses < &cmap[0]
 *	2) self-represention
 *	3) part that represents physical addresses >= &cmap[ncmap]
 *
 * Whole pages of cmap[] entries from part (1) are placed in the free list.
 * The self-ref part is awkward to reclaim and isn't very large, so waste some.
 * Whole pages of cmap[] entries representing holes in physical memory in part
 * (3) are also reclaimed into free-list.  This allows simple 1-1 map of cmap[]
 * entries to physical addresses and accomodates physical memory holes without
 * wasting much memory on useless cmap[] entries.
 */

meminit(first)
	caddr_t first;
{
	register struct cmap *c;
	register int cl;
	register int cm_cl;
	int	ncl;
	int	cm_ncl;
	int	next_cl;
	int	num_cl;
	long	sys_maxRS;
	int	firstfree;			/* cluster # of 1st free page */

	num_cl = ecmap - cmap;
	firstfree = clrnd(btop(first)) / CLSIZE;

	/*
	 * Init free-lists.  Put pages containing useless cmap[] on the
	 * free list.
	 */

	cm_clean.c_next = cm_clean.c_prev = &cm_clean;
	cm_dirty.c_next = cm_dirty.c_prev = &cm_dirty;

	cl = 0;
	while (cl < num_cl) {
		if (cl < firstfree || !page_exists(cl*CLSIZE)) {
			/*
			 * Found a hole.  Find next useful memory.
			 * 1st time thru handle Dynix as a "hole".
			 * Assumes cmap[] is physically contiguous
			 * but virt addrs not necessarily == phys.
			 */
			if (cl < firstfree) {
				next_cl = firstfree;
				ncl = PTETOPHYS(Sysmap[btop(cmap)]) / CLBYTES;
			} else {
				next_cl = cl;
				while (next_cl < num_cl && !page_exists(next_cl*CLSIZE))
					++next_cl;
				ncl = next_cl;
			}
			/*
			 * Put pages containing useless cmap[] entries
			 * into the free-list.
			 */
			cm_cl = ((int) &cmap[cl] + CLOFSET) / CLBYTES;
			cm_ncl = ((int) &cmap[ncl]) / CLBYTES;
			c = &cmap[ PTETOPHYS(Sysmap[cm_cl*CLSIZE]) / CLBYTES ];
			for (; cm_cl < cm_ncl; ++cm_cl, ++c) {
				c->c_gone = 1;
				CM_INS_TAIL_CLEAN(c);
			}
			cl = next_cl;
		} else {
			/*
			 * Add existing page to free list.
			 */
			c = &cmap[cl++];
			c->c_gone = 1;
			CM_INS_TAIL_CLEAN(c);
		}
	}
	avefree = freemem;
	ecmx = freemem;

	/*
	 * Init the list lock and waiting semaphore.
	 */

	init_lock(&mem_alloc, G_MEM);
	init_sema(&mem_wait, 0, 0, G_MEM);

	/*
	 * Insure we have good values in vmtune.
	 */

	setupvm();

	/*
	 * Insure maxRS fits in memory.
	 * The max reasonable Rset should fit in memory with the entire
	 * process representation, and leave at least minfree+dirtylow pages,
	 * and network memory usage (take once and never give back). 
	 */

	if (maxRS > freemem / CLSIZE)		/* sanity and aid heuristic */
		maxRS = freemem / CLSIZE;

	sys_maxRS = (freemem -
			( MAXSZPT			/* biggest PT */
			+ vmtune.vt_minfree		/* buffer zone */
			+ vmtune.vt_dirtylow		/* buffer zone */
			+ nmbclusters * CLSIZE		/* max net mem */
			+ maxRSslop			/* slop */
			)
		) / CLSIZE;
	if (sys_maxRS < maxRS) maxRS = sys_maxRS;
	if (maxRS > MAXADDR/CLBYTES) maxRS = MAXADDR/CLBYTES;
	if (vmtune.vt_maxRS > maxRS) vmtune.vt_maxRS = maxRS;
	if (vmtune.vt_minRS > maxRS) vmtune.vt_minRS = maxRS;

	/*
	 * Compute forkmap, maxforkRS and init forkmap management lock/sema.
	 * maxforkRS must allow two such to fit in maxRS, and to fit within
	 * forkmap.
	 */

	forkmap = (freemem * forkmapmult) / forkmapdiv;
	maxforkRS = (forkmap/2 -
			( MAXSZPT			/* biggest PT */
			+ maxRSslop			/* slop */
			)
		) / CLSIZE;
	if (maxforkRS > maxRS/2)
		maxforkRS = maxRS/2;
	if (maxforkRS < (16 * 1024) / CLBYTES)		/* 16K= arbitrary > 0 */
		panic("meminit: maxforkRS too small");
		/*
		 *+ The variable controlling the maximum resident set size
		 *+ allowed on a fork is too small.  Corrective action:
		 *+ reconfigure the kernel with a higher value.
		 */

	init_lock(&forkmap_mutex, G_FORKMAP);
	init_sema(&forkmap_wait, 0, 0, G_FORKMAP);

	return(freemem);
}

/* 
 * vslock()
 *	Lock a virtual segment.
 *
 * N-pass algorithm:
 *	For each cluster of pages, ref it to fault it in.
 *	If didn't fault at all for this, done, else repeat.
 *
 * In practice, 1 or 2 passes will put it all in the Rset (worst case
 * is n passes for n clusters).  If this becomes a problem, could be
 * smarter about Rset manipulation.
 *
 * This insures the pages are in the Rset.  No actual locking takes
 * place.  Thus vsunlock() becomes unnecessary (hence deleted).
 *
 * Called from physio to ensure that the pages 
 * participating in raw i/o are valid and locked.
 *
 * Caller has validated the user address range.
 */

vslock(base, count, isread)
	caddr_t	base;
	int	count;
	bool_t	isread;
{
	register caddr_t vaddr;
	register int ncl;
	register int i;
	long	fltcnt;
#ifdef	DEBUG
	int	loopcnt = 0;
#endif	DEBUG

	ncl = btoc(count + ((int)base & CLOFSET));
	ASSERT_DEBUG(ncl <= u.u_procp->p_rscurr*CLSIZE, "vslock: can't fit");

	/*
	 * Repeat the loop until don't fault.
	 *
	 * Note: pages placed into previously empty slots in Rset don't
	 * bump u_pffcount.  This is ok since we already know transfer
	 * fits in Rset and once algorithm faults a page into Rset it stays
	 * thru the RAW IO.
	 */

	do {
		ASSERT_DEBUG(loopcnt++ <= (ncl+CLSIZE-1)/CLSIZE, "vslock");
		fltcnt = u.u_pffcount;

		/*
		 * Reach out and ref each cluster.
		 *
		 * If a read, must insure page is seen as modified.
		 */

		vaddr = base;
		for (i = 0; i < ncl; i += CLSIZE, vaddr += CLBYTES) {
			if (isread)
				(void) subyte(vaddr, (char) fubyte(vaddr));
			else
				(void) fubyte(vaddr);
		}

	} while (fltcnt != u.u_pffcount);

#ifdef	DEBUG
	/*
	 * Sanity check: all pte's should be valid when we're done here.
	 */
	{
		register struct pte *pte;
		pte = vtopte(u.u_procp, btop(base));
		do {
			ASSERT(pte->pg_v, "vslock: invalid pte");
			pte += CLSIZE;
			ncl -= CLSIZE;
		} while (ncl > 0);
	}
#endif	DEBUG
}
