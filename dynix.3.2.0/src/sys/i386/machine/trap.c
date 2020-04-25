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
static	char	rcsid[] = "$Header: trap.c 2.61 1992/02/13 00:16:14 $";
#endif

/*
 * trap.c
 *	trap() and syscall() trap handlers.
 *
 * TODO:
 *	Consider ways to merge SFSWAP usage and SW traps.
 */

/* $Log: trap.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/acct.h"
#include "../h/kernel.h"
#include "../h/vmsystm.h"
#include "../h/vmmeter.h"
#include "../h/file.h"
#include "../h/cmn_err.h"
#ifdef	DEBUG
#include "../sys/syscalls.c"
#endif	DEBUG

#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#ifndef	KXX
#include "../balance/cfg.h"
#include "../balance/bic.h"
#endif	KXX

#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/mmu.h"
#include "../machine/pte.h"
#include "../machine/vmparam.h"
#include "../machine/plocal.h"
#include "../machine/trap.h"
#include "../machine/intctl.h"
#include "../machine/inline.h"
#include "../machine/mftpr.h"
#include "../machine/hwparam.h"
#include "../machine/scan.h"

#include <syscall.h>

#ifdef	DEBUG
int	syscalltrace = 0;
int	traptrace = 0;
int	abtdebug = 0;
#endif	DEBUG

#define	B1BUG21			/* 80386 B-1 Errata # 21 (see 9/1/87 errata) */
#define LOCKBUG			/* bug on lock instructions */

/*
 * SZSVC == sizeof system-call instruction (int n), for backing up return PC
 * to re-execute system call.
 */

#define	SZSVC		2

int	weitek_emulation = 1;

extern	struct	sysent	sysent[];
extern	int	nsysent;

char	*trap_type[] = {
	"Integer Zero-Divide",
	"Debug Exception",
	"Non-Maskable Interrupt",
	"Single-Byte Interrupt (breakpoint)",
	"Interrupt On Overflow",
	"Array Bounds Check",
	"Undefined/Illegal Op-Code",
	"Device Not Available (FPU)",
	"System Error",
	"Reserved Trap/Interrupt Vector",	/* T_RES == 9 */
	"Invalid Task-State Segment",
	"Segment/Gate Not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved Vector (15)",			/* "can't" happen */
	"Co-Processor Error (FPU)"
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

bool_t	m_expandorwait();

/*
 * trap()
 *	Called from the trap handler when a processor trap occurs.
 *
 * See machine/reg.h for order of parameters.
 */

/*ARGSUSED*/
trap(type, edi, esi, ebp, unused, ebx, edx, ecx, eax, eip, cs, flags, sp, ss)
	int	type;		/* type of trap */
	unsigned ss;		/* only if inter-segment (user->kernel) */
	unsigned sp;		/* only if inter-segment (user->kernel) */
	unsigned flags;		/* extended-flags (see machine/psl.h) */
	unsigned cs;		/* sense user-mode entry from RPL field */
	unsigned eip;		/* return context */
	unsigned eax;		/* scratch registers */
	unsigned ecx;		/* ditto */
	unsigned edx;		/* more such */
	unsigned ebx;		/* register variable */
	unsigned unused;	/* temp SP (from push-all instruction) */
	unsigned ebp;		/* of interrupted frame */
	unsigned esi;		/* register variable */
	unsigned edi;		/* register variable */
{
	register struct proc *p;
	unsigned fltaddr;			/* address that faulted */
	int	i;
	register int sgs2 = l.eng->e_flags & E_SGS2;
#ifndef	KXX
	u_char	my_slic;
	u_char	bic_slic;
	u_char	accerr_reg;
	extern	struct	ctlr_desc *slic_to_config[];
#endif	KXX
	extern	(*swtrapvec[])();

	p = u.u_procp;

	if (u.u_prof.pr_scale) {
		/*
		 * Avoid race with hardclock.
		 */
		DISABLE();
		u.u_syst = u.u_ru.ru_stime;
		ENABLE();
	}
	l.cnt.v_trap++;

	if (USERMODE(cs)) {
		type |= T_USER;
		u.u_ar0 = (int *) &eax;
	}

#ifdef	DEBUG
	if (traptrace && (type & ~T_USER) != T_PGFLT) {
		printf("<%d,%s>: Trap type %d from %s mode, error code = 0x%x\n",
				p-proc, u.u_comm, type & ~T_USER,
				(type & T_USER) ? "user" : "kernel",
				l.trap_err_code);
		if (traptrace > 1)
			trap_dumpregs((int *)&eax);
	}
#endif	DEBUG

	switch (type) {

	default:
		CPRINTF("%s trap type %d, error code=0x%x\n",
				(type & T_USER) ? "User" : "Kernel",
				(type & ~T_USER),
				l.trap_err_code);
		type &= ~T_USER;
		trap_dumpregs((int *)&eax);
		if ((unsigned)type < TRAP_TYPES) {
			panic(trap_type[type]);
                       /*
                        *+ The processor took one of the listed traps
                        *+ and the kernel couldn't recover from it.
                        */
		} else {
			panic("trap");
                        /*
                         *+ The processor took an unknown trap type and
                         *+ the kernel couldn't recover from it.
                         */
		}
		/*NOTREACHED*/

	case T_DIVERR+T_USER:			/* integer zero-divide */
		u.u_code = FPE_INTDIV_TRAP;
		i = SIGFPE;
		break;

	case T_DBG+T_USER:			/* debug exceptions (user) */
		/*
		 * Handle single-step.
		 */
		i = READ_DSR();
		if (i & DSR_WATCHPT) {
			watchpoint(i, 0, eip);
			goto out;
		} else if (i & DSR_BS) {
			WRITE_DSR(DSR_CLEAR);
			flags &= ~FLAGS_TF;
			i = SIGTRAP;
			break;
		}
		/*
		 * Not single-step, must be watchpoint debug trap.
		 * Watchpoint() turns OFF debug traps and does
		 * mpt_stop() fussing.
		 */
#define	F1BUG
#ifdef F1BUG
		/*
		 * Chip bug. Certain undefined opcodes vector to T_DBG
		 * rather than T_UND. If all DSR bits are clear, assume
		 * it's a bogus interrupt and signal process.
		 */
		/*
	  	 * It's safe to assume that i386 chip bugs won't be present
		 * in i486 processors.
		 */
		if (!sgs2 && (i & (DSR_WATCHPT|DSR_BD|DSR_BT)) == 0) {
			WRITE_DSR(DSR_CLEAR);
			u.u_code |= ILL_RESOP_FAULT;
			i = SIGILL;
			break;
		}
#endif	/* F1BUG */
		goto out;

	case T_DBG:				/* debug exceptions (kernel) */
		i = READ_DSR();
		ASSERT((i & DSR_BS) == 0, "trap: kernel single-step");
                /*
                 *+ The processor took a single-step trap while
                 *+ executing in kernel mode.  Only user processes
                 *+ should take these traps.
                 */
		/*
		 * Not single-step, must be watchpoint debug trap.
		 * These can happen from within kernel mode if kernel
		 * does data transfer to/from location being debugged at.
		 */
		watchpoint(i, 1, eip);
		return;

	case T_NMI+T_USER:			/* NMI from user mode */
		/*
		 * Processor interrupts are OFF at entry, since enter
		 * thru interrupt-gate. NMI's are disabled at processor.
		 */
		/*
		 * If it was an access error I/O space, clear access error
		 * and signal user process rather than panicing.
		 */
#ifdef	KXX
		i = rdslave(va_slic->sl_procid, SL_G_ACCERR);
		if ((i & (SLB_ACCERR|SLB_AEIO)) == 0)
		{
			wrslave(va_slic->sl_procid, SL_G_ACCERR, (u_char)0xbb);
			/*
			 * Toggle nmi accept in the processor control register
			 * so an NMI that arrived concurrently will be seen
			 * when NMI's are reenabled.
			 */
			l.cntrlreg &= ~SLB_E_NMI;
			wrslave(va_slic->sl_procid, SL_P_CONTROL, l.cntrlreg);
			l.cntrlreg |= SLB_E_NMI;
			wrslave(va_slic->sl_procid, SL_P_CONTROL, l.cntrlreg);
			enable_nmi();
			i = SIGBUS;
			break;
		}
#else	Real HW
		my_slic = l.eng->e_slicaddr;
		if (sgs2) {
			register int flag = accerr_flag(my_slic);
			if (anyaccerr(flag) && !isio(flag)) {
				/*
		 		 * Toggle nmi accept in the processor control register
		  	 	 * so an NMI that arrived concurrently will be seen
		  	 	 * when NMI's are reenabled.
			 	 */
		    		reset_snmi(my_slic);
		   		enable_nmi();           /* proc NMI on */
		    		i = SIGBUS;
		    		break;
			}
	 	} else {
			bic_slic = BIC_SLIC(my_slic, slic_to_config[my_slic]->cd_flags);
			accerr_reg = (my_slic & 0x01) ? BIC_ACCERR1 : BIC_ACCERR0;
			i = rdSubslave(bic_slic, PROC_BIC, accerr_reg);
			if ((i & (BICAE_OCC|BICAE_IO)) == 0) {
				wrSubslave(bic_slic, PROC_BIC, accerr_reg, 0xbb);
				/*
			 	 * Toggle nmi accept in the processor control
			 	 * register so an NMI that arrived concurrently
			 	 * will be seen when NMI's are reenabled.
			 	 */
				wrslave(my_slic, PROC_CTL,
					PROC_CTL_NO_NMI | PROC_CTL_NO_SSTEP | 
					PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);
				wrslave(my_slic, PROC_CTL,
					PROC_CTL_NO_SSTEP | PROC_CTL_NO_HOLD |
					PROC_CTL_NO_RESET);
				enable_nmi();		/* proc NMI on */
				i = SIGBUS;
				break;
			}
		}
#ifdef	B1BUG21
		/*
		 * If process using the FPU got a reserved-space access error,
		 * assume this is B-1 Errata # 21 (386 generated a 387 IO
		 * reference with address bit 31 off).  Recover from this in
		 * hardware, and give the process a SIGFPE.
		 */
		if (!sgs2 && l.usingfpu &&
		    (rdslave(my_slic, PROC_FLT) & PROC_FLT_ACC_ERR) == 0) {
			recover_bug_21(p);
			u.u_code = FPE_B1_BUG21;
			i = SIGFPE;
			break;
		}
#endif	B1BUG21
#endif	KXX
		/* fall into... */
	case T_NMI:				/* NMI from kernel mode */
#ifdef	KXX
		/*
		 * Nothing special.
		 */
#else	Real HW
#ifdef	B1BUG21
		/*
		 * Similar to user case, if processor isn't idle, was using the
		 * FPU, and got a reserved-space access error, assume this is
		 * B-1 Errata # 21 (386 generated a 387 IO reference with
		 * address bit 31 off).  Recover from this in hardware, and
		 * give the process a SIGFPE.
		 *
		 * Have test here in case process managed to enter kernel
		 * before taking NMI (eg, thru single-step trap).
		 */
		if (!sgs2 && !l.noproc && l.usingfpu &&
		    (rdslave(l.eng->e_slicaddr, PROC_FLT) & PROC_FLT_ACC_ERR) == 0) {
			recover_bug_21(p);
			u.u_code = FPE_B1_BUG21;
			psignal(p, SIGFPE);
			return;
		}
#endif	B1BUG21
#endif	KXX
		/* Disable SLIC interrupts. Already off at
		 * proc and donmi() turns off NMI.
		 */
		(void) splhi();
		
#ifdef	DEBUG
		printf("Kernel NMI eip=0x%x\n", eip);
#endif	DEBUG
		donmi();
		goto out;

	case T_INT3+T_USER:	 		/* single-byte intr (bkpt) */
		/*
		 * report exec of /etc/init failure
		 */
		if (p == &proc[1] && procmax == &proc[3]) {
			printf("exec of /etc/init failed\n");
                        /*
                         *+ The kernel couldn't exec the program /etc/init,
                         *+ which is required to bring the system up.
			 *+ The root filesystem may not have been specified
			 *+ correctly at boot time.
                         */
			return_fw();
		}
		--eip;				/* point at INT3 again */
		flags &= ~FLAGS_TF;
		i = SIGTRAP;
		break;

	case T_INTO+T_USER:			/* interrupt on overflow */
		u.u_code = FPE_INTOVF_TRAP;
		i = SIGFPE;
		break;

	case T_CHECK+T_USER:			/* array bounds check */
		i = SIGEMT;			/* match 032 "flag" trap */
		break;

	case T_UND+T_USER:			/* undefined/illegal op-code */
#ifdef LOCKBUG
		/*
		 * check to see if this was a lock instuction.
		 */
		if (fubyte((char *)eip) == 0xf0) {
			if (recover_lock_bug(eip)) {
				/*
				 * wasn't an illegal instruction
				 * so fault in the next page.
				 */
				if (u.u_segvcode != eip) {
					u.u_segvcode = eip;
					(void)fubyte((char *)(eip+15));
					goto out;
				}
			}
		}
#endif
		u.u_code = ILL_RESOP_FAULT;
		i = SIGILL;
		break;

	case T_DNA:
		ASSERT(l.eng->e_flags & E_FPU387, "no i387 for FPA emulation");
		/*
		 *+ FPA emulation requires an i387 in order to operate
	  	 *+ correctly.  An FPA operation was attempted on a system
		 *+ with no i387s present.
		 */
	   	ASSERT_DEBUG(!l.usingfpu, "trap: kernel T_DNA usingfpu");
	   	restore_fpu();
	    	goto out;

	case T_DNA+T_USER:		 	/* device not available (FPU) */
		/*
		 * Process quits using FPU at each context switch.
		 * This speeds up context switch for FPU users when there
		 * is no use of FPU between switches, at the expense of up
		 * to an additional trap per dispatch of an FPU using process.
		 */
		if (l.eng->e_flags & E_FPU387) {	/* if 387... */
			ASSERT_DEBUG(!l.usingfpu, "trap: T_DNA usingfpu");
			restore_fpu();			/* turn it back on */
#ifdef COBUG
			/*
			 * Clear these bits--they can be set if
			 * we migrated in the middle of an FPU
			 * instruction.
			 */
			if (u.u_flags & UF_FPSTEP) {
				if ((u.u_flags & UF_OTBIT) == 0) {
					u.u_ar0[FLAGS] &= ~FLAGS_TF;
				}
				u.u_flags &= ~(UF_FPSTEP|UF_OTBIT);
			}

			/*
			 * If i486 C0 bug is present, need to trap on each
			 * FPU operation, so leave FPU set up, but don't
			 * provide access yet.
			 */
			if (l.flags & PL_C0BUG) {
				WRITE_MSW((unsigned)l.fpuoff);
			}
#endif /* COBUG */
			goto out;
		} else {
			/*
			 * no fpu or fpa so emulate
			 */
			uprintf("This kernel does not support floating point emulation\n");
			/*
			 *+ No floating point hardware or software support 
			 *+ exists on this machine. Either add floating point
			 *+ hardware or configure an emulation package.
			 */
			i = SIGFPE;
		}
		break;

	case T_STKFLT+T_USER:			/* stack fault */
		i = SIGSEGV;
		break;

	case T_GPFLT+T_USER:			/* general protection fault */
		/*
		 * In Dynix, these typically only happen when executing
		 * privileged instructions (l.trap_err_code == 0), or by
		 * violating segment limits or otherwise mis-using
		 * segmentation (l.trap_err_code == selector).
		 *
		 * Unfortuneatly, segment limits violation gives error code
		 * == 0, so this can't be distinguished from an illegal
		 * user mode instruction.
		 *
		 * When/if support Virtual-Mode 8086, some additional fuss
		 * goes here.
		 */
#ifdef LOCKBUG
		/*
		 * check to see if this was a lock instuction.
		 */
		if (fubyte((char *)eip) == 0xf0) {
			if (recover_lock_bug(eip)) {
				/*
				 * wasn't an illegal instruction
				 * so fault in the next page.
				 */
				if (u.u_segvcode != eip) {
					u.u_segvcode = eip;
					(void)fubyte((char *)(eip+15));
					goto out;
				}
			}
		}
#endif
#ifdef	DEBUG
		if (traptrace > 1) dumpUPT(p);
#endif	DEBUG
		u.u_code = ILL_PRIVIN_FAULT;
		i = SIGILL;
		break;

	case T_COPERR+T_USER:			/* co-processor error (FPU) */
		ASSERT(l.usingfpu, "trap: !usingfpu");
                /*
                 *+ The processor took a coprocessor error trap and the
                 *+ kernel's data structures indicated that this processor
                 *+ was not using the coprocessor.
                 */
#ifdef	FPU_SIGNAL_BUG
		save_fpu(&u.u_fpusave);		/* save state */
#else
		save_fpu();			/* save state */
#endif
		disable_fpu();			/* turn off in current process*/
		u.u_code =  (u.u_fpusave.fpu_status & FSRTT)
			 &~ (u.u_fpusave.fpu_control & FSRTM);
#ifdef	DEBUG
		if (traptrace) dump_fpu(&u.u_fpusave);
#endif	DEBUG
		i = SIGFPE;
		break;

	case T_SWTCH+T_USER:			/* think about context switch */
		/*
		 * Check SW traps -- reschedule and SW traps come here.
		 * l.runrun==-1 ==> SW traps only; l.runrun==1 ==> maybe both.
		 * This relies on the "&=" below being one instruction to
		 * avoid reentrancy problems (interrupt procedure setting
		 * another bit could "race"); can only happen with SWTON()
		 * calls in interrupts from kernel mode.
		 */
		while (u.u_swtrap) {
			i = ffs((int)u.u_swtrap) - 1;
			u.u_swtrap &= ~(1 << i);
			(*swtrapvec[i])();
		}
		/*
		 * If only scheduled for SW traps and not reschedule,
		 * avoid a reschedule.  Need to avoid race with interrupts
		 * which might set l.runrun=1.
		 */
		DISABLE();
		if (l.runrun == -1)
			l.runrun = 0;
		ENABLE();
		goto out;

	case T_PGFLT:				/* page fault in kernel mode */
		++l.cnt.v_faults;		/* count the fault */
		/*
		 * Chip bug (thru at least B1's) makes page-fault error code
		 * unreliable; thus locore.s (t_pgflt) puts faulting address
		 * in l.trap_err_code.  This assumes pagein() can't fault.
		 */
		fltaddr = l.trap_err_code;	/* faulting address */
#ifdef	DEBUG
		if (abtdebug)
			printf("<%d,%s>: KPF fltaddr=0x%x, eip=0x%x\n",
					p-proc, u.u_comm, fltaddr, eip);
#endif	DEBUG

		/*
		 * If faulting address is in the self-mapped page-table,
		 * user gave a bogus address for useracc/copyin, etc.
		 */

		if (fltaddr >= VA_PT && fltaddr < VA_PT+SZ_FULL_PT) {
			ASSERT(u.u_fltaddr != 0, "trap: PT null u_fltaddr");
                        /*
                         *+ The kernel made a reference to an invalid address
                         *+ through a function that wasn't expecting to access
                         *+ nonkernel space.  Functions that refer to addresses
                         *+ provided by user processes protect themselves
                         *+ against such faults.
                         */
			eip = u.u_fltaddr;
#ifdef	DEBUG
			if (traptrace)
				printf("trap: error kernel PT fault\n");
#endif	DEBUG
			return;
		}

		/*
		 * If it's in user address space, try to page it in.
		 * Else try to grow.
		 * Else fail whatever was going on.
		 *
		 * pagein() checks for valid/read-only or PG_INVAL pte
		 * (since PFEC unreliable).
		 */

		if (fltaddr >= VA_USER
		&&  fltaddr < VA_USER+USER_SPACE-ctob(UPAGES)) {
			if (pagein(fltaddr-VA_USER) || grow(fltaddr-VA_USER))
				return;
			ASSERT(u.u_fltaddr != 0, "trap: User null u_fltaddr");
                        /*
                         *+ The kernel made a reference to an invalid address
                         *+ through a function that wasn't expecting to access
                         *+ nonkernel space.  Functions that refer to addresses
                         *+ provided by user processes protect themselves
                         *+ against such faults.
                         */
			eip = u.u_fltaddr;
#ifdef	DEBUG
			if (traptrace)
				printf("trap: error kernel user-space fault\n");
#endif	DEBUG
			return;
		}

		/*
		 * If it's a ref to FPA space:
		 *	if process isn't FPA user, make it one.
		 *	else, fail the request.
		 */

		if (fltaddr >= VA_FPA+FPA_START && fltaddr < VA_FPA+FPA_SIZE) {
			/*
			 * If process has used FPA before or can turn it on,
			 * just return.  Else error.
			 *
			 * Can fail to turn on FPA if none in the system.
			 */
			if (p->p_fpa || enable_fpa())
				restore_fpa();
			else {
				/* It is known that the kernel will not
				 * execute any FPA instructions unless it
				 * is known that the processor has an FPA
				 * (as in t_fpa).  Thus, no emulation is being
				 * provided here.
				 */
				ASSERT(u.u_fltaddr != 0, "trap: FPA null u_fltaddr");
                                /*
                                 *+ The kernel made a reference to an invalid 
				 *+ address through a function that wasn't 
				 *+ expecting to access nonkernel space.
				 *+ Functions that refer to addresses
                                 *+ provided by user processes protect 
				 *+ themselves against such faults.
                                 */
				eip = u.u_fltaddr;
			}
			return;
		}

		/*
		 * None of the above ==> kernel made a mistake.  Oh, well...
		 */

		CPRINTF("MMU fault: fault address = 0x%x\n", fltaddr);
		CPRINTF("Page-Table Root = 0x%x\n", READ_PTROOT());
		trap_dumpregs((int *)&eax);
		panic("MMU Fault");
                /*
                 *+ The system took a page fault while executing in the
                 *+ kernel.  The faulting address was not a valid
                 *+ reference to user text, data, stack or FPA space.
                 */
		/*NOTREACHED*/

	case T_PGFLT+T_USER:			/* page fault from user mode */
		++l.cnt.v_faults;		/* count the fault */
		/*
		 * Chip bug (thru at least B1's) makes page-fault error code
		 * unreliable; thus locore.s (t_pgflt) puts faulting address
		 * in l.trap_err_code.  This assumes pagein() can't fault.
		 */
		fltaddr = l.trap_err_code;	/* faulting address */
#ifdef	DEBUG
		if (abtdebug)
			printf("<%d,%s>: UPF fltaddr=0x%x, eip=0x%x\n",
					p-proc, u.u_comm, fltaddr, eip);
#endif	DEBUG
		/*
		 * If it's in user address space, try to page it in.
		 * Else try to grow.
		 *
		 * pagein() checks for valid/read-only or PG_INVAL pte
		 * (since PFEC unreliable).
		 */

		if (fltaddr >= VA_USER
		&&  fltaddr < VA_USER+USER_SPACE-ctob(UPAGES)
		&&  (pagein(fltaddr-VA_USER) || grow(fltaddr-VA_USER)))
			goto out;

		/*
		 * If it's a ref to FPA space:
		 *	if process isn't FPA user, make it one.
		 *	else, SIGSEGV.
		 */

		if (fltaddr >= VA_FPA+FPA_START && fltaddr < VA_FPA+FPA_SIZE) {
			switch (p->p_fpa) {
			case FPA_NONE:
				if (enable_fpa()) {
					restore_fpa();
					goto out;
				}
				/*
				 * May race with online of an FPA, however
				 * will be back on an FPA next fault.
				 */
				if (weitek_emulation) {
					p->p_fpa = FPA_SW;
					fpa_init();
#ifdef	FPU_SIGNAL_BUG
					emula_fpa_hw2sw(&u.u_fpasave);
#else
					emula_fpa_hw2sw();
#endif
				}
				break;

			case FPA_HW:
				if (l.fpa) {
					/*
					 * No race with offline, as
					 * p_fpa == FPA_HW implies that
					 * NFPAproc has been incremented.
					 */
					restore_fpa();
					goto out;
				}
				/*
				 * Must have affinitied to a processor which
				 * doesn't have an FPA or we've taken the last
				 * FPA offline (shouldn't happen, but this
				 * code should handle it).
				 * May race with bringing an FPA back online,
				 * however, the next fault, we'll discover
				 * this and switch back to hardware.
				 */
				deref_fpa();
				if (weitek_emulation) {
					p->p_fpa = FPA_SW;
#ifdef	FPU_SIGNAL_BUG
					emula_fpa_hw2sw(&u.u_fpasave);
#else
					emula_fpa_hw2sw();
#endif
				}
				break;

			case FPA_SW:
				if (reenable_fpa()) {
					/*
					 * No reason we can't be on a processor
					 * with an FPA.  May have just turned
					 * off affinity or had an FPA processor
					 * come online.
					 */
					restore_fpa();
					goto out;
				}
				break;

			case FPA_FRCSW:
			default:
				/*
				 * We want the emulation (probably for testing).
				 */
				break;
			}
			if (!weitek_emulation) {
				u.u_code |= FPE_NOFPA_AVAIL;
				i = SIGFPE;
				goto sig;
			}

			/*
			 * Either there is no fpa on the system, or the
			 * process is affinitied to a processor which does
			 * not have an fpa.  Need to emulate the instruction.
			 */
			switch (emula_fpa(fltaddr)) {
			case FPA_EM_OK:
				goto out;
			case FPA_EM_EXCEPTION:
				/*
				 * Some sort of unmasked exception was
				 * generated.
				 */
				emula_fpa_trap();
				i = SIGFPE;
				break;
			case FPA_EM_SEG:
				/*
				 * Encountered some sort of error when
				 * accessing memory; send the process a
				 * segmentation violation.
				 */
				i = SIGSEGV;
				break;
			case FPA_EM_ILLINSTR:
			default:
				/*
				 * Emulation didn't understand the instruction
				 * which was being used to access the FPA.
				 * Seems best to indicate this by indicating
				 * there is no emulation to the program
				 * and shooting it down with a SIGFPE.  This
				 * is more likely to indicate that the failure
				 * was due to the emulation rather than the
				 * program.
				 */
				u.u_code |= FPE_NOFPA_AVAIL;
				i = SIGFPE;
				break;
			}
			goto sig;
		}

		u.u_segvcode = fltaddr-VA_USER;
		i = SIGSEGV;
		break;
	}
#ifdef	DEBUG
	if (traptrace) {
		type &= ~T_USER;
		printf("<%d,%s>: trap type %d (%s) signal %d u_code 0x%x, ip=0x%x, flags=0x%x\n",
			p-proc, u.u_comm, type,
			(type < TRAP_TYPES) ? trap_type[type] : "unknown",
			i, u.u_code, eip, flags);
	}
#endif	DEBUG
sig:
	psignal(p, i);
out:
	/*
	 * If necessary, swap self.
	 * Do before signal/qswtch to decrease latency for swapper.
	 */

	if (p->p_flag & SFSWAP)
		(void) swapout(p, (size_t) p->p_dsize, (size_t) p->p_ssize);

	/*
	 * Pre-emptive context switch?
	 */

	if (l.runrun) {
		ASSERT_DEBUG(USERMODE(cs), "trap: !user redispatch");
		p->p_pri = p->p_usrpri;
		qswtch();
	}

	/*
	 * if (p->p_cursig || ISSIG(p))
	 *	psig();
	 *
	 * No need to lock state to test p_sig.
	 */

	if (p->p_cursig)
		psig();
	else if (QUEUEDSIG(p)) {
		p->p_pri = p->p_usrpri;			/* in case stop */
		i = p_lock(&p->p_state, SPLHI);
		if (issig((lock_t *)NULL) > 0) {
			v_lock(&p->p_state, i);
			psig();
		} else
			v_lock(&p->p_state, i);
	}


	if (u.u_prof.pr_scale) {
		struct timeval stime;

		DISABLE();
		stime = u.u_ru.ru_stime;
		ENABLE();
		/* calculate number of ticks in system */
		i = ((stime.tv_sec - u.u_syst.tv_sec) * 1000 +
			(stime.tv_usec - u.u_syst.tv_usec) / 1000) / (tick / 1000);
		if (i)
			addupc((int)eip, &u.u_prof, i);
	}

	/*
	 * Going back to user mode so fix p_pri, and e_pri (process may have
	 * blocked at higher priority while handling the trap).
	 */

	p->p_pri = p->p_usrpri;
	l.eng->e_pri = p->p_pri;
	SLICPRI(l.eng->e_pri>>2);

#if	defined(MFG) || defined(DEBUG)
	if (va_slic->sl_lmask != SPL0)
		printf("Trap exit - IPL (0x%x), %s (pid %d), trap type = %x\n",
			va_slic->sl_lmask, u.u_comm, p->p_pid, type);
	if ((flags & FLAGS_IF) == 0)
		printf("Trap exit - FLAGS = 0x%x, %s (pid %d), trap type = 0x%x\n",
			flags, u.u_comm, p->p_pid, type);
#endif
}

/*
 * syscall()
 *	Called from the trap handler when a system call occurs
 *
 * Arguments are already copied into u.u_arg[] by syscall entries
 * (see machine/locore.s).
 *
 * See machine/reg.h for order of parameters.
 */

/*ARGSUSED*/
syscall(edi, esi, ebp, unused, ebx, edx, ecx, eax, eip, cs, flags, sp, ss)
	unsigned ss;		/* only if inter-segment (user->kernel) */
	unsigned sp;		/* only if inter-segment (user->kernel) */
	unsigned flags;		/* extended-flags (see machine/psl.h) */
	unsigned cs;		/* sense user-mode entry from RPL field */
	unsigned eip;		/* return context */
	unsigned eax;		/* syscall #, 1st return value */
	unsigned ecx;		/* 2nd return value */
	unsigned edx;		/* more such */
	unsigned ebx;		/* register variable */
	unsigned unused;	/* temp SP (from push-all instruction) */
	unsigned ebp;		/* of interrupted frame */
	unsigned esi;		/* register variable */
	unsigned edi;		/* register variable */
{
	register struct proc *p;
	int	syscall_num = SYS_IDX(eax);	/* strip syscall release # */
#if	defined(MFG) || defined(DEBUG)
	struct a_uarea {
		char s_uarea[UPAGES*NBPG];
	}; 
	static struct a_uarea a_uarea;
#endif

#if	defined(MFG) || defined(DEBUG)
	ASSERT(USERMODE(cs), "syscall");
	ASSERT(u.u_procp->p_noswap == 0, "syscall: entry non-zero p_noswap");
#endif

	/*
	 * Always sample ru_stime, since syscall may be SYS_profil.
	 */

	u.u_syst = u.u_ru.ru_stime;
	l.cnt.v_syscall++;
	u.u_ar0 = (int *) &eax;
	u.u_error = 0;

	/*
	 * Arguments are already in place in u.u_arg[], via syscall entry.
	 */

	u.u_r.r_val1 = 0;

	/*
	 * Set up signal return and do sys call.
	 * Signal wakeup will return here unless changed.
	 * Can alter to RESTARTSYS, etc.
	 *
	 * NOTE: fsetjmp() doesn't save registers.
	 * Note that stacked EIP could change if process stopped in debugger,
	 * debugger altered EIP, then process takes signal -- in this case,
	 * saving entry EIP doesn't help.
	 */

	u.u_eosys = JUSTRETURN;

	if (fsetjmp(&u.u_qsave)) {			/* return from signal */
		ASSERT(u.u_eosys == JUSTRETURN, "syscall: fsetjmp");
		/*
		 *+ A longjump was excuted in a system call but the
		 *+ recovery action was not set to JUSTRETURN.
		 */
		if (u.u_error == 0)
			u.u_error = EINTR;
	} else if (syscall_num >= nsysent) {		/* bad syscall number */
#ifdef	DEBUG
		if (syscalltrace) {
			printf("<%d,%s>syscall[%x] - bad syscall #\n",
					u.u_procp-proc, u.u_comm, syscall_num);
		}
#endif	DEBUG
		nosys();
	} else {					/* do real syscall */
#ifdef	DEBUG
		if (syscalltrace) {
			int	i;
			printf("<%d,%s>%s(", u.u_procp-proc,
					u.u_comm, syscallnames[syscall_num]);
			for(i = 0; i < sysent[syscall_num].sy_narg; i++) {
				if (i > 0)
					printf(",");
				printf("%x", u.u_arg[i]);
			}
			printf(") ");
		}
#endif	DEBUG
		(*(sysent[syscall_num].sy_call))();
	}

	/*
	 * Returned from syscall.  Set up return to user.
	 */

	if (u.u_eosys == RESTARTSYS) {
		eip -= SZSVC;				/* ==> re-execute svc */
	} else if (u.u_error) {
		/*
		 * ENOBUFS recovery:
		 *
		 * m_expandorwait() returns 0 if it cannot expand,
		 * 	otherwise returns 1
		 * NOTE: this does not guarantee that mbufs now exist since
		 * another process can mget them before this process mgets.
		 */
		if (u.u_error == ENOBUFS && m_expandorwait()) {
			eip -= SZSVC;			/* ==> re-execute svc */
		} else {
#ifdef QUOTA
			if (u.u_error == EDQUOT)	/* No new error codes */
				u.u_error = ENOSPC;	/* EDQUOT -> ENOSPC */
#endif
			eax = u.u_error;		/* error # */
			flags |= FLAGS_CF;		/* error indicator */
		}
	} else {
		flags &= ~FLAGS_CF;			/* no error! */
		eax = u.u_r.r_val1;			/* 1st return value */
		ecx = u.u_r.r_val2;			/* maybe garbage */
#ifdef	lint
		lint_ref_int((int)ecx);
#endif	lint
	}
	/*
	 * If system call referenced a file-table entry in a shared open file
	 * table, deref it here.
	 */
	if (u.u_fpref) {
		deref_file(u.u_fpref);
		u.u_fpref = NULL;
	}

	/*
	 * Done.  See about swapping/switching/etc & return.
	 *
	 * Check swap before signal/qswtch to decrease latency for swapper.
	 */

	p = u.u_procp;			/* do here in case fork/signal */

	if (p->p_flag & SFSWAP)
		(void) swapout(p, (size_t) p->p_dsize, (size_t) p->p_ssize);

	/*
	 * Pre-emptive context switch?
	 */

	if (l.runrun) {
		p->p_pri = p->p_usrpri;
		qswtch();
	}

	/*
	 * if (p->p_cursig || ISSIG(p))
	 *	psig();
	 *
	 * No need to lock state to test p_sig.
	 */

	if (p->p_cursig)
		psig();
	else if (QUEUEDSIG(p)) {
		p->p_pri = p->p_usrpri;			/* in case stop */
		(void) p_lock(&p->p_state, SPLHI);
		if (issig((lock_t *)NULL) > 0) {
			v_lock(&p->p_state, SPL0);
			psig();
		} else
			v_lock(&p->p_state, SPL0);
	}

	if (u.u_prof.pr_scale) {
		struct	timeval stime;
		int	i;

		DISABLE();
		stime = u.u_ru.ru_stime;
		ENABLE();
		/* calculate number of ticks in system */
		i = ((stime.tv_sec - u.u_syst.tv_sec) * 1000 +
			(stime.tv_usec - u.u_syst.tv_usec) / 1000) / (tick / 1000);
		if (i)
			addupc((int)eip, &u.u_prof, i);
	}

	/*
	 * Going back to user mode so fix p->pri, and e_pri (process may have
	 * blocked at higher priority while in the system-call).
	 */

	p->p_pri = p->p_usrpri;
	l.eng->e_pri = p->p_pri;
	SLICPRI(l.eng->e_pri>>2);

#if	defined(MFG) || defined(DEBUG)
	if (va_slic->sl_lmask != SPL0) {
		char	s1[10], s2[10], s3[10], s4[10];
		int 	*a = (int *)&(a_uarea.s_uarea[(UPAGES*NBPG)-2048]);

		/*
		 * Grab the entire uarea so we don't disturb
		 * the stack frame and dump it out.
		 * NOTE:  could add mutex around this code to
		 *        handle multiple guys doing this.
		 */
		a_uarea = *(struct a_uarea *)&u;
		printf("Syscall exit - IPL (0x%x), %s (pid %d), eip = 0x%x, u.u_error = %d\n",
			va_slic->sl_lmask, u.u_comm, p->p_pid, eip, u.u_error);
		(void) spl0();
		while(a < (int *)&(a_uarea.s_uarea[UPAGES*NBPG])) {
			hexstr(s1, a[0]); hexstr(s2, a[1]);
			hexstr(s3, a[2]); hexstr(s4, a[3]);
			printf("%x: %s%x %s%x %s%x %s%x\n", 
				(int)a + ((int)&u - (int)&a_uarea),
				s1, a[0], s2, a[1],
				s3, a[2], s4, a[3]);
			a += 4;
		}
	}
	if ((flags & FLAGS_IF) == 0)
		printf("Syscall exit - FLAGS = 0x%x, %s (pid %d), eip = 0x%x\n",
			flags, u.u_comm, p->p_pid, eip);
	ASSERT(p->p_noswap == 0, "syscall: non-zero p_noswap");
#endif

#ifdef	DEBUG
	if (syscalltrace) {
		if (flags & FLAGS_CF)
			printf("<%d,%s>:Error %d\n", p-proc, u.u_comm, eax);
		else
			printf("<%d,%s>:%x,%x ret to %x\n", p-proc,
				u.u_comm, u.u_r.r_val1, u.u_r.r_val2, eip);
	}
#endif	DEBUG
}

/*
 * nosys()
 *	nonexistent system call-- signal process (may want to handle it).
 *
 * Flag error if process won't see signal immediately.
 */

nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN
	||  (u.u_procp->p_sigmask & sigmask(SIGSYS)))
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}

/*
 * nosyscall()
 *	Null system-call handler for binary configuration of alternate
 *	system-call entries (primarily backwards binary compatibility).
 *
 * Print a message and exit the process.
 */

/*ARGSUSED*/
nosyscall(edi, esi, ebp, unused, ebx, edx, ecx, eax, eip, cs, flags, sp, ss)
{
	register struct proc *p = u.u_procp;

	u.u_ar0 = (int *) &eax;		/* can block in exit() for debugger */

	printf("pid %d command %s killed due to unsupported system call 0x%x\n",
			p->p_pid, u.u_comm, eax);
        /*
         *+ A user process attempted to make a system call for which there is
         *+ no system call handler.
         */
	uprintf("sorry, pid %d was killed due to unsupported system call\n",
			p->p_pid);
        /*
         *+ A process made a system call to an unsupported kernel
         *+ function.  The process has been killed.  This is an
         *+ application software bug.
         */
	exit(077<<8);
}

/*
 * trap_dumpregs()
 *	Dump register contents at time of kernel trap-panic.
 */

trap_dumpregs(eax)
	register int	*eax;
{
	CPRINTF("flags=%x eip=%x ebp=%x ksp=%x usp=%x\n",
			eax[FLAGS], eax[EIP], eax[EBP], (&eax[SS])+1, eax[ESP]);
	CPRINTF("eax=%x ebx=%x ecx=%x edx=%x esi=%x edi=%x\n",
			eax[EAX], eax[EBX], eax[ECX],
			eax[EDX], eax[ESI], eax[EDI]);
}

#ifdef	B1BUG21
/*
 * recover_bug_21()
 *	Recover the hardware from 80386 B1 Errata # 21 (see 9/1/87 Errata).
 *
 * If this problem occurs, the 386 generated a reference to the 387 with
 * address bit 31 *off* (ie, IO access to 0x000000F8 or 0x000000FC).  Hardware
 * generates an NMI, with PROC_FLT register showing "reserved space access
 * error".  There are other sources of this error, but since it's rare, assume
 * it's this problem.
 *
 * Caller insures process was using the FPU.
 *
 * Clear the access error, re-initialize the 387 (this re-sync's the 386 and
 * 387), and let the world know.
 *
 * Interrupts and NMI's are still OFF at the processor at entry.
 */

static
recover_bug_21(p)
	register struct proc *p;
{
	static	char	mesg[] = "386/387 Error Recovery;\nContact Sequent Field Service";

	wrslave(l.eng->e_slicaddr, PROC_FLT, 0xbb);	/* clear err */
	init_fpu();					/* re-init 387 */
	/* togle nmi accept bit */
	wrslave(l.eng->e_slicaddr, PROC_CTL, PROC_CTL_NO_NMI |
		PROC_CTL_NO_SSTEP | PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);
	wrslave(l.eng->e_slicaddr, PROC_CTL, PROC_CTL_NO_SSTEP |
		PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);
	enable_nmi();					/* ON int's from NMI */
	printf("WARNING: pid %d: killed due to %s\n", p->p_pid, mesg);
	/*
	*+ The processor recovered from an error indicating a 386 chip bug
	*+ (Errata # 21).  Corrective action:  contact service.
	*/
	uprintf("sorry, pid %d was killed due to %s\n", p->p_pid, mesg);
        /*
         *+ The processor recovered from an error indicating a 386 chip bug
         *+ (Errata # 21).  Corrective action:  contact service.
         */
}
#endif	B1BUG21

#ifdef LOCKBUG

/*
 * recover_lock_bug() will return true if the instruction sequenece
 * is legal according to the Intel 80386 Programmer's Reference Manual (1988)
 * page 17-100
 *
 * The LOCK prefix funcions only with the following instructions:
 * BT,BTS,BTR,BTC			mem,reg/imm
 * XCHG					reg,mem
 * XCHG					mem,reg
 * ADD,OR,ADC,SBB,AND,SUB,XOR		mem,reg/imm
 * NOT,NEG,INC,DEC			mem
 *
 */

#define TWO_BYTE 	0x0f	/* 2 byte instruction */
#define ADDR_PREF	0x67	/* address size prefix */
#define OPER_PREF	0x66	/* operand size prefix */
#define SEG_CS		0x2e	/* C segment overide */
#define SEG_DS		0x3e	/* D segment overide */
#define SEG_ES		0x26	/* E segment overide */
#define SEG_FS		0x64	/* F segment overide */
#define SEG_GS		0x65	/* G segment overide */
#define SEG_SS		0x36	/* S segment overide */

#define MODR_MASK	0x38		/* mask to extract MODR opcode bits */
#define MODRO		3		/* shift to MODR opcode */

#define BT_MSET		0xba		/* BT BTS BTC BTC */
#define MT_MRO_BT	(4<<MODRO)	/* BT_MSET modifier for BT */
#define MT_MRO_BTS	(5<<MODRO)	/* BT_MSET modifier for BTS */
#define MT_MRO_BTR	(6<<MODRO)	/* BT_MSET modifier for BTR */
#define MT_MRO_BTC	(7<<MODRO)	/* BT_MSET modifier for BTC */
#define BT		0xa3		/* 8bit BT */
#define BTS		0xab		/* 8bit BTS */
#define BTR		0xb3		/* 8bit BTR */
#define BTC		0xbb		/* 8bit BTC */

#define XCHG8		0x86		/* 8 bit XCHG */
#define XCHG16		0x87		/* 16 or 32 but XCHG */

#define ADD8		0x00
#define ADD16		0x01
#define OR8		0x08
#define OR16		0x09
#define ADC8		0x10
#define ADC16		0x11
#define SBB8		0x18
#define SBB16		0x19
#define AND8		0x20
#define AND16		0x21
#define SUB8		0x28
#define SUB16		0x29
#define XOR8		0x30
#define XOR16		0x31

#define ARITH_MSET_8_8	0x80		/* arithmatic set 8 - 8 */
#define ARITH_MSET_16_16	0x81	/* arithmatic set 16,32 - 16,32 */
#define ARITH_MSET_16_8	0x83		/* arithmatic set 16,32 - 8 */

#define ADD_MRO		(0<<MODRO)
#define OR_MRO		(1<<MODRO)
#define ADC_MRO		(2<<MODRO)
#define SBB_MRO		(3<<MODRO)
#define AND_MRO		(4<<MODRO)
#define SUB_MRO		(5<<MODRO)
#define XOR_MRO		(6<<MODRO)

#define MON_GROUP_1	0xf6	/* not,neg */
#define MON_GROUP_2	0xf7	/* not,neg */
#define MON_GROUP_3	0xfe	/* inc,dec */
#define MON_GROUP_4	0xff	/* inc,dec */

#define NOT_MRO		(2<<MODRO)
#define NEG_MRO		(3<<MODRO)
#define INCA_MRO	(0<<MODRO)
#define INCB_MRO	(6<<MODRO)
#define DEC_MRO		(1<<MODRO)

int
recover_lock_bug(eip) 
	unsigned	eip;
{	
	char	*pc;
	unsigned i;

	/* 
	 * Lock prefix error recovery
	 * look at next byte.
	 * If it is not supposed to be locked continue with
	 * the illegal instruction fault
	 * otherwise it is likely to be the chip
	 * bug, in which just restart the instruction.
	 * We hope this will resolve since we have
	 * fubyted the rest of instruction in - no
	 * guarentees so maybe cap the operation.
	 */
	pc = (char *)eip;
	for(;;) {
		pc++;
		i = fubyte(pc);
		switch (i) {
		case ADDR_PREF:
		case OPER_PREF:
		case SEG_CS:
		case SEG_DS:
		case SEG_ES:
		case SEG_FS:
		case SEG_GS:
		case SEG_SS:
			/*
			 * skip to next byte on segment overide.
			 * or address or operand size prefix.
			 */
			continue;
		case TWO_BYTE:
			pc++;
			i = fubyte(pc);
			switch (i) {
			case BT:
			case BTS:
			case BTR:
			case BTC:
				return 1;
			case BT_MSET:
				pc++;
				i = fubyte(pc);
				switch (i & MODR_MASK) {
				case MT_MRO_BT:
				case MT_MRO_BTS:
				case MT_MRO_BTR:
				case MT_MRO_BTC:
					return 1;
				}
				return 0;
			default:
				return 0;
			}
		case XCHG8:
		case XCHG16:
		case ADD8:
		case ADD16:
		case OR8:
		case OR16:
		case ADC8:
		case ADC16:
		case SBB8:
		case SBB16:
		case AND8:
		case AND16:
		case SUB8:
		case SUB16:
		case XOR8:
		case XOR16:
			return 1;
		case ARITH_MSET_8_8:
		case ARITH_MSET_16_16:
		case ARITH_MSET_16_8:
			pc++;
			i = fubyte(pc);
			switch (i & MODR_MASK) {
			case ADD_MRO:
			case OR_MRO:
			case ADC_MRO:
			case SBB_MRO:
			case AND_MRO:
			case SUB_MRO:
			case XOR_MRO:
				return 1;
			}
			return 0;
		case MON_GROUP_1:
		case MON_GROUP_2:
			pc++;
			i = fubyte(pc);
			switch (i & MODR_MASK) {
			case NOT_MRO:
			case NEG_MRO:
				return 1;
			}
			return 0;
		case MON_GROUP_3:
		case MON_GROUP_4:	
			pc++;
			i = fubyte(pc);
			switch (i & MODR_MASK) {
			case INCA_MRO:
			case INCB_MRO:
			case DEC_MRO:
				return 1;
			}
			return 0;
		default:
			return 0;
		}
	}
}
#endif
