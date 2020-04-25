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
static	char	rcsid[] = "$Header: sys_rset.c 1.3 1991/04/17 20:56:41 $";
#endif

/*
 * adjrst.c
 *	system-call to squeeze a process resident set.
 *
 * Uses similar algorithm to swapout() to "grab" a process, dealing with
 * races vs running process.  If can "grab" process, squeeze it by calling
 * vallocRSslot() until rset is small enough (similar to vsetRS()).
 *
 * TODO:
 *	Should adjust p_maxrss, p_rscurr also?  Maybe a variant of
 *	squeeze_rset(), or a new function (related) that shrinks p_rscurr
 *	(sort of a call to vsetRS() for arbitrary process -- could mod
 *	vsetRS() for this).
 *
 *	Move LOADED() and SWAPPABLE() definitions to header file, and use
 *	both here and in sched().
 *
 *	squeeze_rset() probably should be a vm_ctl() or proc_ctl()
 *	sub-function.
 *
 *	Unify squeeze_grab_proc() and swapout(): ie, make squeeze_grab_proc()
 *	called both here and in swapout() (maybe with nicer name ;-).
 */

/* $Log: sys_rset.c,v $
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

/*
 * LOADED() tells if process is in-memory and not a system process.
 * SWAPPABLE() tells if in-memory process is easily swappable.
 *
 * These are taken from vm_sched.c -- should move to a header file.
 * If these change in vm_sched.c then MUST CHANGE HERE ALSO!
 */

#define	LOADED(p)	(((p)->p_flag&(SSYS|SLOAD|SNOSWAP))==SLOAD)
#define	SWAPPABLE(p)	(LOADED(p) && (p)->p_noswap==0)

/*
 * squeeze_rset()
 *	Attempt to squeeze pages from a process resident-set.
 *
 * Experimental interface.
 *
 * Returns error code or zero for success.
 */

int
squeeze_rset(pid, rssize)
	int	pid;
	int	rssize;
{
	register struct proc *p;

	if (!suser())
		return EPERM;		/* gotta be root for this */

	if (rssize < 0)
		return EINVAL;		/* tsk, tsk! */

	/*
	 * Look up process by pid.  Pfind() returns locked process state.
	 */

	p = pfind(pid);
	if (p == NULL)
		return ESRCH;

	/*
	 * Try to "grab" process: make sure it can't run while we're
	 * messing with its Rset.
	 */

	if (!squeeze_grab_proc(p)) {
		v_lock(&p->p_state, SPL0);
		return EBUSY;
	}
	v_lock(&p->p_state, SPL0);		/* ok, process can't run */

	/*
	 * Squeeze the process -- give some memory back to the system
	 * (ala vsetRS()).  Don't alter current allowed Rset size, just
	 * squish pages.
	 */

	while (p->p_rssize > rssize) {
		vallocRSslot(p);
	}

	/*
	 * Done with process.
	 */

	squeeze_release_proc(p);

	return 0;				/* succees! */
}

/*
 * squeeze_grab_proc()
 *	Grab a process for Rset "squeezing" if legit.
 *
 * Process must be in memory, not a system process, not "noswap", and not
 * actually running.  Preemption of process is very similar to swapout()
 * (if ever do this for real, should unify these).
 *
 * Caller passes locked process; returned locked.
 *
 * Returns true for success, else false.
 */

static int
squeeze_grab_proc(p)
	register struct proc *p;
{
	/*
	 * Validate proc is still swappable (can race with proc selection),
	 * and insure can mess with it (can't be running).
	 */

	switch (p->p_stat) {

	case SSLEEP:
	case SSTOP:
		/*
		 * Must check p_noswap in case raced with process* selection.
		 * If set, process (typically) won't sleep very long.
		 */
		if (!SWAPPABLE(p))
			return 0;			/* fail */
		break;

	case SRUN:
	case SONPROC:
		/*
		 * Grab G_RUNQ so can determine actual state of process.
		 */
		VOID_P_GATE(G_RUNQ);
		if (p->p_stat == SONPROC || !SWAPPABLE(p)) {
			V_GATE(G_RUNQ, SPLHI);
			return 0;			/* fail */
		}
		remrq(p);				/* pull off run queue */
		V_GATE(G_RUNQ, SPLHI);
		break;

	default:
		/*
		 * Lost race (rare).  Ignore process.
		 * I think this can't happen (rbk) -- got process via
		 * pfind(), process can't exit since exit locks process state.
		 */
		return 0;
	}

	/*
	 * Clear SLOAD to prevent v_sema() and friends from re-inserting
	 * process on runQ.  Set p_noswap to prevent swapin() from trying to
	 * bring it "back in".
	 */

	ASSERT(p->p_noswap == 0, "squeeze_grab_proc: p_noswap");
	p->p_noswap = 1;				/* block swapin */
	p->p_flag &= ~(SLOAD|SFSWAP);			/* logically out */

	return 1;					/* success */
}

/*
 * squeeze_release_proc()
 *	Done squeezing a process Rset -- restore state.
 *
 * Lock state.  If runnable (can't be SONPROC), put on runQ.
 *
 * Caller passes unlocked process state.
 */

static
squeeze_release_proc(p)
	register struct proc *p;
{
	spl_t	s;

	s = p_lock(&p->p_state, SPLHI);

	p->p_flag |= SLOAD;
	p->p_noswap = 0;
	if (p->p_stat == SRUN) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
	}

	v_lock(&p->p_state, s);
}

/*
 *
 *	adjrst(pid, rssize)
 *	
 *	Adjust resident set size of a given pid
 *	Entry in the kernel
 */
adjrst()
{
	register struct a {
		int	arg1;
		int	arg2;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
	uap->arg2 = 0;
#endif	lint
	u.u_error = squeeze_rset(
				uap->arg1,		/* pid */
				uap->arg2		/* rssize */
			);
}
