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
static	char	rcsid[] = "$Header: kern_clock.c 2.21 1991/05/29 00:19:14 $";
#endif

/*
 * kern_clock.c
 * 	Clock handling routines.
 *
 * Conditionals:
 *	-DKPRO		System V flavor kernel profiling.
 */

/* $Log: kern_clock.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/callout.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../balance/engine.h"

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"

#ifdef	ns32000
#define	BASEPRI(ps)	(((ps) & 0xFFFF) == SPL0)
#endif	ns32000
#ifdef	i386
#define	BASEPRI(ps)	(((ps) & 0xFF) == SPL0)
#endif	i386

/*
 * Bump a timeval by a small number of usec's.
 */
#define BUMPTIME(t, usec) { \
	register struct timeval *tp = (t); \
	register time_t	lusec; \
 \
	lusec = tp->tv_usec + (usec); \
	if (lusec >= 1000000) { \
		lusec -= 1000000; \
		tp->tv_sec++; \
	} \
	tp->tv_usec = lusec; \
}

/*
 * Global variables used by clock(s) and timeouts
 */
struct	callout	*callfree, calltodo;
lock_t	time_lck;

#ifdef	KPRO
/*
 * Kernel profiling data-structures.
 * This stuff should move into a header file.
 */
#define	PRFMAX	2048		/* maximum number of text addresses */
#define	PRF_OFF	00		/* profiler off */
#define PRF_ON	01		/* profiler collecting samples */

u_int	prfstat;		/* state of profiler */
u_int	prfmax;			/* number of loaded text symbols */
u_int	prfctr[PRFMAX + 1];	/* counters for symbols; last used for User */
u_int	prfsym[PRFMAX];		/* text symbols */
#endif	KPRO

/*
 * hardclock()
 *	The hz hardware interval timer.
 *
 * Update events relating to real time.
 */

#ifdef	ns32000
/*ARGSUSED*/
hardclock(pc, ps)
#endif	ns32000
#ifdef	i386
/*
 * hardlock() is called directly from locore on the 386; no indirection.
 * Thus stack looks like that at entry to interrupt handler.
 */
/*ARGSUSED*/
hardclock(vec, oldspl, edx, ecx, eax, pc, ps)		/* ps == intr'd CS */
#endif	i386
	caddr_t pc;
{
	register struct proc *p;
	register int cpstate;
	extern	 int auto_nice;

#ifdef	KPRO
	/*
	 * Call the kernel profiler.
	 */
	if (prfstat & PRF_ON)
		prfintr((unsigned)pc, ps);
#endif	KPRO

	/*
	 * Charge the time out based on the mode the cpu is in.
	 * Here again we fudge for the lack of proper interval timers
	 * assuming that the current state has been around at least
	 * one tick.
	 */
	if (USERMODE(ps)) {
		p = u.u_procp;
		/*
		 * CPU was in user state.  Increment
		 * user time counter, and process process-virtual time
		 * interval timer. 
		 */
		BUMPTIME(&u.u_ru.ru_utime, tick);
		/*
		 * Check to see if process has accumulated
		 * more than 10 minutes of user time.  If so
		 * reduce priority to give others a chance.
		 *
		 * Only race with p_nice is if a setpriority
		 * is done between the test for NZERO and the throttle.
		 * The setpriority can be lost, but do we really care?
		 * If we do care, we must lock process state...
		 */
		if (auto_nice 
		&&  p->p_uid
		&&  p->p_nice == NZERO
		&&  u.u_ru.ru_utime.tv_sec > 10 * 60
		&&  (p->p_flag & SNOAGE) == 0) {
			p->p_nice = NZERO+4;
		}
		if (timerisset(&u.u_timer[ITIMER_VIRTUAL].it_value) &&
		    itimerdecr(&u.u_timer[ITIMER_VIRTUAL], tick) == 0)
			psignal(p, SIGVTALRM);
		if (p->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
		if (u.u_prof.pr_scale) {
			SWTON(SWT_PROF);
		}
#ifdef i386
		/*
		 * 80386 parts B1 and younger can lock up if the first byte
		 * of an FPU instruction is in the last 8 bytes of a page,
		 * the last byte of the page is an ESC instruction,
		 * interveaning bytes are all legit prefixes, and reference
		 * to the next page causes a page fault.  Interrupts break the
		 * lockup, but it locks up again unless the following page
		 * is made valid.  See B1 Errata sheet (12/17/86) bug # 17.
		 *
		 * Can't look into user address space here, since we're
		 * at splhi() and can't block.  Thus test what can, then
		 * post SW trap to test the rest.
		 *
		 * DYNIX compilers (C, Fortran, Pascal) don't generate
		 * prefix bytes on FPU instructions.  Thus ok to check that
		 * eip is *exactly* at end of page (thus cutting down the
		 * number of cases a bunch).
		 *
		 * This can be tested per processor.
		 */
		if (l.fpu_pgxbug && ((int)pc & (NBPG-1)) == (NBPG-1)) {
			SWTON(SWT_FPU_PGXBUG);
		}
#endif
	} else {
		/*
		 * CPU was in system state.  If profiling kernel
		 * increment a counter.  If no process is running
		 * then this is a system tick if we were running
		 * at a non-zero IPL (in a driver).  If a process is running,
		 * then we charge it with system time even if we were
		 * at a non-zero IPL, since the system often runs
		 * this way during processing of system calls.
		 * This is approximate, but the lack of true interval
		 * timers makes doing anything else difficult.
		 */
		cpstate = CP_SYS;
		if (l.noproc) {
#ifdef	ns32000
			if (BASEPRI(ps))
#endif	ns32000
#ifdef	i386
			if (BASEPRI(oldspl))
#endif	i386
				cpstate = CP_IDLE;
		} else {
			BUMPTIME(&u.u_ru.ru_stime, tick);
		}
	}

	/*
	 * If the cpu is currently scheduled to a process, then
	 * charge it with resource utilization for a tick, updating
	 * statistics which run in (user+system) virtual time,
	 * such as the cpu time limit and profiling timers.
	 * This assumes that the current process has been running
	 * the entire last tick.
	 */

	if (l.noproc == 0) {
		p = u.u_procp;

#define       cpusoft(u)      (u).u_rlimit[RLIMIT_CPU].rlim_cur
#define       cpuhard(u)      (u).u_rlimit[RLIMIT_CPU].rlim_max

		if (cpstate != CP_IDLE) {
			long secs = u.u_ru.ru_utime.tv_sec+
					u.u_ru.ru_stime.tv_sec+1;
			if (secs > cpusoft(u)) {
				if (secs > cpuhard(u))
					psignal(p, SIGKILL);
				else {
					psignal(p, SIGXCPU);
					if (cpusoft(u) < cpuhard(u))
						cpusoft(u) += 5;
				}
			}
			if (timerisset(&u.u_timer[ITIMER_PROF].it_value) &&
			    itimerdecr(&u.u_timer[ITIMER_PROF], tick) == 0)
				psignal(p, SIGPROF);
		}

		/*
		 * We adjust the priority of the current process.
		 * The priority of a process gets worse as it accumulates CPU
		 * time.  The cpu usage estimator (p_cpu) is increased here and
		 * the formula for computing priorities (in kern_synch.c) will
		 * compute a different value each time the p_cpu increases by 4.
		 * The cpu usage estimator ramps up quite quickly when the
		 * process is running (linearly), and decays away exponentially,
		 * at a rate which is proportionally slower when the system is
		 * busy.  The basic principal is that the system will 90% forget
		 * that a process used a lot of CPU time in 5*loadav seconds.
		 * This causes the system to favor processes which haven't run
		 * much recently, and to round-robin among other processes.
		 */

		if (cp_lock(&p->p_state, SPLHI) != CPLOCKFAIL) {
			p->p_cpticks++;
			if (++p->p_cpu == 0)
				p->p_cpu--;
			if ((p->p_cpu&3) == 0) {
				setpri(p);
				if (p->p_pri >= PUSER)
					p->p_pri = p->p_usrpri;
			}
			v_lock(&p->p_state, SPLHI);
		}

		/*
		 * Bump process PFF virtual time.
		 * If process has accumulated enough virtual time since last
		 * PFF adjustment and PFF is enabled, set up a PFF SW trap
		 * in the process to consider resident-set size adjustment.
		 *
		 * Note that if came from kernel mode, no guarantee the PFF
		 * tick will happen before process exits the kernel (eg, if
		 * process context-switches).  No problem, will post another
		 * SWT_PFF next time; want to insure it grows if faulting
		 * constantly.
		 */

		if (++u.u_pffvtime >= vmtune.vt_PFFvtime	/* enuf time */
		&&  vmtune.vt_PFFvtime > 0			/* PFF on */
		&&  (p->p_flag & SNOPFF) == 0) {		/* !blocked */
			SWTON(SWT_PFF);
		}
	}

	/*
	 * Gather statistics on resource utilization.
	 *
	 * We make a gross assumption: that the system has been in the
	 * state it is in (user state, kernel state, interrupt state,
	 * or idle state) for the entire last time interval, and
	 * update statistics accordingly.
	 *
	 * We maintain statistics shown by user-level statistics
	 * programs:  the amount of time in each cpu state, and
	 * the amount of time each of DK_NDRIVE ``drives'' is busy.
	 *
	 * The "cpu" statistics are kept on a per processor basis.
	 */

	l.cnt.v_time[cpstate]++;
}

/*
 * todclock()
 *	Time of day clock interrupt.
 *
 * Tod clock keeps track of the time of day
 * and updates real-time timeout queues.
 *
 * The gate G_TIME gates the timeout queue as well as the time vector,
 * since all accesses to the queue hold G_TIME, and
 * softclock/untimeout/setitimer also hold time_lck.  The use of two
 * "locks" allows G_TIME to have lower latency, but still give
 * deterministic softclock/untimeout/setitimer behaviour.
 */

todclock()
{
	register struct callout *p1;
	bool_t	dosoftclock = 0;
	extern int mono_P_slic;		/* If >= 0 softclock runs here */
	extern int tickdelta;
	extern long timedelta;

	/*
	 * Update real-time timeout queue.
	 * At front of queue are some number of events which are ``due''.
	 * The time to these is <= 0 and if negative represents the
	 * number of ticks which have passed since it was supposed to happen.
	 * The rest of the q elements (times > 0) are events yet to happen,
	 * where the time for each is given as a delta from the previous.
	 * Decrementing just the first of these serves to decrement the time
	 * to all events.
	 */

	VOID_P_GATE(G_TIME);
	p1 = calltodo.c_next;
	while (p1) {
		if (--p1->c_time > 0)
			break;
		dosoftclock = 1;		/* at least one to do */
		if (p1->c_time == 0)
			break;
		p1 = p1->c_next;
	}

	/*
	 * Increment the time-of-day, and schedule
	 * processing of the callouts at a very low cpu priority,
	 * so we don't keep the relatively high clock interrupt
	 * priority any longer than necessary.
	 * If a mono_P driver exists all timeouts run on the engine
	 * handling the mono_P driver.
	 */
	if (timedelta != 0) {
		register int delta;

		if (timedelta < 0) {
			delta = tick - tickdelta;
			timedelta += tickdelta;
		} else {
			delta = tick + tickdelta;
			timedelta -= tickdelta;
		}
		BUMPTIME(&time, delta);
	} else {
		BUMPTIME(&time, tick);
	}
	V_GATE(G_TIME, SPLHI);			/* assume clock at SPLHI */
	if (dosoftclock)
		sendsoft((mono_P_slic >= 0) ? mono_P_slic : l.eng->e_slicaddr,
			SOFTCLOCK);
}

/*
 * softclock()
 *	Software priority level clock interrupt.
 *
 * Run periodic events from timeout queue.
 *
 * Use drp_lock, to synchronize with untimeout. This also gates the
 * timeout queues to coordinate with timeout, and todclock.
 */

softclock()
{
	register struct callout *p1;
	register caddr_t arg;
	register int (*func)();
	register int a, s;

	for (;;) {
#ifdef	ns32000
		s = drp_lock(&time_lck);
#endif	ns32000
#ifdef	i386
		s = p_lock(&time_lck, SPLHI);
		VOID_P_GATE(G_TIME);
#endif	i386
		if ((p1 = calltodo.c_next) == 0 || p1->c_time > 0) {
			V_GATE(G_TIME, SPLHI);
			v_lock(&time_lck, s);
			break;
		}
		arg = p1->c_arg; func = p1->c_func; a = p1->c_time;
		calltodo.c_next = p1->c_next;
		p1->c_next = callfree;
		callfree = p1;
		V_GATE(G_TIME, s);	/* done with timeout queue */
#ifdef	i386
		splx(s);		/* V_GATE() doesn't lower spl */
#endif	i386
		(*func)(arg, a);
		v_lock(&time_lck, s);	/* done with timeout event */
	}
}

/*
 * timeout()
 *	Arrange that (*fun)(arg) is called in t/hz seconds.
 *
 * G_TIME gates access to the timeout queue.
 */

timeout(fun, arg, t)
	int (*fun)();
	caddr_t arg;
	int t;
{
	GATESPL(s);

	P_GATE(G_TIME, s);
	ltimeout(fun, arg, t);
	V_GATE(G_TIME, s);
}

/*
 * ltimeout()
 *	Arrange that (*fun)(arg) is called in t/hz seconds.
 *
 * Called in cases where G_TIME is already locked. Hence, the l prefix.
 */

ltimeout(fun, arg, t)
	int (*fun)();
	caddr_t arg;
	register int t;
{
	register struct callout *p1, *p2, *pnew;

	if (t == 0)
		t = 1;
	pnew = callfree;
	if (pnew == NULL)
		panic("timeout table overflow");
		/*
		 *+ The kernel internal callout table has overflowed.
		 *+ Corrective action:  either reconfigure the kernel
		 *+ with more callout table entries, or suppress the events 
		 *+ causing the timeouts.  See the NCALLOUT configuration
		 *+ parameter.
		 */
	callfree = pnew->c_next;
	pnew->c_arg = arg;
	pnew->c_func = fun;
	for (p1 = &calltodo; (p2 = p1->c_next) && p2->c_time < t; p1 = p2)
		if (p2->c_time > 0)
			t -= p2->c_time;
	p1->c_next = pnew;
	pnew->c_next = p2;
	pnew->c_time = t;
	if (p2)
		p2->c_time -= t;
}

/*
 * untimeout()
 *	remove a function timeout call from the callout structure.
 *
 * Must use time_lck *and* G_TIME to coordinate with softclock.
 * Timeout routine must not happen after caller to untimeout attempts
 * to remove timeout event. The race can happen if softclock has removed
 * the timeout event before the untimeout and executes the timeout after
 * the untimeout finds its not on the timeout queue.
 *
 * RESTRICTION: This MUST not be called from an interrupt routine as this
 * will cause a deadlock with softclock().
 */

untimeout(fun, arg)
	int (*fun)();
	caddr_t arg;
{
	register struct callout *p1, *p2;
	int s;

#ifdef	ns32000
	s = drp_lock(&time_lck);
#endif	ns32000
#ifdef	i386
	s = p_lock(&time_lck, SPLHI);
	VOID_P_GATE(G_TIME);
#endif	i386
	for (p1 = &calltodo; (p2 = p1->c_next) != 0; p1 = p2) {
		if (p2->c_func == fun && p2->c_arg == arg) {
			if (p2->c_next && p2->c_time > 0)
				p2->c_next->c_time += p2->c_time;
			p1->c_next = p2->c_next;
			p2->c_next = callfree;
			callfree = p2;
			break;
		}
	}
	V_GATE(G_TIME, SPLHI);
	v_lock(&time_lck, s);
}

/*
 * luntimeout()
 *	FUNCTION to remove a function timeout call from the callout structure.
 *
 * Returns 1 if removed, 0 if not found.
 * Assumes caller holds G_TIME *and* time_lck to coordinate with softclock.
 *
 * Timeout routine must not happen after caller to untimeout attempts
 * to remove timeout event. The race can happen if softclock has removed
 * the timeout event before the untimeout and executes the timeout after
 * the untimeout finds its not on the timeout queue.
 */

luntimeout(fun, arg)
	int (*fun)();
	caddr_t arg;
{
	register struct callout *p1, *p2;

	for (p1 = &calltodo; (p2 = p1->c_next) != 0; p1 = p2) {
		if (p2->c_func == fun && p2->c_arg == arg) {
			if (p2->c_next && p2->c_time > 0)
				p2->c_next->c_time += p2->c_time;
			p1->c_next = p2->c_next;
			p2->c_next = callfree;
			callfree = p2;
			return (1);
		}
	}
	return (0);
}

/*
 * hzto()
 *	Compute number of hz until specified time.
 *
 * Used to compute third argument to timeout() from an
 * absolute time.
 *
 * The timer gate (G_TIME) is assumed to be locked by the caller.
 * Therefore, the processor is already at SLPHI.
 */

hzto(tv)
	struct timeval *tv;
{
	register long ticks;
	register long sec;

	/*
	 * If number of milliseconds will fit in 32 bit arithmetic,
	 * then compute number of milliseconds to time and scale to
	 * ticks.  Otherwise just compute number of hz in time, rounding
	 * times greater than representable to maximum value.
	 *
	 * Delta times less than 25 days can be computed ``exactly''.
	 * Maximum value for any timeout in 10ms ticks is 250 days.
	 */

	sec = tv->tv_sec - time.tv_sec;
	if (sec <= 0x7fffffff / 1000 - 1000)
		ticks = ((tv->tv_sec - time.tv_sec) * 1000 +
			(tv->tv_usec - time.tv_usec) / 1000) / (tick / 1000);
	else if (sec <= 0x7fffffff / hz)
		ticks = sec * hz;
	else
		ticks = 0x7fffffff;
	return (ticks);
}

/*
 * profil()
 *	User-level profiling system-call.
 */

profil()
{
	register struct a {
		short	*bufbase;
		unsigned bufsize;
		unsigned pcoffset;
		unsigned pcscale;
	} *uap = (struct a *)u.u_ap;
	register struct uprof *upp = &u.u_prof;

	upp->pr_base = uap->bufbase;
	upp->pr_size = uap->bufsize;
	upp->pr_off = uap->pcoffset;
	upp->pr_scale = uap->pcscale;
}

/*
 * profswt()
 *	Software trap for profiling tick.
 *
 * Generated via SWTON(SWT_PROF) in hardclock(), called from trap() where
 * trap will return to user-mode.
 */

profswt()
{
	addupc(u.u_ar0[PC], &u.u_prof, 1);
}

#ifdef	KPRO
/*
 * prfintr()
 *	Kernel profiling clock tick.
 *
 * Currently keeps a bucket per "symbol", typically entire procedure.
 * Could increase size of buffer and keep fixed-size buckets (and faster!).
 *
 * Called per processor, at frequency hz.
 */

prfintr(pc, ps)
	register  unsigned  pc;
	int ps;
{
	register  int  h, l, m;

	if (USERMODE(ps))
		prfctr[prfmax]++;
	else {
		l = 0;
		h = prfmax;
		while ((m = (l + h) / 2) != l)
			if (pc >= prfsym[m])
				l = m;
			else
				h = m;
		prfctr[m]++;
	}
}
#endif	KPRO
