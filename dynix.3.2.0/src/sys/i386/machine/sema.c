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
static	char	rcsid[] = "$Header: sema.c 2.9 90/08/24 $";
#endif

/*
 * sema.c
 *	C Version of Semaphore routines.
 *
 * i386 version.
 */

/* $Log:	sema.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

/*
 * DEBUG and MFG kernels enable "logging".
 * In this implementation, "logging" stores pid of process that last did a
 * "p_sema" in the semaphore's s_flags field.  This is only 8-bits, so it
 * can be ambiguous (but typically useful enough).
 */

#if	defined(DEBUG) || defined(MFG)
#include "../h/vmmeter.h"
#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"			/* need "l." for cp_sema() */
#define	SEMLOG
#define SCOUNT_RANGE	1000		/* Don't let semaphores wrap */
#endif

/*
 * init_sema()
 *	init the semaphore.
 *
 * This is now a macro in machine/mutex.h
 */

/*
 * p_sema()
 *	P a semaphore.
 *
 * If priority is greater than PZERO signals may wake up the sleeping
 * process.
 *
 * If pri > PZERO and signal pending, then longjmp out.
 * If there is no signal to process, but a STOP signal was processed
 * by issig, return as if the process was V'ed. This is because the
 * STOP can last a very long time and the whole world may have changed.
 * Therefore, the caller must recheck the situation again and call
 * p_sema again if appropriate.
 * Otherwise, decrement the count. If count >= 0, return.
 * Otherwise, put self on semaphore queue, switch away, and sleep
 * waiting for somebody to v_sema().
 *
 * Returns at spl0.
 */

void
p_sema(sema, pri)
	register sema_t *sema;
	int pri;
{
	register struct	proc *p;
	/* spl_t	s;			/* not used in 2nd-gen */

	ASSERT_DEBUG(sema->s_count > -SCOUNT_RANGE, "p_sema wraped sema count");

	if (pri <= PZERO) {
		/*
		 * If can maybe get it, go for it.  If not, need to lock
		 * proc-state before lock sema gate, since gate only masks
		 * interrupt at processor, not SLIC, and when release gate
		 * could take queued interrupt, then deadlock.
		 *
		 * This optimizes case where typically get the sema (eg,
		 * buf-cache hit).
		 */
		if (sema->s_count > 0) {
			P_GATE(sema->s_gate, s);
			if (sema->s_count > 0) {
				--sema->s_count;
#ifdef	SEMLOG
				sema->s_flags = u.u_procp->p_pid;
#endif	SEMLOG
				V_GATE(sema->s_gate, s);
				return;
			}
			V_GATE(sema->s_gate, s);	/* lost race */
		}
		/*
		 * Sema wasn't avail or lost race.  Grab proc-state, then
		 * go for sema again.  Need to handle race with a V().
		 */
		p = u.u_procp;
		(void) p_lock(&p->p_state, SPLHI);
		VOID_P_GATE(sema->s_gate);
		if (--sema->s_count >= 0) {		/* raced with a V() */
#ifdef	SEMLOG
			sema->s_flags = p->p_pid;
#endif	SEMLOG
			V_GATE(sema->s_gate,  SPLHI);
			v_lock(&p->p_state, SPL0);
			return;
		}
		/*
		 * Sema not available.
		 */
	} else {
		int	sigret;
		/*
		 * Pri > PZERO
		 */
		p = u.u_procp;
		(void) p_lock(&p->p_state, SPLHI);
		if (QUEUEDSIG(p) && (sigret = issig((lock_t *)NULL)) != NOSIG) {
			p->p_flag |= SIGWOKE;
			v_lock(&p->p_state, SPL0);
			/*
			 * If the process has been stopped - return.
			 * Since the whole world may have changed while
			 * the process was stopped. The caller is
			 * assumed to tolerate this.
			 */
			if (sigret == STOPPED)
				return;
			longjmp(&u.u_qsave);
		}

		VOID_P_GATE(sema->s_gate);
		if (--sema->s_count >= 0) {
			V_GATE(sema->s_gate, SPLHI);
			v_lock(&p->p_state, SPL0);
			return;
		}
	}

	/*
	 * Semaphore unavailable, so go to sleep.
	 */

	insque(p, sema->s_tail);
	V_GATE(sema->s_gate, SPLHI);		/* proc-state still locked */

	p->p_stat = SSLEEP;
	p->p_wchan = sema;
	p->p_slptime = 0;
	p->p_pri = pri;
	u.u_ru.ru_nvcsw++;			/* voluntary context switch */
	swtch(p);				/* returns at spl0 */

	if (p->p_flag & SIGWOKE) {
		(void) p_lock(&p->p_state, SPLHI);
		if (issig((lock_t *)NULL) > 0) {
			/*
			 * take the signal
			 */
			v_lock(&p->p_state, SPL0);
			longjmp(&u.u_qsave);
		} else
			v_lock(&p->p_state, SPL0);
		/*
		 * Signal that disturbed sleep is no more.
		 */
	}
#ifdef	SEMLOG
	sema->s_flags = p->p_pid;
#endif	SEMLOG
}

/*
 * cp_sema( &sema )
 *	Conditionally P a semaphore.
 *
 * If the count is positive, decrement and return TRUE; else return
 * FALSE.
 */

bool_t
cp_sema(sema)
	register sema_t *sema;
{
	/* register spl_t s_ipl;		/* not used on SGS */

	/*
	 * First check without going for gate.
	 */

	ASSERT_DEBUG(sema->s_count > -SCOUNT_RANGE, "cp_sema wraped sema count");

	if (!sema_avail(sema))
		return(0);

	P_GATE(sema->s_gate, s_ipl);
	if (sema->s_count > 0) {
		--sema->s_count;
#ifdef	SEMLOG
		/*
		 * cp_sema() can be called from interrupt routine on idle
		 * processor -- careful about reading thru u.u_procp.
		 */
		sema->s_flags = (l.noproc ? 0 : u.u_procp->p_pid);
#endif	SEMLOG
		V_GATE(sema->s_gate, s_ipl);
		return(1);
	} else {
		V_GATE(sema->s_gate, s_ipl);
		return(0);
	}
}

/*
 * v_sema( &sema )
 *	V a semaphore.
 *
 * Bump the count.  If there are waiters, wake up the first.
 *
 * Coordinates with force_v_sema() to insure only one wakeup of a
 * give process occurs.
 */

void
v_sema(sema)
	register sema_t *sema;
{
	register struct proc *p;
	spl_t	s_ipl;

	ASSERT_DEBUG(sema->s_count < SCOUNT_RANGE, "v_sema wraped sema count");

	P_GATE(sema->s_gate, s_spl);

	/*
	 * Bump the count.  > 0 ==> return.
	 */

	if (++sema->s_count > 0) {
		V_GATE(sema->s_gate, s_spl);
		return;
	}

	/*
	 * Pull 1st guy off queue, release gate, and make
	 * process ready to run.
	 *
	 * Ok to v_gate() here once process is dequeued IFF we
	 * zap proc's links to tell force_v_sema() that the process is
	 * removed (ie, avoid race with psignal()).  The process
	 * can't be dispatched since it's not on run-Q.  Thus, only
	 * fuss with the process is swapping daemon who
	 * also locks process-state.
	 *
	 * Reset SIGWOKE flag in process-flags to tell p_sema()
	 * a v_sema() woke the process up, not a force_v_sema().
	 *
	 * Note that once setrq(), the process *can* dispatch.
	 */

	p = sema->s_head;
	remque(p);
	p->p_link = NULL;		/* for interlock with force_v_sema() */
	V_GATE(sema->s_gate, s_spl);

	s_ipl = p_lock(&p->p_state, SPLHI);

	p->p_flag &= ~SIGWOKE;		/* v_sema(), not force_v_sema() */
	p->p_stat = SRUN;
	p->p_wchan = (sema_t *)NULL;
	if (p->p_flag & SLOAD) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
		v_lock(&p->p_state, s_ipl);
	} else {
		v_lock(&p->p_state, s_ipl);
		v_sema(&runout);
	}
}

/*
 * force_v_sema(procptr)
 *	Yank a (sleeping) process off a semaphore Q and make it
 *	runnable.
 *
 * Coordinates with v_sema() to insure only one wakes up the process.
 * Set SIGWOKE to indicate to p_sema() that a force_v_sema() did
 * the wakeup.
 *
 * Assumes caller locked process state.  Process state remains locked.
 * Note: there is no consistency check that the process is actually
 * on the semaphores queue here.
 */

void
force_v_sema(p)
	register struct proc *p;
{
	register sema_t *sema;
	/* spl_t s_ipl;				/* Not used on SGS */

	sema = p->p_wchan;

	ASSERT_DEBUG(sema->s_count < SCOUNT_RANGE, "force_v_sema wraped sema count");

	P_GATE(sema->s_gate, s_ipl);

	/*
	 * If v_sema() got to the process first, let it go;
	 * v_sema() will make it run.
	 */

	if (p->p_link == NULL) {
		V_GATE(sema->s_gate, s_ipl);
		return;
	}

	remque(p);			/* yank it out of double-link list */
	++sema->s_count;		/* well, we are waking it up! */
	V_GATE(sema->s_gate, s_ipl);

	p->p_flag |= SIGWOKE;		/* force_v_sema(), not v_sema() */
	p->p_stat = SRUN;
	p->p_wchan = (sema_t *)NULL;
	if(p->p_flag & SLOAD) {
		P_GATE(G_RUNQ, s_ipl);
		setrq(p);
		V_GATE(G_RUNQ, s_ipl);
	} else
		v_sema(&runout);
}

/*
 * cv_sema(sema)
 *	Conditionally V a semaphore.
 *
 * Return a boolean saying if a process was
 * awoken or not (ie, for vall_sema()).
 */

bool_t
cv_sema(sema)
	register sema_t *sema;
{
	register struct proc *p;
	spl_t	s_ipl;		/* saved ipl */

	/*
	 * first check without going for gate.
	 */

	ASSERT_DEBUG(sema->s_count < SCOUNT_RANGE, "cv_sema wraped sema count");

	if (!blocked_sema(sema))
		return(0);

	P_GATE(sema->s_gate, s_ipl);

	if (sema->s_count >= 0) {
		V_GATE(sema->s_gate, s_ipl);		/* lost race */
		return(0);
	}

	++sema->s_count;
	p = sema->s_head;
	remque(p);
	p->p_link = NULL;		/* for interlock with force_v_sema() */
	V_GATE(sema->s_gate, s_ipl);

	s_ipl = p_lock(&p->p_state, SPLHI);

	p->p_flag &= ~SIGWOKE;		/* v_sema(), not force_v_sema() */
	p->p_stat = SRUN;
	p->p_wchan = (sema_t *)NULL;
	if(p->p_flag & SLOAD) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
		v_lock(&p->p_state, s_ipl);
	} else {
		v_lock(&p->p_state, s_ipl);
		v_sema(&runout);
	}

	return(1);
}

/*
 * vall_sema(sema)
 *	Wake up all processes sleeping on the semaphore.
 *
 * The "qlen" variable avoids a problem of using a vanilla vall();
 * i.e. don't get in a loop waking up processes, which
 * dispatch on another processor and *re-block* on the sema, only
 * to be awoken again because the vall() didn't finish yet!
 *
 * Note that this open "windows" where another P() or V() could
 * occur.  The caller is assumed to tolerate this.
 */

void
vall_sema(sema)
	sema_t	*sema;
{
	register int qlen;

	qlen = sema_count(sema);
	while (qlen++ < 0 && cv_sema(sema))
		continue;
}

/*
 * p_sema_v_lock()
 *	Get self queued on the semaphore (if must sleep) and
 *	*then* v_lock() the lock.  If not going to sleep, just
 *	v_lock() the lock before returning.
 *
 * Just like p_sema(), but v_lock(held_lock,spl) just
 * before swtch() or return (depending on if we're going
 * to sleep). Also, the same STOP semantics as p_sema are enforced.
 * If issig is going to process a STOP, then it releases the "held_lock"
 *
 * ns32000 version has 4th parameter: return SPL; this is unused here.
 *
 * Returns at spl0.
 */

/*VARARGS3*/
void
p_sema_v_lock(sema, pri, lock)
	register sema_t	*sema;
	int	pri;
	lock_t	*lock;
{
	register struct	proc *p;
	spl_t	s;

	ASSERT_DEBUG(sema->s_count > -SCOUNT_RANGE, "p_sema_v_lock wraped sema count");

	if (pri <= PZERO) {
		/*
		 * If can maybe get it, go for it.  If not, need to lock
		 * proc-state before lock sema gate, since gate only masks
		 * interrupt at processor, not SLIC, and when release gate
		 * could take queued interrupt, then deadlock.
		 *
		 * This optimizes case where typically get the sema (eg,
		 * buf-cache hit).
		 */
		if (sema->s_count > 0) {
			P_GATE(sema->s_gate, s);
			if (sema->s_count > 0) {
				--sema->s_count;
#ifdef	SEMLOG
				sema->s_flags = u.u_procp->p_pid;
#endif	SEMLOG
				V_GATE(sema->s_gate, s);
				v_lock(lock, SPL0);
				return;
			}
			V_GATE(sema->s_gate, s);	/* lost race */
		}
		/*
		 * Sema wasn't avail or lost race.  Grab proc-state, then
		 * go for sema again.  Need to handle race with a V().
		 */
		p = u.u_procp;
		(void) p_lock(&p->p_state, SPLHI);
		VOID_P_GATE(sema->s_gate);
		if (--sema->s_count >= 0) {		/* raced with a V() */
#ifdef	SEMLOG
			sema->s_flags = p->p_pid;
#endif	SEMLOG
			V_GATE(sema->s_gate, SPLHI);
			v_lock(&p->p_state, SPLHI);
			v_lock(lock, SPL0);
			return;
		}
		/*
		 * Sema not available.
		 */
	} else {
		int	sigret;
		/*
		 * Pri > PZERO
		 */
		p = u.u_procp;
		s = p_lock(&p->p_state, SPLHI);
		if (QUEUEDSIG(p) && (sigret = issig(lock)) != NOSIG) {
			p->p_flag |= SIGWOKE;
			v_lock(&p->p_state, s);
			/*
			 * If the process has been stopped - return.
			 * Since the whole world may have changed while
			 * the process was stopped. The caller is
			 * assumed to tolerate this.  Since stopped, return
			 * at spl0 (in case held lock, need to avoid splx
			 * nesting problems).
			 */
			if (sigret == STOPPED) {
				(void) spl0();
				return;
			}
			/*
			 * If the process hasn't been stopped,
			 * then release the lock before longjmp'n
			 * If GOTSIG the held_lock has been
			 * released by issig().
			 */
			if (sigret == GOTSIG)
				v_lock(lock, SPL0);
			(void) spl0();
			longjmp(&u.u_qsave);
		}

		VOID_P_GATE(sema->s_gate);
		if (--sema->s_count >= 0) {
			V_GATE(sema->s_gate, SPLHI);
			v_lock(&p->p_state, SPLHI);
			v_lock(lock, SPL0);
			return;
		}
	}

	/*
	 * Semaphore unavailable, so go to sleep.
	 */

	insque(p, sema->s_tail);
	V_GATE(sema->s_gate, SPLHI);		/* proc-state still locked */
	v_lock(lock, SPLHI);			/* for v_lock() */

	p->p_stat = SSLEEP;
	p->p_wchan = sema;
	p->p_slptime = 0;
	p->p_pri = pri;
	u.u_ru.ru_nvcsw++;			/* voluntary context switch */
	swtch(p);				/* returns at spl0 */

	if (p->p_flag & SIGWOKE) {
		(void) p_lock(&p->p_state, SPLHI);
		if (issig((lock_t *)NULL) > 0) {
			/*
			 * take the signal
			 */
			v_lock(&p->p_state, SPL0);
			longjmp(&u.u_qsave);
		} else
			v_lock(&p->p_state, SPL0);
		/*
		 * Signal that disturbed sleep is no more.
		 */
	}
#ifdef	SEMLOG
	sema->s_flags = p->p_pid;
#endif	SEMLOG
}

/*
 * v_event( &sema )
 *	V an event semaphore.
 *
 * Bump the count.  If there are waiters, wake up the first.
 * Dont let the count go above 1 - just let p_event know somthing
 * happened not how many.
 *
 * Coordinates with force_v_sema() to insure only one wakeup of a
 * given process occurs.
 * (Only one process ought to be waiting).
 */


void
v_event(sema)
	register sema_t *sema;
{
	register struct proc *p;
	spl_t	s_ipl;

	ASSERT_DEBUG(sema->s_count < SCOUNT_RANGE, "v_sema wraped sema count");

	P_GATE(sema->s_gate, s_spl);

	/*
	 * Bump the count.  > 0 ==> return.
	 */

	if (++sema->s_count > 0) {
		sema->s_count = 1;      /* nail it down */
		V_GATE(sema->s_gate, s_spl);
		return;
	}

	/*
	 * Pull 1st guy off queue, release gate, and make
	 * process ready to run.
	 *
	 * Ok to v_gate() here once process is dequeued IFF we
	 * zap proc's links to tell force_v_sema() that the process is
	 * removed (ie, avoid race with psignal()).  The process
	 * can't be dispatched since it's not on run-Q.  Thus, only
	 * fuss with the process is swapping daemon who
	 * also locks process-state.
	 *
	 * Reset SIGWOKE flag in process-flags to tell p_sema()
	 * a v_sema() woke the process up, not a force_v_sema().
	 *
	 * Note that once setrq(), the process *can* dispatch.
	 */

	p = sema->s_head;
	remque(p);
	p->p_link = NULL;		/* for interlock with force_v_sema() */
	V_GATE(sema->s_gate, s_spl);

	s_ipl = p_lock(&p->p_state, SPLHI);

	p->p_flag &= ~SIGWOKE;		/* v_sema(), not force_v_sema() */
	p->p_stat = SRUN;
	p->p_wchan = (sema_t *)NULL;
	if (p->p_flag & SLOAD) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
		v_lock(&p->p_state, s_ipl);
	} else {
		v_lock(&p->p_state, s_ipl);
		v_sema(&runout);
	}
}
