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
static	char	rcsid[] = "$Header: vm_pt.c 2.16 1991/05/15 15:52:28 $";
#endif

/*
 * vm_pt.c
 *	Various page-table manipulations.
 *
 * This module contains a number of implementation specific procedures.
 */

/* $Log: vm_pt.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/kernel.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

lock_t		upt_alloc;		/* for uptmap mutex */
sema_t		upt_wait;		/* for waiting for space */
struct	map	*uptmap;		/* map base, init in allockds() */

/*
 * This implements a TLB-coherency scheme for Usrptmap[]
 */
#define UPT_Q_SIZE (1024)		/* # entries queued */
struct upt_q {
	long size;			/* arguments to uptfree() */
	long base;
} upt_q[UPT_Q_SIZE];
int upt_q_hd = 0, upt_q_tl = 0;		/* FIFO of stuff to be freed */
int upt_flushing = -1;			/* Marker for what's being freed */
int upt_size = 0;			/* # elements in queue */
int upt_q_ovfl = 0;			/* # we had to drop on overflow */

#ifdef	DEBUG
extern	int	swapdebug;
#ifdef	PERFSTAT
int	cur_uptsize;			/* current total space in uptmap */
int	min_uptsize;			/* minimum space ever in uptmap */
#endif	/* PERFSTAT  */
#endif	/* DEBUG  */
static void upt_q_flush();

/*
 * uptinit()
 *	Initialize user page-table allocation.
 */

uptinit(uptsize, nentry)
	long	uptsize;
	int	nentry;
{
	/*
	 * Init the resource-map.
	 * Note: address "0" is bad; thus, start at CLSIZE and have CLSIZE less
	 * (CLSIZE keeps allocations rounded to CLSIZE boundaries).
	 */

	rminit(uptmap, uptsize-CLSIZE, (long)CLSIZE, "usrpt", nentry);

	/*
	 * Init the locks & semaphores to control this.
	 */

	init_lock(&upt_alloc, G_UPT);
	init_sema(&upt_wait, 0, 0, G_UPT);

#if	defined(DEBUG) && defined(PERFSTAT)
	cur_uptsize = min_uptsize = uptsize-1;
#endif	/* DEBUG&PERFSTAT  */
}

/*
 * uptalloc()
 *	Allocate space for user page-tables/etc.
 */

int
uptalloc(size, must_succeed)
	long	size;				/* how much needed */
	bool_t	must_succeed;			/* true ==> must succeed */
{
	register long	base;
	spl_t		s;

	/*
	 * Loop, trying to allocate (if must succeed).
	 * If don't insist on succeed, make only one try.
	 * If need to wait, use "pv" to insure we're sleeping before
	 * somebody could free some.
	 */

	for (;;) {
		s = p_lock(&upt_alloc, SPLHI);		/* lock resource map */
		base = rmalloc(uptmap, size);
		if (base == 0) {

			/*
			 * Flush the pending stuff if any
			 */
			if (upt_size && (upt_flushing == -1)) {
				upt_flushing = upt_q_hd;
				v_lock(&upt_alloc, s);
				upt_q_flush();
				continue;
			}

			/*
			 * Otherwise fall asleep if we must succeed, or
			 * return a failure if we don't have to
			 */
 			if (!must_succeed) {
				v_lock(&upt_alloc, s);
				return(0);
			}
			(void)p_sema_v_lock(&upt_wait, PRSWAIT, &upt_alloc, s);
		} else {
#if	defined(DEBUG) && defined(PERFSTAT)
			if ((cur_uptsize -= size) < min_uptsize)
				min_uptsize = cur_uptsize;
#endif	/* DEBUG&PERFSTAT  */
			v_lock(&upt_alloc, s);		/* unlock map */
			return(base);
		}
	}
}


/*
 * Flush out stuff enqueued to be freed.
 *
 * This routine sends out the TLB flush, frees the entries into the map,
 * then updates the queue FIFO to reflect all the released entries.
 *
 * To handle TLB inconsistency problems, we must first send out a FlushTLB
 * operation to all, then free up the stuff we said we would.  This code
 * is interesting in that the mutex is the variable upt_flushing, which
 * was set within a lock which has now been released.  This is to allow
 * this function to be invoked without locks held, which simplifies the
 * broadcast TLB flush.
 */
static void
upt_q_flush()
{
	spl_t s;
	register int x;
	register struct upt_q *qp;

	/*
	 * Tell everyone to dump their TLB for pages in the Usrptmap range.
	 */
	newptes((caddr_t)usrpt, (long)Usrptsize*CLSIZE*NBPG);
	FlushTLBnowait((caddr_t)usrpt, Usrptsize*CLSIZE*NBPG);

	/*
	 * Lock the Usrptmap mutex and free the pending entries
	 */
	s = p_lock(&upt_alloc, SPLHI);
	x = upt_q_tl;
	qp = &upt_q[x];
	while (x != upt_flushing) {
		rmfree(uptmap, qp->size, qp->base);
		upt_size -= 1;

		++qp;
		if (++x >= UPT_Q_SIZE) {
			x = 0;
			qp = upt_q;
		}
	}

	/*
	 * Release these entries from the queue
	 */
	upt_q_tl = upt_flushing;

	/*
	 * All done flushing
	 */
	upt_flushing = -1;

	v_lock(&upt_alloc, s);
}

/*
 * uptfree()
 *	Release user page-table space.
 */

uptfree(size, base)
	long	size;
	long	base;
{
	spl_t	s;
	int flushing = 0;
	register int nidx;
	struct upt_q *qp;

	/*
	 * Lock the resource map and enqueue the space to be freed
	 */
	s = p_lock(&upt_alloc, SPLHI);

	/*
	 * Get a place in the freeing queue
	 */
	nidx = upt_q_hd;
	qp = &upt_q[nidx];
	if (++nidx >= UPT_Q_SIZE)
		nidx = 0;

	/*
	 * Drop on floor if we overrun
	 */
	if (nidx == upt_q_tl) {
		upt_q_ovfl += size;
		v_lock(&upt_alloc, s);
		return;
	}

	/*
	 * Otherwise enqueue to free, update FIFO
	 */
	qp->size = size;
	qp->base = base;
	upt_q_hd = nidx;
	upt_size += 1;

#if	defined(DEBUG) && defined(PERFSTAT)
	cur_uptsize += size;
#endif	/* DEBUG&PERFSTAT  */

	/*
	 * If we're over 3/4 full, or there are page table waiters,
	 * flush it out.
	 */
	if ((upt_flushing == -1) &&
			((upt_size >= ((UPT_Q_SIZE * 3) / 4)) ||
			  blocked_sema(&upt_wait))) {
		upt_flushing = upt_q_hd;
		flushing = 1;
	}

	v_lock(&upt_alloc, s);

	/*
	 * If we've signed up to do flushing, let'er rip
	 */
	if (flushing) {
		upt_q_flush();
	}

	/*
	 * If anybody is blocked waiting, wake'em all up.
	 * (strategy: assuming waiters need different sizes, we let them
	 * fight for it; note that this implies random queuing order)
	 *
	 * vall_sema() semantics allow call outside the lock.
	 */
	if (blocked_sema(&upt_wait))
		vall_sema(&upt_wait);
}

#if	defined(DEBUG) && defined(PERFSTAT)
/*
 * uptmap_stat()
 *	Report uptmap space stats.
 */

uptmap_stat()
{
	register struct mapent *ep;
	register int max_chunk;
	register int count;
	spl_t	s;

	printf("Usrptsize=%d, curr=%d, min=%d.\n", Usrptsize, cur_uptsize, min_uptsize);

	s = cp_lock(&upt_alloc, SPLHI);
	if (s == CPLOCKFAIL) {
		printf("Can't lock upt_alloc.\n");
		return;
	}

	for (count = max_chunk = 0, ep = (struct mapent *)(uptmap+1); ep->m_size; ep++, count++) {
		if (ep->m_size > max_chunk)
			max_chunk = ep->m_size;
	}

	v_lock(&upt_alloc, s);

	printf("Uptmap: %d chunks, max = %d.\n", count, max_chunk);
}
#endif	/* DEBUG&PERFSTAT  */

/*
 * vgetpt()
 *	Get page tables for process p.
 *
 * Allocator for memory is argument; process must be locked
 * from swapping if vmemall is used; if memall is used, call
 * will return w/o waiting for memory.  In any case an error
 * return results if no user page table space is available.
 *
 * Allocates pte's for page tables (level 1 & 2) and Rset.
 * Allocates pages for page-tables and Rset.
 *
 * Assumes somebody else (vgetu) allocates pte's and pages for U-area.
 *
 * Resident-set size is taken from p_rssize.  p_rssize is used as
 * a holding place for expected Rset size while the process is being
 * created or is swapped out.  Vgetu() arranges these fields after
 * the U-area is allocated.
 *
 * Caller must fill out rest of proc and U-area representation of Rset.
 */

vgetpt(p, pmemall)
	register struct proc *p;
	int (*pmemall)();
{
	register long ptab;			/* pte idx for page-tables */
	bool_t	must_succeed;
	int	vmemall();			/* always succeeds allocator */

	must_succeed = (pmemall == vmemall);	/* must succeed if vmemall() */

	/*
	 * Allocate pte space for, and pages for page-tables (level 1 && 2)
	 * for the process.  UL1PT pages and UL2PT pages are concatenated,
	 * with UL2PT comming first.
	 *
	 * Then, set up kernel addressibility of the pages, and init
	 * p_ptb1 for context switching.  Finally, fill out UL1PT.
	 *
	 * The page-tables are cleared out for clean post-mortem.
	 * (this also assures clean PT's on expansion swap-in; seems
	 * like undue overhead -- should consider ways to eliminate).
	 *
	 * Note that p->p_szpt can == 0, for system processes (these
	 * have *no* UL1PT or UL2PT).
	 */

	if (p->p_szpt) {
		ASSERT_DEBUG(p->p_szpt >= UL1PT_PAGES &&
				(p->p_szpt % CLSIZE) == 0, "vgetpt: bad szpt");
		ptab = uptalloc((long)p->p_szpt, must_succeed);
		if (ptab == 0)
			goto err0;
		p->p_ul2pt = UL2PT(ptab);
		p->p_pttop = PTTOP(p);

		if ((*pmemall)(&Usrptmap[ptab], p->p_szpt, p, CSYS) == 0)
			goto err1;

		vmaccess(&Usrptmap[ptab], (caddr_t)PTBASE(p), p->p_szpt);

		p->p_ptb1 = PHYSUL1PT(p);

		bzero((caddr_t)p->p_ul2pt, (u_int)((p->p_szpt - UL1PT_PAGES) * NBPG));
#ifdef	i386
		/*
		 * Inherit level-1 page-table from caller, thus insuring
		 * all kernel stuff is mapped.  Also, set up self-reference
		 * in the page-table (page-table may move virtually, but
		 * physical address of level-1 PT never changes unless
		 * process swaps).  Clear FPA mapping in child: it must trap
		 * and restore fpa state from Uarea if an fpa process.
		 */
		pgcopy((caddr_t)VA_L1PT, (caddr_t)L1PT(p));
		*(int *) (L1PT(p) + L1IDX(VA_PT)) = PHYSTOPTE(p->p_ptb1)
						  | PG_V|PG_KW|PG_R|PG_M;
		*(int *) (L1PT(p) + L1IDX(VA_FPA)) = PG_INVAL;
#endif	/* i386  */
		vfill_ul1pt(p);
	} else {
		p->p_pttop = p->p_ul2pt = (struct pte *)0;
		p->p_ptb1 = 0;
	}

	/*
	 * All done, no errors.
	 */

	return(1);

	/*
	 * Error unwinds...  Give it all back and return.
	 * Also, zap fields for clean post-mortem.
	 */
err1:
	if (p->p_szpt)
		uptfree((long)p->p_szpt, ptab);
err0:
	p->p_pttop = p->p_ul2pt = (struct pte *)0;
	p->p_ptb1 = 0;
	return(0);
}

/*
 * vfill_ul1pt()
 *	After allocating page-tables, fill out UL1PT to correctly
 *	point at the UL2PT pages, and update user mapping.
 *
 * This is done by copying the pte's in Usrptmap into the UL1PT, text+data
 * first from bottom-up, then stack from top-down.  Thus, UL1PT winds up
 * with a hole (of zero pte's) in the middle, unless the process is big
 * enough that the all UL1PT entries are filled (==> data+stack pte's
 * "meet" in the same UL2PT page).  (Note: on SGS, stack+data can only
 * meet on a level-2 mapping page boundary).
 *
 * Handling of large process where stack and data pte's "meet" is implicit,
 * due to p_pttop limiting actual used part of UL2PT to max size UL2PT.
 *
 * Does *not* flush user or kernel TLB -- assumes caller flushes when/if
 * appropriate.
 */

vfill_ul1pt(p)
	struct proc *p;
{
	register struct pte *ul1pt;
	register int	l2idx;
	register int	l1idx;
	register int	size;
	caddr_t	low;
	caddr_t	high;

	ul1pt = UL1PT(p);

	/*
	 * First fill the text+data portion of the UL1PT.
	 * Have to turn on user RW to allow user to access the level 2
	 * page-table pages; let level 2 pte's restrict the access.
	 *
	 * Note: casts only used to get more efficient code.
	 */

	l2idx = btokmx(p->p_ul2pt);
	l1idx = 0;
	for (size = ctopt(p->p_dsize); size > 0; size--)
		*(int*)&ul1pt[l1idx++] = *(int*)&Usrptmap[l2idx++] | PG_UW;
	low = (caddr_t)&ul1pt[l1idx];

	/*
	 * Now fill stack portion of UL1PT, top->down.
	 */

	l2idx = VFILL_MAXL2(p);
	l1idx = VFILL_MAXL1(p);
	for (size = ctopt(p->p_ssize+HIGHPAGES); size > 0; size--)
		*(int*)&ul1pt[--l1idx] = *(int*)&Usrptmap[--l2idx] | PG_UW;
	high = (caddr_t)&ul1pt[l1idx];

	/*
	 * Zero out unfilled portion of UL1PT.
	 */

	if (high > low)
		bzero(low, (u_int)(high-low));
}

/*
 * vrelpt()
 *	Release page tables and resident-set structure of process p.
 *
 * Releases both the memory pages and the pte's in uptmap.
 */

vrelpt(p)
	register struct proc *p;
{
	register long mapidx;

	/*
	 * If there are page-tables, delete them.
	 */

	if (p->p_szpt) {
		mapidx = btokmx(PTBASE(p));
		memfree(&Usrptmap[mapidx], p->p_szpt, 1);
		uptfree((long)p->p_szpt, mapidx);
	}
}

/*
 * vusize()
 *	Compute number of pages to be allocated to the u. area
 *	and data and stack area page tables, which are stored on the
 *	disk immediately after the u. area.
 */

vusize(p)
	register struct proc *p;
{
	return (clrnd(UPAGES + SZSWPT(p)));
}

/*
 * vgetu()
 *	Get u area for process p.
 *
 * If a old u area is given, then copy the new area from the old, else
 * swap in as specified in the proc structure.
 *
 * If successful, copies p_rssize into U-area fields and zaps p_rssize
 * to (finally) reflect reality -- the process has no pages in its Rset.
 */
#ifdef	i386
/*
 * Assumes process page-table already exists, so there's a place to put
 * the U-area memory.
 */
#endif	/* i386  */

vgetu(p, palloc, oldu)
	register struct proc *p;
	int (*palloc)();
	struct user *oldu;
{
	register long utab;			/* pte idx for U-area */
	register struct user *uarea;		/* virt address of the U-area */
	int	vmemall();			/* always succeed allocator */

	/*
	 * Allocate pte's in uptmap for the U-area.
	 * Note: must succeed if palloc == vmemall.
	 */

	utab = uptalloc((long)clrnd(UPAGES), (palloc == vmemall));
	if (utab == 0)
		return(0);

	/*
	 * Now allocate pages to hold the U-area.
	 * Then, set up kernel virtual address of the processes U-area.
	 */

	if ((*palloc)(&Usrptmap[utab], clrnd(UPAGES), p, CSYS) == 0) {
		uptfree((long)clrnd(UPAGES), utab);
		return (0);
	}

	uarea = p->p_uarea = (struct user *)kmxtob(utab);

	/*
	 * Establish kernel addressibility of the U-area pages.
	 */

	vmaccess(&Usrptmap[utab], (caddr_t)uarea, UPAGES);

#ifdef	ns32000
	p->p_upte = &Usrptmap[utab];
#endif	/* ns32000  */
#ifdef	i386
	/*
	 * Make addressible as kernel stack, too.
	 * Must not be readable by user, since don't clear
	 * it all during fork (see below).
	 */
#if	UPAGES != 1
	ERROR -- assumes UPAGES==1
#endif
	*(int *) UAREAPTES(p) = *(int *) &Usrptmap[utab];
#endif	/* i386  */

	/*
	 * New u.'s come from forking or inswap.
	 * If fork(), then no resident-set information yet.
	 */

	if (oldu) {
#ifndef	DEBUG
		u_int	stack_offset;
		/*
		 * Copy only relevant part of oldu: struct user and
		 * useful part of stack.
		 */
		*uarea = u;
		stack_offset = (caddr_t)&oldu - (caddr_t)&u;
		bcopy(	(caddr_t)oldu + stack_offset,
			(caddr_t)uarea + stack_offset,
			ctob(UPAGES) - stack_offset);
#else
		/*
		 * Copy entire U-area; this propogates stack overflow marking.
		 */
		ASSERT(oldu == &u, "vgetu: oldu");
		bcopy((caddr_t) &u, (caddr_t) uarea, ctob(UPAGES));
#endif	/* DEBUG  */
		uarea->u_procp = p;
		uarea->u_pffcount = 0;
		uarea->u_pffvtime = 0;
		uarea->u_mmapdel = 0;
	} else {
		swap(p, p->p_swaddr, &Usrptmap[utab], ctob(UPAGES),
						B_READ|B_UAREA, swapdev_vp);
#ifdef	DEBUG
		if (uarea->u_dsize != p->p_dsize
		||  uarea->u_ssize != p->p_ssize
		||  uarea->u_procp != p)
			panic("vgetu: swapin");
		if (swapdebug > 1)
			printf("<%d>: vgetu swapin from swap=%d\n",
				p-proc, p->p_swaddr);
#endif	/* DEBUG  */
	}

	/*
	 * Set up red-zone (un-writeable part) to assist debugging.
	 */

	setredzone(UAREAPTES(p), (caddr_t)0);

	return (1);
}

/*
 * vrelswu()
 *	Release swap space for a u. area.
 */

vrelswu(p)
	struct proc *p;
{
	swapfree((long)ctod(vusize(p)), p->p_swaddr);
	/* p->p_swaddr = 0; */	/* leave for post-mortems */
}

/*
 * vgetswu()
 *	Get swap space for a u. area.
 *
 * Could get fancy and try to (eg) free a tacky text, but have to be
 * VERY careful to avoid swapper deadlocks (see xflush(1)).  Also, this
 * routine is called at SPLHI indirectly by sched(), thus have to be careful
 * about spl nesting.  For now, fail if no instantaneous space.  This can
 * result in "anomoly" where process is killed due to lack of swap space,
 * but tacky texts consume some.  Solution: add more swap space.
 */

vgetswu(p)
	struct proc *p;
{
	return (p->p_swaddr = swapalloc((long) ctod(vusize(p))));
}

/*
 * vrelu()
 *	Release u. area, swapping it out if desired.
 *
 * "swapu" flag is used in error unwinding in swapin().
 */

vrelu(p, swapu)
	register struct proc *p;
{
	register long utab = (long)btokmx((struct pte *)p->p_uarea);

#ifdef	ns32000
	ASSERT_DEBUG(p->p_upte != NULL, "vrelu: null p_upte");
	ASSERT_DEBUG(p != u.u_procp, "vrelu: self");
#endif	/* ns32000  */

	if (swapu) {
#ifdef	DEBUG
		if (swapdebug > 1)
			printf("<%d>: vrelu to swap=%d\n", p-proc, p->p_swaddr);
#endif	/* DEBUG  */
		swap(p, p->p_swaddr, &Usrptmap[utab], ctob(UPAGES),
			B_WRITE|B_UAREA, swapdev_vp);
	}

	memfree(&Usrptmap[utab], clrnd(UPAGES), 1);
	uptfree((long)clrnd(UPAGES), utab);

#ifdef	ns32000
	p->p_upte = (struct pte *)0;
#endif	/* ns32000  */
	p->p_uarea = (struct user *)0;
}
