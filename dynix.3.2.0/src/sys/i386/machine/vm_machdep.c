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
static	char	rcsid[] = "$Header: vm_machdep.c 2.18 1991/11/06 00:19:05 $";
#endif

/*
 * vm_machdep.c
 *	Various machine-dependent VM related routines.
 *
 * Note that there is still some machine dependency in ../sys/vm*.c
 */

/* $Log: vm_machdep.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/mutex.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/mmu.h"

/*
 * vpasspt()
 *	Pass the page tables of process p to process q.
 *
 * Used during vfork().  P and q are not symmetric;
 * p is the giver and q the receiver; after calling vpasspt
 * p will be ``cleaned out''.  Thus before vfork() we call vpasspt
 * with the child as q and give it our resources; after vfork() we
 * call vpasspt with the child as p to steal our resources back.
 *
 * Since U-area is mapped as part of process, must be careful how
 * page-tables are moved -- must preserve U-area maps.
 */

#define	Xp(a)	t = p->a; p->a = q->a; q->a = t;
#define	Xpp(a)	t = (int)(p->a); p->a = q->a; *(int*)(&q->a) = t;

vpasspt(p, q)
	register struct proc *p;
	register struct proc *q;
{
	register int t;
	struct	pte	Uptes[UPAGES];

	/*
	 * Get copy of target U-area mapping pte's, and replace target
	 * U-area map pte's with current (thus mapping running process
	 * Uarea in both page-tables).
	 *
	 * Must also insure new PT maps processor-local stuff, and that
	 * don't block until after running on switched page-tables.
	 */

#if	UPAGES != 1
	ERROR -- assumes UPAGES==1 (easy fix -- need loops)
#endif
	if (p == u.u_procp) {
		*(int *) &Uptes[0] = *(int *) UAREAPTES(q);
		*(int *) UAREAPTES(q) = *(int *) UAREAPTES(p);
		*(int *) (L1PT(q) + L1IDX(VA_PLOCAL)) = l.plocal_pte;
	} else {
		/*
		 * Taking resources back from child, who may still be
		 * executing.  Need to wait for child to sleep, *and*
		 * for child process-state to be unlocked, to insure child
		 * is really done executing (ie, all the way through resume()
		 * until off it's page-tables).  This is very implementation
		 * specific.  Ususally child is done before parent gets here.
		 *
		 * This relies on resume() not releasing process state lock
		 * until after switching to page-tables of new process.
		 */
		while (p->p_stat != SSLEEP)		/* wait for sleep */
			continue;
		while (p->p_state != L_UNLOCKED)	/* & really asleep! */
			continue;
		*(int *) &Uptes[0] = *(int *) UAREAPTES(p);
		*(int *) UAREAPTES(p) = *(int *) UAREAPTES(q);
		*(int *) (L1PT(p) + L1IDX(VA_PLOCAL)) = l.plocal_pte;
	}

	/*
	 * Swap proc structure fields.
	 * Don't bother with p_rshand; out-of-range values in p_rshand get
	 * fixed on 1st vallocRSslot().
	 */

	Xp(p_rssize);
	Xp(p_rscurr);
	Xp(p_szpt);
	Xp(p_ptb1);

	Xpp(p_ul2pt);
	Xpp(p_pttop);

	/*
	 * Still running on old PT, need to switch (and flush TLB).
	 * Note: need the double-mapped Uarea pte's here (from above).
	 */

	WRITE_PTROOT(u.u_procp->p_ptb1);

	/*
	 * Can put target Upte's back now.
	 */

	if (p == u.u_procp) {
		*(int *) UAREAPTES(q) = *(int *) &Uptes[0];
	} else {
		*(int *) UAREAPTES(p) = *(int *) &Uptes[0];
	}
}

/*
 * ptexpand()
 *	Expand/shrink a page table
 *
 * If shrinking, do "in-place" and free unused part.
 * If growing, allocate new and copy over.
 *
 * Caller has released any pages pointed to by parts of page table
 * that may be deleted or re-assigned.
 *
 * This procedure is sensitive to the ordering of UL1PT and UL2PT.
 *
 * Returns success or fail (failure is rare, only occurs if growing and
 * can't get swap-space for Uarea and page-table).
 */

ptexpand(ods, oss, isexec)
	size_t	ods;
	size_t	oss;
	int	isexec;
{
	register struct	proc *p;
	register int change;
	register int knew;
	int	kold;
	int	tdpages;

	p = u.u_procp;
	ASSERT(p->p_szpt > 0, "ptexpand: zero szpt");
        /*
         *+ When the kernel attempted to change the size of a
         *+ process's page table, it found the size to be 0.
         */

	/*
	 * Figure change in size of page-tables.  This uses updated
	 * p_dsize, p_ssize fields.  If no change in size,
	 * then nothing to do, except re-setup UL1PT (in case no
	 * page-table size change but now using a mapping page not
	 * previously used or switched mapping pages between data
	 * and stack).  This might be able to get optimized in expand()
	 * if it notices size change can't need new mapping page.
	 *
	 * If exec, assume no text+data for page-table positions (since
	 * current text is unknown relation to new size, and expand
	 * is fully filling out PT anyhow).
	 */

	change = SZPT(p) - p->p_szpt;
	if (change == 0) {
		if (isexec)
			vfill_ul1pt(p);
		return(1);
	}
	tdpages = isexec ? 0 : clrnd(ctopt(MIN(ods, p->p_dsize)));

	/*
	 * If shrinking arrange to release the unneeded page-table
	 * clusters.
	 */

	if (change < 0) {

		/*
		 * Can only free cluster aligned pages; thus, find
		 * cluster boundary above text+data pages and free the
		 * following clusters.  This cannot include pages
		 * containing stack pte's, since system doesn't map data
		 * and stack pte's in same cluster of level-2 pages.
		 */

		knew = btokmx(PTBASE(p)) + tdpages;
		memfree(&Usrptmap[knew], -change, 1);

		/*
		 * Copy pte's for remaining (undeleted) page-table pages
		 * down into vacated position && update proc fields.
		 * Then free the Usrptmap space we no longer need.
		 *
		 * Must hold memory locked when altering where pte's live,
		 * to avoid races with lmemall() doing realloc (yes, it
		 * *is* a low probability...)  Even lower probability
		 * is that lmemall() caller has stale TLB information
		 * for the new target(s) -- this one we ignore (law of
		 * sufficiently large numbers, and it can only happen
		 * in grow case, below).
		 */

		LOCK_MEM;

		kmcopy(knew, knew-change, p->p_szpt - (tdpages-change));

		p->p_szpt += change;
		p->p_pttop = PTTOP(p);
		p->p_ptb1 = PHYSUL1PT(p);

		UNLOCK_MEM;

		uptfree((long)-change, (long)(btokmx(PTBASE(p)) + p->p_szpt));

		/*
		 * Fill out UL1PT, as it has changed (position and contents).
		 * Flush TLB first, since mapping changed.
		 */

		FLUSH_TLB();
		vfill_ul1pt(p);
		return(1);
	}

	/*
	 * Page-tables increasing in size.
	 *
	 * Change is the number of new page table pages needed.
	 * Kold is the old index in the Usrptmap of the page tables.
	 * Allocate a new kernel map segment of size szpt+change for
	 * the page tables, and the new page table pages in the
	 * middle of this new region.
	 */

	kold = btokmx(PTBASE(p));

	if ((knew=uptalloc((long)(p->p_szpt+change), 0)) == 0)
		goto bad;
	if (memall(&Usrptmap[knew+tdpages], change, p, CSYS) == 0) {
		uptfree((long)(p->p_szpt+change), (long)knew);
		goto bad;
	}

	/*
	 * Tdpages of text+data mapping page ptes go over in place.
	 * Then, the stack mapping page pte's are copied leaving
	 * the new pages in the middle.
	 */

	kmcopy(knew, kold, tdpages);
	kmcopy(knew+tdpages+change, kold+tdpages, p->p_szpt-tdpages);

	/*
	 * Validate and clear the newly allocated page table pages in the
	 * center of the new region of the Usrptmap.  If exec, no need to
	 * zap, since expand fully fills them out.
	 */

	vmaccess(&Usrptmap[knew + tdpages], (caddr_t)kmxtob(knew+tdpages), change);
	if (!isexec)
		bzero((caddr_t)kmxtob(knew+tdpages), (u_int)(change*NBPG));

	/*
	 * Free old Usrptmap space and update proc fields.
	 */

	LOCK_MEM;				/* same reason as above */

	p->p_szpt += change;
	p->p_ul2pt = UL2PT(knew);
	p->p_pttop = PTTOP(p);
	p->p_ptb1 = PHYSUL1PT(p);

	UNLOCK_MEM;

	uptfree((long)p->p_szpt - change, (long)kold);

	/*
	 * Re-fill-out UL1PT and done.
	 */

	vfill_ul1pt(p);
	return(1);

bad:
	/*
	 * Swap out the process so that the unavailable 
	 * resource will be allocated upon swapin.
	 *
	 * If can't get swap space for U-area and page-tables, return
	 * failure (should be very rare).
	 */
	return(swapout(u.u_procp, ods, oss));
}

/*
 * kmcopy()
 *	Copy pte's from one place in Usrptmap to another.
 *
 * Doesn't flush TLB -- caller must do this as appropriate.
 */

static
kmcopy(to, from, count)
	int to;
	int from;
	register int count;
{
	register struct pte *tp = &Usrptmap[to];
	register struct pte *fp = &Usrptmap[from];

	for (; count != 0; count--)
		*tp++ = *fp++;
}

/*
 * setredzone()
 *	Set a red zone in the kernel stack after the u. area.
 */

/*ARGSUSED*/
setredzone(pte, vaddr)
	struct pte *pte;
	caddr_t vaddr;
{
	/* NOP on 80386 */
}

/*
 * chksize()
 *	Check for valid program size
 *
 * Returns 0 for success, else error number.
 */

chksize(tds, ss)
	register unsigned tds, ss;
{
	/*
	 * Check sizes and that can build a page-table for it.
	 */

	if (tds > MAXDSIZ || ss > MAXSSIZ || SZL2PT(tds,ss) > MAXSZPT)
		return (ENOMEM);

	/*
	 * Check for swap map overflow.
	 */

	if (ctod(tds) > maxdmap || ctod(ss) > maxdmap)
		return (ENOMEM);

	return (0);
}

#ifdef	notdef			/* now a macro in machine/mftpr.h */
/*
 * newptes()
 *	Flush TLB for range of kernel addresses.
 *
 * 80386 can't flush individual TLB entry -- thus flush entire TLB.
 */

/*ARGSUSED*/
newptes(vaddr, size)
	caddr_t	vaddr;			/* virt address */
	long	size;			/* # bytes */
{
	FLUSH_TLB();
}
#endif	notdef

/*
 * pagemove()
 *	Move pages from one kernel virtual address to another.
 *
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 *
 * Called *only* in buffer management.
 * Caller handles invalidate of TLB(s).
 */

pagemove(from, to, size)
	caddr_t	from;
	caddr_t	to;
	int	size;
{
	register struct pte *fpte;
	register struct pte *tpte;

	fpte = &Sysmap[btop(from)];
	tpte = &Sysmap[btop(to)];
	while (size > 0) {
		*tpte++ = *fpte;
		*(int *)fpte++ = PG_INVAL;
		size -= NBPG;
	}
}
