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
static	char	rcsid[] = "$Header: vm_page.c 2.20 1991/05/23 22:11:45 $";
#endif

/*
 * vm_page.c
 *	Page-fault handling.
 *
 * Conditionals:
 *	-DMMU_MBUG	Insure 'm' bit is always on for RW pages (ns32000 only)
 *
 * Note: pagein() does *NOT* use splimp()'s to block against interrupt-
 * level memory allocate (for network code; would need raised SPL for extended
 * time).  This code MUST be re-worked in this regard if MONOPROCESSOR.
 */

/* $Log: vm_page.c,v $
 *
 */

#include "../h/param.h"
#include "../h/file.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/buf.h"
#include "../h/cmap.h"
#include "../h/vm.h"

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/mmu.h"
#include "../machine/mftpr.h"

/*
 * pagein()
 *	Handle a page fault.
 *
 * Basic outline:
 *	Figure out type of page (data, stack).
 *	If page is allocated, but just not valid, reclaim it:
 *		Lock memory free-list (to mutex reclaimables and free-list).
 *		Reclaim it.  Handle race where page is re-used before hold lock.
 *		Unlock memory list.
 *		Done
 *	If mapped (data only):
 *		call mapping function to get a reference.
 *		if copy-on-ref, get private copy and release ref to shared page.
 *	If zero-fill:
 *		Allocate a page (careful about blocking), zero it, done.
 *	Compute <dev,bn> from which page operation would take place:
 *		Either inode (FOD) or swap-space.
 *	Lock process from swapping for duration.
 *	Allocate memory for page:
 *		If block here, restart because could have swapped, etc.
 *	Try to arrange for several pages to come in (eg, pre-paging/klustering).
 *	Do swap IO to bring the page in.
 *	After swap:
 *		Instrumentation.
 *		Validate the required new page,
 *		leave prepaged pages reclaimable (not valid).
 *	Common exit:
 *		Stats.
 *
 * Caller verified is invalid with good protection field in pte.
 *
 * Handles mapped file errors by replacing the mapped page with a valid
 * private page and swkill()'ing the process.  Thus unless the address is bad,
 * pagein() succeeds.  Swap IO errors (into private space) are handled by
 * swap() doing the swkill(); for now process has random data in private memory.
 * Some of this can be done better.
 *
 * NB: process can block here, swappable, waiting for memory.  It does
 * this after having selected RSslot.  This is ok, since after swapin the
 * Rset is same size as it was, and the Rset slot at RSslot is still empty.
 *
 * Stack case is currently a subset of data case (no mapped ptes),
 * but suspect no big performance win to have seperate case.
 *
 * PFF can't mess with the Rset structures while it's being messed with
 * here, since PFF interrupt only occurs on interrupt return to user-mode.
 */
#ifdef	i386
/*
 * On the 80386, pagein() returns a boolean value indicating success of the
 * fault handling (since not enough status from page-fault-error-code
 * to decide that way, and kernel access to user space circumvents
 * access checking).
 *
 * 80386 doesn't cache invalid pte, thus no need to flush TLB entry
 * when make a pte valid.  Kernel is always careful to verify access (or
 * only read) before accessing user space, thus kernel won't make user
 * inaccessible pte cached.  If do COW, need to be more careful, since
 * TLB can have valid RO pte cached that gets changed to RW.
 */
#endif	i386

pagein(fvaddr)
	unsigned fvaddr;
{
	register struct pte *pte;
	register struct cmap *c;
	register struct	mmap *um;
	register u_int	v;
	register struct	proc *p = u.u_procp;
	int		type;
	int		i;
	int		prot;
	u_long		paddr;
	u_long		mapoff;
	struct	pte	opte;
	int		klsize;
	int		delta;
	int		result;
#ifdef	ns32000
	unsigned	vaddr;
#endif	ns32000
	struct	dblock	db;

	/*
	 * Classify faulted page into a segment and get a pte
	 * for the faulted page.
	 */

	v = clbase(btop(fvaddr));
#ifdef	ns32000
	vaddr = ctob(v);
#endif	ns32000

	if (isadsv(p, v)) {
		type = CDATA;
		pte = dvtopte(p, v);
	} else {
#ifdef	i386
		if (!isassv(p, v))
			return(0);			/* bad page */
#endif	i386
		type = CSTACK;
		pte = svtopte(p, v);
		ASSERT_DEBUG(isassv(p, v), "pagein: bad vaddr");
	}

#ifdef	ns32000
	/*
	 * 032 MMU provides enough status that can't get here if page is
	 * valid, write on a RO, or totally invalid page.  abttrap() uses
	 * this to not call here if not a legit page-fault.
	 */
	ASSERT(pte->pg_v == 0, "pagein: valid pte");
	/*
	 *+ The status of a page marked in the page table entry
	 *+ was inconsistent with the type of fault taken on that
	 *+ page.  This indicates a probable kernel software bug.
	 */
#endif	ns32000

#ifdef	i386
	/*
	 * Since page-fault error-code is unreliable, can't rely on HW to
	 * tell if doing write on RO page.  There also isn't enough HW
	 * status anyhow to tell if the pte is completely invalid.
	 */
	if (pte->pg_v) {			/* assume write on RO page */
		ASSERT(((*(int *) pte) & PG_PROT) == RO, "pagein: valid !RO");
		/*
		 *+ The system page faulted on a page that was valid and not
		 *+ marked read-only.  A fault should occur only if the page
		 *+ is either invalid or not writable.
		 */
		return(0);
	}
	if (*(int *) pte == PG_INVAL)		/* address space hole */
		return(0);
#endif	i386

	/*
	 * Insure there's room for a page in the resident-set of the process.
	 */

	VALLOC_RS_SLOT(p);

	/*
	 * If page is reclaimable, reclaim it.
	 * Else if zero-fill, allocate and zero-fill.
	 * Else if mapped, get a ref and handle copy-on-ref.
	 * Else page in from swap space.
	 *
	 * Note that can race on reclaimable -- it can be reallocated in
	 * lmemall() while being looked at here; thus re-check after
	 * lock memory lists.
	 */
again:
	if (PTEPF(*pte)) {				/* reclaim! */
		/*
		 * Maybe reclaimable.  Lock memory lists and look again.
		 */
		LOCK_MEM;
		if (PTEPF(*pte) == 0) {			/* lost race */
			UNLOCK_MEM;			/* rare! */
			goto again;
		}
		ASSERT_DEBUG(!PTEMAPPED(*pte) && pte->pg_fod == 0,
							"pagein: reclaim");
		/*
		 * If page not being cleaned (pageout), pull off free list.
		 */

		c = PTETOCMAP(*pte);
		if (c->c_pageout == 0) {
			CM_UNLINK(c);
			l.cnt.v_pgfrec++;
		} else {
			ASSERT_DEBUG(c->c_refcnt == 0, "pagein: IO reclaim");
		}

		/*
		 * Bump ref-count, set 'm' bit if was dirty.
		 */

		c->c_refcnt = 1;
		if (c->c_dirty) {
			pte->pg_m = 1;
			c->c_dirty = 0;
			l.cnt.v_pgdrec++;
		}

		UNLOCK_MEM;

#ifdef	MMU_MBUG				/* ns32000 only */
		*(int*)pte |= PG_V|PG_R|PG_M;	/* for RW page */
#else
		*(int*)pte |= PG_V|PG_R;
#endif	MMU_MBUG
		distcl(pte);
		u.u_ru.ru_minflt++;
		l.cnt.v_pgrec++;
#ifdef	ns32000
		newuptes(vaddr);
#endif	ns32000
		++p->p_rssize;

	} else if (pte->pg_fod) {		/* zero-fill on demand */

		/*
		 * FOD bit set in private page-table ==> zero-fill.
		 * Alloc a cluster of pages, put it in the pte's,
		 * validate, and zap pages.
		 *
		 * Note that may block, and may swap; thus "pte" may
		 * have changed ==> need to recompute it.
		 */
		ASSERT_DEBUG(*(int *)pte & PG_FZERO, "pagein: FOD !ZFOD");
		ASSERT_DEBUG((*(int *)pte & PG_PROT) == RW, "pagein: ZFOD !RW");

		while (!memall(pte, CLSIZE, p, type)) {
			WAIT_MEM;
			if (type == CDATA)
				pte = dvtopte(p, v);
			else
				pte = svtopte(p, v);
		}

		clearseg(PTETOPHYS(*pte));
		*(int *) pte |= RW | PG_V|PG_R|PG_M;
		distcl(pte);
		l.cnt.v_zfod += CLSIZE;
#ifdef	ns32000
		newuptes(vaddr);
#endif	ns32000
		++p->p_rssize;

	} else if (PTEMAPPED(*pte)) {		/* mapped pte */

		/*
		 * Mapped pte ==> get it from whoever is handling the mapping.
		 * If copy-on-ref mapping, get a page in place first and
		 * copy (& release) when done.
		 *
		 * Since process might swap inside mapper, re-compute
		 * pte before set up entry.
		 */

		um = &u.u_mmap[PTETOMAPX(*pte)];
		mapoff = PTETOMAPOFF(p, pte, um);

		ASSERT_DEBUG(type == CDATA, "pagein: !data mapped");
		ASSERT_DEBUG(um->mm_paged, "pagein: mapped !paged");

		prot = (*(int*)pte & (PTE_MMAP_MASK|PG_PROT)) | PG_V|PG_R;
#ifdef	MMU_MBUG
		prot |= PG_M;			/* ns32000 only */
#endif	MMU_MBUG

		if (um->mm_cor) {		/* copy-on-ref! */
			/*
			 * Allocate private page before ref the mmap'd page,
			 * to avoid holding ref if swap out.  Mark it valid
			 * before calling mapper in case swap in mapper (don't
			 * want it to appear "reclaimable").
			 * No need to set mod-bit until after copy page,
			 * since will overwrite entire page below.
			 */
		cor_again:
			while (!memall(pte, CLSIZE, p, CDATA)) {
				WAIT_MEM;
				pte = dvtopte(p, v);
			}
			*(int *) pte |= (prot & ~PTE_MMAP_MASK);
			distcl(pte);
			++p->p_rssize;
			/*
			 * Get a reference to the shared page.  Re-look at
			 * pte since can swap in mapper; if so, start over...
			 * livelock in this case seems extremely unlikely.
			 */
			paddr = (*um->mm_ops->map_refpg)(um->mm_handle, mapoff);
			pte = dvtopte(p, v);
			if (!pte->pg_v) {		/* swapped in mapper */
				if ((paddr & MM_BADMAP) == 0) {
					(*um->mm_ops->map_derefpg)
						(um->mm_handle, mapoff, 0, 1);
				}
				pte = dvtopte(p, v);
				goto cor_again;
			}
			/*
			 * Copy the shared page into the private page.
			 * Release ref to mapped page, and to the map if this
			 * was the last page.  This is essentially a simplified
			 * vmmapfree().  Assume !lastfd -- VTEXT doesn't care
			 * the fd still exists for MAP_PRIVATE.
			 *
			 * If mapper returned an error, arrange to kill 
			 * with "meaningful" error message.  Clear the
			 * private page to avoid showing random data.
			 */
			if (paddr & MM_BADMAP) {
				mmap_swkill(p, (int) paddr);
				clearseg(PTETOPHYS(*pte));
			} else {
				copyphys((u_int) paddr, PTETOPHYS(*pte));
				(*um->mm_ops->map_derefpg)(um->mm_handle,
								mapoff, 0, 1);
			}
			*(int *) pte |= PG_M;
			if ((um->mm_pgcnt -= CLSIZE) == 0) {
				(*um->mm_ops->map_unmap)(um->mm_handle,
						um->mm_off, um->mm_size,
						um->mm_prot);
				if (um->mm_fdidx >= 0) {
					deref_file(&file[um->mm_fdidx]);
				}
			}
			u.u_mmapdel = 1;	/* deleted a mapped page */
		} else {			/* shared mapped file */
			/*
			 * Get a ref to the shared page and install it in PT.
			 * Re-compute pte since can swap in mapper.
			 *
			 * If mapper returns an error, release reference to
			 * mapped page, set up a zero-fill page, and arrange
			 * to fill it.  Also arrange for caller to be killed.
			 * Traps from within kernel mode expect pagein() to
			 * succeed, so let it succeed with bad data.  Process
			 * won't execute in user mode again to ever see the
			 * data anyhow.
			 */
			paddr = (*um->mm_ops->map_refpg)(um->mm_handle, mapoff);
			pte = dvtopte(p, v);
			if (paddr & MM_BADMAP) {
				mmap_swkill(p, (int) paddr);
				(void) vmmapfree(p, pte, 1);
				ptefill(pte, PG_ZFOD, CLSIZE);
				l.cnt.v_nzfod += CLSIZE;
				goto again;		/* easy way out */
			} else {
				for (i=0; i < CLSIZE; i++, pte++, paddr += NBPG)
					*(int*)pte = PHYSTOPTE(paddr) | prot;
				++p->p_rssize;
			}
		}
#ifdef	ns32000
		newuptes(vaddr);
#endif	ns32000
		/* statistics? */

	} else {				/* page in from swap */

		/*
		 * Wasn't reclaimable, zero-fill, or mapped ==> read from swap.
		 * Allocate a page (non-swappable), and remember previous
		 * pte.  pgin_klust() will allocate more pages if appropriate
		 * and there is memory.
		 *
		 * Local VM ==> state of process is stable thru this, can
		 * maybe swap; this cannot make the faulted page valid.
		 *
		 * Since can block while waiting for memory and can swap,
		 * have to re-figure pte after wakeup.
		 */

		opte = *pte;

		while (!memall(pte, CLSIZE, p, type)) {
			WAIT_MEM;
			if (type == CDATA)
				pte = dvtopte(p, v);
			else
				pte = svtopte(p, v);
		}

		/*
		 * Lock this process from swap, construct the new pte.
		 *
		 * Make this process non-swappable during the swap IO.
		 *
		 * No need to mark cmap[] entry "intrans", since this is a
		 * private page.
		 */

		++p->p_noswap;
		pte->pg_prot = opte.pg_prot;
		*(int*)pte |= PG_V|PG_R;

		if (type == CSTACK) {
			result = vstodb(ctod(vtosp(p, v)), ctod(1), 
							&u.u_smap, &db, 1);
		} else {
			result = vstodb(ctod(vtodp(p, v)), ctod(1), 
							&u.u_dmap, &db, 0);
		}

		/*
		 * If the swap location does not exist, cleanup and
		 * kill the process. This situation is illegal and
		 * should never happen!
		 */
		if (result != 0) {
			memfree(pte, CLSIZE, 1);
			ptefill(pte, PG_ZFOD, CLSIZE);
			--p->p_noswap;
			swkill(p, "BAD PAGEIN DMAP");
			goto again;		/* easy way out */
		}

		/*
		 * Try to find adjacent pages to bring in also.
		 */

		ASSERT(opte.pg_pfnum == 0, "pagein: pfnum");
		/*
		 *+ An invalid (not present) page was marked as being on a
		 *+ swap device, but it had a nonzero page frame number.
		 */
#ifdef	MMU_MBUG
		pte->pg_m = 1;				/* ns32000 only */
#endif	MMU_MBUG
		delta = pgin_klust(p, type, v, pte, &klsize);
		db.db_base -= ctod(delta);

		distcl(pte);

		swap(p, db.db_base, pte-delta, klsize * ctob(CLSIZE),
						B_READ|B_PGIN, swapdev_vp);

		/*
		 * Fix page table entries.
		 *
		 * Only page requested in is validated (above), and rest of
		 * pages can be ``reclaimed''.
		 *
		 * pte is still valid from above, points to clbase of faulting
		 * vaddr.
		 *
		 * No need to invalidate TLB for new ptes since we blocked
		 * for swapin, and context switch flushes user TLB.
		 */

		if (klsize > 1) {
			memfree(pte-delta, delta, 0);
			memfree(pte+CLSIZE, klsize*CLSIZE-(delta+CLSIZE), 0);
		}

		/*
		 * All done.  Keep stats, allow swapping again, and fall
		 * thru to common exit.
		 */

		u.u_ru.ru_majflt++;
		l.cnt.v_pgin++;
		l.cnt.v_pgpgin += klsize * CLSIZE;

		--p->p_noswap;
		++p->p_rssize;
	}

	/*
	 * Common exit: do some stats.
	 */

	if (p->p_rssize > u.u_ru.ru_maxrss)
		u.u_ru.ru_maxrss = p->p_rssize;
#ifdef	i386
	return(1);				/* success! */
#endif	i386
}

/*
 * pgin_klust()
 *	Try to locate pages adjacent to the one we are about to
 *	page-in and bring in those, too.
 *
 * Caller does NOT hold memory-lists locked.
 *
 * We knows that the process image is contiguous in chunks;
 * an assumption here is that CLSIZE * KLMAX is a divisor of dmmin,
 * so that by looking at KLMAX chunks of pages, all such will
 * necessarily be mapped swap contiguous.
 *
 * For now, simple (untunable) limit of KLMAX pre-paged.
 *
 * Assumes KLMAX is a power of two.
 *
 * Returns delta from argument pte of base of transfer.
 */

static
pgin_klust(p, type, v, pte0, pkl)
	struct proc *p;
	int	type;
	unsigned v;
	struct pte *pte0;
	int	*pkl;
{
	register struct pte *pte;
	register int ptedelta;
	register int klback;
	register int klforw;
	int	cl;
	int	clmax;
	int	kloff;
	int	klmax;
	int	prot;

	/*
	 * Figure cluster page lives in and max size of corresponding segment.
	 */

	if (type == CSTACK) {
		cl = vtosp(p, v) / CLSIZE;
		clmax = p->p_ssize / CLSIZE;
		ptedelta = -CLSIZE;
	} else {
		cl = vtodp(p, v) / CLSIZE;
		clmax = p->p_dsize / CLSIZE;
		ptedelta = CLSIZE;
	}

	/*
	 * Determine kluster offset, look at "previous" pte's
	 * for pages to bring in.  Insist that pages be not FOD and
	 * have zero pg_pfnum field (==> invalid, not reclaimable).
	 *
	 * Note the test could be done via:
	 *
	 *	(*(int*)pte & (PG_FOD|PTE_MMAP_MASK|PG_PFNUM))
	 *   || *(int*)pte == PG_INVAL
	 *
	 * but the current code is more readable and adapts to changes
	 * more easily.
	 *
	 * NOTE: pte can go from reclaimable to re-fill from swap concurrent
	 * with these loops -- no problem, might just stop looking sooner.
	 * This relies on private PT not having any re-buildable FOD pte's.
	 *
	 * No need to special fuss the cmap[] entries; will free them after
	 * swapping them in.
	 *
	 * Also no need to set pg_m for MMU_MBUG; these pages will be freed
	 * after finish swap, and pg_m is correctly dealt with on reclaim.
	 */

	kloff = cl & (KLMAX-1);

	pte = pte0 - ptedelta;
	for (klback = kloff-1; klback >= 0; klback--, pte -= ptedelta) {
		if (pte->pg_fod
		||  PTEMAPPED(*pte)
		||  PTEPF(*pte)			/* reclaimable | valid */
		||  *(int*)pte == PG_INVAL)
			break;
		ASSERT_DEBUG(pte->pg_v == 0, "pgin_klust: valid pte1");
		prot = *(int*)pte & PG_PROT;
		if (!memall(pte, CLSIZE, p, type))
			break;
		*(int*)pte |= prot;
		distcl(pte);
	}
	++klback;

	/*
	 * Similarly, look "forward" at pte's.
	 */

	if ((cl - kloff) + KLMAX > clmax)
		klmax = clmax - (cl - kloff);
	else
		klmax = KLMAX;

	pte = pte0 + ptedelta;
	for (klforw = kloff+1; klforw < klmax; klforw++, pte += ptedelta) {
		if (pte->pg_fod
		||  PTEMAPPED(*pte)
		||  PTEPF(*pte)
		||  *(int*)pte == PG_INVAL)
			break;
		ASSERT_DEBUG(pte->pg_v == 0, "pgin_klust: valid pte2");
		prot = *(int*)pte & PG_PROT;
		if (!memall(pte, CLSIZE, p, type))
			break;
		*(int*)pte |= prot;
		distcl(pte);
	}
	--klforw;

	/*
	 * Know kluster size now.
	 * Value is delta (in HW pages) from argument 'v' to start of kluster.
	 */

	*pkl = klforw - klback + 1;
	return((type == CSTACK) ? (klforw - kloff) * CLSIZE
				: (kloff - klback) * CLSIZE);
}
