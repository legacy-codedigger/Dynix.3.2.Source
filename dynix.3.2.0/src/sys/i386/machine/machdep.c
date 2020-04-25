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
static	char	rcsid[] = "$Header: machdep.c 2.57 1992/03/16 23:13:45 $";
#endif

/*
 * machdep.c
 *	Machine dependent routines.
 *
 * i386 version.
 */

/* $Log: machdep.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/mount.h"
#include "../h/file.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/acct.h"
#include "../h/cmn_err.h"

#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/clkarb.h"
#include "../balance/clock.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/bic.h"
#include "../balance/cmc.h"
#include "../balance/bdp.h"
#include "../balance/SGSproc.h"
#include "../balance/SGSmem.h"

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/mmu.h"
#include "../machine/mftpr.h"
#include "../machine/hwparam.h"

#include "../sec/sec.h"
#include "../machine/scan.h"

extern	short	upyet;		/* Set when 1st processor is initialized */
extern	bool_t	light_show;	/* twiddling the lights?? */

extern	struct	ctlr_desc *slic_to_config[];

/*
 * setregs()
 *	Machine-dependent register-state setup on exec().
 */

setregs(entry)
	u_long	entry;
{
	/*
	 * Clear floating context in process image.
	 * We don't clear general registers since boot-time uses registers
	 * to pass parameters to /etc/init.
	 */
#ifdef	FPU_SIGNAL_BUG
	init_fpu_state();
	/*
	 * The new image has never use the fpu.  Thus, clear fpu-specific
	 * flags.
	 */
#ifdef	C0BUG
	u.u_flags &= ~(UF_USED_FPU | UF_FPSTEP | UF_OTBIT);
#else
	u.u_flags &= ~UF_USED_FPU;
#endif

#else	/* FPU_SIGNAL_BUG */
	disable_fpu();
	u.u_fpusave.fpu_control = FPU_CONTROL_INIT;
	u.u_fpusave.fpu_status = FPU_STATUS_INIT;
	u.u_fpusave.fpu_tag = FPU_TAG_INIT;
#endif	/* FPU_SIGNAL_BUG */

	if (u.u_procp->p_fpa)
		disable_fpa();

	/*
	 * Set up entry EIP and deal with "close-on-exec" files.
	 * Set up NULL EBP for debuggers.
	 * Also insure direction-flag is zero (languages assume this).
	 */

	u.u_ar0[EIP] = entry;
	u.u_ar0[EBP] = 0;
	u.u_ar0[FLAGS] &= ~FLAGS_DF;
}

/*
 * sendsig()
 *	Send an interrupt to process (ie, post a signal).
 *
 * Stack is set up to allow sigcode in sigvec library routine to call
 * routine, followed by svc to sigcleanup routine below.  After
 * sigcleanup() resets the signal mask and the stack, it returns to user
 * who then unwinds via a "emulated rett" at the end of "sigcode".
 */

sendsig(p, sig, mask)
	int	(*p)();
	register int sig;
	int	mask;
{
	register struct sigcontext *scp;	/* sigcontext, original stack */
	register struct sigcontext *scp1;	/* sigctxt, destination stack */
	register int *regs;
	struct sigcontext lsigc;		/* built locally then copyout */
	struct sigframe {
		int	sf_signum;
		int	sf_code;
		struct	sigcontext *sf_scp;
		int	edx;
		int	ecx;
		int	eax;
		int	sf_uflags;
#ifdef	FPU_SIGNAL_BUG
		struct	fpusave	sf_fpusave;
#ifdef	FPA
		struct	fpasave	sf_fpasave;
#endif	/* FPA */
#endif	/* FPU_SIGNAL_BUG */
	} sigframe;				/* built locally then copyout */
	register struct sigframe *fp;		/* copyout destination */
	int	oonstack;
#ifdef	FPU_SIGNAL_BUG
	unsigned int	sizetocopy;
#endif

	regs = u.u_ar0;
	oonstack = u.u_onstack;
	scp = (struct sigcontext *)regs[ESP] - 1;
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		scp1 = (struct sigcontext *)u.u_sigsp - 1;
		fp = (struct sigframe *)scp1 -1;
		u.u_onstack = 1;
	} else {
		scp1 = scp;
		fp = (struct sigframe *)scp - 1;
	}

	/*
	 * The sigcontext and sigframe structures (used in calling the 
	 * handler) are placed on the stack on which the handler is to 
	 * operate.  Sigcleanup will restore the pc and processor flags
 	 * PORTION of the sigcontext onto the stack to be returned to 
	 * after the handler is finished, so that "iret emulation" in 
	 * sigcode will pop flags and IP off correct stack.  
	 *
	 * If these things are to go onto the system managed stack, we make 
	 * sure there's room for them first. 
	 *
	 * Need only save scratch registers, since signal handler is
	 * careful to save/restore registers (assumed to obey C calling
	 * convention).
	 */

	/*
	 * Note, since fp is below scp1, this assures space for saving
	 * BOTH the sigcontext AND the sigframe structures on the
	 * signal handler's stack.
	 */

	if (!u.u_onstack && (int)fp <= USRSTACK - ctob(u.u_ssize))
		(void) grow((unsigned)fp);

	sigframe.sf_signum = sig;
	if (sig == SIGILL || sig == SIGFPE) {
		sigframe.sf_code = u.u_code;
		if (sig == SIGFPE)
#ifdef	FPA
		/*
		 * FPA clears the exception bits in fpa_trap().
		 *
		 * So for FPA processes, the debugger should look in u.u_code
		 * instead of u.u_fpasave for the type of floating exception.
		 */
		if ((u.u_code & FPA_FPE) == 0)
#endif
			u.u_fpusave.fpu_status &= ~u.u_code;
		u.u_code = 0;
	} else if (sig == SIGSEGV) {
		sigframe.sf_code = u.u_segvcode;
		u.u_segvcode = -1;
	} else
		sigframe.sf_code = 0;
	sigframe.sf_scp = scp1;
	sigframe.eax = regs[EAX];
	sigframe.ecx = regs[ECX];
	sigframe.edx = regs[EDX];
	/*
	 * Save flags for sigreturn().
	 */
	sigframe.sf_uflags = u.u_flags;

#ifdef	FPU_SIGNAL_BUG
	sizetocopy = sizeof(sigframe) - sizeof(sigframe.sf_fpusave)
				      - sizeof(sigframe.sf_fpasave);
	/*
	 * If we have ever used the FPU in our lifetime, save
	 * its state to the signal frame.  This will be the state
	 * we will restore on normal return (sigcleanup) from the
	 * signal handler.
	 */
	if (u.u_flags & UF_USED_FPU) {
		/*
		 * If we haven't used the fpu since the last context
		 * switch, then copy the fpu state from the uarea to
		 * the signal stack.
		 *
		 * Otherwise, save the fpu state directly to the signal
		 * stack.
		 */
		sizetocopy += sizeof(sigframe.sf_fpusave);
		if (l.usingfpu) {
			save_fpu(&sigframe.sf_fpusave);
		} else {
			sigframe.sf_fpusave = u.u_fpusave;
	        }
		/*
		 * FPU state is saved in all cases except for a SIGFPE.
		 * In this case we need to preserve compatibility with 
		 * user-level SIGFPE fault handlers which expect that any 
		 * change they make to the fpu state in the chip will 
		 * persist on return from the handler.  This should be 
		 * removed in the next release.
		 */
		if (sig == SIGFPE) {
			/*
			 * Abnormal case:  give signal handler same state
			 * as caller and be sure that caller inherits any
			 * changes made by the signal handler on return.
			 */
			if (l.usingfpu) {
				/*
				 * Actually, we cannot get here since
				 * trap() has already saved the fpu state
				 * and disabled the fpu for FPE_COP_FAULT's.
				 *
				 * But, let's leave this here for the
				 * sake of future development (i.e.,
				 * if someone decides to change trap,
				 * he/she won't have to worry about this).
				 */
				u.u_fpusave = sigframe.sf_fpusave;
				disable_fpu();
			}
			sigframe.sf_uflags &= ~UF_SAVED_FPU;
		} else {
			/*
			 * Normal case: give signal handler clean fpu
			 * state and be sure to restore caller's state on
			 * return.
			 */
			init_fpu_state();
			sigframe.sf_uflags |= UF_SAVED_FPU;
		}
	}
#ifdef  FPA
	/*
	 * If we have *ever* used the fpa in our lifetime, save its state.
	 */
	if (u.u_procp->p_fpa) {
		sizetocopy = sizeof(sigframe);
		/*
		 * Let's avoid any covert channels.
		 */
		if ((sigframe.sf_uflags & UF_SAVED_FPU) == 0) {
			bzero((char *)&sigframe.sf_fpusave,
						sizeof(struct fpusave));
		}

		if (u.u_procp->p_fpa == FPA_HW) {
			/*
			 * If we haven't used the fpa since the last context
			 * switch, then copy the fpa state from the uarea to
			 * the signal stack.
			 *
			 * Otherwise, save the fpa state directly to the
			 * signal stack.
			*/
			if (*(int *)VA_FPA_PTE == PG_INVAL) {
				bcopy((char *)&u.u_fpasave,
					(char *)&sigframe.sf_fpasave,
					sizeof(struct fpasave));
			} else {
				save_fpa(&sigframe.sf_fpasave);
			}
		} else {	/* p_fpa == FPA_SW || p_fpa == FPA_FRCSW */
			emula_fpa_sw2hw(&sigframe.sf_fpasave);
		}

		/*
		 * FPA state is saved in all cases except for a SIGFPE.
		 * In this case we need to preserve compatibility with 
		 * user-level SIGFPE fault handlers which expect that any 
		 * change they make to the fpa state in the chip will persist 
		 * on return from the handler.  This should be removed in the 
		 * next release (V2.0) of PTX.
		 */
		if(sig == SIGFPE) {
			/*
			 * Abnormal case:  give signal handler same
			 * state as caller and be sure that caller
			 * inherits any changes made by the signal
			 * handler on return.
			 *
			 * If fpa emulation, then the emulation registers
			 * already have valid caller's state.
			 *
			 * If fpa hardware, then fpa hardware registers
			 * also still have valid caller's state.
			 *
			 * Thus, all we need to do is be sure *not* to
			 * reset the fpa to clean state.
			 */
			sigframe.sf_uflags &= ~UF_SAVED_FPA;
		} else {
			/*
			 * Normal case: give signal handler clean fpa
			 * state and be sure to restore caller's state
			 * on return.
			 */
			init_fpa_state();
			sigframe.sf_uflags |= UF_SAVED_FPA;
		}
	}
#endif  /* FPA */
#endif	/* FPU_SIGNAL_BUG */

#ifdef	FPU_SIGNAL_BUG
	if (copyout((caddr_t)&sigframe, (caddr_t)fp,
	     sizetocopy) == EFAULT)
		goto bad;
#else	/* FPU_SIGNAL_BUG */
	if (copyout((caddr_t)&sigframe, (caddr_t)fp,
	     sizeof (struct sigframe)) == EFAULT)
		goto bad;
#endif	/* FPU_SIGNAL_BUG */

	/*
	 * sigcontext goes on previous stack.
	 */

	lsigc.sc_onstack = oonstack;
	lsigc.sc_mask = mask;

	/* 
	 * Setup return frame.
	 *
	 * Note that the sp saved here is where the pc and processor flags from
	 * the saved sigcontext structure WILL BE RESTORED TO in sigcleanup().
	 */

	lsigc.sc_sp = (int)&scp->sc_flags;
	lsigc.sc_pc = regs[EIP];
	lsigc.sc_flags = regs[FLAGS];
#ifdef COBUG
	if (u.u_flags & UF_FPSTEP) {
		lsigc.sc_flags = (lsigc.sc_flags & ~FLAGS_TF) | (
			 (u.u_flags & UF_OTBIT) ? FLAGS_TF : 0);
	}
#endif
	if (copyout((caddr_t)&lsigc, (caddr_t)scp1,
	     sizeof (struct sigcontext)) == EFAULT)
		goto bad;

	regs[EAX] = (int)p;			/* real user signal handler */
	regs[ESP] = (int)fp;
	regs[EIP] = (unsigned int)u.u_sigtramp;	/* sigcode trampoline */
	return;

	/*
	 * Process has trashed its stack; give it an illegal
	 * instruction to halt it in its tracks.
	 */
bad:
	u.u_signal[SIGILL] = SIG_DFL;
	u.u_procp->p_sigignore &= ~sigmask(SIGILL);
	u.u_procp->p_sigcatch &= ~sigmask(SIGILL);
	u.u_procp->p_sigmask &= ~sigmask(SIGILL);
	psignal(u.u_procp, SIGILL);
}

/*
 * sigcleanup()
 *	Clean up state after a signal has been handled.
 *
 * Reset signal mask and stack state from context left by sendsig (above).
 *
 * Restore user SP so library routine can "simulate iret" with
 * "popf" and "ret".
 *
 * This is called as a normal syscall; thus must arrange to restore
 * EAX and ECX as if they were return values from the syscall.
 *
 * NOTE: The user-level longjmp() function assumes knowledge of the
 * 	 sigclean and sigcontext structures.  Any change to the format 
 * 	 or size of these structures must be mirrored in longjmp().
 *	 (you're lucky, I learned the hard way :-)).
 */

sigcleanup()
{
	register int *regs;
	struct sigclean {
		struct	sigcontext *sf_scp;
		int	edx;
		int	ecx;
		int	eax;
		int	sf_uflags;
#ifdef	FPU_SIGNAL_BUG
		struct	fpusave	sf_fpusave;
#ifdef	FPA
		struct	fpasave	sf_fpasave;
#endif	/* FPA */
#endif	/* FPU_SIGNAL_BUG */
	} sigclean;
	struct sigcontext sc_clean;
#ifdef	FPU_SIGNAL_BUG
	unsigned int	fpusave_offset;
	unsigned int	sizetocopy;	
	regs = u.u_ar0;
	/*
	 * First get the sigframe that remains after the handler was called.
	 */
	sizetocopy = sizeof(struct sigclean) - sizeof(struct fpusave)
					     - sizeof(struct fpasave);

	if (copyin((caddr_t)regs[ESP],(caddr_t)&sigclean,sizetocopy) == EFAULT)
		goto bad;

	/*
	 * If we saved floating point context in sendsig(), go fetch.
	 *
	 * If the fpu state was saved, but not the fpa state, then
	 * we'll just copyin the fpu state.  If we saved both the fpu
	 * state and the fpa state, then we'll copy it all in.  But,
	 * if we saved the fpa state but not the fpu state, then the
	 * fpu state save area is guaranteed to be full of zeros.  Thus,
	 * we'll copy it in along with the fpa state (i.e., don't think
	 * that this is worth additional complexity).
	 */
	if (sigclean.sf_uflags & (UF_SAVED_FPU | UF_SAVED_FPA)) {
		sizetocopy = sizeof(struct fpusave);
		if (sigclean.sf_uflags & UF_SAVED_FPA)
			sizetocopy += sizeof(struct fpasave);
		fpusave_offset = (u_int)(&(((struct sigclean *)0)->sf_fpusave));
		if (copyin((caddr_t)regs[ESP]+fpusave_offset,
				   (caddr_t)&(sigclean.sf_fpusave),
				   sizetocopy) == EFAULT) {
			goto bad;
		}
	}

#else	/* FPU_SIGNAL_BUG */	

	regs = u.u_ar0;

	/*
	 * First get the sigframe that remains after the handler was called.
	 */

	if (copyin((caddr_t)regs[ESP], (caddr_t)&sigclean,
	     sizeof (struct sigclean)) == EFAULT)
		goto bad;

#endif	/* FPU_SIGNAL_BUG */	

	/*
	 * Get the signal context.
	 */

	if (copyin((caddr_t)sigclean.sf_scp, (caddr_t)&sc_clean,
	     sizeof (struct sigcontext)) == EFAULT)
		goto bad;

	u.u_onstack = sc_clean.sc_onstack & 01;

	(void) p_lock(&u.u_procp->p_state, SPLHI);	/* see sigblock() */
	u.u_procp->p_sigmask = sc_clean.sc_mask
			& ~(sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));
	v_lock(&u.u_procp->p_state, SPL0);
		 
	/*
	 * If we are changing stacks because of some form of onstack
	 * usage, we must restore the pc and processor status from the
	 * sigcontext onto the stack we are returning to.  Otherwise,
	 * the needed pc and processor status are already in the right
	 * position, and there's no need to touch them.
	 */

	if (sc_clean.sc_sp != (int)&sigclean.sf_scp->sc_flags) {
		/*
		 * "Guarantee" that the needed space is available.
		 */
		if (!u.u_onstack &&
		    (int)sc_clean.sc_sp <= USRSTACK - ctob(u.u_ssize))
			(void) grow((unsigned)sc_clean.sc_sp);
		if (copyout((caddr_t)&sc_clean.sc_flags, 
			(caddr_t)sc_clean.sc_sp, 2*sizeof(int)) == EFAULT)
			goto bad;
	}
	regs[ESP] = sc_clean.sc_sp;

#ifdef COBUG
	if (sigclean.sf_uflags & UF_FPSTEP) {
		regs[FLAGS] = (regs[FLAGS] & ~FLAGS_TF) | (
			 (sigclean.sf_uflags & UF_OTBIT) ? FLAGS_TF : 0);
		u.u_flags &= ~(UF_FPSTEP|UF_OTBIT);
	}
#endif

#ifdef  FPU_SIGNAL_BUG
	/*
	 * If we saved the fpu state in sendsig(), restore it here.
	 */
	if (sigclean.sf_uflags & UF_SAVED_FPU) {
		u.u_fpusave = sigclean.sf_fpusave;
		if (l.usingfpu) {
			/*
			 * Disable the fpu so that the next float operation
			 * will fault-in the fpu state saved in the uarea.
			 */
			disable_fpu();
		}
	}
#ifdef  FPA
	/*
	 * If we saved the fpa state in sendsig(), restore it here.
	 */
	if (sigclean.sf_uflags & UF_SAVED_FPA){
		if (u.u_procp->p_fpa == FPA_HW) {
			bcopy((char *)&sigclean.sf_fpasave, 
			      (char *)&u.u_fpasave,
			      sizeof(struct fpasave));
			/*
			 * If we have been using the FPA, then disable it so
			 * that the next reference will fault in the fpa state
			 * saved in the uarea.
			 *
			 * Since we may not be about to context switch, be 
			 * sure to release any stale fpa mapping that may 
			 * exist in the TLB.
			 */
			if (*(int *)VA_FPA_PTE != PG_INVAL) {
				*(int *)VA_FPA_PTE = PG_INVAL;
				FLUSH_TLB();
			}
		} else {	/* p_fpa == FPA_SW || p->p_fpa == FPA_FRCSW */
			ASSERT(u.u_procp->p_fpa != FPA_NONE,
				"Sigcleanup(): bad fpa state");
			emula_fpa_hw2sw(&sigclean.sf_fpasave);
		}
	}
#endif  /* FPA */
#endif	/* FPU_SIGNAL_BUG */

	/*
	 * Restore temp registers.  Use u. return args for EAX, ECX since
	 * this is treated as a syscall.
	 */

	u.u_r.r_val1 = sigclean.eax;
	u.u_r.r_val2 = sigclean.ecx;
	regs[EDX] = sigclean.edx;
	u.u_error = 0;					/* just in case */
	return;

bad:
	/*
	 * Process has trashed its stack; give it an illegal
	 * instruction to halt it in its tracks.
	 */
	u.u_signal[SIGILL] = SIG_DFL;
	u.u_procp->p_sigignore &= ~sigmask(SIGILL);
	u.u_procp->p_sigcatch &= ~sigmask(SIGILL);
	u.u_procp->p_sigmask &= ~sigmask(SIGILL);
	psignal(u.u_procp, SIGILL);
	u.u_error = 0;					/* just in case */
}



/*
 * This is the 4.3 sigreturn().
 */

sigreturn()
{

	register struct a {
		int	r_eax;
		int	r_eip;
		int	r_cs;
		int	r_efl;
		int	r_sp;
	} *args = (struct a *)u.u_ar0;

	struct { 
		struct sigcontext *scp; 
		int 		   edx;
		int		   ecx;
		int		   eax;
		int		   sf_uflags;
#ifdef	FPU_SIGNAL_BUG
		struct fpusave	   sf_fpusave;
#ifdef	FPA
		struct fpasave	   sf_fpasave;
#endif	/* FPA */
#endif	/* FPU_SIGNAL_BUG */
	} sf;
	struct sigcontext sc;
#ifdef	FPU_SIGNAL_BUG
	unsigned int	fpusave_offset;
	unsigned int	sizetocopy;

	sizetocopy = sizeof(sf) - sizeof(struct fpusave) 
				- sizeof(struct fpasave);

	if (copyin((caddr_t)args->r_sp, (caddr_t)&sf, sizetocopy) == EFAULT)
		goto bad;

	/*
	 * If we saved floating point context in sendsig(), go fetch.
	 *
	 * If the fpu state was saved, but not the fpa state, then
	 * we'll just copyin the fpu state.  If we saved both the fpu
	 * state and the fpa state, then we'll copy it all in.  But,
	 * if we saved the fpa state but not the fpu state, then the
	 * fpu state save area is guaranteed to be full of zeros.  Thus,
	 * we'll copy it in along with the fpa state (i.e., don't think
	 * that this is worth additional complexity).
	 */
	if (sf.sf_uflags & (UF_SAVED_FPU | UF_SAVED_FPA)) {
		sizetocopy = sizeof(struct fpusave);
		if (sf.sf_uflags & UF_SAVED_FPA)
			sizetocopy += sizeof(struct fpasave);
		fpusave_offset = (u_int)((caddr_t)&sf.sf_fpusave
					       - (u_int)&sf);
		if (copyin((caddr_t)args->r_sp+fpusave_offset,
				   (caddr_t)&(sf.sf_fpusave),
				   sizetocopy) == EFAULT) {
			goto bad;
		}
	}


#else	FPU_SIGNAL_BUG

	if (copyin((caddr_t)args->r_sp, (caddr_t)&sf, sizeof sf) == EFAULT)
		goto bad;

#endif	FPU_SIGNAL_BUG
	if (copyin((caddr_t)sf.scp, (caddr_t)&sc, sizeof sc) == EFAULT)
		goto bad;
	u.u_onstack = sc.sc_onstack & 01;

	(void) p_lock(&u.u_procp->p_state, SPLHI);	/* see sigblock() */

	u.u_procp->p_sigmask =
		sc.sc_mask &~ (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));
	v_lock(&u.u_procp->p_state, SPL0);

	args->r_sp = sc.sc_sp + 2*sizeof(int);
	args->r_eax = sf.eax;
	((int *)args)[ECX] = sf.ecx;
	((int *)args)[EDX] = sf.edx;

	/* should complain if illegal */
	args->r_efl = sc.sc_flags & ~0x3f200 | 0x1200;	

#ifdef COBUG
	if (sf.sf_uflags & UF_FPSTEP) {
		args->r_efl = (args->r_efl & ~FLAGS_TF) | (
				 (sf.sf_uflags & UF_OTBIT) ? FLAGS_TF : 0);
		u.u_flags &= ~(UF_FPSTEP|UF_OTBIT);
	}
#endif

#ifdef	FPU_SIGNAL_BUG
	/*
	 * If we saved the fpu state in sendsig(), restore it here.
	 */
	if (sf.sf_uflags & UF_SAVED_FPU) {
		u.u_fpusave = sf.sf_fpusave;
		if (l.usingfpu) {
			/*
			 * Disable the fpu so that the next float operation
			 * will fault-in the fpu state saved in the uarea.
			 */
			disable_fpu();
		}
	}
#ifdef  FPA
	/*
	 * If we saved the fpa state in sendsig(), restore it here.
	 */
	if (sf.sf_uflags & UF_SAVED_FPA){
		if (u.u_procp->p_fpa == FPA_HW) {
			bcopy((char *)&sf.sf_fpasave, 
			      (char *)&u.u_fpasave,
			      sizeof(struct fpasave));
			/*
			 * If we have been using the FPA, then disable it so
			 * that the next reference will fault in the fpa state
			 * saved in the uarea.
			 *
			 * Since we may not be about to context switch, be 
			 * sure to release any stale fpa mapping that may 
			 * exist in the TLB.
			 */
			if (*(int *)VA_FPA_PTE != PG_INVAL) {
				*(int *)VA_FPA_PTE = PG_INVAL;
				FLUSH_TLB();
			}
		} else {	/* p_fpa == FPA_SW || p->p_fpa == FPA_FRCSW */
			ASSERT(u.u_procp->p_fpa != FPA_NONE,
				"Sigcleanup(): bad fpa state");
			emula_fpa_hw2sw(&sf.sf_fpasave);
		}
	}
#endif  /* FPA */
#endif	/* FPU_SIGNAL_BUG */

	/* compensate for -2 in syscall */
	args->r_eip = sc.sc_pc + 2;	

	/* fake a restarted call to avoid register clobbering */
	u.u_eosys = RESTARTSYS;	
	u.u_error = 0;
	return;
bad:
	/*
	 * Process has trashed its stack; give it an illegal
	 * instruction to halt it in its tracks.
	 */
	u.u_signal[SIGILL] = SIG_DFL;
	u.u_procp->p_sigignore &= ~sigmask(SIGILL);
	u.u_procp->p_sigcatch &= ~sigmask(SIGILL);
	u.u_procp->p_sigmask &= ~sigmask(SIGILL);
	psignal(u.u_procp, SIGILL);
	u.u_error = 0;
	return;
}


/*
 * start_engine()
 *	Start another processor by "unpausing" it.
 *
 * Called by tmp_ctl TMP_ONLINE command.
 *
 * The semaphore tmp_onoff is assumed to be held by the caller.
 * This semaphore guarantees that only one on/off line transaction
 * occurs at a time.  No real need to single-thread these on SGS,
 * but doesn't hurt and provides some basic sanity (who knows, maybe
 * there is some hidden reason! ;-)
 */

start_engine(eng)
	register struct engine *eng;
{
	spl_t	s = splhi();

#ifdef	KXX

	/*
	 * Release from reset and un-pause at the same time.
	 */

	wrslave(eng->e_slicaddr, SL_P_PCCHS, SLB_RES | SLB_PAUSE);

#else	Real HW

#ifdef	notyet				/* SGS VLSI doesn't support flush yet */
	u_char	bic_slic;
	u_char	chan_ctl;

	/*
	 * Re-enable the appropriate BIC channel (this is left disabled
	 * by an offline).  This is a NOP on early rev BIC's.
	 *
	 * Note that only one SLIC on the processor board talks to the BIC.
	 */

	bic_slic = BIC_SLIC(eng->e_slicaddr,
			slic_to_config[eng->e_slicaddr]->cd_flags);
	chan_ctl = (eng->e_slicaddr & 0x01) ? BIC_CDIAGCTL1 : BIC_CDIAGCTL0;

	wrSubslave(bic_slic, PROC_BIC, chan_ctl, 
		(u_char) (rdSubslave(bic_slic, PROC_BIC, chan_ctl) & ~BICCDC_DISABLE));
#endif	notyet				/* SGS VLSI doesn't support flush yet */

	/*
	 * Un-hold the processor, turn on the LED, and *don't* reset.
	 * Also enable NMI's: it's ok for 1st online (don't expect any NMI
	 * sources) and subsequent online's need NMI's enabled here since they
	 * don't execute localinit() to enable NMI's.  This gives small risk
	 * of strange crash if NMI is asserted on 1st online (since processor
	 * is an 8086 at this time); if a problem, need to keep state in
	 * e_flags whether the processor has ever been online'd before, and
	 * initialize PROC_CTL differently here 1st time vs subsequent times.
	 */

	if (light_show && fp_lights)
		FP_LIGHTON(eng - engine);

	if (eng->e_flags&E_SGS2)
		unhold_proc((int)eng->e_slicaddr);
	else
		wrslave(eng->e_slicaddr, PROC_CTL,
		PROC_CTL_NO_SSTEP | PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);

#endif	KXX

	splx(s);
}

/*
 * halt_engine()
 *	Halt processor via pause and reset.
 *
 * Turn off processor light.
 * Done implicitly via reset on B8K.
 * If fp_lights then done explicitly.
 *
 * Called by tmp_ctl with TMP_OFFLINE command to shutdown a processor.
 */

#ifndef	KXX
#ifdef	notyet				/* SGS VLSI doesn't support flush yet */
static	struct	proc_cmcs {
	u_char	pc_subaddr;		/* sub-slave address of CMC */
	u_int	pc_diag_flag;		/* flags -- all zero if CMC in use */
}	proc_cmcs[] = {
	{ PROC_CMC_0, CFG_SP_CMC0|CFG_SP_DRAM_0|CFG_SP_TRAM_0|CFG_SP_SRAM_0 },
	{ PROC_CMC_1, CFG_SP_CMC1|CFG_SP_DRAM_1|CFG_SP_TRAM_1|CFG_SP_SRAM_1 },
};
#endif	notyet				/* SGS VLSI doesn't support flush yet */
#endif	KXX

halt_engine(eng)
	register struct engine *eng;
{
	spl_t	s = splhi();

#ifdef	KXX

	/*
	 * Pause processor; then reset it!
	 */

	wrslave(eng->e_slicaddr, SL_P_PCCHS, ~SLB_PAUSE);
	wrslave(eng->e_slicaddr, SL_P_PCCHS, ~(SLB_RES | SLB_PAUSE));

#else	Real HW

#ifdef	notyet				/* SGS VLSI doesn't support flush yet */
	register struct ctlr_desc *cd = slic_to_config[eng->e_slicaddr];
	register struct proc_cmcs *pc;
	register int	i;
	u_char		bic_slic;
	u_char		chan_ctl;
	u_char		cmc_mode;
#endif	notyet				/* SGS VLSI doesn't support flush yet */
	u_char		slicid = eng->e_slicaddr;

	/*
	 * HOLD the processor, but don't reset it (also turn OFF led).
	 * Wait for processor to be HELD.
	 */

	if (eng->e_flags&E_SGS2) {
		/*
		 * No way for a remote processor to turn off the LED.
		 */
		hold_proc(slicid);
		while (!isheld(slicid))
			continue;
	} else {
		wrslave(slicid, PROC_CTL,
				PROC_CTL_LED_OFF | PROC_CTL_NO_NMI |
				PROC_CTL_NO_SSTEP | PROC_CTL_NO_RESET);
		while (rdslave(slicid, PROC_STAT) & PROC_STAT_NO_HOLDA)
			continue;
	}

#ifdef	notyet				/* SGS VLSI doesn't support flush yet */
	/*
	 * NOTE: the flush algorithm is probably WRONG!  Verify/fix
	 * when VLSI does support the flush function.
	 */
	/*
	 * Flush the processor's cache.
	 *
	 * For each cache set, if it passed all power-up diagnostics then
	 * tell the CMC it's a "master", flush it, make it a "slave" again.
	 */

	for (pc = proc_cmcs, i = 0; i < cd->cd_p_nsets; i++, pc++) {

		/*
		 * If any diagnostic flag is on, this cache set wasn't in use.
		 */

		if (eng->e_diag_flag & pc->pc_diag_flag)
			continue;

		/*
		 * Make the CMC the "master" and start the flush.
		 */

		cmc_mode = rdSubslave(slicid, pc->pc_subaddr, CMC_MODE);
		wrSubslave(slicid, pc->pc_subaddr, CMC_MODE,
				cmc_mode & ~(CMCM_SLAVE | CMCM_DISA_FLUSH));

		/*
		 * Wait for flush to finish.
		 */

		while (rdSubslave(slicid, pc->pc_subaddr, CMC_STATUS) & CMCS_FLUSH)
			continue;

		/*
		 * Make the CMC a "slave" again.
		 */

		wrSubslave(slicid, pc->pc_subaddr, CMC_MODE, cmc_mode);
	}

	/*
	 * Isolate the processor from the bus by turning off the appropriate
	 * BIC channel.  This is a NOP on early rev BIC's.
	 *
	 * Note that only one SLIC on the processor board talks to the BIC.
	 */

	bic_slic = BIC_SLIC(slicid, cd->cd_flags);
	chan_ctl = (slicid & 0x01) ? BIC_CDIAGCTL1 : BIC_CDIAGCTL0;

	wrSubslave(bic_slic, PROC_BIC, chan_ctl, 
		(u_char) (rdSubslave(bic_slic, PROC_BIC, chan_ctl) | BICCDC_DISABLE));
#endif	notyet				/* SGS VLSI doesn't support flush yet */
#endif	KXX

	if (light_show && fp_lights)
		FP_LIGHTOFF(eng - engine);

	splx(s);
}

#define	TOP_KSTK	((int *)(VA_UAREA + UPAGES * NBPG))
#define	BOT_KSTK	((int *)(VA_UAREA + sizeof(struct user)))
#define	POPECX		0x59			/* pops a single argument */
#define	ADDLESP		0xc483			/* addl $xxx, %esp */
/*
 * Use these variables to bound stack when !upyet
 *	they're initialized in printstack.
 */
int *bottom_bp, *top_bp;

#ifndef KLINT
/*VARARGS*/
printstack(dummy)
{
	int	*i =  &dummy;

	printf("Cpu registers:\n");
	printf(" eax=%x ebx=%x ecx=%x edx=%x\n", i[7], i[4], i[6], i[5]);
	printf(" esi=%x edi=%x ebp=%x esp=%x\n", i[1], i[0], i[15], &i[18]);
	/*
	 * for bounding stack when !upyet
	 */
	bottom_bp = (int *)(&dummy)[-2];
	top_bp = (int *)(((int)bottom_bp + UPAGES*NBPG) & ~CLOFSET);
	hexdumpstack(dummy);
	printf("Stack @ 0x%x\n",
		(((int*)VA_PT)[btop(VA_UAREA)] & PG_PFNUM)
		+ ((int)&dummy - VA_UAREA));
	printstacktrace( (int *) ((&dummy)[-2]) );
}

/*ARGSUSED*/
hexdumpstack(dummy)
{
	int	*a = (int *)(((&dummy)[-2]) &~ 0xf);
	char	s1[10], s2[10], s3[10], s4[10];

	printf("Hex dump of panic stack:\n");
	/*
	 * stop dump when address a is below stack top, and
	 * while not on a page boundary.
	 */
	while (a < TOP_KSTK) {
		if (!upyet && (a >= top_bp))
			break;
		hexstr(s1, a[0]);
		hexstr(s2, a[1]);
		hexstr(s3, a[2]);
		hexstr(s4, a[3]);
		printf("%x: %s%x %s%x %s%x %s%x\n", a,
			s1, a[0], s2, a[1],
			s3, a[2], s4, a[3]);
		a += 4;
	}
}
#endif /* KLINT */

hexstr(s, n)
	char	*s;
	int	n;
{
	int	i = 0;
	u_int	mask = 0xf0000000;

	bcopy("       ", s, 8);
	while (mask && (n & mask) == 0) {
		i++;
		mask >>= 4;
	}
	s[i] = '\0';
}

#ifndef KLINT
/*ARGSUSED*/
printstacktrace(bp)
	register int *bp;			/* current frame pointer */
{
	int	*newbp = (int *)(bp[0]);	/* next frame pointer */
	int	*oldpc = (int *)(bp[1]);	/* pc this frame returns to */
	int	*oldap = &bp[2];		/* point to first argument */
	int	 nargs;				/* number of arguments passed */
	extern	int etext;

	if ((oldpc < &etext) && ((*oldpc & 0xffff) == ADDLESP))
		nargs = ((*oldpc>>16) & 0xff) / 4;
	else if ((oldpc < &etext) && ((*oldpc & 0xff) == POPECX))
		nargs = 1;
	else
		nargs = 5;	/* default for OS */
		
	printf("@ 0x%x call(", oldpc);
	while (nargs) {
		printf("0x%x", *oldap++);
		if (--nargs)
			printf(", ");
	}
	printf(")\n");


	if (!upyet && (newbp < top_bp) && (newbp > bp) && (newbp > bottom_bp))
		printstacktrace(newbp);
	else if ((newbp < TOP_KSTK) && (newbp > bp) && (newbp > BOT_KSTK))
		printstacktrace(newbp);
}
#endif /* KLINT */

/*
 * pause_self()
 *	Stop processor via pause self.
 *
 * Typically called from from NMI handler when a panic is in progress.
 * There is no return.
 *
 * Caller must call with SPLHI.
 */

pause_self()
{
	caddr_t	sp;				/* stack variable, -4(%ebp) */

	/*
	 * Save all registers.  Save "panic" sp in l.panicsp.
	 * Also save page-table in use by this processor when the
	 * system went down.
	 */

#ifdef lint
	sp = (caddr_t)0;
#else
	asm("pushal");
	asm("movl %esp, -4(%ebp)");
#endif lint
	l.panicsp = sp;
	l.panic_pt = READ_PTROOT();
	if (l.usingfpu)
#ifdef	FPU_SIGNAL_BUG
		save_fpu(&u.u_fpusave);
#else
		save_fpu();
#endif
#ifdef FPA
	/* if have fpa, not idle and using it */
	if (l.fpa && !l.noproc && u.u_procp->p_fpa == FPA_HW)
#ifdef	FPU_SIGNAL_BUG
		save_fpa(&u.u_fpasave);
#else
		save_fpa();
#endif
#endif FPA

	/*
	 * Turn off light show if enabled.
	 */

	if (light_show) {
		if (fp_lights)
			FP_LIGHTOFF(l.me);
#ifdef	KXX
		if (fp_lights <= 0)
			(void) rdslave(l.eng->e_slicaddr, SL_P_LIGHTOFF);
#else
		*(int *) PHYS_LED = 0;
#endif	KXX
	}

	/*
	 * Pause myself!
	 */

	l.eng->e_flags |= E_PAUSED;

#ifdef	KXX

	wrslave(l.eng->e_slicaddr, SL_P_PCCHS, ~SLB_PAUSE);
	/*NOTREACHED*/

#else	Real HW

	/*
	 * HOLD the processor, but don't reset it (also turn OFF led).
	 * Wait for processor to be HELD.  The while() will not actually
	 * ever finish.  Call lwrslave() and lrdslave() so we don't go away
	 * with slic_mutex held.
	 *
	 * In this case, *don't* isolate the processor from the bus, since
	 * may want to take a dump.  Since pausing self, not clear we could
	 * isolate self from bus.
	 */
#ifdef	DEBUG			/*XXX*/
	flush_cache();		/*XXX*/
#endif	DEBUG			/*XXX*/

	if (l.eng->e_flags&E_SGS2) {
		/*
		 * No way for a remote processor to turn off the LED.
		 */
		hold_proc(l.eng->e_slicaddr);
		while (isheld(l.eng->e_slicaddr))
			continue;
	} else {
		lwrslave(l.eng->e_slicaddr, PROC_CTL,
				PROC_CTL_LED_OFF | PROC_CTL_NO_NMI |
				PROC_CTL_NO_SSTEP | PROC_CTL_NO_RESET);
		while (lrdslave(l.eng->e_slicaddr, PROC_STAT) & PROC_STAT_NO_HOLDA)
			continue;
	}
#endif	KXX
}

/*
 * donmi()
 *	Report reason for NMI (decode errors, etc) and stop the machine.
 */

donmi()
{
	register int sgs2 = l.eng->e_flags & E_SGS2;

#ifdef	KXX
	register struct cpuslic *sl = (struct cpuslic *)va_slic;
	u_char	regval;		/* Value read from slic */

	/*
	 * Currently, NMIs are presented to the processor in these situations.
	 *	- access error
	 *		- Bus Timeout
	 *		- ECC Uncorrectable error
	 *	- Cache Parity error.
	 *	- SLIC NMI (sent by software)
	 *	- kernel stack overflow (only if -DMMU_BPT_REDZONE)
	 * In all cases, the processor pauses itself.
	 */

	if (!upyet) {
		/* disable NMIs */
		wrslave(sl->sl_procid, SL_P_CONTROL, ~SLB_E_NMI);
	} else {
		(void) splhi();

		/* disable NMIs */
		l.cntrlreg &= ~SLB_E_NMI;
		wrslave(sl->sl_procid, SL_P_CONTROL, l.cntrlreg);
	}

	/*
	 * Check for access error.
	 */
	regval = rdslave(sl->sl_procid, SL_G_ACCERR);
	if ((regval & SLB_ACCERR) == 0) {
		CPRINTF("Processor ");
		if (!upyet)
			CPRINTF("?? ");
		else
			CPRINTF("%d ", l.me);
		printf("(slic %d) Access Error\n", sl->sl_procid);
		/*
		 *+ A bus access error occurred, generating a
		 *+ Non-Maskable Interrupt.
		 */
		access_error(regval);
	}
	/*
	 * Check for Cache Parity error.
	 */
	regval = rdslave(sl->sl_procid, SL_P_CACHEPAR);
	if ((regval & SLB_CPARMSK) != SLB_CPARMSK) {
		CPRINTF("Processor ");
		if (!upyet)
			CPRINTF("?? ");
		else {
			/*
			 * Disable cache...
			 */
			l.cntrlreg &= ~(SLB_ENB0|SLB_INV0|SLB_ENB1|SLB_INV1);
			wrslave(sl->sl_procid, SL_P_CONTROL, l.cntrlreg);
			CPRINTF("%d ", l.me);
		}

		printf("(slic %d) Cache Parity Error\n", sl->sl_procid);
		printf("Cache Parity Error Status = 0x%x\n", regval);
		/*
		 *+ A cache parity error occurred, generating a
		 *+ Non-Maskable Interrupt.
		 */
	}

	/*
	 * Check for SLIC NMI.
	 */
	regval = sl->sl_nmiint;
	if (regval == PAUSESELF)
		pause_self();

	panic("NMI");
        /*
         *+ A Non-Maskable Interrupt was generated from the SLIC chip.
         */
	/*NOTREACHED*/

#else	Real HW
	u_char 	slicid = va_slic->sl_procid;
	u_char 	regval;
	u_char 	bic_slic;
	u_char 	accerr_reg;

	/*
	 * Currently, NMIs are presented to the processor in these situations:
	 *
	 *	- SLIC NMI (sent by software)
	 *	- Access error from access to reserved processor LOCAL address
	 *	- Access error:
	 *		- Bus Timeout
	 *		- ECC Uncorrectable error
	 *		- Processor fatal error (SGS only)
	 *	- Cache Parity error (these hold the processor & freeze the bus)
	 *
	 * In all cases, the processor pauses itself.
	 */

	/*
	 * Disable processor acceptence of NMI's. Other interrupts
	 * turned off by caller (currently only trap()), ie donmi
	 * must be called at splhi and cli.
	 */
	if (sgs2)
		disable_snmi(slicid);
	else
		wrslave(slicid, PROC_CTL, PROC_CTL_NO_NMI | PROC_CTL_NO_SSTEP | 
				  PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);

	if (sgs2) {
		register int flag = accerr_flag(slicid);

		/*
		 * First check for SLIC NMI or local reserved location access.
		 */
		if (va_slic->sl_nmiint == PAUSESELF)
			pause_self();

		if (va_slic->sl_nmiint == 0) {
			cmn_err(CE_PANIC, "RESERVED LOCATION ACCESS ERROR");
			/*
			 *+ The processor tried to access a reserved address that
			 *+ caused a Non-Maskable Interrupt to be generated.
			 */
			/*NOTREACHED*/
		}

		/*
		 * Not a board-local source (cache parity errors freeze the bus,
		 * so we're not executing if that happened).  Thus, it should be
		 * a bus access error, stored in per-channel BIC access error register.
		 *
		 * Assumes BIC access error register has identical bit assignments
		 * as 1st gen (032) processor board access error register, since
		 * access_error() is coded this way.
		 *
		 * Since system needs to be "up" enough to have filled out
		 * slic_to_config[]; just panic if not yet up far enough.
		 */

		if (slic_to_config[slicid] == NULL) {
			cmn_err(CE_PANIC, "ACCESS ERROR");
			/*
			 *+ The processor has discovered a bus access error
			 *+ that occurred early in the boot sequence.
			 *+ The information needed to map the error
			 *+ information to the source of the access error 
			 *+ hasn't been initialized yet.
			 */
			/*NOTREACHED*/
		}

		if (anyaccerr(flag)) {
			if (upyet) {
				cmn_err(CE_WARN,"Processor %d (slic %d) Access Error\n",
					l.me, slicid);
				/*
				 *+ A bus access error occurred, generating a
				*+ Non-Maskable Interrupt.
			 	*/
			} else {
				cmn_err(CE_WARN,"Processor ?? (slic %d) Access Error\n",
					slicid);
				/*
				 *+ A bus access error occurred, generating a
				*+ Non-Maskable Interrupt.
			 	*/
			}
			sgs2_access_error(flag);
		}
	} else {
		/*
		 * First check for SLIC NMI or local reserved location access.
		 */

		regval = rdslave(slicid, PROC_FLT);

		if ((regval & PROC_FLT_SLIC_NMI) == 0) {	/* SLIC NMI */
			if (va_slic->sl_nmiint == PAUSESELF)
				pause_self();
			panic("SLIC NMI");
			/*
			 *+ A Non-Maskable Interrupt was received 
			 *+ from the SLIC chip.
			 */
			/*NOTREACHED*/
		}

		if ((regval & PROC_FLT_ACC_ERR) == 0) {		/* Reserved Space */
			panic("RESERVED LOCATION ACCESS ERROR");
			/*
			 *+ The processor tried to access a reserved address
			 *+ that caused a Non-Maskable Interrupt to be generated.
			 */
			/*NOTREACHED*/
		}

		/*
		 * Not a board-local source (cache parity errors freeze the bus,
		 * so we're not executing if that happened).  Thus, it should be
		 * a bus access error, stored in per-channel BIC access error register.
		 *
		 * Assumes BIC access error register has identical bit assignments
		 * as 1st gen (032) processor board access error register, since
		 * access_error() is coded this way.
		 *
		 * Since system needs to be "up" enough to have filled out
		 * slic_to_config[]; just panic if not yet up far enough.
		 */

		if (slic_to_config[slicid] == NULL) {
			panic("ACCESS ERROR");
			/*
			 *+ The processor has discovered a bus access error
			 *+ that occurred early in the boot sequence.
			 *+ The information needed to map the error
			 *+ information to the source of the access error
			 *+ hasn't been initialized yet.
			 */
			/*NOTREACHED*/
		}
		bic_slic = BIC_SLIC(slicid, slic_to_config[slicid]->cd_flags);
		accerr_reg = (slicid & 0x01) ? BIC_ACCERR1 : BIC_ACCERR0;

		regval = rdSubslave(bic_slic, PROC_BIC, accerr_reg);

		if ((regval & BICAE_OCC) == 0) {
			printf("Processor %d (slic %d) Access Error\n", l.me, slicid);
			/*
			 *+ A bus access error occurred, generating a
			 *+ Non-Maskable Interrupt.
			 */
			access_error(regval);
			/*NOTREACHED*/
		}
	}

	/*
	 * All else failed ==> just panic.
	 *
	 * When/if handle cache parity errors somehow (unfreeze bus and
	 * un-hold processors), should do something here.  For now, system
	 * is *gone* if these happen.
	 */

	panic("NMI");
        /*
         *+ A Non-Maskable Interrupt was generated by an unknown source.
         */
	/*NOTREACHED*/
#endif	KXX
}

/*
 * Clear the nmi error, returning true if the nmi was the result of
 * a bus timeout.
 */
clearnmi()
{
	u_char	my_slic = va_slic->sl_procid;
	int	timeout = 0;
#if 0
	int	sgs2 = l.eng->e_flags&E_SGS2;
#else
	extern int starttype;
	int	sgs2 = starttype == SLB_SGS2PROCBOARD;
#endif

	if (sgs2) {
		int flag = accerr_flag(my_slic);

		clraccerr(my_slic);	/* Takes it out of the BIC */
		reset_snmi(my_slic);	/* ... and the SIC */
		enable_snmi(my_slic);
		timeout = istimeout(flag);
	} else {
		extern	struct	ctlr_desc *slic_to_config[];
		u_char	bic_slic = BIC_SLIC(my_slic, slic_to_config[my_slic]->cd_flags);
		u_char	accerr_reg = (my_slic & 0x01) ? BIC_ACCERR1 : BIC_ACCERR0;
		u_char	regval = rdSubslave(bic_slic, PROC_BIC, accerr_reg);

		/*
		 * Writing anything to the BIC's access-error register clears it.
		 */
		wrSubslave(bic_slic, PROC_BIC, accerr_reg, 0xbb);
		timeout = ((~regval)&SLB_ATMSK) == SLB_AETIMOUT;
	}
	return(timeout);
}

/*
 * swt_fpu_pgxbug()
 *	See about backing out of FPU page-cross chip bug.
 *
 * i386 parts B1 and younger can lock up if the first byte of an FPU
 * instruction is in the last 8 bytes of a page, the last byte of the
 * page is an ESC instruction, interveaning bytes are all legit prefixes,
 * and reference to the next page causes a page fault.  Interrupts break the
 * lockup, but it locks up again unless the following page is made valid.
 * See B1 Errata sheet (12/17/86) bug # 17.
 *
 * hardclock() tested that this processor may suffer from the bug and the
 * pc is near the end of page.  hardclock() can't do the "fix" since it
 * might fault, and don't want to fault (and block) at raised SPL.
 *
 * Since fubyte() is relatively cheap, just fetch from the following page;
 * this is sufficient to work around the problem, and cheaper than trying to
 * fully test for the problem (would use fubyte() or equivalent to test
 * for the ESC at end of page).
 */

swt_fpu_pgxbug()
{
	(void) fubyte((caddr_t) u.u_ar0[PC] + NBPG);
}


#ifdef	DEBUG		/*XXX to EOF */
#ifndef	KXX		/*XXX*/
/*
 * flush_cache()
 *	Read enough memory locations to insure cache is flushed (no dirty
 *	cache blocks).
 */
#define	START_CACHE_FLUSH	((int *)CD_LOC)
#define	END_CACHE_FLUSH		((int *)((int)CD_LOC + 128*1024))
flush_cache()
{
	register int *p;

	for (p = START_CACHE_FLUSH; p < END_CACHE_FLUSH; p++)
		flush_ref(*p);
}
flush_ref(x)
{
#ifdef	lint
	lint_ref_int(x);
#endif	lint
}
#endif	KXX		/*XXX*/
#endif	DEBUG		/*XXX to EOF */


#if !defined(KXX)
dump_SGShw_regs()
{
	dump_SGSproc_regs();
#if defined(DEBUG) || defined(MFG)
	dump_SGSmem_regs();
#endif DEBUG||MFG
}

/*
 * For each SGS processor in the system, print it's
 *	status and fault registers, selected BDP LO and BDP HI registers,
 *	and selected BIC registers.
 */
#ifndef KLINT
dump_SGSproc_regs()
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register int i, unit;

	/*
	 * Load pointer to table of contents
	 * for SGS procesor boards in the system.
	 */
	toc = &CD_LOC->c_toc[SLB_SGSPROCBOARD];		/* SGS processors */

	/*
	 * Locate the array of descriptors for SGS memory boards.
	 *	search this array and print out information
	 *	on each configured memory board
	 */
	cd = &CD_LOC->c_ctlrs[toc->ct_start];
	unit = 0;
	for (i = 0; i < toc->ct_count; i++, cd++) {
		if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) {
			continue;
		}
		printf("processor %d ", unit);
		unit++;
		printf("stat 0x%x flt 0x%x\n", 
			rdslave(cd->cd_slic, PROC_STAT),
			rdslave(cd->cd_slic, PROC_FLT));
#if defined(DEBUG) || defined(MFG)
		dump_bdp_regs(cd->cd_slic, PROC_BDP_LO, 0);
		dump_bdp_regs(cd->cd_slic, PROC_BDP_HI, 1);
		/*
		 * Print bic info only on odd slic addresses
		 */
		if (cd->cd_slic & 1)
			dump_bic_regs(cd->cd_slic, PROC_BIC);
#endif DEBUG||MFG
	}
}
#endif /*KLINT*/

#if defined(DEBUG) || defined(MFG)
/*
 * For each SGS memory board in the system, print it's
 *	status register, selected BDP LO and BDP HI registers,
 *	and selected BIC registers.
 */
dump_SGSmem_regs()
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register int	i, unit;

	/*
	 * Load pointer to table of contents for SGS memory
	 * boards in the system.
	 */
	toc = &CD_LOC->c_toc[SLB_SGSMEMBOARD];

	/*
	 * Locate the array of descriptors for SGS memory boards.
	 *	search this array and print out information
	 *	on each configured memory board
	 */
	cd = &CD_LOC->c_ctlrs[toc->ct_start];
	unit = 0;
	for (i = 0; i < toc->ct_count; i++, cd++) {
		if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
			continue;
		printf("memory board %d ", unit);
		printf("status 0x%x\n", rdslave(cd->cd_slic, MEM_EDC));
		dump_bdp_regs(cd->cd_slic, MEM_BDP_LO, 0);
		dump_bdp_regs(cd->cd_slic, MEM_BDP_HI, 1);
		dump_bic_regs(cd->cd_slic, MEM_BIC);
		unit++;
	}
}

/*
 * dump bic registers selected 
 */
dump_bic_regs(slicid, bic)
u_char	slicid, bic;
{

	printf("BIC- errctl 0x%x buserr 0x%x slicerr 0x%x\n",
		rdSubslave(slicid,  bic, BIC_ERRCTL),
		rdSubslave(slicid,  bic, BIC_BUSERR),
		rdSubslave(slicid,  bic, BIC_SLICERR));
	printf("     accerr0 0x%x accerr1 0x%x cdiagctl0 0x%x cdiagctl1 0x%x\n",
		rdSubslave(slicid,  bic, BIC_ACCERR0),
		rdSubslave(slicid,  bic, BIC_ACCERR1),
		rdSubslave(slicid,  bic, BIC_CDIAGCTL0),
		rdSubslave(slicid,  bic, BIC_CDIAGCTL1));
}

/*
 * dump selected bdp registers
 */
dump_bdp_regs(slicid, bdp, lh)
u_char	slicid, bdp;
int lh;
{
	printf("BDP-%s datapar 0x%x syspar 0x%x sysparad 0x%x\n",
		(lh==0)?"L":"H",
		rdSubslave(slicid,  bdp, BDP_DATAPAR),
		rdSubslave(slicid,  bdp, BDP_SYSPAR),
		rdSubslave(slicid,  bdp, BDP_SYSPARAD));
}
#endif DEBUG||MFG
#endif KXX

/*
 * hold_proc(slicid)
 * 
 *	Called to assert the FORCE HOLD bit of the External Control register
 *	of the SIC of the processor indicated by slicid.  The indicated 
 *	processor will not return until it has been "unheld()".
 *
 *	Assumes that the target processor is an SGS2 (i486).  The caller
 *	assures this.
 *	
 *	Caller must call with SPLHI.  No locks (or gates) can be held by 
 *	the target processor since it is about to go away (remember the
 *	target processor may be the caller).
 */
hold_proc(slicid)
u_char	slicid;
{
	u_char	regval;

	/*
	 * This mix of bits represents the state of the bits for "normal"
	 * operation, except that we have now set EXT_CTL1_FORCE_HOLD
	 * to cause ourselves to be held.
	 */
	regval = (EXT_CTL1_NO_486_PARITY_ERR | EXT_CTL1_NO_RESET_SLIC_ERR |
		  EXT_CTL1_NO_BUS_SCAN | EXT_CTL1_NORMAL_OSC | 
		  EXT_CTL1_NO_START_INVAL| EXT_CTL1_FORCE_HOLD);

	/*
	 * If the target processor is also the caller then let's
	 * be sure to use lwrslave/lrdslave instead of wrslave/rdslave.
	 * Otherwise, we would go away while holding the slic_mutex lock,
	 * causing all other processors to block.
	 */
	if (l.eng->e_slicaddr == slicid) {
		lwrslave(slicid, EXT_CTL1, regval);
		while (lrdslave(slicid, EXT_CTL0) & EXT_CTL0_NO_HOLD_ACK) 
			continue;
	} else {
		wrslave(slicid, EXT_CTL1, regval);
		while (rdslave(slicid, EXT_CTL0) & EXT_CTL0_NO_HOLD_ACK) 
			continue;
	}
}

#ifdef	FPU_SIGNAL_BUG
init_fpu_state()
{
        u.u_fpusave.fpu_control = FPU_CONTROL_INIT;
        u.u_fpusave.fpu_status = FPU_STATUS_INIT;
        u.u_fpusave.fpu_tag = FPU_TAG_INIT;
        disable_fpu();
}

init_fpa_state()
{
        register struct proc *p=u.u_procp;

        if (p->p_fpa == FPA_HW) {
                fpa_init();
                if (*(int *)VA_FPA_PTE != PG_INVAL) {
                        *(int *)VA_FPA_PTE = PG_INVAL;
                        FLUSH_TLB();
                }
        } else {        /* FPA_SW | FPA_FRCSW */
                bzero((char *)u.u_fpaesave, sizeof(struct fpaesave));
                u.u_fpaesave->fpae_pcr = FPA_INIT_PCR;
        }
}
#endif  /* FPU_SIGNAL_BUG */
