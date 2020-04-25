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
static	char	rcsid[] = "$Header: kern_synch.c 2.20 90/11/10 $";
#endif

/*
 * kern_synch.c
 *	Context Switching and process priority calculation routines.
 */

/* $Log:	kern_synch.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/buf.h"

#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/clkarb.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#ifdef	i386
#include "../machine/hwparam.h"
#endif	i386

#define	mask(bit)	(1 << (bit))

/*
 * Global switching variables.
 */

#define	NQS	32			/* 32 run queues */
struct	prochd	{			/* linked list of running processes */
	struct	proc *ph_link;
	struct	proc *ph_rlink;
};

static	struct	prochd	qs[NQS];	/* queue per priority group */
static	int	whichqs;		/* bit per non-empty priority group */
static	struct	engine	*last_nudged;	/* last proc nudged for re-dispatch */

#ifdef	FPA
/*
 * Similar to above, but only for processes using an FPA.
 */
static	struct	prochd	qs_fpa[NQS];	/* queue per priority group */
static	int	whichqs_fpa;		/* bit per non-empty priority group */
#endif	FPA

/*
 * Constants to digital decay and forget 90% of usage in 5*loadav time.
 */

#undef ave
#define	ave(a,b) ((int) ( ((int)(a*b)) / (b+FSCALE) ) )

int	nrscale = 2;
long	ccpu = 951;		/* exp(-1/20) */

extern	bool_t	light_show;	/* set if want processor LEDs to show busy */

/*
 * schedcpu()
 *	Recompute process priorities, once a second
 */

schedcpu()
{
	register struct proc *p;
	register int a;
	register spl_t s_ipl;

	/*
	 * If someone is waiting on lbolt, wake 'em up.
	 * Should be a rare event.
	 */

	if (blocked_sema(&lbolt))
		vall_sema(&lbolt);

	for (p = proc; p < procmax; p++) {
		/*
		 * If NULL and SZOMB states prevelant, then it
		 * will be faster to check the stat, lock and check
		 * for SZOMB again...
		 */
		if ((s_ipl = cp_lock(&p->p_state, SPLHI)) == CPLOCKFAIL)
			continue;
		if (!p->p_stat || p->p_stat == SIDL || p->p_stat == SZOMB) {
			v_lock(&p->p_state, s_ipl);
			continue;
		}
		if (p->p_time != 127)
			p->p_time++;
		if (p->p_stat==SSLEEP || p->p_stat==SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		if (p->p_flag&SLOAD) {
			/*
			 * If greater than hz ticks, truncate to hz.
			 */
			if (p->p_cpticks > hz)
				p->p_cpticks = hz;
			p->p_pctcpu = (ccpu * p->p_pctcpu +
			    (FSCALE-ccpu) * ((p->p_cpticks*FSCALE)/hz))
				/ FSCALE;
		}
		p->p_cpticks = 0;
		a = ave((p->p_cpu & 0xff), avenrun[0]*nrscale) +
			p->p_nice - NZERO;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		setpri(p);
		if (p->p_pri >= PUSER) {
			VOID_P_GATE(G_RUNQ);
			if (p->p_stat == SRUN && (p->p_flag & SLOAD) &&
			    p->p_pri != p->p_usrpri &&
			    p->p_affinity == ANYENG) {
				remrq(p);
				p->p_pri = p->p_usrpri;
				setrq(p);
			} else {
				p->p_pri = p->p_usrpri;
				if (p->p_stat == SONPROC)
					engine[p->p_engno].e_pri = p->p_pri;
			}
			V_GATE(G_RUNQ, SPLHI);
		}
		v_lock(&p->p_state, s_ipl);
	}
	vmmeter();
	timeout(schedcpu, (caddr_t)0, hz);
}

/*
 * setpri()
 *	Set user priority.
 *
 * Ignores p_cpu if process has non-aging priority set; "nice" still
 * works, however.
 */

setpri(pp)
	register struct proc *pp;
{
	register int pri;

	if (pp->p_flag & SNOAGE)
		pri = 0;
	else
		pri = (pp->p_cpu & 0xff)>>2;
	pri += PUSER + 2*(pp->p_nice - NZERO);
	if (pri > PIDLE-1)
		pri = PIDLE-1;
	pp->p_usrpri = pri;
}

/*
 * timeslice()
 *	Timeslicing.  Timeout event to occur every 1/10 sec.
 *
 * If there exists a process on the run-queue of higher priority
 * than the lowest priority processor, nudge the lowest priority
 * processor. Timeslice roundrobin amongst processors of with same
 * priority via last_nudged mechanism.
 *
 * Also, nudge all engines with both affinity and non-empty affinity
 * run-queues.
 *
 * FPA processors are also time-sliced, if FPA processes are running.
 * Must insure FPA processes get "fair" access to FPA processors.
 */

timeslice()
{
	register struct engine *bestpick;
	register struct engine *eng;
	register int eminpri;
	register int lowpri = -1;
#ifdef	FPA
	register struct engine *bestpick_fpa;
	register int lowpri_fpa = -1;
#endif	FPA
	GATESPL(s_ipl);

	P_GATE(G_RUNQ, s_ipl);

	/*
	 * Find lowest priority general processor
	 * (and lowest priority FPA processor, if there are FPA's).
	 *
	 * Loop is guaranteed to find a "bestpick" since even an idle
	 * processor has priority > initial lowpri.
	 */

	eng = last_nudged;
	do {
		if (++eng == engine_Nengine)
			eng = engine;
		if (eng->e_flags & E_NOWAY)
			continue;
		if (eng->e_state == E_BOUND) {
			if (eng->e_head != (struct proc *)eng)
				nudge(PUSER, eng);
			continue;
		}
		eminpri = MIN(eng->e_pri, eng->e_npri);
		if (eminpri > lowpri) {
			bestpick = eng;
			lowpri = eminpri;
		}
#ifdef	FPA
		/*
		 * Keep track of FPA processors seperately.
		 */
		if ((eng->e_flags & E_FPA) && eminpri > lowpri_fpa) {
			bestpick_fpa = eng;
			lowpri_fpa = eminpri;
		}
#endif	FPA
	} while (eng != last_nudged);

	if (whichqs != 0 && ((ffs(whichqs)-1) << 2) <= lowpri) {
		nudge(PUSER, bestpick);
		last_nudged = bestpick;
	}

#ifdef	FPA
	/*
	 * Time-slice FPA processors if there are FPA processes to run
	 * and haven't already nudged processor (eg, in system with FPA
	 * on each processor).  Use of single "last_nudged" is heuristic;
	 * if FPA process came ready, setrq() would move last_nudged also.
	 */
	if (whichqs_fpa != 0 &&
	    bestpick_fpa != bestpick &&
	    ((ffs(whichqs_fpa)-1) << 2) <= lowpri_fpa) {
		nudge(PUSER, bestpick_fpa);
		last_nudged = bestpick_fpa;
	}
#endif	FPA

	V_GATE(G_RUNQ, s_ipl);

	timeout(timeslice, (caddr_t)0, hz/10);
}

/*
 * rqinit()
 *	Initialize the (doubly-linked) run queues to be empty.
 */

rqinit()
{
	register int i;

	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
	last_nudged = &engine[0];

#ifdef	FPA
	for (i = 0; i < NQS; i++)
		qs_fpa[i].ph_link = qs_fpa[i].ph_rlink = (struct proc *)&qs_fpa[i];
#endif	FPA
}

/* 
 * setrq()
 *	Place process p on the appropriate run-queue.
 *
 * If process p has higher priority than the lowest running processor,
 * then nudge that processor to reschedule. If this process is bound to
 * processor "p->p_affinity" and that processor is not currently running
 * a bound process then nudge processor "p->p_affinity".
 *
 * The run-queue is assumed locked by the caller with SPLHI.
 */

setrq(p)
	register struct proc *p;
{
	register struct engine *eng;
	register int eminpri;
	register struct engine *bestpick = NULL;
	register int lowpri;

	/* 
	 * If process is bound, nudge target processor if it
	 * is not already running a bound process
	 */

	if (p->p_affinity != ANYENG) {
		insque(p, engine[p->p_affinity].e_tail);
		if (engine[p->p_affinity].e_state == E_GLOBAL)
			bestpick = &engine[p->p_affinity];
	}

#ifdef	FPA
	/*
	 * Else if process is an FPA user, put it on global FPA run-queue,
	 * and see about nudging an FPA processor to run this process.
	 */

	else if (p->p_fpa == FPA_HW) {
		insque(p, qs_fpa[p->p_pri>>2].ph_rlink);
		whichqs_fpa |= mask(p->p_pri>>2);
		/*
		 * Try to find good candidate processor to execute
		 * this process.  "Good" means lowest prio executing
		 * process/or with an FPA.
		 */
		lowpri = p->p_pri;
		eng = last_nudged;
		do {
			if ((eng->e_flags & E_NOWAY) == 0 &&
			    (eng->e_flags & E_FPA) &&
			    eng->e_state == E_GLOBAL &&
			    (eminpri = MIN(eng->e_pri, eng->e_npri)) > lowpri) {
				bestpick = eng;
				lowpri = eminpri;
			}
			if (++eng == engine_Nengine)
				eng = engine;
		} while (eng != last_nudged);
	}
#endif	FPA

	/*
	 * Otherwise place process on global run-queue,
	 * and look for a processor to run it.
	 */

	else {
		insque(p, qs[p->p_pri>>2].ph_rlink);
		whichqs |= mask(p->p_pri>>2);
		/*
		 * Try to find good candidate processor to execute
		 * this process.  "Good" means lowest prio executing
		 * process/or.
		 */
		lowpri = p->p_pri;
		eng = last_nudged;
		do {
			if ((eng->e_flags & E_NOWAY) == 0 &&
			    eng->e_state == E_GLOBAL &&
			    (eminpri = MIN(eng->e_pri, eng->e_npri)) > lowpri) {
				bestpick = eng;
				lowpri = eminpri;
			}
			if (++eng == engine_Nengine)
				eng = engine;
		} while (eng != last_nudged);
	}

	/*
	 * If found a lower priority processor, suggest it should re-dispatch.
	 */

	if (bestpick != NULL) {
		nudge(p->p_pri, bestpick);
		last_nudged = bestpick;
	}
}

/*
 * setrun()
 *	Make process runnable.
 *
 * Place process on the run-queue if the process is
 * memory resident (not swapped), otherwise "goose"
 * the swapper to bring the process into memory.
 *
 * Process state p_state is assumed locked by the caller at SPLHI.
 */

setrun(p)
	register struct proc *p;
{
	p->p_stat = SRUN;
	if (p->p_flag & SLOAD) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
	} else
		v_sema(&runout);
}

/*
 * nudge()
 *	Signal a processor to reschedule.
 *
 * The "signal" is a BIN0 resched software interrupt.
 *
 * The run-queue is locked by the caller.
 */

nudge(pri, eng)
	register char pri;		/* new nudged pri */
	register struct engine *eng;	/* processor to nudge */
{
	/*
	 * If the processor in question has been nudged,
	 * then all thats necessary is to update its npri.
	 */

	if (eng->e_npri != PIDLE) {
		eng->e_npri = pri;
		return;
	}

	eng->e_npri = pri;

	/*
	 * If the processor to be nudged is the current processor, then
	 * setting the l.runrun flag is sufficient. If the processor
	 * is idle, it will immediately find something on the run-queue.
	 * So the nudge is unnecessary.
	 *
	 * Important to set l.runrun=1 instead of ++, since SWTON() can set
	 * it to -1.
	 */

	if (eng == l.eng)
		l.runrun = 1;
	else if (eng->e_pri != PIDLE) {
		sendsoft(eng->e_slicaddr, RESCHED);
	}
}

/*
 * remrq()
 *	Remove a process from its run-queue.
 *
 * If the process was the last on that global runque, the corresponding
 * bit in whichqs is cleared.
 *
 * RUNQ is locked and called at SPLHI.
 */

remrq(p)
	register struct proc *p;
{
	register int bit;

	remque(p);
	if (p->p_affinity == ANYENG) {
		bit = p->p_pri>>2;
#ifdef	FPA
		if (p->p_fpa == FPA_HW) {
			if (qs_fpa[bit].ph_link == (struct proc *) &qs_fpa[bit])
				whichqs_fpa &= ~mask(bit);
			return;
		}
#endif	FPA
		if (qs[bit].ph_link == (struct proc *) &qs[bit])
			whichqs &= ~mask(bit);
	}
}

/*
 * swtch()
 *	Context switch away.  Calling process needs to block on something.
 *
 * Called when a process voluntarily releases its processor.
 * Called from p_sema, exit, runme, and swapout.
 *
 * A pointer to the current process is passed only when called from p_sema.
 * This indicates that the process state lock must be released after
 * switching context from the current process. In all other cases, the
 * processor is already running from the per processor private stack and 
 * uarea before swtch is called.
 */
#ifdef	i386
/*
 * Since call save() and resume(), *MUST* have enough registers declared
 * to insure all register variables saved in this context (save/resume
 * don't save/restore registers).  Any variables not in registers must be
 * re-loaded after return from use_private().
 *
 * On non-zero return from save() (ie, after switch out and back),
 * registers are not loaded -- thus any use of local registers must
 * be re-initialized.  Note that no such use is made.
 *
 * V_GATE() enables processor interrupts (P_GATE() disables interrupts
 * at processor), so have to splhi() in some cases.
 *
 * i386 compiler only supports 3 register variables.
 */
#endif	i386

swtch(p)
	register struct proc *p;
{
	register struct proc *chosen;
	register int bit;
	register struct engine *myeng;

	/*
	 * Code in panic/pause_self assumes that noproc is set before
	 * using the private stack/pages. This is true currently
	 * but if it changes, need to mod the way fpa state is
	 * saved on panic.
	 */
	l.noproc = 1;
	myeng = l.eng;
	VOID_P_GATE(G_RUNQ);
	for (;;) {
		l.runrun = 0;

		/*
		 * See about going "offline".
		 */

		if (myeng->e_flags & E_SHUTDOWN) {

			/*
			 * No longer need runQ.  Since V_GATE() might
			 * re-enable interrupts, go to SPLHI seperately.
			 */

			V_GATE(G_RUNQ, SPLHI);
			(void) splhi();

			/*
			 * Save caller state if necessary and change to
			 * per-processor private stack.
			 */

			if (p) {
				if (save() != 0)
					return;
				use_private();
				v_lock(&p->p_state, SPLHI);
			}

			/*
			 * Can now go "offline".  If return, have online'd
			 * again, so re-enter dispatch loop.  Re-setup
			 * local variables in case processor has fewer
			 * registers than those declared.
			 */

			offline_self();

			myeng = l.eng;
			p = NULL;
			VOID_P_GATE(G_RUNQ);
			continue;
		}

		/*
		 * If there is an affinity bound process, take it.
		 * Else if there are ready FPA processes and processor supports
		 *	FPA, take an FPA process if there are no higher
		 *	priority non-FPA processes.
		 * Else, take highest prio process from runQ (if non-empty).
		 * Else, idle.
		 */

		if (myeng->e_head != (struct proc *)myeng) {
			chosen = myeng->e_head;
			remque(chosen);
			myeng->e_state = E_BOUND;
			break;
		}
#ifdef	FPA
		/*
		 * Note: if same priority FPA process and non-FPA,
		 * this favors the FPA process.  This is easily changed.
		 */
		else if (whichqs_fpa != 0 && l.fpa &&
			(whichqs == 0 || ffs(whichqs_fpa) <= ffs(whichqs))) {
			bit = ffs(whichqs_fpa) - 1;
			chosen = qs_fpa[bit].ph_link;
			remque(chosen);
			ASSERT_DEBUG(chosen->p_fpa == FPA_HW, "swtch: chosen !p_fpa");
			/*
			 * If last in queue, turn off whichqs_fpa bit.
			 */
			if (qs_fpa[bit].ph_link == (struct proc *)&qs_fpa[bit])
				whichqs_fpa &= ~mask(bit);
			myeng->e_state = E_GLOBAL;
			break;
		}
#endif	FPA
		else if (whichqs != 0) {
			bit = ffs(whichqs) - 1;
			chosen = qs[bit].ph_link;
			remque(chosen);
			/*
			 * If last in queue, turn off whichqs bit.
			 */
			if (qs[bit].ph_link == (struct proc *)&qs[bit])
				whichqs &= ~mask(bit);
			myeng->e_state = E_GLOBAL;
			break;
		} else {
			/*
			 * Idle -- no ready processes.
			 *
			 * Save caller state and change to per-processor
			 * private stack, if necessary.
			 */
			if (p) {
				if (save() != 0)
					return;		/* return from resume */
				use_private();
#ifdef	ns32000
				v_lock(&p->p_state, SPLHI);
#endif	ns32000
#ifdef	i386
				/*
				 * Interrupts masked at processor for gates,
				 * V_GATE() doesn't fuss with SLIC SPL.
				 */
				v_lock(&p->p_state, SPL0);
				myeng = l.eng;		/* re-setup local var */
#endif	i386
				p = NULL;
			}
			myeng->e_state = E_GLOBAL;
			myeng->e_pri = PIDLE;
			myeng->e_npri = PIDLE;
			V_GATE(G_RUNQ, SPL0);
			idle();
			continue;
		}
	}
	l.cnt.v_swtch++;
	resume(p, chosen, p);
}

/*
 * idle()
 *	Processor idles, looking at runQ for something to do.
 *
 * Set SLIC interrupt arbitration priority to reflect idle processor.
 *
 * The processor loops checking for either a shutdown request or a 
 * process on a run-queue.
 *
 * Running on per processor private stack and uarea.
 */

#ifdef	ns32000
#define	DISABLE()	(void) splhi()
#define	ENABLE()	(void) spl0()
#endif	ns32000

static
idle()
{
	/*
	 * Adjust SLIC arbitration priority to reflect idle state of this
	 * processor.
	 */

	SLICPRI(PIDLE/4);

	/*
	 * Becoming idle -- turn off lights.
	 */

	if (light_show) {
		DISABLE();
		if (fp_lights)
			FP_LIGHTOFF(l.me);
#if	defined(ns32000) || defined(KXX)
		if (fp_lights <= 0)
			(void) rdslave(l.eng->e_slicaddr, SL_P_LIGHTOFF);
#else
		*(int *) PHYS_LED = 0;
#endif	ns32000||KXX
		ENABLE();
	}

	/*
	 * Spin until something to do and this engine can do it.
	 */

	do {
		while (whichqs == 0 && l.eng->e_head == (struct proc *)l.eng &&
#ifdef	FPA
		       (whichqs_fpa == 0 || !l.fpa) &&
#endif	FPA
		       ((l.eng->e_flags & E_SHUTDOWN) == 0)) {
			continue;			/* spin in cache */
		}
	} while (CP_GATE(G_RUNQ) == CPGATEFAIL);

	/*
	 * No longer idle, should be less likely to arbitrate for interrupt.
	 */

	SLICPRI((PUSER-1)/4);

	/*
	 * Exiting idle loop -- turn on lights.
	 */

	if (light_show) {
		if (fp_lights)
			FP_LIGHTON(l.me);
#if	defined(ns32000) || defined(KXX)
		if (fp_lights <= 0)
			(void) rdslave(l.eng->e_slicaddr, SL_P_LIGHTON);
#else
		*(int *) PHYS_LED = 1;
#endif	ns32000||KXX
	}
}

/*
 * qswtch()
 *	Queue the current process and swtch processes if necessary.
 *
 * The current process is not immediately released. Qswtch is
 * invoked after the processor has been nudged. That is, when
 * a preemption occurs. This is attempt to avoid cache ripple.
 */
#ifdef	i386
/*
 * Since call save() and resume(), *MUST* have enough registers
 * declared to insure all register variables saved in this context
 * (save/resume don't save/restore registers).  Any variables not
 * in registers must be re-loaded after return from use_private().
 *
 * On non-zero return from save() (ie, after switch out and back),
 * registers are not loaded -- thus any use of local registers must
 * be re-initialized.  Note that no such use is made.
 *
 * V_GATE() enables processor interrupts (P_GATE() disables interrupts
 * at processor), so have to splhi() in some cases.
 */
#endif	i386

qswtch()
{
	register struct proc *chosen, *p;
	register int bit;
	register struct engine *myeng;

	u.u_ru.ru_nivcsw++;		/* involuntary swtch */

	myeng = l.eng;
	p = u.u_procp;

	VOID_P_GATE(G_RUNQ);		/* runQ gate is released by resume */
	p->p_stat = SRUN;

	/*
	 * Fast in-line setrq, same effect as setrq
	 * except that no preemption checking is done.
	 */

	if (p->p_affinity != ANYENG) {
		insque(p, engine[p->p_affinity].e_tail);
	}
#ifdef	FPA
	else if (p->p_fpa == FPA_HW) {
		insque(p, qs_fpa[p->p_pri>>2].ph_rlink);
		whichqs_fpa |= mask(p->p_pri>>2);	/* non-empty */
	}
#endif	FPA
	else {
		insque(p, qs[p->p_pri>>2].ph_rlink);
		whichqs |= mask(p->p_pri>>2);		/* non-empty */
	}

	/*
	 * Code in panic/pause_self assumes that noproc is set before
	 * using the private stack/pages. This is true currently
	 * but if it changes, need to mod the way fpa state is
	 * saved on panic.
	 */
	l.noproc = 1;
	l.runrun = 0;

	/*
	 * See about going "offline".
	 */

	if (myeng->e_flags & E_SHUTDOWN) {

		/*
		 * Save caller state, change to per processor private stack,
		 * and release RUNQ gate.
		 */

		if (save() != 0)
			return;
		use_private();
		V_GATE(G_RUNQ, SPLHI);

		/*
		 * Now can go "offline".  If return, processor is online
		 * again; call swtch() to re-dispatch.  Use swtch(NULL)
		 * since remainder of qswtch() assumes non-empty runQ.
		 */

		offline_self();

		swtch((struct proc *) NULL);
		/*NOTREACHED*/
	}

	/*
	 * If there is an affinity bound process, run it.
	 * Else take highest prio process from runQ;
	 * runQ can't be empty since just put caller into it.
	 *
	 * If there are ready FPA processes and processor supports FPA, take
	 * an FPA process if there are no higher priority non-FPA processes.
	 */

	if (myeng->e_head != (struct proc *)myeng) {
		chosen = myeng->e_head;
		remque(chosen);
		myeng->e_state = E_BOUND;
	}
#ifdef	FPA
	/*
	 * Note: if same priority FPA process and non-FPA,
	 * this favors the FPA process.  This is easily changed.
	 */
	else if (whichqs_fpa != 0 && l.fpa &&
		(whichqs == 0 || ffs(whichqs_fpa) <= ffs(whichqs))) {
		bit = ffs(whichqs_fpa) - 1;
		chosen = qs_fpa[bit].ph_link;
		remque(chosen);
		ASSERT_DEBUG(chosen->p_fpa == FPA_HW, "qswtch: chosen !p_fpa");
		/*
		 * If last in queue, turn off whichqs_fpa bit.
		 */
		if (qs_fpa[bit].ph_link == (struct proc *)&qs_fpa[bit])
			whichqs_fpa &= ~mask(bit);
		myeng->e_state = E_GLOBAL;
	}
#endif	FPA
	else {
		bit = ffs(whichqs) - 1;
		chosen = qs[bit].ph_link;
		remque(chosen);
		/*
		 * If last in queue, turn off whichqs bit.
		 */
		if (qs[bit].ph_link == (struct proc *)&qs[bit])
			whichqs &= ~mask(bit);
		myeng->e_state = E_GLOBAL;
	}

	/*
	 * If chose self, not much to do.  Else save context and
	 * resume new process.
	 */

	if (chosen == p) {
		l.noproc = 0;
		myeng->e_npri = PIDLE;
		p->p_stat = SONPROC;
		V_GATE(G_RUNQ, SPL0);
	} else {
		l.cnt.v_swtch++;
		resume(p, chosen, (struct proc *)NULL);
	}
}

/*
 * runme()
 *	Switch current process to processor (engine) engnum.
 *
 * Return previous affinity (p->p_affinity).
 *
 * Runme is called internally by driver interface code.
 * The affinity system call is similar but has different validity checking.
 */

runme(engnum)
	register int engnum;		/* target processor */
{
	register int old;
	register spl_t ipl;

	/*
	 * If current and target the same...
	 */

	old = u.u_procp->p_affinity;
	if (engnum == old)
		return(old);

	ipl = p_lock(&engtbl_lck, SPLHI);
	if (engnum != ANYENG && (engine[engnum].e_flags & E_NOWAY)) {
		v_lock(&engtbl_lck, ipl);
		return(E_UNAVAIL);
	}

	if (old == ANYENG)
		engine[engnum].e_count++;
	else {
		engine[old].e_count--;
		if (engnum != ANYENG)
			engine[engnum].e_count++;
	}
	v_lock(&engtbl_lck, ipl);

	u.u_procp->p_affinity = engnum;
	
	/*
	 * If the process is already on the target processor,
	 * simply return. The old value is returned so that 
	 * previous affinity can be restored.
	 */

	if (engnum == l.me)
		return(old);

	/*
	 * Place process on the run-queue and swtch to another process.
	 */
#ifdef	i386
	/*
	 * Done in a lower-level procedure to allow use of "old" register
	 * after save() returns.  This is only really necessary on i386
	 * (for now), but doesn't hurt ns32000.
	 */
#endif	i386

	runme_swtch();
	return(old);				/* on selected processor */
}

/*
 * runme_swtch()
 *	Do the switch for an affinity move.
 *
 * Also used to switch on to an FPA processor.
 */
#ifdef	i386
/*
 * Since call save() and resume(), *MUST* have enough registers
 * declared to insure all register variables saved in this context
 * (save/resume don't save/restore registers).  Any variables not
 * in registers must be re-loaded after return from use_private().
 *
 * On non-zero return from save() (ie, after switch out and back),
 * registers are not loaded -- thus any use of local registers must
 * be re-initialized.  Note that no such use is made.
 */
#endif	i386

runme_swtch()
{
	register spl_t ipl;
#ifdef	i386
	register int dummy1, dummy2;		/* need 3 register vars */
#ifdef	lint
	dummy2 = dummy1 = ipl = 0;
	lint_ref_int(dummy1); lint_ref_int(dummy2); lint_ref_int(ipl);
#endif	lint
#endif	i386

	u.u_ru.ru_nvcsw++;		/* voluntary context switch */
	P_GATE(G_RUNQ, ipl);
	setrq(u.u_procp);
	/*
	 * Code in panic/pause_self assumes that noproc is set before
	 * using the private stack/pages. This is true currently
	 * but if it changes, need to mod the way fpa state is
	 * saved on panic.
	 */
	l.noproc = 1;
	if (save() != 0)
		return;			/* return on new processor */
	use_private();
	V_GATE(G_RUNQ, ipl);
	swtch((struct proc *)NULL);	/* cast is for lint! */
	/*NOTREACHED*/
}

/*
 * offline_self()
 *	Actually go offline.
 *
 * Called when processor context switches after being asked to go offline.
 * Coordinates with process doing the "offline" system call (tmp_ctl()),
 * who actually stops this processor.
 */

static
offline_self()
{
	/*
	 * Insure interrupts masked and none pending.
	 */

	(void) splhi();
	flush_intr();

	/*
	 * No way for off-board engine to turn off our light, so turn it
	 * off for ourselves.
	 */
	if (light_show) {
#if	defined(ns32000) || defined(KXX)
		if (fp_lights <= 0)
			(void) rdslave(l.eng->e_slicaddr, SL_P_LIGHTOFF);
#else
		*(int *) PHYS_LED = 0;
#endif	ns32000||KXX
	}

	/*
	 * Tell offline initiator it's now ok to stop this processor.
	 */

	v_sema(&eng_wait);

	/*
	 * Loop waiting to be halted.  Interrupts remain off.
	 *
	 * It is HW dependent if loop will exit; offline implementation
	 * can reset processor or just "hold"/"pause" it, as appropriate.
	 * If "reset", processor will completely restart; else it just
	 * continues the loop (but E_SHUTDOWN is off before un-holding
	 * processor).  See start_engine() and halt_engine() for details.
	 */

	while (l.eng->e_flags & E_SHUTDOWN)
		continue;

	/*
	 * Loop exited ==> processor has been offline'd and online'd again
	 * without being reset (kinda like taking a nap); tell online
	 * initiator we're no longer offline (see tmp_ctl()).
	 *
	 * No need to re-init engine structure since it had to be clean
	 * (no queued processes) when processor went offline.  Ok to fuss
	 * e_flags since no other processor will modify them until after
	 * V eng_wait sema.
	 *
	 * No need to flush TLB since processor only accesses private
	 * resources or non-changing address space until context switch.
	 */

	l.eng->e_flags &= ~E_OFFLINE;		/* on-line, now! */
#ifdef	FPA
	if (l.eng->e_flags & E_FPA)
		online_fpa();			/* declare FPA on again */
#endif	FPA
	v_sema(&eng_wait);			/* let "online" complete */
}
