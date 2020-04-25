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
static	char	rcsid[] = "$Header: vm_swap.c 2.17 91/01/03 $";
#endif

/*
 * vm_swap.c
 *	Manage swapout's and swapin's.
 */

/* $Log:	vm_swap.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/cmap.h"
#include "../h/vm.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"

/*
 * swapout()
 *	Arrange for a process to be swapped out.
 *
 * Queues process and gooses pageout process to get the swap started.
 * Pageout process does all the work, to provide one source of pageout
 * operations.
 *
 * Self swap (ptexpand) is coordinated by holding proc state locked
 * until switch away.
 *
 * ods and oss arguments are the old data size and the stack size
 * of the process, supplied during page table expansion swaps.
 *
 * If can't alloc swap space (for U-area and page-tables), swkill the process
 * and return failure (this should be rare); else return success (true).
 *
 * Caller is `p' or assures `p' can't run.
 *
 * `p' has locked state if not self, is not locked if is self.
 *
 * Returns 1 for success, 0 for swap-space failure (page-table), -1 for
 * process can't be preempted right now (but did set SFSWAP).  -1 is only
 * possible in non-self-swaps.
 *
 *** Issue: ods, oss really necessary?
 */

swapout(p, ods, oss)
	register struct proc *p;
	size_t	ods;
	size_t	oss;
{
	int	noswap;
	spl_t	s;
	static	char nospace[] = "no swap space for page table";

	/*
	 * If not self, hold locked proc-state.  Must insure validate
	 * proc is still swappable (can race with proc selection), and
	 * insure can swap the mother (can't be running), then turn off
	 * SLOAD to sync with v_sema/etc.
	 * Else (is self), will lock state later.
	 *
	 * In either case, must be able to allocate swap-space for
	 * U-area and page-table.
	 */
	if (p != u.u_procp) {
		switch(p->p_stat) {
		case SSLEEP:
		case SSTOP:
			/*
			 * Must check p_noswap in case raced with process
			 * selection.  If set, process (typically) won't sleep
			 * very long.
			 */
			if (p->p_noswap) {
				p->p_flag |= SFSWAP;		/* force swap */
				v_lock(&p->p_state, SPL0);
				return(-1);
			}
			break;


		case SRUN:
		case SONPROC:
			/*
			 * Must grab G_RUNQ so can determine actual
			 * state of process.  Must also check p_noswap
			 * since we may have raced with proc setting this.
			 */
			VOID_P_GATE(G_RUNQ);
			if (p->p_stat == SONPROC || p->p_noswap) {
				/*
				 * Can't swap-out a running proc.
				 */
				p->p_flag |= SFSWAP;		/* force swap */
				if (p->p_stat == SONPROC)
					nudge(PSWP, &engine[p->p_engno]);
				V_GATE(G_RUNQ, SPLHI);
				v_lock(&p->p_state, SPL0);
				return(-1);			/* fail */
			}
			remrq(p);
			V_GATE(G_RUNQ, SPLHI);
			break;

		default:
			/*
			 * Lost race (rare).  Ignore process.
			 */
			v_lock(&p->p_state, SPL0);
			return(0);
		}
		if (vgetswu(p) == 0) {
			if (p->p_stat == SRUN) {
				VOID_P_GATE(G_RUNQ);
				setrq(p);
				V_GATE(G_RUNQ, SPLHI);
			}
			v_lock(&p->p_state, SPL0);
			swkill(p, nospace);
			return(0);				/* failed */
		}
		ASSERT(p->p_noswap == 0, "swapout: p_noswap");
		/*
		 *+ The kernel attempted to swap out a process that had marked
		 *+ itself as temporarily being nonswappable.
		 */
		p->p_noswap = 1;				/* block swapin */
		p->p_flag |= SWPSYNC;				/* sync when done */
		p->p_flag &= ~(SLOAD|SFSWAP);			/* logically out */
		v_lock(&p->p_state, SPL0);
	} else {
		/*
		 * Self swap.
		 */
		++p->p_noswap;
		if (vgetswu(p) == 0) {
			swkill(p, nospace);
			return(0);				/* failed */
		}
	}

	p->p_uarea->u_odsize = ods;
	p->p_uarea->u_ossize = oss;

	/*
	 * If self-swap: save/restore p_noswap (p_noswap coordinates
	 * with swapper), no swap-synch (nobody waiting); also, lock
	 * and keep locked proc-state to synch with pageout.
	 * Did non-self swap fuss above.
	 */

	if (p == u.u_procp) {					/* self swap */
		(void) p_lock(&p->p_state, SPLHI);
		p->p_flag &= ~(SWPSYNC|SLOAD|SFSWAP);
		noswap = p->p_noswap - 1;
		p->p_noswap = 1;
		/*
		 * Must not block now until switch away.
		 */
	}

	/*
	 * Enqueue process to be swapped out (FIFO).
	 */

	s = p_lock(&swpq_mutex, SPLHI);

	if (swpq_tail)
		proc[swpq_tail].p_swpq = (p-proc);
	swpq_tail = (p-proc);
	if (swpq_head == 0) {
		swpq_head = swpq_tail;
	}
	p->p_swpq = 0;

	v_lock(&swpq_mutex, s);

	v_sema(&drain_dirty);				/* goose pageout */

	/*
	 * If self swap, switch away (and drop self-state lock);
	 * when come back we've been swapped out and back in again.
	 */

	if (p == u.u_procp) {
		p->p_stat = SRUN;			/* about to loose it */
		swtch(p);
		p->p_noswap = noswap;
	}

	return(1);					/* success */
}

/*
 * swapout_proc() 
 *	Insure process p is all on swap space.
 *
 * Called by pageout process to force out all data+stack pages of
 * a process to its swap space, then write page-tables and U-area.
 *
 * Caller arranged that swap-space exists for U-area and page-tables,
 * and that process cannot execute.
 */

swapout_proc(p)
	register struct proc *p;
{
	extern size_t vsswap();
	register struct user *utl;
	size_t	mem_used;
	int	flag;
	spl_t	s;

	ASSERT_DEBUG((p->p_flag & SLOAD) == 0, "swapout_proc: SLOAD");
	ASSERT_DEBUG(p->p_swaddr != 0, "swapout_proc: p_swaddr");

	flush_tlb();			/* paranoia; mapping may have changed */

	utl = p->p_uarea;
	utl->u_ru.ru_nswap++;

	/*
	 * Swap out all its data+stack, anything necessary for dmap's,
	 * and references to paged mapped things if process is not a
	 * vfork parent.  u_mmapmax is "u." relative so need to be careful
	 * to get "utl" relative value.
	 *
	 * Also figure the memory "used" by this process; heuristic is
	 * sum of private pages actually in resident-set, and for each
	 * mapped object, 1/N * # pages of that map in resident-set
	 * (where N == # sharing processes).  This number is stored in
	 * p_rssize while the process is swapped out, and used by the
	 * sched() to estimate size when bringing it back in.
	 */

	if ((p->p_flag & SNOVM) == 0) {
		register struct mmap *um;
		struct	mmap	*ummax;
		struct	map_stat map_stat;
		size_t	map_used[NUMMAP];
		int	i;

		bzero((caddr_t) map_used, sizeof(map_used));
		mem_used = vsswap(p, dptopte(p, 0), CDATA, 0,
					utl->u_odsize, &utl->u_dmap, map_used);
		mem_used += vsswap(p, sptopte(p, CLSIZE-1), CSTACK, 0,
					utl->u_ossize, &utl->u_smap, map_used);
		vsswapout(&utl->u_dmap);
		vsswapout(&utl->u_smap);
		ummax = &utl->u_mmap[utl->u_mmapmax - u.u_mmap];
		for (i = 0, um = utl->u_mmap; um < ummax; um++, i++) {
			if (um->mm_pgcnt != 0 && um->mm_paged) {
				(*um->mm_ops->map_stat)
					(um->mm_handle, &map_stat);
				(*um->mm_ops->map_swpout)
					(um->mm_handle, um->mm_off,um->mm_size);
				mem_used += map_used[i] / map_stat.ms_count;
			}
		}
	} else
		mem_used = 0;

	ASSERT(p->p_rssize == 0, "swapout_proc: p_rssize");
	/*
	 *+ The swapper just completed swapping out a process and found
	 *+ its resident set size to be nonzero.
	 */

	/*
	 * Swap out page-tables, U-area, and open-file table extension.
	 */

	swdspt(p, utl, B_WRITE);

	/*
	 * Current implementation doesn't swap open-file table objects.
	 */

	vrelu(p, 1);

	/*
	 * Almost done: release page-tables, Rset; do stat's, and goose
	 * swapper to try and bring it back in (how fickle!).
	 */

	vrelpt(p);
	p->p_rssize = mem_used;			/* remember for swap-in */

	s = p_lock(&p->p_state, SPLHI);
	p->p_noswap = 0;
	p->p_time = 0;				/* need locked state to zap */
	flag = p->p_flag;
	p->p_flag &= ~SWPSYNC;
	v_lock(&p->p_state, s);

	if (flag & SWPSYNC)
		v_sema(&swapout_sync);		/* tell 'em swapout is done */
	else {
		/*
		 * Was a self-swap; tell swapper to swap in
		 */
		v_sema(&runout);
	}

	l.multprog--;
	l.cnt.v_swpout++;
}

/*
 * swapin()
 *	Try to swap a process in.
 *
 * Returns success indication; will fail if not enuf memory for any of this.
 */

swapin(p)
	register struct proc *p;
{
	register struct user *ua;
	register struct mmap *um;
	struct	mmap	*ummax;
	spl_t	s;

	/*
	 * Must allocate page-table, Rset, and space for U-area.
	 * If any fail, unwind and return 'nope'.
	 *
	 * vgetpt() bzero's the PT (==> clean even if swapin() expanded).
	 */

	p->p_szpt = SZPT(p);
	if (vgetpt(p, memall) == 0)
		goto err0;

	if (vgetu(p, memall, (struct user *)0) == 0)
		goto err1;

	ua = p->p_uarea;

	/*
	 * If process had an open-file-table extension, try to bring it in.
	 */

	/*
	 * Current implementation doesn't swap open-file table objects.
	 */

	/*
	 * If process is not a vfork parent, try to swap in any swap-space
	 * representation and paged maps it has.  u_mmapmax is "u." relative
	 * so need to be careful to get "ua" relative value.
	 */

	if ((p->p_flag & SNOVM) == 0) {
		if (!vsswapin(&ua->u_dmap))
			goto err3;
		if (!vsswapin(&ua->u_smap))
			goto err4;
		ummax = &ua->u_mmap[ua->u_mmapmax - u.u_mmap];
		for (um = ua->u_mmap; um < ummax; um++) {
			if (um->mm_pgcnt != 0
			&&  um->mm_paged
			&&  (*um->mm_ops->map_swpin)(um->mm_handle, um->mm_off, um->mm_size) != 0) {
				for (um--; um >= ua->u_mmap; um--) {
					if (um->mm_pgcnt != 0 && um->mm_paged) {
						(*um->mm_ops->map_swpout)
							(um->mm_handle,
							um->mm_off,
							um->mm_size);
					}
				}
				goto err5;
			}
		}
	}

	/*
	 * Got the memory resources; bring the page-table back,
	 * and release U-area + PT swap space.
	 */

	swdspt(p, ua, B_READ);
	vrelswu(p);

#ifdef	i386
	/*
	 * vgetu() set up addressibility of U-area, but swdspt()
	 * clobbers this.  Changing swdspt() to use short count causes
	 * page-table IO's to not be a multiple of NBPG.
	 * Thus, re-establish U-area mapping.
	 */
#if	UPAGES != 1
	ERROR -- assumes UPAGES==1
#endif
	*(int *) UAREAPTES(p) = *(int *) &Usrptmap[btokmx((struct pte *)p->p_uarea)];
#endif	i386

	/*
	 * If runnable (can't be SONPROC), put on runQ.
	 */

	p->p_rssize = 0;			/* resident-set is empty */

	s = p_lock(&p->p_state, SPLHI);
	p->p_flag |= SLOAD;
	p->p_time = 0;
	if (p->p_stat == SRUN) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
	}
	v_lock(&p->p_state, s);

	l.multprog++;
	l.cnt.v_swpin++;

	return (1);

	/*
	 * Error unwinds gathered here.  It's ugly but it works.
	 */

err5:	vsswapout(&ua->u_smap);
err4:	vsswapout(&ua->u_dmap);
err3:	vrelu(p, 0);				/* don't write */
err1:	vrelpt(p);
err0:	return(0);
}

/*
 * swdspt()
 *	Swap the data and stack page tables in or out.
 *
 * Only hard thing is swapping out when new pt size is different than old.
 * If we are growing new pt pages, then we must spread pages with 2 swaps.
 *
 * There is no shrinking of page-tables here, since ptexpand() takes care
 * of that.
 */

swdspt(p, utl, rdwri)
	register struct proc *p;
	register struct user *utl;
	int	rdwri;
{
	int	szswpt;
	int	ssz;

	szswpt = SZSWPT(p);

	/*
	 * If swap out (or in) and increasing size of page-table, do two IO's.
	 *
	 * Note: two IO's on swapin assures new page-table space (allocated
	 * by vgetpt()) is zero.  This can be done better.
	 */

	if (utl->u_odsize == p->p_dsize && utl->u_ossize == p->p_ssize) {
		swpt(rdwri, p, 0, dptopte(p,0), (size_t)(szswpt * NBPG));
	} else {
		ASSERT(SZPT(p) >= p->p_szpt, "swdspt: shrink");
		/*
		 *+ When the kernel was swapping out a process's data and 
		 *+ stack page tables, it appeared that the process was 
		 *+ attempting to shrink its page table size. 
		 */
		if (utl->u_ossize) {
			ssz = ctopt(utl->u_ossize);
			swpt(rdwri, p, szswpt - ssz,
			sptopte(p, utl->u_ossize-1), (size_t)(ssz * NBPG));
		}
		if (utl->u_odsize)
			swpt(rdwri, p, 0, dptopte(p,0), ctopt(utl->u_odsize)*NBPG);
	}
#ifdef	DEBUG
	{	register int i;
		register struct pte *pte;

		for (i = 0; i < utl->u_odsize; i += CLSIZE) {
			pte = dptopte(p, i);
			if (pte->pg_v) {
				if (!pte->pg_fod
				&&  PTEMAPPED(*pte)
				&&  !utl->u_mmap[PTETOMAPX(*pte)].mm_paged)
					continue;
			} else if (pte->pg_fod)
				continue;
			else if (PTEPF(*pte) == 0)
				continue;
			printf("pte=%x\n", pte);
			panic("swdspt1");
		}
		for (i = utl->u_ossize-1; i > 0; i -= CLSIZE) {
			pte = sptopte(p, i);
			if (pte->pg_v) {
				if (!pte->pg_fod
				&&  PTEMAPPED(*pte)
				&&  !utl->u_mmap[PTETOMAPX(*pte)].mm_paged)
					continue;
			} else if (pte->pg_fod)
				continue;
			else if (PTEPF(*pte) == 0)
				continue;
			printf("pte=%x\n", pte);
			panic("swdspt2");
		}
		if (utl->u_odsize != 0) {
			/*
			 * Some daemons (eg, NFS biod) do a vrelvm() but don't
			 * really zap pte's.  These set p_dsize=p_ssize=0.
			 */
			ASSERT(utl->u_ossize != 0, "swdspt4");
			pte = dptopte(p, utl->u_odsize);
			for (; pte < sptopte(p, utl->u_ossize-1); pte++) {
				ASSERT(*(int*)pte == 0, "swdspt3");
			}
		}
	}
#endif	DEBUG
}

/*
 * swpt()
 *	Perform the IO to move part/all of a page-table in/out.
 *
 * `pte' arg is any pte in first page of pte's to be swapped.
 *
 * Note that pte arg passed to swap() may not be a cluster aligned pte;
 * swap arranges B_PTBIO, which IO support routines can handle.
 */

swpt(rdwri, p, doff, pte, count)
	int	rdwri;
	struct proc *p;
	int	doff;
	struct pte *pte;
	size_t	count;
{
	ASSERT_DEBUG(p->p_swaddr != 0, "swpt: p_swaddr");

	if (count != 0)
		swap(p, p->p_swaddr + ctod(UPAGES) + ctod(doff),
			&Usrptmap[btokmx(pte)],
			(int)count, rdwri|B_PAGET, swapdev_vp);
}
