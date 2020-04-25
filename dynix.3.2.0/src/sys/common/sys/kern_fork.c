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
static	char	rcsid[] = "$Header: kern_fork.c 2.22 91/01/21 $";
#endif

/*
 * kern_fork.c
 *	Fork, vfork system calls and newproc().
 */

/* $Log:	kern_fork.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/mutex.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/file.h"
#include "../h/acct.h"
#include "../h/conf.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

/*
 * Local (mostly) variables.
 */

lock_t		proc_list;		/* locks proc-list for fork */
					/* also, mpid, pidhash[], procmax */
struct	proc	*procmax;		/* 1 + higest proc[] slot in use */

long	forkmap;			/* fork memory "resource" */
long	maxforkRS;			/* max # clusters when doing fork */
lock_t	forkmap_mutex;			/* mutex's forkmap manipulation */
sema_t	forkmap_wait;			/* wait here for formap space */
extern	int maxuprc;			/* max # per user procs allowed */
#ifdef	PERFSTAT
long	min_forkmap = 0x7fffffff;	/* low-water mark */
#endif	PERFSTAT

/*
 * fork()
 *	fork system call.
 */

#define	FM_VFORK	0x01		/* vfork flag */
#define	FM_SHARE_OFILE	0x02		/* share open file-table */

fork()
{
	cfork(0);			/* share nothing with parent */
}

shvfork(resource_mask)
{
	fork1((resource_mask & FM_SHARE_OFILE) | FM_VFORK);
}

shfork(resource_mask)
{
	cfork(resource_mask & FM_SHARE_OFILE);
}

cfork(resource_mask)			/* shared resource fork */
	int	resource_mask;
{
	register long	old_rssize;
	register long	mem_needs;
	register spl_t	s;

	/*
	 * Create swap space for child.  Newproc/procdup will copy this
	 * to child if all goes well.
	 */

	if (vsclone(&u.u_dmap, &u.u_cdmap) == 0) {
		u.u_error = ENOMEM;
		u.u_r.r_val2 = 0;
		return;
	}
	if (vsclone(&u.u_smap, &u.u_csmap) == 0) {
		vsrele(&u.u_cdmap);
		u.u_error = ENOMEM;
		u.u_r.r_val2 = 0;
		return;
	}

	/*
	 * If current Rset size is too big for a fork, shrink it and
	 * restore later.  Avoids deadlock problem of single process
	 * with too much Rset trying to fork.
	 */

	old_rssize = u.u_procp->p_rscurr;
	if (old_rssize > maxforkRS) {
		vsetRS(maxforkRS);
	}

	/*
	 * Insure don't over-comit memory to forks.
	 * Non-signalable sleep here; not worth the fuss for signalable.
	 */

	mem_needs = 2 * (u.u_procp->p_szpt + UPAGES + u.u_procp->p_rscurr*CLSIZE);
	s = p_lock(&forkmap_mutex, SPLIMP);
	while (mem_needs > forkmap) {
		p_sema_v_lock(&forkmap_wait, PRSWAIT, &forkmap_mutex, s);
		s = p_lock(&forkmap_mutex, SPLIMP);
	}
	forkmap -= mem_needs;
#ifdef	PERFSTAT
	if (forkmap < min_forkmap)
		min_forkmap = forkmap;
#endif	PERFSTAT
	v_lock(&forkmap_mutex, s);

	/*
	 * Do the fork().  If fail, nobody using the swap space so release it.
	 */

	fork1(resource_mask & ~FM_VFORK);
	if (u.u_error) {
		vsrele(&u.u_cdmap);
		vsrele(&u.u_csmap);
	}

	/*
	 * Parent only: done with fork memory "resource": put it back.
	 */

	if (u.u_r.r_val2 == 0) {			/* parent */
		s = p_lock(&forkmap_mutex, SPLIMP);
		forkmap += mem_needs;
		v_lock(&forkmap_mutex, s);
		if (blocked_sema(&forkmap_wait))
			vall_sema(&forkmap_wait);
	}

	/*
	 * If shrunk Rset, get it back.  In child, this inherits Rset size.
	 */

	if (old_rssize > u.u_procp->p_rscurr)
		vsetRS(old_rssize);
}

/*
 * vfork()
 *	"Virtual" fork system call.  More efficient than fork() if
 *	will shortly exec().
 *
 * Parent and child swap page-tables and child runs on parent
 * page-tables (and memory) until it exec's or exit's.  Must be
 * very careful to coordinate this in a TMP since memory-allocator
 * (lmemall) and pageout daemon (pageout) follow vfork chains.
 */

vfork()
{
	fork1(FM_VFORK);
}

/*
 * fork1()
 *	Actually do the fork.
 *
 * Error unwinds (if any) are done by caller.
 */

fork1(resource_mask)
	int	resource_mask;
{
	register struct proc *p;

	p = u.u_procp;
	if (newproc(resource_mask)) {
		u.u_r.r_val1 = p->p_pid;		/* parent pid */
		u.u_r.r_val2 = 1;			/* child */
		u.u_start = time;
		u.u_acflag = AFORK;
		u.u_ioch = 0;
		/*
		 * If being traced by multi-process debugger, stop
		 * if parent isn't the debugger.
		 */
		if (u.u_procp->p_flag & SMPTRC) {
			p = u.u_procp;
			(void) p_lock(&p->p_state, SPLHI);	/* no races! */
			if ((p->p_flag & SMPTRC) && p->p_mptpid != u.u_r.r_val1)
				mpt_stop(PTS_FORK);
			v_lock(&p->p_state, SPL0);
		}
		return;
	}

	/*
	 * Parent or error.  newproc() already set child pid in r_val1.
	 * Vfork() syscall libc xface requires R1 not modified if error;
	 * syscall insures this.
	 */

	u.u_r.r_val2 = 0;			/* parent */
}

/*
 * newproc()
 *	Create a new process-- the internal version of sys fork.
 *
 * It returns 1 in the new process, 0 in the old.
 *
 * Caller must check u.u_error to see if this succeeded.
 */

newproc(resource_mask)
	int	resource_mask;
{
	register struct proc *parent = u.u_procp;	/* parent process */
	register struct proc *child;			/* new process */
	register int	n;
	register struct file *fp;
	register struct mmap *um;
	struct	ofile_tab *oft;
	int	isvfork = (resource_mask & FM_VFORK);
	spl_t		s;
	struct	proc	*procalloc();

	/*
	 * Clone parent process open-file table object.
	 * If parent wants to share, just add a reference.
	 */

	if (resource_mask & FM_SHARE_OFILE)
		oft = ofile_addref(u.u_ofile_tab);
	else
		oft = ofile_clone(u.u_ofile_tab, &u.u_error);
	if (oft == NULL)
		return 0;

	/*
	 * Allocate a process slot.
	 */

	s = p_lock(&proc_list, SPL6);		/* lock proc-list/etc */

	child = procalloc();
	if (child == NULL) {
		v_lock(&proc_list, s);
		ofile_deref(oft);
		return(0);
	}

	/*
	 * Make a proc table entry for the new process.
	 */

	child->p_stat = SIDL;

	/* p_pid was filled out by procalloc() */
	child->p_nice = parent->p_nice;	/* before p_uid for get/setpriority */
	child->p_uid = parent->p_uid;
	child->p_suid = parent->p_suid;

	child->p_ppid = parent->p_pid;
	child->p_pptr = parent;
	child->p_sptr = parent->p_cptr;		/* proc[1] mutex by proc_list */
	parent->p_cptr = child;

	child->p_sigmask = parent->p_sigmask;
	child->p_sigcatch = parent->p_sigcatch;
	child->p_sigignore = parent->p_sigignore;

	/*
	 * We will take along those signals that cannot be caught,
	 * blocked, or ignored. So if one is trying to madly kill
	 * a runaway forker, the child will not get a chance...
	 */

	child->p_sig = parent->p_sig & (sigmask(SIGKILL) | sigmask(SIGSTOP));

	/*
	 * Put process on pid-hash list.  At this point, process can
	 * be found (pfind()) for signals.
	 */

	n = PIDHASH(child->p_pid);
	child->p_idhash = pidhash[n];
	pidhash[n] = child - proc;

	child->p_pgrp = parent->p_pgrp;	/* Now process can be found via pgrp */

	if (isvfork) {
		/*
		 * vfork().  Set up for minimal memory resources.
		 * Hold proc_list until set child flags to avoid
		 * races with process being looked up in pfind()
		 * and messed with.
		 */
		child->p_flag = SVFORK | (parent->p_flag & SFORK);
		v_lock(&proc_list, s);

		child->p_ndx = parent->p_ndx;

		child->p_dsize = child->p_ssize = 0;
		child->p_szpt = SZPT(child);
		child->p_rscurr = 1;			/* minimal Rset */

		l.cnt.v_cntvfork++;
		l.cnt.v_sizvfork += parent->p_dsize + parent->p_ssize;
	} else {
		/*
		 * Real fork.  Set to do process copy.
		 */
		child->p_flag = (parent->p_flag & SFORK);
		v_lock(&proc_list, s);

		child->p_ndx = child - proc;

		child->p_dsize = parent->p_dsize;
		child->p_ssize = parent->p_ssize;
		child->p_szpt = parent->p_szpt;
		child->p_rscurr = parent->p_rscurr;	/* inherit Rset size */

		l.cnt.v_cntfork++;
		l.cnt.v_sizfork += parent->p_dsize + parent->p_ssize;

		/*
		 * If there are mappings, "dup" them (real fork() only).
		 * Vfork() uses parent address space, thus no need to dup.
		 * procdup() will copy Uarea.
		 */

		for (um = u.u_mmap; um < u.u_mmapmax; um++) {
			if (um->mm_pgcnt != 0) {
				(*um->mm_ops->map_dup)(um->mm_handle,
					um->mm_off, um->mm_size, um->mm_prot);
				if (um->mm_fdidx >= 0) {
					fp = &file[um->mm_fdidx];
					FDBUMP(fp);
				}
			}
		}
	}

	timerclear(&child->p_realtimer.it_value);

	child->p_usrpri = parent->p_usrpri;	/* inherit parent prio */
	child->p_pri = parent->p_pri;		/* inherit parent prio */
	child->p_maxrss = parent->p_maxrss;
	child->p_noswap = 0;
	child->p_time = 0;
	child->p_slptime = 0;
	child->p_cpu = 0;
	child->p_pctcpu = 0;
	child->p_cpticks = 0;
	child->p_rssize = 0;			/* start with none */
	child->p_rshand = 0;			/* clean place to start */

	/*
	 * Inherit processor affinity.
	 * Only real need on where to do this is it must be done
	 * before child becomes executable.
	 */

	child->p_affinity = parent->p_affinity;
	if (child->p_affinity != ANYENG) {
		s = p_lock(&engtbl_lck, SPLHI);
		/*
		 * If parent has affinity the fork must be running on
		 * the engine that it is bound.
		 */
		l.eng->e_count++;
		v_lock(&engtbl_lck, s);
	}

#ifdef	FPA
	/*
	 * Similarly if parent is using an FPA, assume child does too.
	 */
	if (parent->p_fpa)
		fork_fpa(child, parent);

#endif	FPA

	l.multprog++;

	/*
	 * Increase reference counts on shared objects.
	 */

	VN_HOLD(u.u_cdir);
	if (u.u_rdir)
		VN_HOLD(u.u_rdir);
	crhold(u.u_cred);

	/*
	 * Allow custom system calls to handle fork.
	 * Done here since already dup'd file descriptors,
	 * and don't expect need to use child process image.
	 */

	cust_sys_fork(child, isvfork);

	/*
	 * Proc[] sema's and lock's were init'd at boot time and
	 * remain valid after process exit.  Thus, no redundant init.
	 */

	/*
	 * Partially simulate the environment of the new process so that
	 * when it is actually created (by copying) it will look right.
	 *
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 *
	 * Procdup() arranges appropriate return for child, so no special
	 * action here.
	 */

	parent->p_noswap++;			/* dis-allow swaps of parent */
	if (procdup(child, isvfork))
		return (1);

	/*
	 * Child exists and has Uarea, but not yet scheduled and not yet
	 * swappable.
	 *
	 * If parent had an open-file table extension object, install
	 * the child's copy.
	 */

	child->p_uarea->u_ofile_tab = oft;

#ifdef	FPA
	/*
	 * Call hook to duplicate auxiliary data structures
	 */
	shadow_fork(parent,child);
#endif	/* FPA */

	/*
	 * If parent is a multi-process debugger, child goes on list.
	 * Else if parent is traced by MP debugger, so is child.
	 */

	if (parent->p_flag & (SMPDBGR|SMPTRC)) {

		/*
		 * Can race with debugger exiting, thus check again after
		 * proc_list locked.
		 */

		s = p_lock(&proc_list, SPL6);

		if (parent->p_flag & SMPDBGR) {

			child->p_mpts = parent->p_mptc;
			parent->p_mptc = child-proc;
			child->p_mptpid = parent->p_pid;
			child->p_uarea->u_mptdbgr = parent;

			(void) p_lock(&child->p_state, SPLHI);
			child->p_flag |= SMPTRC;
			v_lock(&child->p_state, SPL6);

		} else if (parent->p_flag & SMPTRC) {

			child->p_mpts = u.u_mptdbgr->p_mptc;
			u.u_mptdbgr->p_mptc = child-proc;
			child->p_mptpid = parent->p_mptpid;
			/* procdup() dup'd u_mptdbgr */

			(void) p_lock(&child->p_state, SPLHI);
			child->p_flag |= SMPTRC;
			v_lock(&child->p_state, SPL6);

		}

		v_lock(&proc_list, s);
	}

	/*
	 * If vfork(), swap VM resources with child, which already has its
	 * own U-area.  Then put child on runQ, wait for it to exit or exec,
	 * and swap the resources back.
	 *
	 * For normal fork, child already in good shape -- just put on runQ.
	 * Must set p_stat = SRUN inside runQ gate to avoid race with swapper
	 * trying to remrq() the process (which isn't on the runQ yet!).
	 */

	if (isvfork) {
		/*
		 * Virtual fork.  Swap resources with child, then
		 * put on runQ.  Must swap VM with child while
		 * holding memory locked to mutex against memory realloc
		 * and pageout daemon.
		 */

		LOCK_MEM;

		s = p_lock(&parent->p_state, SPLHI);
		parent->p_flag |= SNOVM;
		v_lock(&parent->p_state, s);

		vpassvm(parent, child);
		parent->p_vflink = child;

		UNLOCK_MEM;

		/*
		 * Make child runnable and add to run queue; then can
		 * finally be swapped again.
		 */

		P_GATE(G_RUNQ, s);
		child->p_stat = SRUN;
		setrq(child);
		V_GATE(G_RUNQ, s);

		parent->p_noswap--;

		/*
		 * Wait for child to be done with VM resources, then
		 * switch back and let child finish what it was doing.
		 */

		p_sema(&parent->p_vfork, PZERO-1);
		ASSERT_DEBUG(child->p_flag & SLOAD, "newproc: vfork");

		LOCK_MEM;

		s = p_lock(&parent->p_state, SPLHI);
		parent->p_flag &= ~SNOVM;
		v_lock(&parent->p_state, s);

		vpassvm(child, parent);
		parent->p_vflink = NULL;
		child->p_ndx = child - proc;

		UNLOCK_MEM;

		v_sema(&child->p_vfork);
	} else {
		/*
		 * Real fork.
		 * Make child runnable and add to run queue; then can
		 * finally be swapped again.
		 */

		P_GATE(G_RUNQ, s);
		child->p_stat = SRUN;
		setrq(child);
		V_GATE(G_RUNQ, s);

		parent->p_noswap--;
	}

	/*
	 * Set up child PID in parent U-area return value.
	 * 0 return means parent.
	 */

	u.u_r.r_val1 = child->p_pid;
	return (0);
}

/*
 * procalloc()
 *	Allocate a process slot.
 *
 * Split from fork1()/newproc() since TMP must only scan proc-list once,
 * and want newproc() to do it.  This provides better modularity and allows
 * better use of registers.
 *
 * Returns pointer to proc-slot with unique pid, or NULL if fails.
 * Bumps procmax if needed.
 *
 * Assumes caller locked the proc-list (proc_list lock).
 *
 * Mpid can be bumped here without assigning the pid to a process,
 * if this call fails.
 */

static struct proc *
procalloc()
{
	register struct proc *p1;
	register struct proc *p2;
	register uidcnt;			/* # procs of callers uid */

	/*
	 * Look for slot for process.  While doing this, find a unique
	 * pid, and count # processes callers uid currently has.
	 *
	 * Note: clash on pid is rare; thus simple re-start of algorithm
	 * is sufficient.
	 */
retry:
	mpid++;
	if (mpid >= 30000)			/* wrap around */
		mpid = 1;

	uidcnt = 0;
	p2 = NULL;
	for (p1 = proc; p1 < procmax; p1++) {
		if (p1->p_pid==mpid || p1->p_pgrp==mpid)
			goto retry;
		if (p1->p_stat==NULL && p2==NULL)
			p2 = p1;
		else {
			if (p1->p_uid==u.u_uid && p1->p_stat!=NULL)
				uidcnt++;
		}
	}

	/*
	 * If need be, bump procmax and use new slot.
	 * Note that procmax points to one entry *beyond* highest in use,
	 * for consistency with previous use of procNPROC.
	 */

	if (p2 == NULL) {
		if (procmax < procNPROC)
			p2 = procmax++;
		else
			tablefull("proc");
	}

	/*
	 * Disallow if:
	 *	No processes at all;
	 *	not su and too many procs owned; or
	 *	not su and would take last slot.
	 * Adjust procmax back if just bumped it above.
	 * Else is legal: fill out p_pid.
	 */

	if (p2==NULL || (u.u_uid!=0 && (p2==procNPROC-1 || uidcnt>maxuprc))) {
		u.u_error = EAGAIN;
		if (p2 == procmax-1)
			procmax--;
		p2 = NULL;
	} else
		p2->p_pid = mpid;

	/*
	 * Return found proc[] slot with filled-in p_pid, or NULL.
	 */

	return(p2);
}
