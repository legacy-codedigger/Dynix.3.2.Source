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
static	char	rcsid[] = "$Header: kern_exit.c 2.26 91/03/11 $";
#endif

/*
 * kern_exit.c
 *	Exit() and wait() system calls.
 */

/* $Log:	kern_exit.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/mbuf.h"
#include "../h/vnode.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

/*
 * rexit()
 *	Exit system call: pass back caller's arg.
 */

rexit()
{
	register struct a {
		int	rval;
	} *uap;

	uap = (struct a *)u.u_ap;
	exit((uap->rval & 0377) << 8);
}

/*
 * exit()
 *	Calling process dies.
 *
 * Allow custom system calls a chance to deal with exit; call when
 * process image and all file descriptors still exist.
 * Release resources.
 * Save u. area statistics for parent to look at.
 * Enter zombie state.
 * Wake up parent and init processes, and dispose of children.
 */

int	log_exit_nombufs;		/* # times couldn't get one */

exit(rv)
	int rv;
{
	register struct proc *p;
	register struct proc *q;
	register struct proc *next;
	register int i;
	struct	mbuf	*m = m_getclr(M_DONTWAIT, MT_ZOMBIE);
	struct	mmap	*um;
	int	x;
	spl_t	s;

	p = u.u_procp;

	/*
	 * If a vfork child, give VM resources back to the parent.
	 * Done here to avoid deadlock with SMPTRC (debugger is parent).
	 */

	if (p->p_flag & SVFORK) {
		/*
		 * Ok to race with parent switching VM resources here,
		 * since we're not swappable and p_sema doesn't need
		 * those resources.  No need to clear SVFORK and SWTOK,
		 * since new fork into this proc-slot writes new flags
		 * (and zaps p_noswap).
		 */
		++p->p_noswap;
		cust_sys_exit();
		v_sema(&p->p_pptr->p_vfork);
		p_sema(&p->p_vfork, PZERO-1);
	}

	/*
	 * If process traced by a multi-process debugger, tell it we're
	 * about to die, then remove self from debugger's list.  These
	 * can race with debugger exiting and turning off SMPTRC.
	 */

	if (p->p_flag & SMPTRC) {
		(void) p_lock(&p->p_state, SPLHI);
		if (p->p_flag & SMPTRC)
			mpt_stop(PTS_EXIT);
		v_lock(&p->p_state, SPL0);
		mpt_remove_process(p);
	}

	p->p_sigignore = ~0;			/* redundant */

	disable_fpu();				/* not using FPU anymore */
#ifdef	FPA
	if (p->p_fpa) disable_fpa();		/* not using FPA anymore */
#endif	FPA

	untimeout(realitexpire, (caddr_t)p);

	/*
	 * Close all files here, to drop references to file-table entries
	 * that might be mapped.
	 */
	ofile_deref(u.u_ofile_tab);
	if (u.u_fpref != NULL) {
		/*
		 * Got here without exiting from syscall that bump'd reference.
		 * Only current case is in procxmt() for PT_KILL.
		 */
		closef(u.u_fpref);
	}

	/*
	 * Mark maps associated with file descriptors that are going away
	 * (optimization to avoid sync'ng out mapped file).
	 */

	for (um = u.u_mmap; um < u.u_mmapmax; um++) {
		if (um->mm_fdidx >= 0 && um->mm_pgcnt
		&&  file[um->mm_fdidx].f_count == 1
		)
			um->mm_lastfd = 1;
	}

	/*
	 * Release virtual memory, if not a vfork.  Handled vfork case above.
	 */

	if ((p->p_flag & SVFORK) == 0) {
		p->p_noswap++;				/* avoid swaps */
		cust_sys_exit();
		vrelvm();
	}

	VN_RELE(u.u_cdir);
	if (u.u_rdir) {
		VN_RELE(u.u_rdir);
	}
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	semexit();
	acct(rv);
	crfree(u.u_cred);

	l.multprog--;
	/*
	 * Code in panic/pause_self assumes that noproc is set before
	 * using the private stack/pages. This is true currently
	 * but if it changes, need to mod the way fpa state is
	 * saved on panic.
	 */
	l.noproc = 1;		/* Set so hardclock doesn't mess u.u_ru */

	ASSERT(p->p_pid != 1, "exit: init died");
	/*
	 *+ The init process has exited.
	 */
	p->p_xstat = rv;
	if (m == 0) {
		p->p_ru = (struct rusage *)0;		/* loose some stats */
		++log_exit_nombufs;
	} else {
		p->p_ru = mtod(m, struct rusage *);
		*p->p_ru = u.u_ru;
		ruadd(p->p_ru, &u.u_cru);
	}

	/*
	 * Decrement processor affinity count if applicable.
	 * Do this here, since we've done all the device closing/etc
	 * that this process will do.
	 */

	if (p->p_affinity != ANYENG) {
		s = p_lock(&engtbl_lck, SPLHI);
		l.eng->e_count--;
		v_lock(&engtbl_lck, s);
	}

	/*
	 * Remove process from hash list.
	 */

	s = p_lock(&proc_list, SPL6);

	i = PIDHASH(p->p_pid);
	x = p - proc;
	if (pidhash[i] == x)
		pidhash[i] = p->p_idhash;
	else {
		for(i = pidhash[i]; i != 0; i = proc[i].p_idhash)
			if (proc[i].p_idhash == x) {
				proc[i].p_idhash = p->p_idhash;
				goto found;
			}
		panic("exit: pidhash");
		/*
		 *+ An exiting process's pid was not found
		 *+ on the process pid hash list.
		 */
	}
found:
	/*
	 * Give children to init process.
	 * If the process has any children, tell init about them.
	 *
	 * proc_list and p_state must be locked (in that order) when
	 * changing the p_pptr of an active process (non-ZOMBIE).
	 * Therefore, use p_pptr (read it) locking one or the other is
	 * sufficient. p_state is also needed locked for signal purposes;
	 * lpsignal() and access to p_sig in particular.
	 */

	for(q = p->p_cptr; q; q = next) {
		next = q->p_sptr;
		q->p_sptr = proc[1].p_cptr;
		proc[1].p_cptr = q;

		(void) p_lock(&q->p_state, SPLHI);
		q->p_pptr = &proc[1];
		q->p_ppid = 1;

		/*
		 * Traced processes are killed since their existence
		 * means someone is screwing up.
		 * Stopped processes are sent a hangup and a continue
		 * (unless stopped due to a MP debugger -- these are
		 * handled below if exiting process is the debugger).
		 * This is designed to be ``safe'' for setuid
		 * processes since they must be willing to tolerate
		 * hangups anyways.
		 *
		 * Having locked state avoids race where process is trying
		 * to stop, uses original parent pointer, and stops such
		 * that the below test of SSTOP "wins" the race and doesn't
		 * see the SSTOP transition (see issig()).
		 * This could have caused init to have stopped, ~STRC child.
		 * [This can't happen on 4.2bsd since there was an spl5()
		 * above which precludes terminal signals, no other process
		 * is running to be able to send the signal, and the child
		 * isn't running and deciding to stop.]
		 */
		if (q->p_flag&STRC) {
			q->p_flag &= ~STRC;
			lpsignal(q, SIGKILL);
			/*
			 * If stopped waiting to be continued by old-style
			 * debugger, wake it up (unusual case of MP debugger
			 * debugging old-style debugger, and old-style exits).
			 */
			if ((q->p_flag & (SMPTRC|STRCSTP)) == (SMPTRC|STRCSTP))
				setrun(q);
		} else if (q->p_stat == SSTOP && !(q->p_flag&SMPSTOP)) {
			lpsignal(q, SIGHUP);
			lpsignal(q, SIGCONT);
		}
		/*
		 * Protect this process from future
		 * tty signals, clear TSTP/TTIN/TTOU if pending.
		 */
		q->p_sig &= ~(sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU));
		v_lock(&q->p_state, SPL6);
		/*
		 * Clear TSTP/TTIN/TTOU for children and their descendants.
		 */
		clrttysig(q);
	}
	if (p->p_cptr) {
		v_event(&proc[1].p_zombie);
		p->p_cptr = NULL;			/* no more kids */
	}

	/*
	 * If process is a multi-process debugger, release debug "children".
	 * Any SMPSTOP actual children were left alone, above.
	 * Note: proc_list still held.
	 */

	if (p->p_flag & SMPDBGR) {
		for(q = &proc[p->p_mptc]; q != &proc[0]; q = &proc[q->p_mpts]) {
			(void) p_lock(&q->p_state, SPLHI);
			q->p_flag &= ~SMPTRC;
			/*
			 * SMPSTOP ==> if hung in mpt_stop(), let it continue,
			 * unless it was started by the above setrun() and
			 * not yet cleared SMPSTOP (race on locking p_state).
			 */
			if ((q->p_flag & SMPSTOP) && (q->p_stat == SSTOP))
				setrun(q);
			v_lock(&q->p_state, SPL6);
		}
	}

	v_lock(&proc_list, s);

#ifdef	FPA
	/*
	 * Free any auxiliary data structures
	 */
	shadow_exit(p);
#endif	/* FPA */

	/*
	 * Need to release U-area (which contains current stack), and
	 * page-tables (which may contain per-process kernel level-1 PT,
	 * depending on the HW).
	 *
	 * Since TMP, can't assume memory stays around thru swtch, thus must
	 * switch to private stack.  Must not block once this is done.
	 * Also must not use old values of stack variables or arguments
	 * since stack is gone.
	 *
	 * This code assumes memfree()/etc use lock/gate to lock the
	 * memory free-lists.  It won't work if mem_alloc is a semaphore,
	 * since must not block once on private stack.
	 */

	ASSERT_DEBUG(p->p_szpt != 0, "exit: szpt");

	use_private();				/* get on private stack */
	vrelpt(p);				/* Release page-tables */
	vrelu(p, 0);				/* Release U-area */

	/*
	 * Just about dead.  Lock self state so we can become a zombie
	 * and tell parent that we just died.  Parent wait() synch's
	 * with this by locking process state.
	 */

	(void) p_lock(&p->p_state, SPLHI);
	p->p_stat = SZOMB;
	p->p_flag &= ~STRC;
	psignal(p->p_pptr, SIGCHLD);
	v_event(&p->p_pptr->p_zombie);
	v_lock(&p->p_state, SPL0);
	swtch((struct proc *)NULL);	/* cast is for lint! */
	/* Dead Babe */
}

/*
 * clrttysig - Clear tty signals. Called from exit.
 *
 * Protect this process and all descendants from future
 * tty signals. That is, clear TSTP/TTIN/TTOU if pending.
 *
 * The caller must have proc_list locked at SPL6 to ensure
 * p-c-s chain integrity.
 *
 * Unclear that clearing pending signals here performs any
 * useful function.  This should be removed if not necessary.
 */

clrttysig(top)
	register struct proc *top;
{
	register struct proc *p;
	spl_t s_ipl;

	p = top;
	for(;;) {
		s_ipl = p_lock(&p->p_state, SPLHI);
		p->p_sig &= ~(sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU));
		v_lock(&p->p_state, s_ipl);
		/*
		 * Search for children.
		 */
		if (p->p_cptr) {
			p = p->p_cptr;
			continue;
		}
		/*
		 * Search for siblings.
		 */
		for(;;) {
			if (p == top)
				return;		/* don't include top's */
			if (p->p_sptr) {
				p = p->p_sptr;
				break;
			}
			p = p->p_pptr;
		}
	}
	/*NOTREACHED*/
}

/*
 * wait()
 *	Wait system call.
 *
 * C-bit is on for wait3(), else older flavor wait().
 * This interface should be split into two seperate interfaces.
 */
#ifdef	i386
/*
 * i386 kernel uses C-flag (carry) to distinguish, and
 * wait3() args are in u_arg[].
 */
#endif	i386

wait()
{
	struct rusage ru, *rup;

#ifdef	ns32000
	if ((u.u_ar0[MODPSR] & (PSR_C<<PSRADJ)) == 0) {
		u.u_error = wait1(0, (struct rusage *)0);
		return;
	}
	rup = (struct rusage *)u.u_ar0[R2];
	u.u_error = wait1(u.u_ar0[R1], &ru);
#endif	ns32000

#ifdef	i386
	if ((u.u_ar0[FLAGS] & FLAGS_CF) == 0) {
		u.u_error = wait1(0, (struct rusage *)0);
		return;
	}
	rup = (struct rusage *)u.u_arg[1];
	u.u_error = wait1(u.u_arg[0], &ru);
#endif	i386

	if (u.u_error)
		return;
	if (rup)
		u.u_error = copyout((caddr_t)&ru, (caddr_t)rup, sizeof (struct rusage));
}

/*
 * wait1()
 *	Wait system call implementation.
 *
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped (traced) children,
 * and pass back status from them.
 */

wait1(options, ru)
	register int options;
	struct rusage *ru;
{
	register struct proc *p;
	register struct proc *q;
	register struct proc *child;
	static struct rusage nullru;		/* use when no mbufs when exited */
	spl_t	s;

	p = u.u_procp;
	/*
	 * For each iteration re-init sema since we're about to
	 * scan the children.  A concurrent exit of a child can
	 * be missed, but it will v_sema() our p_zombie, so
	 * we'll not block below.
	 */

	clear_event(&p->p_zombie, s);

	for(;;) {

		q = NULL;
		for(child = p->p_cptr; child; q = child, child = child->p_sptr) {
			if (child->p_stat == SZOMB) {

				/*
				 * Synch with exiting child: insure it's gone.
				 * Then copy status/statistics info.
				 * If signal attempt is made after this, it 
				 * will no longer find the pid.
				 */

				s = p_lock(&child->p_state, SPLHI);
				u.u_r.r_val1 = child->p_pid;
				u.u_r.r_val2 = child->p_xstat;
				child->p_pid = 0;
				child->p_xstat = 0;
				child->p_pgrp = 0;	/* for gsignal */
				v_lock(&child->p_state, s);

				if (ru) {
					if (child->p_ru)
						*ru = *child->p_ru;
					else
						*ru = nullru;
				}
				if (child->p_ru) {
					ruadd(&u.u_cru, child->p_ru);
					(void) m_free(dtom(child->p_ru));
				}

				/*
				 * Put child to rest.
				 */

				child->p_ru = 0;
				child->p_sig = 0;
				child->p_wchan = NULL;
				child->p_cursig = 0;
				child->p_sigcatch = 0;
				child->p_sigmask = 0;
				child->p_sigignore = 0;

				/*
				 * Adjust procmax down if we were the
				 * high-water mark, and free child proc[]
				 * table entry.
				 * Also, remove child from p-c-s chain. This
				 * needs to be protected for proc 1, but since
				 * we are going to lock proc_list anyhow, we
				 * will do it all the time within the lock.
				 * We handle the low-probability race with
				 * proc[1]'s p-c-s list as a special case.
				 */

				s = p_lock(&proc_list, SPL6);
				if (child == p->p_cptr)
					p->p_cptr = child->p_sptr;
				else {
					if (q == NULL) {
						ASSERT(p == &proc[1], "wait1: list");
						/*
						 *+ A process other than init 
						 *+ is searching a
						 *+ parent-child-sibling linked
						 *+ list other than its own.
						 */
						for(q = p->p_cptr; q; q = q->p_sptr)
							if (q->p_sptr == child)
								break;
						ASSERT(q != NULL, "wait1: list1");
						/*
						 *+ init has discovered an 
						 *+ error in the 
						 *+ parent-child-sibling
						 *+ linked list of an exiting
						 *+ process.
						 */
					}
					q->p_sptr = child->p_sptr;
				}
				child->p_pptr = NULL;
				/* child cleared self p_cptr in exit() */

				if (child+1 == procmax) {
					for(q = child-1; q > &proc[1]; q--) {
						if (q->p_stat != NULL)
							break;
					}
					procmax = q+1;
				}
				child->p_stat = NULL;	/* free entry */
				v_lock(&proc_list, s);
				return (0);
			}

			/*
			 * If stopped child, then return it if tracing or
			 * interested in stopped children.
			 * Ok to check state w/o entering child state lock
			 * since signal/v_sema to parent is done after child
			 * changes state to SSTOP (HW specific proof of this).
			 * We must check p_stat again since the child may
			 * have received a SIGCONT...
			 */

			if ((child->p_stat == SSTOP)
			&&  (child->p_flag&SWTED) == 0
			&&  (child->p_flag&STRC || options&WUNTRACED)) {
				s = p_lock(&child->p_state, SPLHI);
				if (child->p_stat == SSTOP) {
					child->p_flag |= SWTED;
					u.u_r.r_val1 = child->p_pid;
					u.u_r.r_val2 = (child->p_cursig<<8) | WSTOPPED;
					v_lock(&child->p_state, s);
					return (0);
				}
				/* lost the race. Child no longer asleep */
				v_lock(&child->p_state, s);
			}
		}

		/*
		 * Didn't find one to wait on.  Error if reason was no children.
		 */

		if (p->p_cptr == NULL)
			return (ECHILD);

		/*
		 * We have children.  Return immediately if options say to.
		 */

		if (options&WNOHANG) {
			u.u_r.r_val1 = 0;
			return (0);
		}

		/*
		 * In case we get interrupted, arrange to restart syscall.
		 * Process is only one to set its own SOUSIG bit.
		 */

		if ((p->p_flag&SOUSIG) == 0 && setjmp(&u.u_qsave)) {
			u.u_eosys = RESTARTSYS;
			return (0);
		}

		/*
		 * Wait for a child to change state.
		 */

		(void)p_event(&p->p_zombie, PWAIT);
	}
}
