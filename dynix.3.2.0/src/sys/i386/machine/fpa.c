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
static	char	rcsid[] = "$Header: fpa.c 2.9 1992/02/13 00:15:27 $";
#endif

/*
 * fpa.c
 *	Code to handle SGS Weitek floating-point accelerator (FPA).
 *
 * Some of this code should eventually be expanded in-line in hot places
 * (eg, restore_fpa()).
 */

/* $Log: fpa.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../balance/engine.h"
#include "../balance/cfg.h"
#include "../balance/slic.h"

#include "../machine/intctl.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"
#include "../machine/hwparam.h"

static	int	NFPAonline;		/* # processors with FPA's online */
static	int	NFPAprocs;		/* # processes using FPA right now */
extern	int	NFPA;			/* # fpa's on processors in the system*/

static	struct	pte	fpa_pte;	/* pte for L2 PT page that maps FPA */

#define	FPA_PTE			((struct pte *) VA_FPA_PTE)
#define	FPA_REG(va_fpa,reg)	(* ((long *) ((long)(va_fpa) + (reg))) )

/*
 * alloc_fpa()
 *	Boot-time allocation of level-2 page to map FPA.
 *
 * Don't bother allocating map if there are no FPA's on any processor.
 */

#ifdef	KXX
u_long	PHYS_WEITEK_FPA;		/* just to satisfy the reference */
#endif	KXX

alloc_fpa()
{
	register struct pte *pte;
	register u_long i;

	if (NFPA != 0) {
		callocrnd(NBPG);
		pte = (struct pte *) calloc(NBPG);
		*(int *) &fpa_pte = PHYSTOPTE(pte) | PG_UW|PG_R|PG_M|PG_V;
#ifdef	KXX
		PHYS_WEITEK_FPA = (u_long) calloc(FPA_SIZE);	/* debug */
#endif	KXX
		for (i = 0; i < FPA_SIZE; i += NBPG, ++pte)
			*(int *) pte = PHYSTOPTE(PHYS_WEITEK_FPA + i)
				     | PG_UW|PG_R|PG_M|PG_V;
	}
}

/*
 * initialize_fpa()
 *	Do the grunt work of initializing the calling processor's FPA.
 *
 * Seperate from init_fpa() to allow base address of FPA to be passed in;
 * kernel has to init FPA early on 1st processor booted, before running
 * on kernel page-tables.  Some versions of FPA assert their error interrupt
 * after reset and before being initialized.
 *
 * The initialization sequence is taken from WTL 1163 spec (7/86), pg 16,
 * modified by letter from Christopher Tice (Weitek) to Bruce Gilbert
 * dated 5/28/87 (for 20Mhz and old/new parts).
 */

initialize_fpa(fpa)
	register u_long	fpa;
{
	u_long	fpa_pcr;
	bool_t	warn = 0;
	extern	struct	ctlr_desc *slic_to_config[];

	/*
	 * Must handle initialization differently for 20Mhz parts (old vs new)
	 * vs 16Mhz.  Ugh.
	 */

	if (slic_to_config[va_slic->sl_procid]->cd_p_speed == 20) {
		FPA_REG(fpa, FPA_LDCTX) = 0xB8000000;	/* is new 1163? */
		fpa_pcr = FPA_REG(fpa, FPA_STCTX);	/* new ==> bit 15 set */
		if (fpa_pcr & 0x8000) {			/* New 1163 */
			/*
			 * Set up new 1163 and 20Mhz clock, 5-clock flow-thru.
			 */
			FPA_REG(fpa, FPA_LDCTX) = 0x56000000;
			FPA_REG(fpa, FPA_LDCTX) = 0x98000000;
		} else {				/* Old 1163 */
			/*
			 * Use "old" initialization sequence (4-clock 1164/1165
			 * flow-thru timer).  Using an old 1163 at 20Mhz ==>
			 * print a warning.  However, only print after init is
			 * done to avoid FPA error interrupt.
			 */
			FPA_REG(fpa, FPA_LDCTX) = 0x16000000;
			warn = 1;
		}
	} else {
		/*
		 * 16Mhz.  Use "old" init sequence, regardless of old vs new
		 * 1163 (4-clock 1164/1165 flow-thru timer).
		 */
		FPA_REG(fpa, FPA_LDCTX) = 0x16000000;
	}

	FPA_REG(fpa, FPA_LDCTX) = 0x64000000;	/* 1164 Accumulate Timer */
	FPA_REG(fpa, FPA_LDCTX) = 0xA0000000;	/* 1165 Accumulate Timer */
	FPA_REG(fpa, FPA_LDCTX) = 0x30000000;	/* Reserved Mode Bits (== 0) */
	FPA_REG(fpa, FPA_LDCTX) = FPA_INIT_PCR;	/* Rnd Modes and Excep Mask */

	/*
	 * Now ok to print warning if appropriate.
	 */

	if (warn) {
		printf("WARNING: using old 1163 FPA at 20Mhz.\n");
                /*
                 *+ An old-style floating-point accelerator unit has
                 *+ been detected on a 20MHz machine.  This is not a
                 *+ valid hardware configuration.
                 */
	}
}

/*
 * init_fpa()
 *	Initialize FPA at online time.
 *
 * Called from localinit() in processors that have FPAs.  Processor is
 * already running on "kernel" page-table.
 */

init_fpa()
{
	*(int *) FPA_PTE = *(int *) &fpa_pte;	/* enable mapping */
	initialize_fpa((u_long)VA_FPA);		/* init via page-table */
	*(int *) FPA_PTE = PG_INVAL;		/* disable mapping */
	FLUSH_TLB();				/* just in case */
}

/*
 * online_fpa()
 *	Declare online FPA.
 *
 * Called when FPA processor with useable FPA has been brought online.
 */

online_fpa()
{
	spl_t	s;

	/*
	 * Bump NFPAonline so the system knows there's an FPA available.
	 */

	s = p_lock(&engtbl_lck, SPLHI);
	++NFPAonline;
	v_lock(&engtbl_lck, s);
}

/*
 * offline_fpa()
 *	See if it's ok to offline a processor with FPA.
 *
 * Called by tmp_ctl(TMP_OFFLINE) holding engine-table locked.
 * Return true for ok, else false.
 */

/*ARGSUSED*/
offline_fpa(eng)
	struct	engine	*eng;		/* processor that wants to go offline */
{
	/*
	 * Can't take last processor with FPA offline if there are FPA
	 * processes hanging around.  Affinity bound FPA process dis-allows
	 * offline of the particular processor by being affinity bound.
	 */

	if (NFPAprocs != 0 && NFPAonline == 1)
		return(0);

	/*
	 * No FPA processes or there are other processors with FPA.
	 * Assume this one isn't available any more.
	 */

	--NFPAonline;
	return(1);
}

/*
 * enable_fpa()
 *	Turn ON use of FPA in a process.
 *
 * Called on 1st fault to FPA address space in a process.
 *
 * Returns true for success, false if can't run with an FPA.
 */

enable_fpa()
{
	register struct proc *p = u.u_procp;
	spl_t	s;

	/*
	 * Lock engine table to insure no concurrent on/off-line's
	 * and insure there is at least one processor with FPA.
	 * Fail if process is affinity bound to processor with no FPA.
	 *
	 * Once bump NFPAonline, tmp_ctl() won't let the last FPA processor
	 * go offline.
	 */

	s = p_lock(&engtbl_lck, SPLHI);
	if (NFPAonline == 0 || (p->p_affinity != ANYENG && !l.fpa)) {
		v_lock(&engtbl_lck, s);
		return(0);
	}
	++NFPAprocs;
	v_lock(&engtbl_lck, s);

	/*
	 * If we're not running on a processor with FPA now, context switch
	 * to one that has an FPA.
	 */

	p->p_fpa = FPA_HW;			/* mark process using FPA */
	if (!l.fpa)
		runme_swtch();
	ASSERT_DEBUG(l.fpa, "enable_fpa: !l.fpa");

	/*
	 * Initialize FPA save area.
	 */

	fpa_init();

	/*
	 * Done.  Return success.
	 */

	return(1);
}

/*
 * This should be included with "enable_fpa" with an argument indicating
 * that it is a reenable. XXX.jds
 */
/*
 * reenable_fpa()
 *	Turn ON use of FPA in a process which is currently emulating
 *	the FPA.
 *
 * Returns true for success, false if can't run with an FPA.
 */

reenable_fpa()
{
	register struct proc *p = u.u_procp;
	spl_t	s;

	/*
	 * Lock engine table to insure no concurrent on/off-line's
	 * and insure there is at least one processor with FPA.
	 * Fail if process is affinity bound to processor with no FPA.
	 *
	 * Once bump NFPAonline, tmp_ctl() won't let the last FPA processor
	 * go offline.
	 */

	s = p_lock(&engtbl_lck, SPLHI);
	if (NFPAonline == 0 || (p->p_affinity != ANYENG && !l.fpa)) {
		v_lock(&engtbl_lck, s);
		return(0);
	}
	++NFPAprocs;
	v_lock(&engtbl_lck, s);

	/*
	 * If we're not running on a processor with FPA now, context switch
	 * to one that has an FPA.
	 */

	p->p_fpa = FPA_HW;			/* mark process using FPA */
	if (!l.fpa)
		runme_swtch();
	ASSERT_DEBUG(l.fpa, "reenable_fpa: !l.fpa");

	/*
	 * Convert the emulation registers to the hardware registers.
	 */

#ifdef	FPU_SIGNAL_BUG
	emula_fpa_sw2hw(&u.u_fpasave);
#else
	emula_fpa_sw2hw();
#endif

	/*
	 * Done.  Return success.
	 */

	return(1);
}

fpa_init()
{
	bzero((caddr_t) u.u_fpasave.fpa_regs, sizeof(u.u_fpasave.fpa_regs));
	u.u_fpasave.fpa_pcr = FPA_INIT_PCR;
}

/*
 * fork_fpa()
 *	Process using FPA is forking, keep track.
 *
 * Called by newproc() before procdup(), so saved state is duplicated in child.
 */

fork_fpa(child, parent)
	struct  proc    *child;                 /* child in the fork */
	struct  proc    *parent;                /* parent in the fork */
{
	spl_t	s;


	/*
	 * Count the new FPA user.
	 */

	if ((child->p_fpa = parent->p_fpa) != FPA_HW)
		return;

	s = p_lock(&engtbl_lck, SPLHI);
	++NFPAprocs;
	v_lock(&engtbl_lck, s);

	/*
	 * Save FPA state so child will get this context.
	 */
#ifdef	FPU_SIGNAL_BUG
	save_fpa(&u.u_fpasave);
#else
	save_fpa();
#endif
}

/*
 * disable_fpa()
 *	Turn OFF use of FPA in a process, as if it never used FPA.
 *
 * Used in exit() and exec() when no longer using FPA.
 */

disable_fpa()
{
	spl_t	s;
	int     ofpa;



	/*
	 * Loose one FPA user.
	 */

	ofpa = u.u_procp->p_fpa;
	u.u_procp->p_fpa = FPA_NONE;
	if (ofpa != FPA_HW)
		return;


	s = p_lock(&engtbl_lck, SPLHI);
	--NFPAprocs;
	v_lock(&engtbl_lck, s);

	/*
	 * Insure mapping is off in the process.  Don't bother flushing TLB,
	 * no new refs to FPA before a context switch.
	 */

	*(int *) FPA_PTE = PG_INVAL;		/* disable mapping */
}

/*
 * restore_fpa()
 *	Restore FPA context to previous state.  Process is already running
 *	on a processor with an FPA.
 *
 * FPA trap unconditionally masks all FPA exceptions, so no fear of
 * this trapping by loading an exception-generating PCR.
 *
 * This should be expanded in-line in trap() for efficiency once it all works.
 */

restore_fpa()
{
	ASSERT_DEBUG(*(int *)FPA_PTE == PG_INVAL, "restore_fpa: mapped");
	ASSERT_DEBUG(l.fpa, "restore_fpa: !l.fpa");

	*(int *) FPA_PTE = *(int *) &fpa_pte;		/* enable mapping */

	FPA_REG(VA_FPA, FPA_LDCTX) = u.u_fpasave.fpa_pcr;
	bcopy(	(caddr_t) u.u_fpasave.fpa_regs,
		(caddr_t) &FPA_REG(VA_FPA, FPA_LOAD_R1),
		sizeof(u.u_fpasave.fpa_regs));
}

#ifdef	FPU_SIGNAL_BUG
/*
 * save_fpa(fpap)
 *	Save FPA context to the indicated struct fpasave area.
 *
 * Used to save state for fork() or core(), when FPA trap occurs, and
 * when saving state before entering a signal handler (in sendsig()).
 */

save_fpa(fpap)
register struct fpasave *fpap;
{
	ASSERT_DEBUG(l.fpa, "save_fpa: !l.fpa");

	if (*(int *)FPA_PTE != PG_INVAL) {
		/*
		 * Only save state if currently mapped.  If not mapped,
		 * state is already saved.
		 */
		fpap->fpa_pcr = FPA_REG(VA_FPA, FPA_STCTX);
		bcopy(	(caddr_t) &FPA_REG(VA_FPA, FPA_STOR_R1),
			(caddr_t) fpap->fpa_regs,
			sizeof(fpap->fpa_regs));
		/*
		 * Don't bother turning off FPA mapping.  Only need to
		 * snapshot context in struct user.
		 */
	}
}
#else	/* FPU_SIGNAL_BUG */
/*
 * save_fpa()
 *	Save FPA context.
 *
 * Used to save state for fork() or core(), and when FPA trap occurs.
 */

save_fpa()
{
	ASSERT_DEBUG(l.fpa, "save_fpa: !l.fpa");

	if (*(int *)FPA_PTE != PG_INVAL) {
		/*
		 * Only save state if currently mapped.  If not mapped,
		 * state is already saved.
		 */
		u.u_fpasave.fpa_pcr = FPA_REG(VA_FPA, FPA_STCTX);
		bcopy(	(caddr_t) &FPA_REG(VA_FPA, FPA_STOR_R1),
			(caddr_t) u.u_fpasave.fpa_regs,
			sizeof(u.u_fpasave.fpa_regs));
		/*
		 * Don't bother turning off FPA mapping.  Only need to
		 * snapshot context in struct user.
		 */
	}
}
#endif	/* FPU_SIGNAL_BUG */

/*
 * Decrement the number of FPA processes on the system.
 * No check for whether the current process has an FPA is made.
 */
deref_fpa()
{
	spl_t	s;

	s = p_lock(&engtbl_lck, SPLHI);
	--NFPAprocs;
	v_lock(&engtbl_lck, s);
}

#ifndef lint

/*
 * Increment the number of FPA processes on the system.
 * No check for whether the current process has an FPA is made.
 */
ref_fpa()
{
	spl_t   s;

	s = p_lock(&engtbl_lck, SPLHI);
	++NFPAprocs;
	v_lock(&engtbl_lck, s);
}
#endif lint

/*
 * fpa_trap()
 *	Called from locore interrupt handler to deal with FPA exception.
 *
 * Timing of FPA exception relative to time it takes process to enter
 * kernel insures process can't have gotten far enough to hold
 * any locked resources (eg, self process state).  Thus it's safe to
 * post signal here.  Similar argument if system-call argument causes
 * access to FPA space.
 *
 * If this were to be a problem, could set some (eg) u. state field and
 * poll during in-voluntary context switch in trap() and syscall().  Bump
 * l.runrun to insure the poll happens before return to user mode.
 *
 * Prefer to post signal here to avoid extra overhead in trap() and syscall().
 */

fpa_trap(pcr)
	long	pcr;			/* PCR with some exception bits on */
{
	register struct proc *p = u.u_procp;
	register int	ebits;

	ASSERT_DEBUG(p->p_fpa, "fpa_trap: skew");

	/*
	 * Set u_code to the un-masked exception bits, marked as FPA floating
	 * exception.  Restore PCR with relevent AE bits cleared.
	 */

	ebits = ((pcr & FPA_PCR_AE) & ~(pcr >> FPA_PCR_EM_SHIFT));
	u.u_code = FPA_FPE | ebits;
	FPA_REG(VA_FPA, FPA_LDCTX) = pcr & (~ebits);

	/*
	 * Signal process and set l.runrun to insure the signal is seen
	 * before process returns to user mode (if SLIC and FPA interrupt
	 * race, SLIC wins; thus FPA trap return goes to kernel mode).
	 *
	 * Note that this can increase effective SLIC interrupt latency
	 * by having FPA trap go off after SLIC interrupt has enterred
	 * kernel.  Worst case would be on clock interrupt, this will have
	 * us at SPLHI for a while (but psignal() goes SPLHI anyhow...).
	 * Don't expect this to be a big problem, since interrupt handlers
	 * do psignal()'s already.
	 */

	psignal(p, SIGFPE);
	l.runrun = 1;

	/*
	 * No need to save context, process will context switch or
	 * explicitly call save_fpa() before any access to u.u_fpasave.
	 *
	 * Eg, core() or stop for signal.
	 */
}
