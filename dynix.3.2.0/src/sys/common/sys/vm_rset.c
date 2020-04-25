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
static	char	rcsid[] = "$Header: vm_rset.c 2.10 91/03/31 $";
#endif

/*
 * vm_rset.c
 *	Various resident-set manipulations.
 */

/* $Log:	vm_rset.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/mmu.h"
#include "../machine/mftpr.h"

/*
 * RSET_DELTA() gives actual change to Rset size given # pages growth/shrink
 * in process.  Based on tunable parameters (see vm_ctl()).
 */

#define	RSET_DELTA(d) \
	((((d) * vmtune.vt_RSexecmult) / vmtune.vt_RSexecdiv) / CLSIZE)

/*
 * vinitRS()
 *	Init Rset to given size.
 *
 * Called during exec to give process proper sized Rset.
 */

vinitRS(tds, ss)
	size_t	tds;			/* initial text+data size (HW pages) */
	size_t	ss;			/* initial stack size (HW pages) */
{
	ASSERT_DEBUG(u.u_procp->p_rssize == 0, "vinitRS: non-zero p_rssize");
	u.u_procp->p_rshand = 0;
	vsetRS((long) RSET_DELTA(tds + ss + vmtune.vt_RSexecslop));
}

/*
 * vexpandRS()
 *	Incrementally adjust Rset, but stay within Rset bounds.
 *
 * Called in `brk' syscall (obreak) to up Rset size in anticipation of
 * process really using the space it allocated.
 */

vexpandRS(delta)
	int	delta;				/* # HW pages */
{
	/*
	 * Set new size as a delta from current size.
	 */

	vsetRS(u.u_procp->p_rscurr + RSET_DELTA(delta));
}

/*
 * vsetRS()
 *	Set size of resident-set, coerced to within bounds.
 *
 * If shrink below current resident-set size, push out pages to get
 * p_rssize <= p_rscurr.
 */

vsetRS(rssize)
	register long	rssize;		/* # Rset entries */
{
	register struct	proc *p = u.u_procp;

	/*
	 * Coerce size into legit range.
	 */

	if (rssize > p->p_maxrss)
		rssize = p->p_maxrss;
	if (rssize < vmtune.vt_minRS)
		rssize = vmtune.vt_minRS;
	if (rssize > vmtune.vt_maxRS)
		rssize = vmtune.vt_maxRS;
	if (rssize > maxRS) {
		rssize = maxRS;
	}

	/*
	 * Set new size & push some pages out of current resident-set if
	 * now too big.
	 */

	p->p_rscurr = rssize;

	while (p->p_rssize > rssize)
		vallocRSslot(p);

	/*
	 * Zap PFF statistics (since have just "adjusted").
	 */

	u.u_pffcount = u.u_pffvtime = 0;
}

/*
 * vallocRSslot()
 *	Allocate a slot in process Rset.  Most uses via VALLOC_RS_SLOT().
 *
 * This is the page-replacement algorithm.  This is basically a "local-clock"
 * (eg, circular list of pages) replacement relying on reclaim to
 * approximate LRU page replacement.  Algorithm searches page-table
 * circularly to pick pages to replace (optionally using referenced-bits
 * to notice non-recently ref'd page).
 *
 * Use of page-table as data-structure to represent resident set avoids
 * overhead of resident-set "array" or other data-structure; not clear
 * FIFO replacement is that much better than circular, anyhow, given the
 * ability to reclaim from free or dirty list.  Bigest overhead is in
 * processes that have large virtual holes (other than between data and
 * stack); if this becomes a problem, could use other techinques to skip
 * (bit per Meg of populated addr space, string-op to skip PG_INVAL pte's,
 * run-length encode some finite # of holes, others?).
 *
 * Algorithm looks ahead up to `rset_ref_lookahead' clusters, zapping
 * reference-bits as clusters are looked at.  If find an un-referenced
 * cluster, use it and stop looking.  This trades away overhead now (not
 * zap ref bits for all `lookahead' pages) for a possible less-optimal
 * replacement choice later.  However, when don't find a non-ref'd, have
 * zapped ref bits in "lookahead" clusters, so next such fault recovers
 * the heuristic.  Current algorithm also simplifies interface in those
 * cases where don't really want to look-ahead (eg, vsetRS() shrink);
 * once each "lookahead" clusters we'll zap some more ref-bits.
 *
 * Alternatively could insist on looking ahead all of rset_ref_lookahead,
 * and remember 1st un-ref'd cluster found.  Would need argument to toggle
 * look-ahead at all since some situations don't really need it.
 *
 * Argument is u.u_procp *except* during fork (vmdup()).
 *
 * Assumes caller has exclusive access to argument process's page-tables
 * and Rset structures.
 *
 * Assumes p->p_rssize >= p->p_rscurr (ie, there is a need to replace a page),
 * and p->p_rssize > 0.
 *
 * NOTE: if rset_ref_lookahead > 1 doesn't give any/much performance
 * advantage, this can be simplified.
 */

int	rset_ref_lookahead = 1;		/* # clusters to look at for replace */
					/* must be >= 1 or algorithm breaks */

vallocRSslot(p)
	register struct proc *p;
{
	register struct	pte *pte;
	register u_int	v;
	register int count;
	register struct pte *pte0 = NULL;
	u_int	v0;

	/*
	 * Look thru next "rset_ref_lookahead" pages, zapping ref-bits
	 * for the future.  Start where previous search left off.
	 * Remember 1st replaceable cluster, and use 1st non-ref'd found.
	 * If find non-ref'd cluster, quit looking.
	 */

	v = p->p_rshand - CLSIZE;
	count = MIN(p->p_rssize, rset_ref_lookahead);
	ASSERT_DEBUG(count > 0, "vallocRSslot: count");

	do {
		/*
		 * Look thru page-table (round-robin), skip hole between
		 * data and stack, and wrap-around top of address space
		 * to bottom.  Looking for valid pageable cluster; if a
		 * valid page is mapped, must check if a paged map.  Note
		 * that "valid" ==> !pg_fod, so ok to check PTEMAPPED().
		 *
		 * Ideally could encode "non-paged" state in pte, and
		 * avoid looking at u_mmap[] entry while looking for
		 * replaceable page.  However, encoding is too tight on
		 * (eg) i386, if avoid use of Intel "reserved" pte bits.
		 *
		 * This code intimately understands the page-table
		 * representation.
		 */
		do {
			v += CLSIZE;
			if (v >= btop(USRSTACK)) {		/* wraparound */
				v = 0;
				pte = dvtopte(p, 0);
			} else if (isadsv(p, v))		/* data page */
				pte = dvtopte(p, v);
			else if (isassv(p, v))			/* stack page */
				pte = svtopte(p, v);
			else {					/* skip hole */
				v = sptov(p, p->p_ssize-1);
				pte = svtopte(p, v);
			}
		} while (!pte->pg_v ||
			 (PTEMAPPED(*pte) &&
			  !p->p_uarea->u_mmap[PTETOMAPX(*pte)].mm_paged));
		/*
		 * pte,v represent valid pageable cluster.  If it hasn't
		 * been referenced recently, use it.  Otherwise zap its
		 * ref-bits and continue.  Remember 1st cluster found.
		 */
		if (!anycl(pte, pg_ref))		/* !ref'd ==> done */
			break;
		if (pte0 == NULL) {			/* remember 1st */
			pte0 = pte;
			v0 = v;
		}
		zapcl(pte, PG_R);			/* zap ref'd bits */
	} while (--count > 0);

	/*
	 * If found an un-ref'd cluster, use it and set next starting
	 * point to 1st page found, or page after unref'd page (if it's 1st).
	 * Otherwise, use 1st found and next time start at one after this.
	 */

	if (count > 0) {
		if (pte0)
			p->p_rshand = v0;
		else
			p->p_rshand = v + CLSIZE;
	} else {
		pte = pte0;
		p->p_rshand = v0 + CLSIZE;
	}

	/*
	 * Release process's reference to this page (don't detach).
	 */

	if (PTEMAPPED(*pte))
		(void) vmmapfree(p, pte, 0);
	else
		memfree(pte, CLSIZE, 0);

	/*
	 * Insure TLB is consistent, since have invalidated one page,
	 * and possibly turned ref-bits off in other valid pte's.
	 *
	 * Flush entire TLB, since on 32032 with 2k pages individual
	 * virtual address invalidates are slow, and on 80386 there is
	 * no single-entry TLB flush.
	 */

	if (p == u.u_procp)
		FLUSH_USER_TLB(p->p_ptb1);

	/*
	 * Remember that Rset is now smaller, and bump u_pffcount since a
	 * page was removed.
	 */

	p->p_rssize--;					/* just took one */
	p->p_uarea->u_pffcount++;			/* count the remove */
}

/*
 * vpffintr()
 *	Software trap handler to check if running process needs to
 *	adjust size of its resident set.
 *
 * Scheduled in hardclock() when process virtual-time exceeds
 * vmtune.vt_PFFvtime.
 *
 * This is running in the same process that hardclock() examined (locore
 * insures this); thus is ok to p_sema().
 *
 * Since this is a SW trap handler, is only entered when about to return
 * to user-mode.  Thus, all kernel procedures that mess with Rsets can do
 * so mutex'd against this procedure.  Similarly, vpffintr() cannot be
 * reentered (in any given process).
 */

vpffintr()
{
	long	pff_rate;
	int	delta;

	/*
	 * Can race with process setting SNOPFF or doing a vsetRS()
	 * (clearing u_pffvtime) after hardclock() sets SW trap.
	 */

	if ((u.u_procp->p_flag & SNOPFF) || u.u_pffvtime == 0)
		return;

	/*
	 * Compute PFF rate (normalized to #faults/second) and
	 * adjust size of resident-set.
	 *
	 * Always call vsetRS() to insure Rset is within bounds
	 * and zap PFF statistics.
	 */

	pff_rate = (u.u_pffcount * hz) / u.u_pffvtime;

	if (pff_rate > vmtune.vt_PFFhigh)
		delta = vmtune.vt_PFFincr;
	else if (pff_rate < vmtune.vt_PFFlow)
		delta = -vmtune.vt_PFFdecr;
	else
		delta = 0;

	vsetRS(u.u_procp->p_rscurr + delta);
}
