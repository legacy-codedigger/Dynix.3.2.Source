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
static	char	rcsid[] = "$Header: vm_proc.c 2.24 1992/02/13 00:29:11 $";
#endif

/*
 * vm_proc.c
 *	Process related VM routines.
 */

/* $Log: vm_proc.c,v $
 *
 *
 *
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/mutex.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/cmap.h"
#include "../h/vm.h"
#include "../h/seg.h"

#include "../machine/mftpr.h"
#include "../machine/plocal.h"

#ifdef	ns32000					/* ns32000 specific chip-bug */
#if	defined(SLOW_US) || defined(MFG)
#include "../machine/hwparam.h"
#endif	defined(SLOW_US) || defined(MFG)
#endif	ns32000

/*
 * vgetvm()
 *	Get virtual memory resources for a new process.
 *
 * Called during exec to re-size page-tables for new process image.
 * expand()/ptexpand() takes care of freeing excess page-table.
 *
 * Returns 0 for success, else error #.
 */

vgetvm(tds, ss)
	size_t	tds;				/* text + data size */
	size_t	ss;				/* stack size */
{
	struct	proc	*p = u.u_procp;
	int		error = 0;

	p->p_dsize = p->p_ssize = 0;
	u.u_dsize = u.u_ssize = 0;

	if (!expand((int)tds, (int)ss, 1, PG_ZFOD))
		error = ENOMEM;

	return(error);
}

/*
 * vrelvm()
 *	Release the virtual memory resources (memory
 *	pages, and swap area) associated with the current process.
 *
 * Caller must not be swappable.  Used at exit or execl.
 */

vrelvm()
{
	register struct proc *p = u.u_procp;

	p->p_rssize -= vmemfree(dptopte(p, 0), (int)p->p_dsize);
	p->p_rssize -= vmemfree(sptopte(p, p->p_ssize-1), (int)p->p_ssize);
	ASSERT(p->p_rssize == 0, "vrelvm: non-zero rss");
	/*
	 *+ All virtual memory resources of a process were freed,
	 *+ but it had a nonzero resident set size.
	 */

	/*
	 * Release swap space.  Any pageouts in progress are covered by
	 * FIFO writes of overlapped sectors.
	 */

	vsrele(&u.u_dmap);
	vsrele(&u.u_smap);
	p->p_dsize = 0;
	p->p_ssize = 0;
	u.u_dsize = 0;
	u.u_ssize = 0;
}

/*
 * vpassvm()
 *	Pass virtual memory resources from p to q.
 *
 * Called in newproc() when starting/ending a vfork().
 *
 * Caller holds mem_alloc sema, to avoid races in lmemall() and pageout()
 * trying to locate ptes for pages.
 */

#ifdef	DEBUG
int	vf_cp_mmap_back;	/* # times copied u_mmap[] back to vfork parent */
#endif	DEBUG

vpassvm(p, q)
	register struct proc *p, *q;
{
	register struct user *up = p->p_uarea;
	register struct user *uq = q->p_uarea;

	/*
	 * Pass fields related to vm sizes.
	 */

	uq->u_dsize = q->p_dsize = p->p_dsize; up->u_dsize = p->p_dsize = 0;
	uq->u_ssize = q->p_ssize = p->p_ssize; up->u_ssize = p->p_ssize = 0;

	/*
	 * Pass paging statistics.
	 */

	uq->u_pffvtime = up->u_pffvtime; up->u_pffvtime = 0;
	uq->u_pffcount = up->u_pffcount; up->u_pffcount = 0;

	/*
	 * Pass swap space maps.  Don't bother zapping up's maps, the process
	 * loosing the maps won't use them again.
	 */

	uq->u_dmap = up->u_dmap;
	uq->u_smap = up->u_smap;

	/*
	 * Pass u. paging statistics.
	 */

	uq->u_ru = up->u_ru;
	bzero((caddr_t)&up->u_ru, sizeof (struct rusage));
	uq->u_cru = up->u_cru;
	bzero((caddr_t)&up->u_cru, sizeof (struct rusage));

	/*
	 * If parent getting VM back and child has deleted some
	 * mapped pages, get u_mmap[] array back from child.
	 */

	if (up->u_mmapdel && q == u.u_procp) {
		bcopy((caddr_t) up->u_mmap, (caddr_t) uq->u_mmap, sizeof(u.u_mmap));
		uq->u_pmapcnt = up->u_pmapcnt;
		uq->u_mmapdel = 1;
		ASSERT_DEBUG(uq->u_mmapmax==up->u_mmapmax,"vpassvm: u_mmapmax");
#ifdef	DEBUG
		++vf_cp_mmap_back;
#endif	DEBUG
	}

	/*
	 * And finally, pass the page tables themselves.
	 * On return we are running on the other set of
	 * page tables, but still with the same u. area.
	 */

	vpasspt(p, q);
}

/*
 * expand()
 *	Change the size of the data+stack regions of the process.
 *
 * If the size is shrinking, release virtual memory.
 * Then let ptexpand() adjust the page-tables (shrink or grow as needed).
 * If it's growing, initialize new page table entries with fill_pte
 * (either zero-fill on demand, or completely invalid).
 *
 * Handles case of failure of ptexpand() to grow page-table, returning
 * failure if this occurs.
 */

expand(dchange, schange, isexec, fill_pte)
	register int	dchange;		/* delta to data size */
	register int	schange;		/* delta to stack size */
	int		isexec;			/* call is due to exec() */
	int		fill_pte;		/* PG_ZFOD | PG_INVAL */
{
	register struct proc *p;
	size_t ods, oss;

	p = u.u_procp;
	if (dchange == 0 && schange == 0)
		return(1);

	ASSERT_DEBUG((dchange % CLSIZE) == 0 && (schange % CLSIZE) == 0,
					"expand: bad change");

	/*
	 * Update the sizes to reflect the change.  Note that we may
	 * swap as a result of a ptexpand, but this will work, because
	 * the routines which swap out will get the current
	 * sizes from the arguments they are passed, and when the process
	 * resumes the lengths in the proc structure are used to 
	 * build the new page tables.
	 *
	 * If shrinking, release the now unwanted pages; careful to
	 * decrement "segment" size after releasing pages since process
	 * logically has this space until done.
	 *
	 * Note: if isexec, then dchange >= 0 and schange >= 0.
	 */

	ods = u.u_dsize;
	oss = u.u_ssize;

	if (dchange < 0)
		p->p_rssize -= vmemfree(dptopte(p,p->p_dsize+dchange),-dchange);

	p->p_dsize += dchange;
	u.u_dsize += dchange;

	if (schange < 0)
		p->p_rssize -= vmemfree(sptopte(p, p->p_ssize-1), -schange);

	p->p_ssize += schange;
	u.u_ssize += schange;

	/*
	 * Adjust the page-table size if necessary.
	 * If fail, unwind and return.
	 */

	if (!ptexpand(ods, oss, isexec)) {
		p->p_dsize = u.u_dsize = ods;
		p->p_ssize = u.u_ssize = oss;
		return(0);
	}

	/*
	 * If either segment shrunk (sstk(),brk(), but unusual)
	 * or both grew (exec(), not so unusual), then must zap pte's
	 * between last data pte and last stack pte, since they may
	 * indicate fod or R/W permissions.
	 */

	if (isexec || dchange < 0 || schange < 0) {
		struct pte *first = dptopte(p, p->p_dsize);
		ptefill(first, PG_INVAL, (size_t)(sptopte(p, p->p_ssize-1) - first));
#ifdef	i386
		/*
		 * This is only case where must flush TLB on 386; processor
		 * doesn't cache invalid pte's.
		 */
		FLUSH_USER_TLB(p->p_ptb1);
#endif	i386
	}

	/*
	 * If growing, initialize new pte's.
	 * If exec, don't bother filling data pte's -- caller will fill these.
	 * Stack expansions always grow with PG_ZFOD.
	 */

	if (dchange > 0 && !isexec) {
		ptefill(dptopte(p, p->p_dsize-dchange), fill_pte, (size_t)dchange);
		if (fill_pte == PG_ZFOD)
			l.cnt.v_nzfod += dchange;
	}

	if (schange > 0) {
		ptefill(sptopte(p, p->p_ssize-1), fill_pte, (size_t)schange);
		l.cnt.v_nzfod += schange;
	}

#ifdef	ns32000
	/*
	 * We changed mapping for the current process,
	 * so must update the hardware translation.
	 * Unclear if this can be dropped on 32000, and do the flush
	 * above (processor may cache invalid pte's).
	 */
	FLUSH_USER_TLB(p->p_ptb1);
#endif	ns32000

#ifdef	DEBUG
	/*
	 * Insure space between end of data pte's and "start" of stack ptes
	 * is null.
	 */
	{	register struct pte *pte, *lim;
		lim = sptopte(p, p->p_ssize-1);
		for (pte = dptopte(p, p->p_dsize); pte < lim; pte++) {
			ASSERT(*(int*)pte == PG_INVAL, "expand: non-zero hole");
		}
	}
#endif	DEBUG

	return(1);
}

/*
 * procdup()
 *	Create a duplicate copy of the current process
 *	in process slot p, which has been partially initialized
 *	by newproc().
 *
 * Could deadlock here if two large proc's get page tables
 * and then each gets part of his UPAGES if they then have
 * consumed all the available memory.  This can only happen when
 * the set of processes doing fork's & vfork's overcommit memory
 * but don't allow themselves to be swapped.
 *
 * This is avoided by limiting the number of concurrent forks based
 * in their memory needs.  See kern_fork.c (fork(), vfork()).
 */

procdup(p, isvfork)
	register struct proc *p;
{
	register struct user *uarea;		/* -> p's U-area */

	/*
	 * Allocate page tables for new process, waiting
	 * for memory to be free.  Passing vmemall to vgetpt()
	 * insures we don't return until the mess is allocated.
	 */

	(void) vgetpt(p, vmemall);

	/*
	 * Get a u. for the new process, a copy of our u.
	 *
	 * Save floating point registers in u first if necessary.
	 * This should really go into some machine dependent module...
	 */
#ifdef i386
	if (l.usingfpu)
		save_fpu_fork();
#else
	if (l.usingfpu)
#ifdef	FPU_SIGNAL_BUG
		save_fpu(&u.u_fpusave);
#else
		save_fpu();
#endif
#endif i386

	(void) vgetu(p, vmemall, &u);
	uarea = p->p_uarea;

	/*
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from setjmp.
	 *
	 * This is done by "pushing" a resume context onto the child
	 * stack that causes it to call longjmp() with the passed argument
	 * (u_ssave) when it is first dispatched.  This avoids tests
	 * of process-state bits during context switch.
	 */

	if (setjmp(&uarea->u_ssave)) {
		/*
		 * Return 1 in child.
		 */
		return (1);
	}

#ifdef	ns32000
	mfpr(sp, *(int*)&uarea->u_sp);		/* give him a valid SP */
#endif	ns32000
	pushlongjmp(uarea, &u.u_ssave);		/* "push" a call to longjmp */

	/*
	 * If the new process is being created in vfork(), then
	 * exchange vm resources with it.  We end up with just a
	 * u. area and an empty text, data, and stack region.
	 *
	 * Caller (newproc) does the vpassvm() call when time is right.
	 */

	if (isvfork) {
		/*
		 * Return 0 in parent.
		 */
		return (0);
	}

	/*
	 * A real fork; clear vm statistics of new process.
	 * Equivalent things happen during vfork() in vpassvm().
	 */

	bzero((caddr_t)&uarea->u_ru, sizeof (struct rusage));
	bzero((caddr_t)&uarea->u_syst, sizeof (struct timeval));
	bzero((caddr_t)&uarea->u_cru, sizeof (struct rusage));
	uarea->u_dmap = u.u_cdmap;
	uarea->u_smap = u.u_csmap;
	uarea->u_pffcount = uarea->u_pffvtime = 0;

	/*
	 * Duplicate data and stack space of current process
	 * in the new process.
	 */

	vmdup(p, dptopte(p, 0), dptov(p, 0), p->p_dsize, CDATA);
#ifdef	ns32000					/* ns32000 specific chip-bug */
#if	defined(SLOW_US) || defined(MFG)
	/*
	 * If not slow_us_on make sure last page of stack is ZFOD only.
	 */
	{
		extern bool_t slow_us_on;

	if (!slow_us_on && (*(int *)sptopte(u.u_procp, 0) == (PHYSTOPTE(ICUADDR)|PG_V|PG_URKW)))
		*(int *)sptopte(u.u_procp, 0) = PG_ZFOD;
	}
#endif	defined(SLOW_US) || defined(MFG)
#endif	ns32000
	vmdup(p, sptopte(p, p->p_ssize-1), sptov(p, p->p_ssize-1), p->p_ssize, CSTACK);
	/*
	 * Return 0 in parent.
	 */

	return (0);
}

/*
 * vmdup()
 *	Duplicate a portion of current process in another.
 *
 * No bother setting SNOPFF: caller will get a flurry of faults, but averaged
 * over time is small effect.  Target process isn't running and is not
 * incurring faults.
 *
 * Caller insures exclusive access to target pte's.
 */

static
vmdup(p, pte, v, count, type)
	register struct proc *p;
	register struct pte *pte;
	register unsigned v;
	register size_t	count;
	int	type;
{
	register struct pte *opte;

	opte = vtopte(u.u_procp, v);

	for (; count != 0; count -= CLSIZE, v += CLSIZE, pte += CLSIZE, opte += CLSIZE) {
		/*
		 * FOD or totally invalid (hole from mmap()) ==> copy pte's.
		 */
		if (opte->pg_fod || *(int*)opte == PG_INVAL) {
			copycl(pte, opte);
		}
		/*
		 * Else if mapped: copy if phys mapped, else copy invalid ref.
		 */
		else if (PTEMAPPED(*opte)) {
			copycl(pte, opte);
			if (u.u_mmap[PTETOMAPX(*opte)].mm_paged)
				zapcl(pte, PG_PFNUM|PG_V);
		}
		/*
		 * Else must copy the page to the child.
		 *
		 * Could re-create FOD pte if parent has reclaim link
		 * to c_holdfod page.
		 *
		 * Validate pte but don't set ref-bit.  If process larger
		 * than resident set, this makes vallocRSslot() faster.
		 */
		else {
			VALLOC_RS_SLOT(p);
			(void) vmemall(pte, CLSIZE, p, type);
			p->p_rssize++;
			copyseg(v, PTETOPHYS(*pte));
			*(int *) pte |= (*(int *)opte & PG_PROT) | PG_V | PG_M;
			distcl(pte);
		}
	}

	/*
	 * NB: don't set u_ru.ru_maxrss (should track p_rssize), since
	 * 1st fault will set this in pagein().
	 */
}
