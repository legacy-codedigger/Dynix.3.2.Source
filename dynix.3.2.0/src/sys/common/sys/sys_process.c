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
static	char	rcsid[] = "$Header: sys_process.c 2.14 1992/02/13 00:28:13 $";
#endif

/*
 * sys_process.c
 *	Process control functions (ptrace, etc).
 */

/* $Log: sys_process.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/acct.h"


#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

#ifdef  FPA
#include "../machine/plocal.h"
#include "../machine/mftpr.h"
#endif	FPA

struct	dbg_ipc	ipc;			/* debugger and debuggee synch */

/*
 * ptrcinit()
 *	Initialize semaphores in ipc structure.
 */

ptrcinit()
{
	init_sema(&ipc.ip_lock, 1, 0, G_PTRACE);
	init_sema(&ipc.ip_synch, 0, 0, G_PTRACE);
}

/*
 * ptrace()
 *	process-trace system call.
 *
 * pt_legal has a '1' bit for each function legal in a non-MP debugger.
 */

#define	PT_BIT(x)	(1 << (x))
#define	PT_LEGAL(x)	(pt_legal & PT_BIT(x))

static	long	pt_legal = 
	PT_BIT(PT_CHILD) | PT_BIT(PT_RTEXT) | PT_BIT(PT_RDATA) |
	PT_BIT(PT_RUSER) | PT_BIT(PT_WTEXT) | PT_BIT(PT_WDATA) |
	PT_BIT(PT_WUSER) | PT_BIT(PT_CONTSIG) | PT_BIT(PT_KILL) |
	PT_BIT(PT_SSTEP) | 
	PT_BIT(XPT_RREGS) | PT_BIT(XPT_WREGS) | PT_BIT(XPT_RPROC) |
	PT_BIT(XPT_SETSIGMASK) | PT_BIT(XPT_DUPFD) | PT_BIT(XPT_OPENT);

ptrace()
{
	register struct proc *p;
	register struct proc *c;
	register struct a {
		int	req;
		int	pid;
		caddr_t	addr;
		int	data;
	} *uap = (struct a *)u.u_ap;
	struct	pt_stop	ptw;
	int	trace;
	int	found;
	int	clearf;
	bool_t	mptrace;
	int	dbgpid;

	/*
	 * PT_CHILD declares self a child of a debugger.
	 */

	p = u.u_procp;
	if (uap->req <= PT_CHILD) {
		(void) p_lock(&p->p_state, SPLHI);
		/*
		 * disallow tracing if parent is init.
		 */
		if(p->p_pptr == &proc[1])
			u.u_error = ESRCH;
		else
			p->p_flag |= STRC;
		v_lock(&p->p_state, SPL0);
		u.u_sigpass = 0;
		return;
	}

	/*
	 * Request XPT_MPDEBUGGER used to declare self a multi-process debugger.
	 * Some new ptrace functions only usable by new debugger.
	 */

	if (uap->req == XPT_MPDEBUGGER) {
		if ((p->p_flag & SMPDBGR) == 0) {
			(void) p_lock(&p->p_state, SPLHI);
			p->p_flag |= SMPDBGR;
			v_lock(&p->p_state, SPL0);
			p->p_mptc = 0;		/* init traced process tree */
		}
		return;
	}
	if (p->p_flag & SMPDBGR) {
		trace = SMPTRC;
		clearf = 0;
		mptrace = 1;
	} else {
		if (uap->req > XPT_MAX_FUNCT || !PT_LEGAL(uap->req)) {
			u.u_error = EINVAL;
			return;
		}
		trace = STRC;
		clearf = SWTED;
		mptrace = 0;
	}

	/*
	 * Some functions don't look at pid or need stopped descendant.
	 */

	switch(uap->req) {

	case XPT_STOPSTAT:
		/*
		 * Report (once) a stopped descendant.
		 * Sense SMPSTOP since SSTOP is also used for other stops.
		 * Do with proc_list locked, since descendant could be forking.
		 */

		(void) p_lock(&proc_list, SPL6);
		for(c = &proc[p->p_mptc]; c != &proc[0]; c = &proc[c->p_mpts]) {
			if ((c->p_flag & (SMPSTOP|SMPWTED)) == SMPSTOP) {
				v_lock(&proc_list, SPL0);
				(void) p_lock(&c->p_state, SPLHI);
				ASSERT((c->p_flag & SMPTRC) && c->p_stat==SSTOP, "ptrace: mpt stop");
				/*
				 *+ A process that has been stopped by a 
				 *+ multiprocess debugger either doesn't think
				 *+ it is being traced by a debugger
				 *+ or isn't in the stopped state.
				 */
				c->p_flag |= SMPWTED;
				ptw.ps_pid = c->p_pid;
				ptw.ps_reason = c->p_mptstop;
				v_lock(&c->p_state, SPL0);
				u.u_error = copyout((caddr_t)&ptw, uap->addr, sizeof(ptw));
				return;
			}
		}
		v_lock(&proc_list, SPL0);

		/*
		 * Didn't find one.
		 */

		u.u_error = ESRCH;
		return;

	case XPT_SIGNAL:
		/*
		 * Signal processes.
		 * Non-zero `pid' ==> do this one only; else do'em all.
		 */

		if (uap->data <= 0 || uap->data > NSIG) {
			u.u_error = EINVAL;
			return;
		}

		found = 0;
		(void) p_lock(&proc_list, SPL6);
		for(c = &proc[p->p_mptc]; c != &proc[0]; c = &proc[c->p_mpts]) {
			if (uap->pid == 0 || uap->pid == c->p_pid) {
				psignal(c, uap->data);
				found++;
			}
		}
		v_lock(&proc_list, SPL0);

		if (!found)
			u.u_error = ESRCH;
		return;

	case XPT_DEBUG:				/* debug unrelated process */
	case XPT_OPENT:				/* get RO fd for process text */
		u.u_error = EINVAL;		/* not yet supported */
		return;
	}

	/*
	 * Other requests insist on stopped & traced child,
	 * and exclusive access to ipc structure.
	 * Set up ipc structure for the particular request.
	 */

	p_sema(&ipc.ip_lock, IPCPRI);

	switch(ipc.ip_req = uap->req) {

	case XPT_WREGS:
		u.u_error = copyin(uap->addr, (caddr_t)&ipc.ip_regs, sizeof(struct pt_regset));
		break;

	case XPT_WATCHPT_SET:
		u.u_error = copyin(uap->addr, (caddr_t)&ipc.ip_watchpt, sizeof(struct pt_watchpt));
		break;

	case XPT_DUPFD:
		/*
		 * Pass file-table entry in ip_data, target file-descriptor
		 * number in ip_addr.
		 * Note: no flags passed (thus loose EXCLOSE).
		 */
		ipc.ip_data = (int) getf(uap->data);
		ipc.ip_addr = uap->addr;
		break;

	default:
		ipc.ip_data = uap->data;
		ipc.ip_addr = uap->addr;
		break;
	}

	if (u.u_error) {
		v_sema(&ipc.ip_lock);
		return;
	}

	/*
	 * Look up process and insure ok to mess with it.
	 */

	dbgpid = p->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		v_sema(&ipc.ip_lock);
		u.u_error = ESRCH;
		return;
	}
	if (p->p_stat != SSTOP
	||  (mptrace && p->p_mptpid != dbgpid)
	||  (!mptrace && p->p_ppid != dbgpid)
	||  !(p->p_flag & trace)) {
		v_lock(&p->p_state, SPL0);
		v_sema(&ipc.ip_lock);
		u.u_error = ESRCH;
		return;
	}

	/*
	 * XPT_RPROC special case -- return proc structure.
	 * Didn't really need ipc structure, but doesn't hurt and more
	 * convenient code.
	 */

	if (uap->req == XPT_RPROC) {
		v_lock(&p->p_state, SPL0);
		v_sema(&ipc.ip_lock);
		u.u_error = copyout((caddr_t)p, uap->addr, sizeof(struct proc));
		return;
	}

	/*
	 * Change process's flags and place it on the run queue.
	 */

	p->p_flag &= ~clearf;
	setrun(p);
	v_lock(&p->p_state, SPL0);

	/*
	 * Sleep until awoken by child in procxmt.
	 */

	p_sema(&ipc.ip_synch, IPCPRI);

	u.u_r.r_val1 = ipc.ip_data;
	u.u_error = ipc.ip_error;
	if (uap->req == XPT_RREGS && u.u_error == 0)
		u.u_error = copyout(	(caddr_t)&ipc.ip_regs,
					uap->addr,
					sizeof(struct pt_regset));
	v_sema(&ipc.ip_lock);
}

/*
 * procxmt()
 *	Code that the child process executes to implement the command
 *	of the parent process in tracing.
 *
 * The current process state is relocked before signalling parent if
 * procxmt returns to issig.  That is, it is not locked if exit is called.
 */

#define	PHYSOFF(p, o)	((physadr)(p)+((o)/sizeof(((physadr)0)->r[0])))

procxmt(stop_reason)
	int	stop_reason;
{
	register struct proc *p = u.u_procp;
	register int i;
	register int *dp;
	extern	int ipcreg[];
	extern	int nipcreg;

	p->p_slptime = 0;
	i = ipc.ip_req;
	ipc.ip_error = 0;
	switch (i) {

	case PT_RTEXT:					/* read user I */
		if (!useracc(ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuiword(ipc.ip_addr);
		break;

	case PT_RDATA:					/* read user D */
		if (!useracc(ipc.ip_addr, 4, B_READ))
			goto error;
		ipc.ip_data = fuword(ipc.ip_addr);
		break;

	case PT_RUSER:					/* read u */
		i = (int)ipc.ip_addr;
		if (i<0 || i >= ctob(UPAGES))
			goto error;
		ipc.ip_data = *(int *)PHYSOFF(&u, i);
		break;

	case PT_WTEXT:					/* write user I */
		if ((i = suiword(ipc.ip_addr, ipc.ip_data)) < 0) {
			int	op1, op2;
			if (op1 = chgprot(ipc.ip_addr, RW)) {
				if ((i = suiword(ipc.ip_addr, ipc.ip_data)) < 0
				&&  (op2 = chgprot(ipc.ip_addr+(sizeof(int)-1), RW))) {
					i = suiword(ipc.ip_addr, ipc.ip_data);
					(void) chgprot(ipc.ip_addr+(sizeof(int)-1), op2);
				}
				(void) chgprot(ipc.ip_addr, op1);
			}
		}
		if (i < 0)
			goto error;
		break;

	case PT_WDATA:					/* write user D */
		if (suword(ipc.ip_addr, 0) < 0)
			goto error;
		(void) suword(ipc.ip_addr, ipc.ip_data);
		break;

	case PT_WUSER:					/* write u */
		i = (int)ipc.ip_addr;
		dp = (int *)PHYSOFF(&u, i);
		/* Check if saved register */
		for (i=0; i<nipcreg; i++)
			if (dp == &u.u_ar0[ipcreg[i]])
				goto ok;

		/* Floating point registers saved in u */
		if (dp >= (int *)&u.u_fpusave && dp < (int *)(&u.u_fpusave+1))
			goto ok;
#ifdef	FPA
		/* Floating point accelerator registers saved in u */
		if (dp >= (int *)&u.u_fpasave && dp < (int *)(&u.u_fpasave+1))
			goto ok;
#endif	FPA
		if (dp == &u.u_ar0[IPC_PSW]) {
			ipc.ip_data &= ~IPC_PSW_USRCLR;
			ipc.ip_data |= IPC_PSW_USRSET;
			goto ok;
		}
		goto error;
	ok:
		*dp = ipc.ip_data;
		break;

	case XPT_UNDEBUG:				/* quit debugging it */
		/*
		 * Debugger can't die before this is done, since it's waiting
		 * (un-interruptable) for child to finish the procxmt().
		 */
		mpt_remove_process(p);			/* remove from list */
		(void) p_lock(&p->p_state, SPLHI);
		p->p_flag &= ~(SMPTRC|SMPWTED);		/* OFF MP debugging */
		v_lock(&p->p_state, SPL0);
		/* fall into ... */

	case PT_SSTEP:					/* single step */
	case PT_CONTSIG:				/* continue w/signal */
		if ((int)ipc.ip_addr != 1)
			u.u_ar0[IPC_PC] = (int)ipc.ip_addr;
		if ((unsigned)ipc.ip_data > NSIG)
			goto error;
		p->p_cursig = ipc.ip_data;		/* see issig */
		if (i == PT_SSTEP) {
			u.u_ar0[IPC_PSW] |= IPC_PSW_SSTEP;
		}
		(void) p_lock(&p->p_state, SPLHI);
		v_sema(&ipc.ip_synch);
		return(1);

	case PT_KILL:					/* force exit */
		/*
		 * Don't reenter exit.
		 */
		if (stop_reason == PTS_EXIT) {
			(void) p_lock(&p->p_state, SPLHI);
			v_sema(&ipc.ip_synch);
			return(1);
		}
		v_sema(&ipc.ip_synch);
		exit(p->p_cursig);

	case XPT_SETSIGMASK:				/* pass-thru sig mask */
		u.u_sigpass = ipc.ip_data;
		ipc.ip_data = 0;
		break;

	case XPT_RREGS:					/* read registers */
		/*
		 * Get copy of general registers and processor status.
		 */

		dp = &ipc.ip_regs.pr_genfirst;
		for (i = 0; i < nipcreg; i++)
			*dp++ = u.u_ar0[ipcreg[i]];
		ipc.ip_regs.pr_psw = u.u_ar0[IPC_PSW];

		/*
		 * Get copy of floating registers and status.
		 * Since process must have context switched,
		 * u.u_fpusave has latest FPU registers.
		 */

		bcopy(	(caddr_t)&u.u_fpusave,
			(caddr_t)&ipc.ip_regs.pr_fpu,
			sizeof(u.u_fpusave));
#ifdef	FPA
		bcopy(	(caddr_t)&u.u_fpasave,
			(caddr_t)&ipc.ip_regs.pr_fpa,
			sizeof(u.u_fpasave));
#endif	FPA
		ipc.ip_data = 0;
		break;

	case XPT_WREGS:					/* write registers */
		/*
		 * Re-write general registers and processor status.
		 */

		dp = &ipc.ip_regs.pr_genfirst;
		for(i = 0; i < nipcreg; i++)
			u.u_ar0[ipcreg[i]] = *dp++;
		u.u_ar0[IPC_PSW] = (ipc.ip_regs.pr_psw & ~IPC_PSW_USRCLR)
				   | IPC_PSW_USRSET;

		/*
		 * Re-write floating registers and status.
		 */

		bcopy(	(caddr_t)&ipc.ip_regs.pr_fpu,
			(caddr_t)&u.u_fpusave,
			sizeof(u.u_fpusave));
#ifdef	FPA
		bcopy(	(caddr_t)&ipc.ip_regs.pr_fpa,
			(caddr_t)&u.u_fpasave,
			sizeof(u.u_fpasave));
#endif	FPA
		ipc.ip_data = 0;
		break;

	case XPT_DUPFD:					/* dup fd to process */
		ipc.ip_error = ofile_dup(
					u.u_ofile_tab,
					(int) ipc.ip_addr,
					(struct file *)(ipc.ip_data)
				);
		break;

	case XPT_WATCHPT_SET:
		ipc.ip_error = dbg_watchpt_set(&ipc.ip_watchpt);
		break;

	case XPT_WATCHPT_CLEAR:
		ipc.ip_error = dbg_watchpt_clear(&ipc.ip_watchpt);
		break;

	default:
	error:
		ipc.ip_error = EIO;
		break;
	}
	/*
	 * Must acquire process state lock before release IPC structure
	 * to avoid race with debugger's exit().
	 */
	(void) p_lock(&p->p_state, SPLHI);
	v_sema(&ipc.ip_synch);
	return (0);
}

/*
 * mpt_stop()
 *	Stop and tell multi-process debugger about it,
 *	then do whatever it says.
 *
 * Assumes caller proc-state is locked.
 */

mpt_stop(reason)
	int	reason;				/* why are we stopping? */
{
	register struct proc *p = u.u_procp;

	ASSERT(p->p_flag & SMPTRC, "mpt_stop");
	/*
	 *+ A process has been stopped by a multiprocess debugger,
	 *+ but the multiprocess trace bit isn't set in the process flags.
	 */

	/*
	 * Set up to stop, and tell debugger we're going to.
	 * SMPSTOP is for XPT_STOPSTAT and exit().
	 */

	p->p_mptstop = reason;
	p->p_flag |= SWTED|SMPSTOP;		/* real parent can't see it */
	p->p_flag &= ~SMPWTED;			/* not yet seen by debugger */
	psignal(u.u_mptdbgr, SIGCHLD);		/* tell debugger we stopped */

	/*
	 * Do as master desires, until allowed to continue.
	 * Implicit continue if debugger dies.
	 */

#ifdef  FPA
	if (p->p_fpa == FPA_SW || p->p_fpa == FPA_FRCSW)
#ifdef	FPU_SIGNAL_BUG
			emula_fpa_sw2hw(&u.u_fpasave);
#else
			emula_fpa_sw2hw();
#endif
#endif  FPA

	do {
		p->p_stat = SSTOP;
		swtch(p);
		if ((p->p_flag & SMPTRC) == 0) {
			(void) p_lock(&p->p_state, SPLHI);
			break;
		}
	} while(!procxmt(reason));

#ifdef  FPA
	if (p->p_fpa == FPA_SW || p->p_fpa == FPA_FRCSW)
#ifdef	FPU_SIGNAL_BUG
		emula_fpa_hw2sw(&u.u_fpasave);
#else
		emula_fpa_hw2sw();
#endif
#endif  FPA

	p->p_flag &= ~(SWTED|SMPSTOP);		/* ok for parent to see again */
}

/*
 * mpt_remove_process()
 *	Remove a process from the debugger's list (multi-process debugger only).
 *
 * Called from exit() when process is really going to die, and in procxmt()
 * for XPT_UNDEBUG command.
 */

mpt_remove_process(p)
	register struct proc *p;
{
	register struct proc *q;
	register struct proc *next;

	/*
	 * Can race with debugger exiting, and turning off SMPTRC.
	 */

	(void) p_lock(&proc_list, SPL6);
	if (p->p_flag & SMPTRC) {
		q = &proc[u.u_mptdbgr->p_mptc];
		if (p == q)
			u.u_mptdbgr->p_mptc = p->p_mpts;
		else {
			for(; p != (next = &proc[q->p_mpts]); q = next) {
				ASSERT(next != &proc[0], "mpt_remove_process");
				/*
				 *+ A process being debugged by a 
				 *+ multiprocess debugger is exiting, but it
				 *+ doesn't appear in the debugger's
				 *+ list of traced processes.
				 */
			}
			q->p_mpts = p->p_mpts;
		}
	}
	v_lock(&proc_list, SPL0);
}

/*
 * proc_ctl()
 *	Various process control functions.
 *
 * Currently supports only on/off of priority aging.
 *
 * Sequent local system call (ie, non-standard).
 */

proc_ctl()
{
	register struct a {
		int	cmd;				/* sub-function */
		int	who;				/* pid (0 ==> self) */
		int	arg;				/* additional arg */
	} *uap = (struct a *)u.u_ap;
#ifdef FPA
	int	old;
#endif /* FPA */
	register struct proc *p;
	extern	bool_t	root_prio_noage;

	switch(uap->cmd) {
	
	case PROC_PRIOAGE:
		/*
		 * Turn ON/OFF priority aging for an individual process.
		 * Should support pgrp and/or uid based search?
		 *
		 * Anybody can turn on aging, only root (or others if
		 * if configured) can turn it off.
		 */

		if (uap->arg == 0 && root_prio_noage && !suser())
			break;

		/*
		 * Get locked proc structure, default `who' is self.
		 * Insist on root or matching euid.
		 */

		if (uap->who == 0)
			uap->who = u.u_procp->p_pid;
		p = pfind(uap->who);
		if (p == NULL) {
			u.u_error = ESRCH;
			break;
		}
		if (u.u_uid && u.u_uid != p->p_uid) {
			v_lock(&p->p_state, SPL0);
			u.u_error = EPERM;
			break;
		}

		if (uap->arg)				/* want aging */
			p->p_flag &= ~SNOAGE;
		else					/* non-aging */
			p->p_flag |= SNOAGE;

		v_lock(&p->p_state, SPL0);
		break;

#ifdef	FPA
	case PROC_FRCEMFPA:
		/*
		 * Force emulation of the FPA.  Refers to the current
		 * process only.  Used for testing.
		 */
		p = u.u_procp;
		old = p->p_fpa;
		if (uap->arg == -1) {
			/*
			 * Query.
			 */
			goto out_fpa;
}
		if (!uap->arg) {
			if (p->p_fpa != FPA_FRCSW) {

				/*
				 * Turning off forced emulation and haven't
				 * even turned it on yet.  Nop.
				 */
				goto out_fpa;
}

			/*
			 * Reduce to FPA_SW, if possible, we'll
			 * move to a FPA processor at the next fault.
			 */
			p->p_fpa = FPA_SW;
			goto out_fpa;
		}
		/*
		 * Forcing emulation.
		 */
		switch (p->p_fpa) {
		case FPA_NONE:
			/*
			 * Initialize the fpa registers.
			 */
			fpa_init();
#ifdef	FPU_SIGNAL_BUG
			emula_fpa_hw2sw(&u.u_fpasave);
#else
			emula_fpa_hw2sw();
#endif
			break;
		case FPA_HW:
			if (l.fpa) {
#ifdef	FPU_SIGNAL_BUG
				save_fpa(&u.u_fpasave);
#else
				save_fpa();
#endif
				disable_fpa();
				FLUSH_TLB();
			}
#ifdef	FPU_SIGNAL_BUG
			emula_fpa_hw2sw(&u.u_fpasave);
#else
			emula_fpa_hw2sw();
#endif
			break;
		case FPA_SW:
		case FPA_FRCSW:
		default:
			break;
		}
		p->p_fpa = FPA_FRCSW;
	out_fpa:
		u.u_r.r_val1 = old;
		break;
#endif	FPA

	default:
		u.u_error = EINVAL;
		break;
	}
}
