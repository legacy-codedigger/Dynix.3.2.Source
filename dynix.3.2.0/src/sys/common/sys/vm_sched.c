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
static	char	rcsid[] = "$Header: vm_sched.c 2.18 1991/07/09 00:15:38 $";
#endif

/*
 * vm_sched.c
 *	Swapper and various periodic system-related computations.
 *
 * TODO:
 *	Measure/determine if hardswap selection is reasonable.
 *	Determine if deficit calculation is appropriate (eg, not too
 *		conservative or liberal, fits process needs, etc), and
 *		ages reasonably.
 */

/* $Log: vm_sched.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/kernel.h"
#include "../h/cmn_err.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

struct	vmtotal	total;
unsigned rate_v_pgin, rate_v_pgout;	/* page in/out rates */
unsigned sum_v_pgin, sum_v_pgout;	/* page in/out sums  */

sema_t 	swapout_sync;			/* for sync w/swapouts */
sema_t	runout;				/* wait here for someone who wants in */

int	maxslp = MAXSLP;		/* sleep-time threshold */
int	maxpgio = 0;			/* page-rate threshold */
int	klin = KLMAX/4;			/* for 'size' cost estimate */
int	verylowfree = 3*KLMAX*CLSIZE;	/* extremely low free mem: force swap */
int	deficit_age = 5;		/* deficit age factor; see vmmeter() */

long	avenrun[3];			/* load average, of runnable procs */

int	avefree;			/* 5-sec average free memory */
int	avefree30;			/* 30-sec average free memory */
int	avedirty;			/* 5-sec average dirty memory */
int	avedirty30;			/* 30-sec average dirty memory */

int	deficit;			/* memory for in-comming procs */

extern	sema_t	upt_wait;		/* waiting for Usrptmap[] space */

#ifdef	DEBUG
int	swapdebug;
#endif	DEBUG

static ulong	swapfails = 0;		/* swapin failure counter */
/*
 * Successor function for walking the proc[] array in a circular
 * fashion--wraps from top to proc[1], as proc[0] is never a swap
 * candidate.  "cap" is expected to be a local variable filled in
 * with procmax at the time the function runs.
 */
#define SUCC(p) ((((p)+1) >= cap) ? (&proc[1]) : ((p)+1))

/*
 * setupvm()
 *	Setup the paging/swapping constants for the system.
 *
 * Called after the system is (mostly) initialized and the amount
 * of memory is known.
 *
 * Threshold constants are defined in ../machine/vmparam.h.
 */

setupvm()
{
	/*
	 * Setup thresholds for paging:
	 *	desfree		is amount of memory desired free.  if less
	 *			than this for extended period, do swapping.
	 *	minfree		is minimal amount of free memory which is
	 *			tolerable.
	 *	dirtyhigh	size of dirty-list before pageout daemon starts
	 *			writing pages to swap area(s) and frees them.
	 *	dirtylow	size of dirty-list where pageout daemon quits
	 *			writing pages.
	 *	maxdirty	is max size of dirty-list before swapper
	 *			starts trying to off-load pageout().
	 *
	 * All of these are run-time alterable by vm_ctl() syscall.
	 */

	if (vmtune.vt_desfree == 0) {
		vmtune.vt_desfree = DESFREE / NBPG;
		if (vmtune.vt_desfree > freemem / DESFREEFRACT)
			vmtune.vt_desfree = freemem / DESFREEFRACT;
		if (vmtune.vt_desfree < (freemem * MINDESFREEPERCENT) / 100 )
			vmtune.vt_desfree = (freemem * MINDESFREEPERCENT) / 100;
	}

	if (vmtune.vt_minfree == 0) {
		vmtune.vt_minfree = MINFREE / NBPG;
		if (vmtune.vt_minfree > vmtune.vt_desfree / MINFREEFRACT)
			vmtune.vt_minfree = vmtune.vt_desfree / MINFREEFRACT;
	}

	if (vmtune.vt_dirtyhigh == 0) {
		vmtune.vt_dirtyhigh = DIRTYHIGH / NBPG;
		if (vmtune.vt_dirtyhigh > freemem / DIRTYHIGHFRACT)
			vmtune.vt_dirtyhigh = freemem / DIRTYHIGHFRACT;
	}

	if (vmtune.vt_dirtylow == 0) {
		vmtune.vt_dirtylow = DIRTYLOW / NBPG;
		if (vmtune.vt_dirtylow > vmtune.vt_dirtyhigh / DIRTYLOWFRACT)
			vmtune.vt_dirtylow = vmtune.vt_dirtyhigh / DIRTYLOWFRACT;
	}

	if (vmtune.vt_maxdirty == 0)
		vmtune.vt_maxdirty = vmtune.vt_dirtyhigh * MAXDIRTYMULT;

	/*
	 * Maxpgio thresholds how much paging is acceptable.
	 * This figures that 2/3 busy on an arm is all that is
	 * tolerable for paging.  We assume one operation per disk rev.
	 */

	if (maxpgio == 0)
		maxpgio = (DISKRPM * 2) / 3;
}

/*
 * sched()
 *	The main loop of the scheduling (swapping) process, which
 *	runs in proc-slot [0] as pid 0.
 *
 * The basic idea is:
 *	see if anyone wants to be swapped in;
 *	swap out processes until there is room;
 *	swap him in;
 *	repeat.
 * If the paging rate is too high, the average free memory
 * is very low, or the average dirty memory is too high,
 * then we do not consider swapping anyone in,
 * but rather look for someone to swap out.
 *
 * The runout sema is goosed whenever someone is swapped out.
 * Sched waits on it awaiting work.
 *
 * Sched waits on lbolt whenever it cannot find enough core
 * (by swapping out or otherwise) to fit the selected swapped process;
 * lets things settle before looping.
 */

/*
 * LOADED() tells if process is in-memory and not a system process.
 * SWAPPABLE() tells if in-memory process is easily swappable.
 */

#define	LOADED(p)	(((p)->p_flag&(SSYS|SLOAD|SNOSWAP))==SLOAD)
#define	SWAPPABLE(p)	(LOADED(p) && (p)->p_noswap==0)

#define	NBIG	4
#define	MAXNBIG	MAXNUMCPU
int	nbig = NBIG;			/* maybe set to Nengine? */

struct bigp {
	struct	proc *bp_proc;
	int	bp_pri;
	struct	bigp *bp_link;
} bigp[MAXNBIG], bplist;

sched()
{
	register struct proc *rp;
	register struct proc *p;
	register struct bigp *bp;
	register struct bigp *nbp;
	int	outpri;
	int	inpri;
	int	rppri;
	int	sleeper;
	int	desperate;
	int	deservin;
	int	needs;
	int	divisor;
	int	biggot;
	int	def;
	int	hard;
	spl_t	s;
	struct	proc *cap;
	static struct proc *last_rp;
	static struct proc *last_hard_rp;

	last_hard_rp = last_rp = &proc[1];
loop:
	deservin = 0;
	sleeper = 0;
	p = NULL;
#ifdef	DEBUG
	if (swapdebug) printf(" L");
#endif	DEBUG

	/*
	 * See if paging system is overloaded; if so swap someone out.
	 * Conditions for hard outswap are:
	 *	if need Usrptmap[] space (mix it up).
	 * or
	 *	1. if there are at least 2 runnable processes (on the average)
	 * and  2. instantaneous free-space is extremely low
	 * or	2a. the short (5-second) and longer (30-second) average
	 *	    memory is less than desirable.
	 * and	2b. the paging rate is excessive, average memory is too low, or
	 *	    dirty-memory is too high
	 */

	if (blocked_sema(&upt_wait)
	||  (avenrun[0] >= 2*FSCALE/nonline
	  &&  (freemem < verylowfree
	    ||  (imax(avefree, avefree30) < vmtune.vt_desfree
	      &&  (avedirty > vmtune.vt_maxdirty
		|| rate_v_pgin + rate_v_pgout > maxpgio
		|| avefree < vmtune.vt_minfree)))))
	{
		desperate = 1;
#ifdef	DEBUG
		if (swapdebug) printf("D");
#endif	DEBUG
		/*
		 * Since desperate, first try removing bmap cache data,
		 * and if unsuccessful, uneeded text images.
		 */
		if (!BmapPurge())
			(void) xflush(1);
		goto hardswap;
	}
	desperate = 0;

	/*
	 * Not desperate for core,
	 * look for someone who deserves to be brought in.
	 * 1st zap runout count in case multiple units were posted (we'll
	 * see all the processes here).
	 *
	 * Of all outswapped runnable processes, find highest 'prio' based
	 * on time swapped out, some measure of cost to swap it in (use
	 * estimate of time for # pagein operations it will likely do),
	 * how long it slept last time, and its execution priority.
	 * p_rssize holds estimate of # pages the process was using when
	 * it got swapped out (see swapout_proc()).
	 *
	 * No need to lock while looking at swapped out process; it cannot
	 * come back into memory unless we do it.
	 */

	lockup_sema(&runout, s);			/* "prove" no race... */
	sema_count(&runout) = 0;			/* w/swapout_proc... */
	unlock_sema(&runout, s);			/* doing v_sema */

	/*
	 * Search through procs circularly, starting with where we left off
	 * from last scan.
	 */
	outpri = -20000;

	/*
	 * Set up the loop with a cap at current procmax.  If procmax increases
	 * while the loop is running, we are guaranteed that our services will
	 * run again and see him (due to incrementing runout).  If procmax
	 * decreases then at worst we will scan some more empty proc slots.
	 */
	cap = procmax;
	if (last_rp >= cap)
		last_rp = &proc[1];
	rp = last_rp;
	do {
		switch(rp->p_stat) {
		case SRUN:
			if ((rp->p_flag & SLOAD) == 0 && rp->p_noswap == 0) {
				/*
				 * Calculate priority based on the time out,
				 * cost of bringing back, how long it's been
				 * out, and what its priority is.
				 */
				rppri = rp->p_time
				      - rp->p_rssize / (maxpgio/2*klin)
				      + rp->p_slptime
				      - (rp->p_nice-NZERO)*8;
				if (rppri > outpri) {
					p = rp;
					outpri = rppri;
				}
			}
			break;

		case SSLEEP:
		case SSTOP:
			/*
			 * If memory is below desired minimum, try to throw
			 * out a long-term sleeper.
			 */
			if (freemem < vmtune.vt_desfree
			&&  rp->p_slptime > maxslp
			&&  SWAPPABLE(rp)) {
				/*
				 * Kick out deadwood.
				 * NOW must lock and test process again.
				 * This doesn't race (harmfully) with exit/fork
				 * of same proc slot, since must be sleeping or
				 * stopped after we hold the lock.
				 */
				if ((s = cp_lock(&rp->p_state, SPLHI))
						== CPLOCKFAIL)
					break;
				if ((rp->p_stat != SSLEEP
				&& rp->p_stat != SSTOP)
				||  rp->p_slptime <= maxslp
				||  !SWAPPABLE(rp)) {
					/*
					 * Lost race: process changed state.
					 */
					v_lock(&rp->p_state, s);
					break;
				}
				if (swapout(rp, (size_t) rp->p_dsize,
						(size_t) rp->p_ssize) > 0) {
					p_sema(&swapout_sync, PSWP);
					last_rp = SUCC(rp);
					goto loop;
				}
			}
			break;
		}
	} while ((rp = SUCC(rp)) != last_rp);

	/*
	 * Walked the entire list, therefore will start one forward next time.
	 */
	last_rp = SUCC(last_rp);

	/*
	 * No one wants in, so nothing to do.
	 * Any procs we passed over above have goosed runout ==> won't sleep.
	 */
	if (outpri == -20000) {
#ifdef	DEBUG
		if (swapdebug) printf("R");
#endif	DEBUG
		p_sema(&runout, PSWP);
		goto loop;
	}

	/*
	 * Examine requested resident size, if it is too large, then
	 * trim it.
	 */
	{
		int maxrs = MIN(p->p_maxrss, vmtune.vt_maxRS);
		
		if (p->p_rscurr > maxrs) {
			p->p_rscurr = maxrs;
			if (p->p_rssize > p->p_rscurr) 
				p->p_rssize = p->p_rscurr;
				
		}
	}

	/*
	 * Decide how deserving this guy is.  If he is deserving we will
	 * be willing to work harder to bring him in.  Needs is an
	 * estimate of how much core he will need.  If he has been out
	 * for a while, then we will bring him in with 1/2 the core he
	 * will need, otherwise we are conservative.
	 *
	 * If process has pending signal that's not caught, masked, or
	 * ignored, let it in easier -- heuristic to avoid needing lots
	 * of free memory to stop or kill swapped out processes.
	 *
	 * Note: if process has maps and these are also swapped out, will
	 * need space for their page-tables.  Currently not taken into
	 * account (in particular, since Uarea is swapped out, can't look
	 * at u_mmap[]).  If held pointers to struct mmap's in struct proc
	 * and not swap these with process, could get more accurate (but
	 * this costs in complexity and static size).
	 */

	deservin = 0;
	divisor = 1;
	if (outpri > maxslp/2) {
		deservin = 1;
		divisor = 2;
	}
	if (p->p_sig & ~(p->p_sigmask|p->p_sigignore|p->p_sigcatch)) {
		needs = UPAGES + SZPT(p);
		divisor = 1;
	} else
		needs = UPAGES + SZPT(p) + p->p_rssize * CLSIZE;
	def = deficit;					/* copy: avoid races */
	if (freemem - def > needs / divisor) {
		def += needs;
		if (swapin(p)) {
			deficit = def;
			goto loop;
		}
		if ((swapfails++ % 1000) == 0) {
			/* send warning to console */
			cmn_err(CE_WARN, "sched: swapin failed, Usrptmap depleted");
			/*
			 *+ The kernel is printing a warning that an attempt
			 *+ to swap in a process failed due to insufficient
			 *+ space in the user process page table map.
			 */
		}
	} else {
		/* Insufficient memory, shrink resident size by 25 % */
		p->p_rssize = (p->p_rssize * 3) /4;
		if (p->p_rssize < vmtune.vt_minRS) {
			p->p_rssize = vmtune.vt_minRS;
		}
		p->p_rscurr = (p->p_rscurr * 3) /4;
		if (p->p_rscurr < vmtune.vt_minRS) {
			p->p_rscurr = vmtune.vt_minRS;
		}
		ASSERT_DEBUG(p->p_rssize <= p->p_rscurr,
			"sched: rssize > rscurr");
	}

	/*
	 * Need resources (kernel map or memory), swap someone out.
	 * Select the nbig largest jobs, then the oldest of these
	 * is ``most likely to get booted.''
	 *
	 * We make two passes; first time only look at swappable procs
	 * (increases probability of success when try to swapout).  2nd
	 * pass doesn't look at p_noswap, allowing set of SFSWAP to
	 * goose process into swapping self.
	 *
	 * We don't lock proc-states to avoid unnecessary latencies and
	 * overheads.  Thus races can occur, and are dealt with.
	 */
hardswap:
#ifdef	DEBUG
	if (swapdebug) printf("H");
#endif	DEBUG
	for (hard = 0; hard <= 1; hard++) {
		sleeper = 0;
		if (nbig > MAXNBIG)
			nbig = MAXNBIG;
		if (nbig < 1)
			nbig = 1;
		biggot = 0;
		bplist.bp_link = 0;

		/*
		 * Search circularly starting from where we left off last
		 * time.
		 */
		cap = procmax;
		if (last_hard_rp >= cap)
			last_hard_rp = &proc[1];
		rp = last_hard_rp;
		do {
			if (!LOADED(rp) || (!hard && rp->p_noswap) ||
					(rp->p_stat==SZOMB))
				continue;

			if (rp->p_slptime > maxslp &&
			    (rp->p_stat==SSLEEP&&rp->p_pri>PZERO||rp->p_stat==SSTOP)) {
				/*
				 * Remember longest long-term sleeper.
				 */
				if (sleeper < rp->p_slptime) {
					p = rp;
					sleeper = rp->p_slptime;
				}

				/*
				 * If we're not going to find anyone older,
				 * leave the loop and save CPU
				 */
				if (sleeper == 127)
					break;
			} else if (!sleeper
			       &&  (rp->p_stat==SONPROC||rp->p_stat==SRUN||
					rp->p_stat==SSLEEP)) {
				/*
				 * Maintain list of nbig largest processes,
				 * sorted by increasing 'size'.
				 */
				rppri = rp->p_rssize;
				if (biggot < nbig)
					nbp = &bigp[biggot++];
				else {
					nbp = bplist.bp_link;
					if (nbp->bp_pri > rppri)
						continue;
					bplist.bp_link = nbp->bp_link;
				}
				for (bp = &bplist; bp->bp_link; bp = bp->bp_link)
					if (rppri < bp->bp_link->bp_pri)
						break;
				nbp->bp_link = bp->bp_link;
				bp->bp_link = nbp;
				nbp->bp_pri = rppri;
				nbp->bp_proc = rp;
			}
		} while ((rp = SUCC(rp)) != last_hard_rp);

		/*
		 * Record where the loop ended--either on a long-term sleeper,
		 * or because we have scanned the entire proc list.
		 */
		last_hard_rp = SUCC(rp);

		/*
		 * If didn't find a long-term sleeper, find oldest of the procs
		 * that's still in memory.
		 */
#ifdef	DEBUG
		if (swapdebug)
			printf("%c%d", (sleeper ? 'S' : 'N'), biggot);
#endif	DEBUG
		if (!sleeper) {
			p = NULL;
			inpri = -1000;
			for (bp = bplist.bp_link; bp; bp = bp->bp_link) {
				rp = bp->bp_proc;
				rppri = rp->p_time+rp->p_nice-NZERO;
				if (rppri >= inpri) {
					p = rp;
					inpri = rppri;
				}
			}
		}
		/*
		 * If we found a long-time sleeper, or we are desperate and
		 * found anyone to swap out, or if someone deserves to come
		 * in and we didn't find a sleeper, but found someone who
		 * has been in core for a reasonable length of time, then
		 * we kick the poor luser out.
		 *
		 * Should consider trying other procs if selected proc fails to
		 * be easily swappable?
		 */
		if (sleeper || desperate && p || deservin && inpri > maxslp) {
			s = p_lock(&p->p_state, SPLHI);
			if (!LOADED(p)) {
				/*
				 * Lost race; try again after a brief pause
				 * to let things settle.
				 */
				v_lock(&p->p_state, s);
#ifdef	DEBUG
				if (swapdebug) printf("l");
#endif	DEBUG
				break;
			}
			/*
			 * Try to swap it out.  If fail (swap space or SFSWAP
			 * set) sleep on lbolt (more settle).
			 */
			if (swapout(p, (size_t) p->p_dsize, (size_t) p->p_ssize) > 0) {
				p_sema(&swapout_sync, PSWP);	/* got it! */
				goto loop;
			} else {
				int	count;
				/*
				 * Goosed process into swapping itself out.
				 * Wait a while to give it time to do that
				 * (avoid starting more such swaps until
				 * this one has had some time).
				 *
				 * Rather than worry about exactly how long
				 * it might take, sleep at lbolt and poll
				 * the process to see if it's swapped out
				 * yet.  Do this for a max of maxslp/4 seconds,
				 * then restart swap scheduler loop.
				 *
				 * p_uarea goes NULL when process is
				 * essentially swapped out.
				 */
#ifdef	DEBUG
				if (swapdebug) printf("s");
#endif	DEBUG
				for (count = 0; count < maxslp/4; count++) {
					p_sema(&lbolt, PSWP);
					if (p->p_uarea == NULL)
						break;
				}
				goto loop;
			}
		} else if (desperate)
			continue;			/* try harder once */
		else {
#ifdef	DEBUG
			if (swapdebug) printf("F");
#endif	DEBUG
			break;
		}
	}

	/*
	 * Want to swap someone in, but can't so wait on lbolt.
	 */

#ifdef	DEBUG
	if (swapdebug) printf("P");
#endif	DEBUG
	p_sema(&lbolt, PSWP);
	goto loop;
}

/*
 * vmmeter()
 *	Called once per second to compute various system totals and rates.
 */

vmmeter()
{
	register struct engine *eng;
	register struct vmmeter *vmp;
	register int	i;
	register unsigned nsum_v_pgin = 0;
	register unsigned nsum_v_pgout = 0;
	int	def;
	int	delta;
	static	int	vmtot = 0;

	/*
	 * Age the deficit.  Use local copy of "deficit" to avoid races.
	 */

	def = deficit;
	def -= imax(def / deficit_age, (klin * CLSIZE * maxpgio) / 4);
	if (def < 0) def = 0;
	deficit = def;

	ave(avefree, freemem, 5);
	ave(avefree30, freemem, 30);
	ave(avedirty, dirtymem, 5);
	ave(avedirty30, dirtymem, 30);

	/*
	 * Go thru the per-processor vmmeter structure and sum v_pgin
	 * and v_pgout fields.
	 * Note that we don't lock here; thus, the statistics will be
	 * inaccurate due to races with other processors (but don't
	 * expect much inaccuracy).
	 *
	 * Note: v_swpin, v_swpout are computed directly into sum in
	 * vm_swap.c; they are not maintained in the rate structure.
	 */

	for (eng = engine, i = 0; i < Nengine; i++, eng++) {
		if (eng->e_flags & E_NOWAY)
			continue;
		vmp = &(((struct plocal *)(eng->e_local->pp_local))->cnt);
		nsum_v_pgin += vmp->v_pgin;
		nsum_v_pgout += vmp->v_pgout;
	}

	/*
	 * Compute the rates handling counter wrap around.
	 */

#define MAXUNS	4294967295
	if (nsum_v_pgin < sum_v_pgin)
		delta = MAXUNS - sum_v_pgin + nsum_v_pgin + 1;
	else
		delta = nsum_v_pgin - sum_v_pgin;
	ave(rate_v_pgin, delta, 5);
	if (nsum_v_pgout < sum_v_pgout)
		delta = MAXUNS - sum_v_pgout + nsum_v_pgout + 1;
	else
		delta = nsum_v_pgout - sum_v_pgout;
	ave(rate_v_pgout, delta, 5);
	sum_v_pgin = nsum_v_pgin;
	sum_v_pgout = nsum_v_pgout;
#undef MAXUNS

	/*
	 * Every 5th second do vmtotal().
	 */

	if (++vmtot >= 5) {
		vmtot = 0;
		vmtotal();
	}

	/*
	 * In unusual memory situations, and in any case once in a while,
	 * wakeup swapper.
	 */

	if (freemem < verylowfree
	||  avefree < vmtune.vt_minfree
	||  avedirty > vmtune.vt_maxdirty
	||  blocked_sema(&upt_wait)
	||  proc[0].p_slptime > maxslp/2)
		(void) cv_sema(&runout);

	/*
	 * Recover from lost race between mpv_sema(&mem_wait, &mem_alloc, xxx)
	 * and memory being freed.  This race is *very* low probability;
	 * if it occurs, this code recovers within one second.
	 *
	 * This is only necessary if semaphore implementation doesn't support
	 * pv_sema() (eg, SGS), or when using the fine-grain memory locking
	 * algorithms.  Since it doesn't cost much, no #ifdef.
	 */

	if (blocked_sema(&mem_wait) && freemem >= KLMAX*CLSIZE)
		vall_sema(&mem_wait);
}

/*
 * vmtotal()
 *	Each 5 seconds gather some longer-term stats (includes load-average).
 *
 * NOTE: could try to count shared-memories statistics better here.
 */

vmtotal()
{
	register struct proc *p;
	register int	nrun = 0;

	total.t_vm = 0;
	total.t_avm = 0;
	total.t_rm = 0;
	total.t_arm = 0;
	total.t_rq = 0;
	total.t_dw = 0;
	total.t_pw = 0;
	total.t_sl = 0;
	total.t_sw = 0;

	for (p = &proc[1]; p < procmax; p++) {
		if (p->p_flag & SSYS)
			continue;
		if (p->p_stat) {
			total.t_vm += p->p_dsize + p->p_ssize;
			if (p->p_flag & SLOAD)
				total.t_rm += p->p_rssize * CLSIZE;
			switch (p->p_stat) {

			case SSLEEP:
			case SSTOP:
				if (p->p_pri <= PZERO)
					nrun++;
				if (p->p_noswap)		/* approx */
					total.t_pw++;
				else if (p->p_flag & SLOAD) {
					if (p->p_pri <= PZERO)
						total.t_dw++;
					else if (p->p_slptime < maxslp)
						total.t_sl++;
				} else if (p->p_slptime < maxslp)
					total.t_sw++;
				if (p->p_slptime < maxslp)
					goto active;
				break;

			case SONPROC:
			case SRUN:
			case SIDL:
				nrun++;
				if (p->p_flag & SLOAD)
					total.t_rq++;
				else
					total.t_sw++;
		active:
				total.t_avm += p->p_dsize + p->p_ssize;
				if (p->p_flag & SLOAD)
					total.t_arm += p->p_rssize * CLSIZE;
				break;
			}
		}
	}
	total.t_free = avefree;

	loadav(avenrun, nrun);
}

/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 */

long	cexp[3] = {
	920,			/* 0.9200444146293232 == exp(-1/12) */
	983,			/* 0.9834714538216174 == exp(-1/60) */
	994,			/* 0.9944598480048967 == exp(-1/180) */
};

/*
 * loadav()
 *	Compute a tenex style load average of a quantity on
 *	1, 5 and 15 minute intervals.
 *
 * The load-average values are scaled by 1000, so they can be computed
 * in integer arithmetic.
 */

loadav(avg, n)
	register long *avg;
	int n;
{
	register int i;

	for (i = 0; i < 3; i++)
		avg[i] = (cexp[i] * avg[i]) / FSCALE + 
			 (n * (FSCALE - cexp[i])) / nonline;
}
