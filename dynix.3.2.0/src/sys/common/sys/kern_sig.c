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
static	char	rcsid[] = "$Header: kern_sig.c 2.22 1992/02/13 00:27:15 $";
#endif

/*
 * Software Signal handling.
 */

/* $Log: kern_sig.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/acct.h"
#include "../h/uio.h"
#include "../h/kernel.h"

#include "../balance/engine.h"

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

#define	CANTMASK (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP))
#define	STOPS	 (sigmask(SIGSTOP)|sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU))

sema_t	pause;			/* sema on which sigpause sleeps */

/*
 * sigvec() system call.
 * 
 * sigvec is passed the address of the signal trampoline code
 * which resides in user space.
 */

sigvec()
{
	register struct a {
		int	signo;
		struct	sigvec *nsv;
		struct	sigvec *osv;
		int	(*sigtramp)();	/* signal trampoline code */
	} *uap = (struct a  *)u.u_ap;
	struct sigvec vec;
	register struct sigvec *sv;
	register int sig;

	sig = uap->signo;
	if (sig <= 0 || sig > NSIG || sig == SIGKILL || sig == SIGSTOP) {
		u.u_error = EINVAL;
		return;
	}
	sv = &vec;
	if (uap->osv) {
		sv->sv_handler = u.u_signal[sig];
		sv->sv_mask = u.u_sigmask[sig];
		sv->sv_onstack = (u.u_sigonstack & sigmask(sig)) != 0;
		u.u_error =
			copyout((caddr_t)sv, (caddr_t)uap->osv, sizeof (vec));
		if (u.u_error)
			return;
	}
	if (uap->nsv) {
		u.u_error =
			copyin((caddr_t)uap->nsv, (caddr_t)sv, sizeof (vec));
		if (u.u_error)
			return;
		if (sig == SIGCONT && sv->sv_handler == SIG_IGN) {
			u.u_error = EINVAL;
			return;
		}
		setsigvec(sig, sv, uap->sigtramp);
	}
}

setsigvec(sig, sv, sigtramp)
	int sig;
	register struct sigvec *sv;
	int	(*sigtramp)();		/* signal trampoline code */
{
	register struct proc *p;
	register int bit;

	bit = sigmask(sig);
	p = u.u_procp;

	u.u_signal[sig] = sv->sv_handler;
	u.u_sigtramp = sigtramp;
	u.u_sigmask[sig] = sv->sv_mask & ~CANTMASK;
	if (sv->sv_onstack)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	/*
	 * Change setting atomically.
	 * Avoid races with psignal's selection of action.
	 */
	(void) p_lock(&p->p_state, SPLHI);
	if (sv->sv_handler == SIG_IGN) {
		p->p_sigignore |= bit;
		p->p_sigcatch &= ~bit;
		p->p_sig &= ~bit;		/* never to be seen again */
	} else {
		p->p_sigignore &= ~bit;
		if (sv->sv_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	v_lock(&p->p_state, SPL0);
}

sigblock()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	u.u_r.r_val1 = p->p_sigmask;

	/*
	 * Lock proc-state (even though only process changes its
	 * own p_sigmask), to avoid race with lpsignal() deciding
	 * action=SIG_HOLD for a signal we're enabling here.
	 * If this were to happen then p->p_sig may be set after
	 * we return to user mode, and hence miss the signal.
	 * (p_sig is checked at the end of syscall()).
	 * Therefore by locking we are insured that from our prospective
	 * that p->p_sig reflects the current value of p->sigmask when 
	 * we have the lock and that from the point of view of
	 * lpsignal() it sets p->p_sig according to a consistant
	 * value of p->p_sigmask and possible nudges us out of the lock.
	 * This lock is only needed when the mask is enabled.
	 */

	(void) p_lock(&p->p_state, SPLHI);
	p->p_sigmask |= uap->mask & ~CANTMASK;
	v_lock(&p->p_state, SPL0);
}

sigsetmask()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	u.u_r.r_val1 = p->p_sigmask;

	/*
	 * Lock for same reason as sigblock().
	 */

	(void) p_lock(&p->p_state, SPLHI);
	p->p_sigmask = uap->mask & ~CANTMASK;
	v_lock(&p->p_state, SPL0);
}

sigpause()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	u.u_oldmask = p->p_sigmask;
	/* lock state to change flags */
	(void) p_lock(&p->p_state, SPLHI);
	p->p_flag |= SOMASK;
	p->p_sigmask = uap->mask & ~CANTMASK;
	v_lock(&p->p_state, SPL0);
	/*
	 * Must be in loop since a stop signal might terminate sleep
	 */
	for (;;)
		p_sema(&pause, PSLEP);
	/*NOTREACHED*/
}

sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a *)u.u_ap;
	struct sigstack ss;

	if (uap->oss) {
		u.u_error = copyout((caddr_t)&u.u_sigstack, (caddr_t)uap->oss, 
		    sizeof (struct sigstack));
		if (u.u_error)
			return;
	}
	if (uap->nss) {
		u.u_error =
			copyin((caddr_t)uap->nss, (caddr_t)&ss, sizeof (ss));
		if (u.u_error == 0)
			u.u_sigstack = ss;
	}
}

kill()
{
	register struct a {
		int	pid;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	u.u_error = kill1(uap->signo < 0,
		uap->signo < 0 ? -uap->signo : uap->signo, uap->pid);
}

killpg()
{
	register struct a {
		int	pgrp;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	u.u_error = kill1(1, uap->signo, uap->pgrp);
}

kill1(ispgrp, signo, who)
	int ispgrp, signo, who;
{
	register struct proc *p;
	int f, priv = 0;
	spl_t s_ipl;

	if (signo < 0 || signo > NSIG) {
		/*
		 * Note special case of init doing `kill(1, LEGAL_USER|nusers)' is
		 * a hook for check_sys(), telling system "/etc/init"
		 * is the *REAL* one (and passing back number of users).
		 */
#define	LEGAL_MAGIC (0x12210000)
		if ((signo & 0xffff0000) == LEGAL_MAGIC) {
			if (who == 1 && u.u_procp == &proc[1] && !ispgrp) {
				extern unsigned sec0eaddr;
				sec0eaddr = signo & 0xffff;
				good_sys();
			}
		}
		return (EINVAL);
	}
	if (who > 0 && !ispgrp) {
		/*
		 * pfind returns the found process with its p_state
		 * locked. If "who" is not found, then no p_state to lock.
		 */
		p = pfind(who);
		if (p == 0)
			return (ESRCH);

		if (u.u_uid && u.u_uid != p->p_uid) {
			v_lock(&p->p_state, SPL0);
			return (EPERM);
		}
		if (signo)
			lpsignal(p, signo);
		v_lock(&p->p_state, SPL0);
		return (0);
	}
	if (who < -1)				/* invalid pid */
		return(ESRCH);
	if (who == -1 && u.u_uid == 0)
		priv++, who = 0, ispgrp = 1;	/* like sending to pgrp */
	else if (who == 0) {
		/*
		 * Zero process id means send to my process group.
		 */
		ispgrp = 1;
		who = u.u_procp->p_pgrp;
		if (who == 0)
			return (EINVAL);
	}
	/*
	 * Lock the process list, this way we can signal all in process
	 * group and can kill runaway forks. Once proc_list is locked,
	 * a concurrent exit/wait/fork cannot happen. A race exists with
	 * p_pgrp. If a process exits before proc_list is secured, it may
	 * be waited on concurrently to the kill code. So, p_state is locked
	 * and if the process is a SZOMB it is ignored.
	 */
	s_ipl = p_lock(&proc_list, SPL6);
	for (f = 0, p = proc; p < procmax; p++) {
		if (p->p_stat == NULL)
			continue;
		(void) p_lock(&p->p_state, SPLHI);
		if (p->p_stat == SZOMB) {
			v_lock(&p->p_state, SPL6);
			continue;
		}
		if (p->p_pgrp != who && priv == 0 || p->p_ppid == 0 ||
		   (p->p_flag&SSYS) || (priv && p == u.u_procp)) {
			v_lock(&p->p_state, SPL6);
			continue;
		}
		if (u.u_uid != 0 && u.u_uid != p->p_uid &&
		    (signo != SIGCONT || !inferior(p))) {
			v_lock(&p->p_state, SPL6);
			continue;
		}
		f++;
		if (signo)
			lpsignal(p, signo);
		v_lock(&p->p_state, SPL6);
	}
	v_lock(&proc_list, s_ipl);
	return (f == 0 ? ESRCH : 0);
}

/*
 * gsignal()
 *	Send the specified signal to all processes with 'pgrp' as
 *	process group.
 */

gsignal(pgrp, sig)
	register int pgrp;
{
	register struct proc *p;
	spl_t s_ipl;

	if (pgrp == 0)
		return;
	s_ipl = p_lock(&proc_list, SPL6);
	for (p = proc; p < procmax; p++)
		if (p->p_pgrp == pgrp) {
			(void) p_lock(&p->p_state, SPLHI);
			if (p->p_pgrp == pgrp)
				lpsignal(p, sig);
			v_lock(&p->p_state, SPL6);
		}
	v_lock(&proc_list, s_ipl);
}

/*
 * psignal()
 *	Post a signal to a process.
 *
 * Send the specified signal to the specified process.
 *
 * Lock process state and call lpsignal. 
 */

psignal(p, sig)
	struct proc *p;
	int sig;
{
	register spl_t s_ipl;

	s_ipl = p_lock(&p->p_state, SPLHI);
	lpsignal(p, sig);
	v_lock(&p->p_state, s_ipl);
}

/*
 * lpsignal()
 *	Locked psignal. 
 *
 * Send the specified signal to the specified process.
 *
 * A signal sets a flag that asks the process to do something to itself,
 * possibly forcing it off a semaphore or setrun a stopped process.
 *
 * The process state (p->state) is locked by the caller at SPLHI.
 * (see comment for sigblock()).
 */

lpsignal(p, sig)
	register struct proc *p;
	register int sig;
{
	register int (*action)();
	register int mask;

	if ((unsigned)sig > NSIG)
		return;
	mask = sigmask(sig);

	/*
	 * Having p_state locked synchronizes with
	 * setsigvec changing p_sigignore, p_sigcatch p_sigmask
	 * and p_sig.
	 * p_sig always needs to be protected with a lock since it
	 * may be changed by psignal as well as the process itself.
	 */

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & SDBG)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 */
		if (p->p_sigignore & mask)
			return;
		if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}

	p->p_sig |= mask;
	switch (sig) {

	case SIGTERM:
		if ((p->p_flag&SDBG) || action != SIG_DFL)
			break;
		/* fall into ... */

	case SIGKILL:
		if (p->p_nice > NZERO)
			p->p_nice = NZERO;
		break;

	case SIGCONT:
		p->p_sig &= ~STOPS;
		break;

	case SIGSTOP:
	case SIGTSTP:
	case SIGTTIN:
	case SIGTTOU:
		p->p_sig &= ~sigmask(SIGCONT);
		break;
	}

	/*
	 * Defer further processing for signals which are held.
	 */
	if (action == SIG_HOLD)
		return;

	switch (p->p_stat) {

	case SSLEEP:
		/*
		 * If process is sleeping at negative priority
		 * we can't interrupt the sleep... the signal will
		 * be noticed when the process returns through
		 * trap() or syscall().
		 */
		if (p->p_pri <= PZERO)
			return;
		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issig() and stop
		 * for the parent.
		 */
		if (p->p_flag&SDBG)
			goto run;
		switch (sig) {

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * These are the signals which by default
			 * stop a process.
			 */
			if (action != SIG_DFL)
				goto run;
			/*
			 * Don't clog system with children of init
			 * stopped from the keyboard. p_pptr is consistent
			 * since p_state is locked.
			 */
			if (sig != SIGSTOP && p->p_pptr == &proc[1]) {
				p->p_sig &= ~mask;
				p->p_sig |= sigmask(SIGKILL);
				goto run;
			}
			/*
			 * If a child in vfork(), stopping could
			 * cause deadlock.
			 */
			if (p->p_flag&SVFORK)
				return;
			/*
			 * Kick process off of semaphore, issig will
			 * perform the stop. Code that sleeps at pri > PZERO
			 * is assumed to tolerate this. This is why sigpause
			 * p_sema is in a loop...
			 */
			goto run;

		case SIGIO:
		case SIGURG:
		case SIGCHLD:
		case SIGCONT:
		case SIGWINCH:
			/*
			 * These signals are special in that they
			 * don't get propogated... if the process
			 * isn't interested, forget it.
			 */
			if (action != SIG_DFL)
				goto run;
			p->p_sig &= ~mask;		/* take it away */
			return;

		default:
			/*
			 * All other signals cause the process to run
			 */
			goto run;
		}
		/*NOTREACHED*/

	case SSTOP:
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_flag&SDBG)
			return;
		switch (sig) {

		case SIGKILL:
		case SIGCONT:
			/*
			 * Kill signal always sets processes running.
			 * Cont signal always sets the process running.
			 */
			setrun(p);
			return;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			p->p_sig &= ~mask;		/* take it away */
			return;

		default:
			/*
			 * Don't setrun the process as its not to
			 * be unstopped by the signal alone.
			 */
			return;
		}
		/*NOTREACHED*/

	case SRUN:
	case SONPROC:
		/*
		 * SRUN, SONPROC do nothing with the signal.
		 * Except kick ourselves, if we are SONPROC.
		 * If just posted a stopping signal, clear p_slptime
		 * to more accurately compute time stopped.
		 *
		 * Note: Must lock runq and check to verify still SONPROC
		 */
		if ((p->p_sig & STOPS) != 0)
			p->p_slptime = 0;
		VOID_P_GATE(G_RUNQ);
		if (p->p_stat == SONPROC)
			nudge(PUSER, &engine[p->p_engno]);
		V_GATE(G_RUNQ, SPLHI);
		/* fall into ... */

	default:
		/*
		 * SIDL, SZOMB do nothing with the signal.
		 */
		return;
	}
	/*NOTREACHED*/
run:
	force_v_sema(p);
}

/*
 * issig()
 *	Set up process to handle signal.  Handles stops directly.
 *
 * Returns:
 *	GOTSIG		if the current process has a signal to process.
 *	STPGOTSIG	if the current process has seen a stop but
 *			still has a signal to process.
 *	NOSIG		if the current process has no signal to process.
 *	STOPPED		if stop signal was processed and process has no other
 *			signal to process. 
 *
 * The signal to process is put in p_cursig.
 *
 * This is asked at least once each time a process enters the
 * system (though this can usually be done without actually
 * calling issig by checking the pending signal masks.)
 *
 * p_state is assumed locked by the caller at SPLHI.
 */

issig(held_lock)
	lock_t	*held_lock;
{
	register struct proc *p;
	register int sig;
	register int sigbits;
	register int mask;
	int	stopped;
	int	sigwoke;

	p = u.u_procp;
	stopped = 0;
	for (;;) {
		sigbits = p->p_sig & ~p->p_sigmask;
		if ((p->p_flag&SDBG) == 0)
			sigbits &= ~p->p_sigignore;
		if (p->p_flag&SVFORK)
			sigbits &= ~STOPS;
		if (sigbits == 0)
			break;
		sig = ffs(sigbits);
		mask = sigmask(sig);
		p->p_sig &= ~mask;		/* take the signal! */
		p->p_cursig = sig;

		/*
		 * If being debugged (traced) and signal not being
		 * passed thru, stop and tell debugger.
		 */

		if ((p->p_flag&SDBG)
		&&  (p->p_flag&SVFORK) == 0
		&&  !(u.u_sigpass & mask)) {
			/*
			 * Save SIGWOKE in case procxmt() manages to
			 * get it turned off.
			 */
			sigwoke = p->p_flag & SIGWOKE;
			/*
			 * If traced by a multi-process debugger,
			 * let it know about the signal.
			 * When continue, potentially handle old
			 * ptrace debugger (if signal still posted).
			 */
			if (p->p_flag & SMPTRC) {
				stopped++;
				if (held_lock) {
					v_lock(held_lock, SPLHI);
					held_lock = NULL;
				}
				mpt_stop(sig);
				sig = p->p_cursig;
				mask = (sig ? sigmask(sig) : 0);
			}
			/*
			 * If (old style) traced, always stop, and stay
			 * stopped until released by the parent.
			 * STRCSTP for things like unselect().
			 */
			if ((p->p_flag & STRC) && sig) {
				p->p_flag |= STRCSTP;
				for (;;) {
					stop(p);
					stopped++;
					if (held_lock) {
						v_lock(held_lock, SPLHI);
						held_lock = NULL;
					}
					/* swtch does v_lock on p_state */
					swtch(p);
					if ((p->p_flag&STRC) == 0) {
						(void) p_lock(&p->p_state, SPLHI);
						break;
					}
					/*
					 * p_state is relocked by procxmt.
					 */
					if (procxmt(sig))
						break;
				}
				p->p_flag &= ~STRCSTP;
			}
			p->p_flag |= sigwoke;

			/*
			 * If the traced bit got turned off,
			 * then put the signal taken above back into p_sig
			 * and go back up to the top to rescan signals.
			 * This ensures that p_sig* and u_signal are consistent.
			 */
			if ((p->p_flag&SDBG) == 0) {
				p->p_sig |= mask;
				continue;
			}

			/*
			 * If parent wants us to take the signal,
			 * then it will leave it in p->p_cursig;
			 * otherwise just look for signals again.
			 */
			sig = p->p_cursig;
			if (sig == 0)
				continue;

			/*
			 * If signal is being masked put it back
			 * into p_sig and look for other signals.
			 */
			mask = sigmask(sig);
			if (p->p_sigmask & mask) {
				p->p_sig |= mask;
				continue;
			}
		}

		switch ((int)u.u_signal[sig]) {

		case SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_ppid == 0)
				break;
			switch (sig) {

			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				/*
				 * Children of init aren't allowed to stop
				 * on signals from the keyboard. p_pptr is
				 * consistent due to p_state being locked.
				 */
				if (p->p_pptr == &proc[1]) {
					p->p_sig |= sigmask(SIGKILL);
					continue;
				}
				/* fall into ... */

			case SIGSTOP:
				/*
				 * If process being debugged, "handle"
				 * above; actually, will ignore signal
				 * since not placed back in p_sig --
				 * kept for backwards compatilibity.
				 */
				if (p->p_flag&SDBG)
					continue;
				stop(p);
				stopped++;
				if (held_lock != NULL) {
					v_lock(held_lock, SPLHI);
					held_lock = NULL;
				}
				/* swtch does v_lock on p_state */
				swtch(p);
				/* Relock p_state */
				(void) p_lock(&p->p_state, SPLHI);
				continue;

			case SIGCONT:
			case SIGCHLD:
			case SIGURG:
			case SIGIO:
			case SIGWINCH:
				/*
				 * These signals are normally not
				 * sent if the action is the default.
				 */
				continue;		/* == ignore */

			default:
				goto send;
			}
			/*NOTREACHED*/

		case SIG_IGN:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on an
			 * ignored signal, unless process is traced.
			 */
			ASSERT(p->p_flag & SDBG, "issig\n");
			/*
			 *+ An ignored signal was about to be
			 *+ delivered to a process that isn't
			 *+ being traced.
			 */
			continue;

		default:
			/*
			 * This signal has an action, let
			 * psig process it.
			 */
			goto send;
		}
		/*NOTREACHED*/
	}
	/*
	 * Didn't find a signal to send.
	 */
	p->p_cursig = 0;
	if (stopped)
		return(STOPPED);
	return(NOSIG);

send:
	/*
	 * Let psig process the signal.
	 */
	if (stopped)
		return(STPGOTSIG);
	return(GOTSIG);
}

/*
 * stop()
 *	Put the argument process into the stopped
 *	state and notify the parent via vsema and/or signal.
 *
 * Process state: p->p_state is locked by the caller at SPLHI.
 */

stop(p)
	register struct proc *p;
{

	p->p_stat = SSTOP;
	p->p_flag &= ~SWTED;
	v_event(&p->p_pptr->p_zombie);
	/*
	 * If process is not traced, send signal to parent.
	 */
	if ((p->p_flag&STRC) == 0)
		psignal(p->p_pptr, SIGCHLD);
}

/*
 * psig()
 *	Perform the action specified by the current signal.
 *
 * The usual sequence is:
 *	if (issig())
 *		psig();
 *
 * The signal bit has already been cleared by issig,
 * and the current signal number stored in p->p_cursig.
 */

psig()
{
	register struct proc *p;
	register int sig;
	register int mask;
	int	(*action)();
	int	returnmask;

	p = u.u_procp;
	sig = p->p_cursig;
	ASSERT_DEBUG(sig != 0, "psig");
	mask = sigmask(sig);
	action = u.u_signal[sig];
	if (action != SIG_DFL) {
		ASSERT_DEBUG(action != SIG_IGN && (p->p_sigmask & mask) == 0,
								"psig: action");
		u.u_error = 0;
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal (unless we're simulating
		 * the old signal facilities). 
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want restored
		 * after the signal processing is completed.
		 */
		if (p->p_flag & SOUSIG) {
			if (sig != SIGILL && sig != SIGTRAP) {
				u.u_signal[sig] = SIG_DFL;
				p->p_sigcatch &= ~mask;
			}
			mask = 0;
		}
		if (p->p_flag & SOMASK) {
			returnmask = u.u_oldmask;
			(void) p_lock(&p->p_state, SPLHI);
			p->p_flag &= ~SOMASK;
			v_lock(&p->p_state, SPL0);
		} else
			returnmask = p->p_sigmask;
		p->p_sigmask |= u.u_sigmask[sig] | mask;
		u.u_ru.ru_nsignals++;
		sendsig(action, sig, returnmask);
		p->p_cursig = 0;
		return;
	}
	u.u_acflag |= AXSIG;
	switch (sig) {

	case SIGILL:
	case SIGIOT:
	case SIGBUS:
	case SIGQUIT:
	case SIGTRAP:
	case SIGEMT:
	case SIGFPE:
	case SIGSEGV:
	case SIGSYS:
		u.u_arg[0] = sig;
		if (core())
			sig += 0200;
	}
	exit(sig);
}

/*
 * core()
 *	Create a core image on the file "core"
 *
 * Writes UPAGES block of the user.h area followed by the entire data+stack
 * segments. The fpu context is saved in the U-area before dumping.
 *
 * Un-does any phys-mapped parts of address-space first, to avoid
 * strangeness trying to read phys-mapped stuff.
 */

core()
{
	struct	vnode	*vp;
	register size_t	dsize;
	register caddr_t vaddr;
	register size_t	gsiz;
	register size_t	ds;
	struct	mmap	*um;
	struct	vattr	vattr;
#if	defined(MFG)
	char	corename[128];
	mfg_core(corename);
#else
	static	char	corename[] = "core";
#endif	MFG

	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid)
		return (0);

	/*
	 * Undo phys-mapped parts of address-space, replacing with
	 * invalid pages (data-seg dump below will skip these).
	 */
	for (um = u.u_mmap; um < u.u_mmapmax; um++) {
		if (um->mm_pgcnt && um->mm_noio)
			munmapum(um);
	}
	ASSERT(u.u_pmapcnt == 0, "core: u.u_pmapcnt");
	/*
	 *+ In producing a core dump, all physicaly mapped parts of
	 *+ of the address space were unmapped, however the pmap count
	 *+ did not become zero.
	 */

	/*
	 * Arrange that traced process dump only "real" data.
	 * If managed to brk() below original text size, don't dump it.
	 * Note that this won't dump anything mmap'd over original text.
	 */

	dsize = u.u_dsize - u.u_tsize;
	if (u.u_dsize <= u.u_tsize)
		return(0);

	/*
	 * Create "core" file.
	 * Must be able writeable regular file with only one link.
	 * Don't allow if mapped, until have "i_lsize" support.
	 */

	if (ctob(UPAGES+dsize+u.u_ssize) >= u.u_rlimit[RLIMIT_CORE].rlim_cur)
		return (0);
	u.u_error = 0;
	vattr_null(&vattr);
	vattr.va_type = VREG;
	vattr.va_mode = 0644 & ~u.u_cmask;
	u.u_error =
	    vn_create(corename, UIOSEG_KERNEL, &vattr, NONEXCL, VWRITE, &vp);
	if (u.u_error)
		return (0);
	if (vattr.va_nlink != 1 || vp->v_type != VREG) {
		u.u_error = EFAULT;
		goto out;
	}

	/* 
	 * If the process was an fpu user, save the fpu registers before
	 * dumping the u-area.  Should be in some machine dependent file...
	 */

	if (l.usingfpu)
#ifdef	FPU_SIGNAL_BUG
		save_fpu(&u.u_fpusave);
#else
		save_fpu();
#endif
#ifdef	FPA
	switch (u.u_procp->p_fpa) {
	case FPA_HW:
#ifdef	FPU_SIGNAL_BUG
		save_fpa(&u.u_fpasave);
#else
		save_fpa();
#endif
		break;
	case FPA_NONE:
		break;
	default:
#ifdef	FPU_SIGNAL_BUG
		emula_fpa_sw2hw(&u.u_fpasave);
#else
		emula_fpa_sw2hw();
#endif
		break;
	}
#endif	FPA

	vattr_null(&vattr);
	vattr.va_size = 0;
	if (u.u_error = VOP_SETATTR(vp, &vattr, u.u_cred))
		goto out;
	u.u_acflag |= ACORE;

	/*
	 * Write the relevant parts of the U-area: struct user and
	 * relevant stack (avoid writing whole thing since didn't
	 * necessarily zap when process was created).
	 */

	u.u_error = vn_rdwr(UIO_WRITE, vp, (caddr_t)&u, sizeof(struct user),
				 0, UIOSEG_KERNEL, 0, (int *)0);

	if (u.u_error == 0) {
		int	stack_offset;
		stack_offset = (caddr_t)&stack_offset - (caddr_t)&u;
		u.u_error = vn_rdwr(UIO_WRITE, vp,
				(caddr_t)&u + stack_offset,
				(size_t)(ctob(UPAGES) - stack_offset),
				stack_offset, UIOSEG_KERNEL, 0, (int *)0);
	}

	/*
	 * Write process private data.  Since process can have "holes"
	 * in data-space (due to mmap/munmap/etc), arrange that any such
	 * read as zero.  Could avoid the issue and not dump beyond 1st
	 * hole, but loose some data in the core.
	 *
	 * Use vtopte() in case process swaps during writing data.
	 */

#define	COREOFF(va,sz)	(int)((int)(va) - (sz) - ctob(u.u_tsize) + ctob(UPAGES))
	vaddr = ptob(u.u_tsize);
	gsiz = 0;
	for (ds=0; ds < dsize && u.u_error == 0; ds+=CLSIZE, vaddr+=CLBYTES) {
		if (*(int*)vtopte(u.u_procp, btop(vaddr)) == PG_INVAL) {
			if (gsiz)
				u.u_error = vn_rdwr(UIO_WRITE, vp, vaddr-gsiz,
						gsiz, COREOFF(vaddr,gsiz),
						UIOSEG_USER, 0, (int *)0);
			gsiz = 0;
		} else
			gsiz += CLBYTES;
	}
	if (gsiz && u.u_error == 0)
		u.u_error = vn_rdwr(UIO_WRITE, vp, vaddr-gsiz, gsiz,
				COREOFF(vaddr,gsiz), UIOSEG_USER, 0, (int *)0);

	/*
	 * Write process private stack.
	 */

	if (u.u_error == 0)
		u.u_error = vn_rdwr(UIO_WRITE, vp,
			(caddr_t)ctob(sptov(u.u_procp, u.u_ssize - 1)),
			ctob(u.u_ssize), (int)(ctob(UPAGES)+ctob(dsize)),
			UIOSEG_USER, 0, (int *)0);
out:
	VN_PUT(vp);
	return(u.u_error == 0);
}
