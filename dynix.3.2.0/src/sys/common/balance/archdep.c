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
static	char	rcsid[] = "$Header: archdep.c 2.44 1992/02/13 00:26:13 $";
#endif

/*
 * archdep.c
 *	Architecture dependent routines.
 *
 * Balance/Symmetry version.
 */

/* $Log: archdep.c,v $
 *
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/cmn_err.h"
#include "../ufs/mount.h"

#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/clkarb.h"
#include "../balance/clock.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/hwparam.h"
#include "../machine/mftpr.h"

#ifdef i386
#include "../machine/scan.h"
#endif /* i386 */

#include "../sec/sec.h"

#include "../ssm/ssm_misc.h"

#define	masksig(s)	(1<<((s)-1))

#define SYNC_TIMEOUT	4 	/* minimum sync timeout in seconds */
#define SYNC_EST	100 	/* estimted number of buffers to sync per second */

int	sync_timeout;		/* ticks to timeout disk sync on reboot */
int	sync_est=SYNC_EST;	/* estimted number of buffers to sync per second */
int	bufbusy, obufbusy;	/* Used when syncing disks. */

extern	u_char	cons_scsi;	/* slic address of console scsi */
struct	sec_cib *cbdcib;	/* address of Console board device */
struct	sec_gmode cbdgsm;	/* get/setmodes command */

struct	sec_cib *wdtcib;	/* address of watchdog timer cib */
struct	sec_smode wdtsm;	/* for setmodes and startio commands */
extern	int wdt_timeout;	/* how long before timeout */
extern	int ssm_wdt_timeout;	/* how long before timeout */
int	wdt_polltime = HZ/2;	/* interval between "I'm alive" status sends */
int	wdtreset();

int	ndiskoffline;		/* # of offline / unusable disk drives */
lock_t	diskoff_lock;		/* Synchronizes access to ndiskoffline */

struct	reboot	bootdata;
lock_t	uncmem_lock;
extern	int	memintvl;	/* polling interval in Hz */
extern	short	upyet;		/* Set when 1st processor is initialized */

char	*bootname;	 	/* for get_vers() */
int	boot_len = BNAMESIZE;

/*
 * physstrat()
 *	Start an IO given buffer pointer and driver strategy routine.
 *
 * Wait for the IO to complete unless it's a "dirty" page write.
 *
 * Used by swapio(), physio().
 */

physstrat(bp, strat, prio)
	struct buf *bp;
	int	(*strat)();
	int	prio;
{
	int	flags;

	flags = bp->b_flags;
	BIODONE(bp) = 0;				/* new IO starting */
	(*strat)(bp);

	/*
	 * pageout IO doesn't wait for pushed pages.
	 * Note: use `flags' since don't want to talk to bp after give it
	 * to driver strat routine.
	 */

	if (flags & B_DIRTY)
		return;

	p_sema(&bp->b_iowait, prio);
}

/*
 * disk_online()
 *	A disk has come online.
 *
 * Decrement "ndiskoffline" and turn on FP_IO_ONLINE if fp_lights set.
 */

disk_online()
{
	spl_t	s_ipl;

	if (upyet)
		s_ipl = p_lock(&diskoff_lock, SPLHI);

	--ndiskoffline;
	if (ndiskoffline == 0 && fp_lights)
		FP_IO_ONLINE;

	if (upyet)
		v_lock(&diskoff_lock, s_ipl);
}

/*
 * disk_offline()
 *	A disk has gone offline.
 * Increment "ndiskoffline" and hit FP_IO_OFFLINE if fp_lights set.
 */

disk_offline()
{
	spl_t	s_ipl;

	if (upyet)
		s_ipl = p_lock(&diskoff_lock, SPLHI);

	++ndiskoffline;
	if (fp_lights)
		FP_IO_OFFLINE;

	if (upyet)
		v_lock(&diskoff_lock, s_ipl);
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 * If we are called by multiple processors concurrently, the first
 * one attempts to sync disk, the rest print a message and pause themselves.
 * All register vars are saved on stack.
 */

/*
 * In case console is off, panicstr contains argument to last call to panic.
 */

#define ARGS    a0,a1,a2,a3,a4,a5,a6,a7,a8,a9

char	*panicstr;
lock_t	panic_lock;		/* protect against concurrent panics */
bool_t	dblpanic = 0;		/* double panic? */
struct panic_data panic_data;	/* collection of panic info */

#ifndef KLINT
panic(s)
	char *s;
{
#ifdef	i386
	register caddr_t edi;			/* Known to be edi */
	register int i;
	register int bootopt;
	register struct engine *eng;
	register spl_t	s_ipl;
#endif	i386
#ifdef	ns32000
	register spl_t	s_ipl;
	register int bootopt;
	register struct engine *eng;
	register int i;
	register caddr_t r3;	/* Known to be r3 */
#endif	ns32000
	bool_t	stillalive;

	/*
	 * Save stack pointer. Asm used. This code not placed in
	 * "boot" routine because it is necessary to save the scratch
	 * registers for concurrently panicing processors.
	 * l.panicsp points to the a copy of the fp on the stack.
	 * Also save page-table in use by this processor when the
	 * system went down.
	 */

#ifdef	i386
#ifdef lint
	edi = (caddr_t)0;
#else
	asm("pushal");
	asm("movl %esp, %edi");
#endif lint
#endif	i386

#ifdef	ns32000
#ifdef lint
	r3 = (caddr_t)0;
#else
	asm("save	[r0,r1,r2,r3,r4,r5,r6,r7]");
	asm("sprd	fp,tos");
	asm("sprd	sp,r3");
#endif lint
#endif	ns32000

	bootopt = RB_AUTOBOOT;
	if (!upyet) {
		panicstr = s;
		printf("panic: %s\n", s);
		printstack();
#ifdef i386
		dump_SGShw_regs();
#endif i386
		boot(RB_PANIC, bootopt);
	}

	if ((!panic_data.pd_engine) && (s_ipl = cp_lock(&panic_lock, SPLHI)) != CPLOCKFAIL) {
		panic_data.pd_engine = l.eng;
#ifdef	i386
		panic_data.pd_sp = edi;
		l.panicsp = edi;
		l.panic_pt = READ_PTROOT();
#endif	i386
#ifdef	ns32000
		panic_data.pd_sp = r3;
		l.panicsp = r3;
#endif	ns32000
		if (l.usingfpu)
#ifdef	FPU_SIGNAL_BUG
			save_fpu(&u.u_fpusave);
#else
			save_fpu();
#endif
#ifdef FPA
		/* if have fpa, not idle and using it */
		if (l.fpa && !l.noproc && u.u_procp->p_fpa)
#ifdef	FPU_SIGNAL_BUG
			save_fpa(&u.u_fpasave);
#else
			save_fpa();
#endif
#endif FPA
		panicstr = s;
		if (!l.noproc)
			panic_data.pd_proc = u.u_procp;
		/*
		 * Send nmi to all other engines.
		 * Keep trying until all respond - for robustness.
		 */
		for (i = 0; i < 100; i++) {
			stillalive = 0;
			for (eng=engine; eng < engine_Nengine; eng++) {
				if ((eng != l.eng)
				&& ((eng->e_flags&E_OFFLINE) != E_OFFLINE)
				&& ((eng->e_flags&E_PAUSED) != E_PAUSED)) {
					stillalive++;
					nmIntr(eng->e_slicaddr, PAUSESELF);
				}
			}
			if (stillalive == 0)
				break;
#ifdef	i386
			DELAY(40000);
#endif	i386
#ifdef	ns32000
			DELAY(10000);
#endif	ns32000
		}
		v_lock(&panic_lock, s_ipl);
		printf("panic: %s\n", s);
		printstack();
#ifdef i386
		dump_SGShw_regs();
#endif i386
	} else {	/* Panic in progress */
		if (l.eng == panic_data.pd_engine) {
			bootopt |= RB_NOSYNC;
			dblpanic = 1;
#ifdef	i386
			panic_data.pd_dblsp = edi;
#endif	i386
#ifdef	ns32000
			panic_data.pd_dblsp = r3;
#endif	ns32000
			printf("Double panic: %s\n", s);
		} else {
			/* multiple processors in panic */
			(void) splhi();
			printf("Concurrent panic: %s\n", s);
			pause_self();
		}
	}

	/*
	 * Return to Firmware
	 */
	boot(RB_PANIC, bootopt);
	/*NOTREACHED*/
}
#endif KLINT

/*
 * flush_intr()
 *	Flush pending interrupts.
 *
 * Used when shutting down processor to insure pending interrupts
 * are cleared (and handled).
 */

flush_intr()
{
	int	counter;

	/*
	 * While there is a pending (HW or SW) interrupt, open
	 * a window to let it in.
	 */

	SLICPRI(0);				/* try not to win arbitration */

	for (;;) {
		if ((va_slic->sl_ictl & (SL_HARDINT|SL_SOFTINT)) == 0)
			break;
		(void) spl0();
		for (counter = 0; counter < 10; counter++)
			continue;		/* window to take int */
		(void) splhi();
	}
}

/*
 * tod_sync()
 *	TOD clock handler when syncing disks.
 *
 * This routine will, if necessary, timeout the disk sync.
 * Thus, during panic situations, there will be a higher probability
 * of returning to FW.
 */

tod_sync()
{
	/*
	 * Increment time as usual.
	 */

	todclock();

	/*
	 * If already sync'ed then take no action.
	 * If number of buffers to sync hasn't changed for sync_timeout
	 * ticks, then give up and return to firmware.
	 */

	if (bufbusy == 0)
		return;
	if (bufbusy != obufbusy) {
		obufbusy = bufbusy;
		sync_timeout = (SYNC_TIMEOUT+(bufbusy/sync_est)) * hz;
		return;
	}
	if (--sync_timeout == 0) {
		printf("Sync timeout\n");
                /*
                 *+ During a panic or shutdown sequence, the kernel's
                 *+ synch of disk cache information to the hard disk
                 *+ didn't complete before timing out.  This might be
                 *+ due to a kernel software or hardware problem.
                 *+ The result can be more filesystem corruption than
                 *+ would have been the case had the synch finished.
                 */
		return_fw();
	}
}
/*
 * boot()
 *	Reboot the machine.
 *
 * Boot routine returns to Firmware.  If called by panic it tries to sync
 * up disks and returns specifying that the alternate boot name is to be
 * booted.  This is normally the Memory dumper.
 *
 * Only ONE engine is alive at this point.
 */

boot(paniced, howto)
	int paniced;
	register int howto;
{
	if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
	   (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD))
		ssm_boot(paniced, howto);
	else
		sec_boot(paniced, howto);
}

/*
 * sec_boot()
 * 	SEC dependent reboot the machine.
 *
 * Sets up a return of control to the SCED firmware.
 * If called by panic it returns specifying that the
 * alternate boot name is to be booted (normally the
 * memory dumper).  Attempts to sync the disks
 * prior to returning to firmware.
 *
 * Invoked when only one engine is alive.
 */
sec_boot(paniced, howto)
	int paniced;
	register int howto;
{
	register struct sec_gmode *cbdmptr = &cbdgsm;
	register struct sec_smode *wdtsmptr = &wdtsm;
	register spl_t s_ipl;
	extern etext;
	extern bool_t dblpanic;

	if ((!upyet) || ((paniced == RB_PANIC) && dblpanic))
		return_fw();

	/*
	 * Get powerup reboot structure.
	 */

	cbdmptr->gm_status = 0;
	bootdata.re_powerup = 1;	/* 0 booted data, 1 powerup values */
	cbdmptr->gm_un.gm_board.sec_reboot = &bootdata;
	cbdcib->cib_inst = SINST_GETMODE;
	cbdcib->cib_status = (int *)&cbdgsm;
	s_ipl = splhi();
	mIntr(cons_scsi, 7, SDEV_SCSIBOARD);
	splx(s_ipl);

	while ((cbdmptr->gm_status & SINST_INSDONE) == 0)
		continue;

	if (cbdmptr->gm_status != SINST_INSDONE) {
		CPRINTF("Cannot get Console Board modes\n");
		return_fw();
	}

	/*
	 * Now tell FW how to reboot
	 */

	bootdata.re_powerup = 0;	/* 0 booted data, 1 powerup values */
	cbdmptr->gm_un.gm_board.sec_dopoll = 0;		/* no change */
	cbdmptr->gm_un.gm_board.sec_powerup = 0;	/* no change */
	cbdmptr->gm_un.gm_board.sec_errlight = SERR_LIGHT_SAME;

	if (paniced == RB_PANIC) {
		/*
		 * Do a dump, set cfg addr to end of kernel text,
		 * and turn on error light.
		 */
		bootdata.re_boot_flag |= RB_AUXBOOT;
		bootdata.re_cfg_addr[1] = (unsigned char *)&etext;
		cbdmptr->gm_un.gm_board.sec_errlight = SERR_LIGHT_ON;
	} else
		bootdata.re_boot_flag = howto;

	cbdmptr->gm_status = 0;
	cbdcib->cib_inst = SINST_SETMODE;
	cbdcib->cib_status = (int *)&cbdgsm;
	s_ipl = splhi();
	mIntr(cons_scsi, 7, SDEV_SCSIBOARD);
	splx(s_ipl);

	while ((cbdmptr->gm_status & SINST_INSDONE) == 0)
		continue;

	if (cbdmptr->gm_status != SINST_INSDONE) {
		CPRINTF("Cannot set Console Board modes\n");
		return_fw();
	}

	if (paniced == RB_BOOT) {
		/*
		 * Set watchdog for 1 minute.
		 * Prevent ERROR light from going on...
		 */

		untimeout(wdtreset, (caddr_t)0);	/* Stop wdt reset */

		wdtsmptr->sm_status = 0;
		wdtsmptr->sm_un.sm_wdt_mode = 60;	/* Set for minute! */

		wdtcib->cib_inst = SINST_SETMODE;
		wdtcib->cib_status = (int *)&wdtsm;
		s_ipl = splhi();
		mIntr(cons_scsi, 7, SDEV_WATCHDOG);
		splx(s_ipl);

		while ((wdtsmptr->sm_status & SINST_INSDONE) == 0)
			continue;

		if (wdtsmptr->sm_status != SINST_INSDONE) {
			CPRINTF("Cannot Setmode Watchdog\n");
			return_fw();
		}

		wdtsmptr->sm_status = 0;
		wdtcib->cib_inst = SINST_STARTIO;
		wdtcib->cib_status = (int *)&wdtsm;
		s_ipl = splhi();
		mIntr(cons_scsi, 7, SDEV_WATCHDOG);
		splx(s_ipl);

		while ((wdtsmptr->sm_status & SINST_INSDONE) == 0)
			continue;

		if (wdtsmptr->sm_status != SINST_INSDONE) {
			CPRINTF("Cannot Restart Watchdog\n");
			return_fw();
		}
	}
	reboot_sync(howto);
	return_fw();
}

/*
 * return_fw()
 *	Return to Firmware
 */
return_fw()
{
	if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
	   (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD))
		ssm_return_fw();
	else
		sec_return_fw();
}

/*
 * reboot_sync()
 *	Sync the disks prior to returning to Firmware.
 *	May be overridden.  Assume we're the only
 *	active processor.
 */
reboot_sync(howto)
	int howto;
{
	register struct buf *bp;
	int nbusy;

	(void) spl1();
	if ((howto&RB_NOSYNC)==0 && mounttab[0].m_bufp) {
		CPRINTF("syncing disks... ");

		/*
		 * Take over TOD clock ticks...
		 */
		(void) splhi();
		sync_timeout = (SYNC_TIMEOUT+(nbuf/sync_est)) * hz;
		obufbusy = bufbusy = nbuf;	/* assume nbuf busy buffers */
		int_bin_table[TODCLKBIN].bh_hdlrtab[TODCLKVEC] = tod_sync;
		(void) spl1();

		update();
		bflush(NULLVP);

		/*
		 * Loop until the sync completes. tod_sync() will
		 * return to firmware if the sync cannot complete.
		 */
		for (;;) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; ) {
				if (bp->b_flags & B_INVAL)
					continue;
				if (!BAVAIL(bp))
					nbusy++;
			}
			if (nbusy != bufbusy) {
				bufbusy = nbusy;
				if (nbusy == 0)
					break;
				CPRINTF("%d ", nbusy);
			}
		}
		CPRINTF("done\n");
	}
}

/*
 * shutdown_light_show()
 *	Turn off processor activity lights and light show.
 */
shutdown_light_show()
{
	register struct	ctlr_toc *toc;
	register int 	i;
	extern bool_t	light_show;

	/*
	 * Get table of contents pointer for processor board.
	 */
#ifdef	ns32000
	toc = &CD_LOC->c_toc[SLB_PROCBOARD];		/* 032 processors */
#endif	ns32000

#ifdef	i386
#ifdef	KXX
	toc = &CD_LOC->c_toc[SLB_KXXBOARD];		/* K20 processors */
#else
	toc = &CD_LOC->c_toc[SLB_SGSPROCBOARD];		/* SGS processors */
#endif	KXX
#endif	i386

	/*
	 * Turn off light show - if enabled.
	 * Since panic may be called before initialization is complete,
	 * all front panel processor lights are turned off.
	 */

	if (light_show) {
		if (fp_lights) {
			for (i = 0; i < toc->ct_count; i++)
				FP_LIGHTOFF(i);
		}
#if	defined(ns32000) || defined(KXX)
		if (fp_lights <= 0)
			(void) rdslave(va_slic->sl_procid, SL_P_LIGHTOFF);
#else
		*(int *) PHYS_LED = 0;
#endif
	}
}

/*
 * sec_return_fw()
 *	Return control to the SCED firmware
 *
 * Invoke custom panic handlers (if panicing)
 * and turn off the light show prior to returning.
 */
sec_return_fw()
{
	register int	i;
	extern	bool_t	conscsi_yet;
	extern	char	*panicstr;
	extern	int	(*cust_panics[])();

	if (upyet)
		(void) splhi();

	/*
	 * If a panic, call custom panic handlers.
	 */

	if (panicstr != NULL)
		for (i = 0; cust_panics[i] != NULL; i++)
			(*cust_panics[i])();

	shutdown_light_show();

	/*
	 * If the console scsi has not yet received its INIT command
	 * then use the powerup cib.
	 */

	if (!conscsi_yet) {
		struct sec_powerup *scp;
		scp = (struct sec_powerup *) (CD_LOC->c_cons->cd_sc_init_queue);
		scp->pu_cib.cib_inst = SINST_RETTODIAG;
		scp->pu_cib.cib_status = SRD_BREAK;
	} else {
		cbdcib->cib_inst = SINST_RETTODIAG;
		cbdcib->cib_status = (!upyet) ? SRD_BREAK : SRD_REBOOT;
	}
#if	defined(DEBUG) && defined(i386) && !defined(KXX)	/*XXX*/
	flush_cache();						/*XXX*/
#endif	DEBUG&&i386&&!KXX					/*XXX*/
	mIntr(cons_scsi, 7, SDEV_SCSIBOARD);

	/*
	 * SCED will take control.
	 */

	for (;;);
	/*NOTREACHED*/
}

/*
 * Watchdog timer routines.
 *
 * Hit watchdog timer every half second.
 */

/*
 * wdtinit()
 *	Initialize watchdog timeout interval.
 */

wdtinit()
{
	register struct sec_smode *wdtsmptr;
	spl_t s_ipl;

        if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
           (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD)) {
		if (ssm_wdt_timeout <= 0)
			return;
		s_ipl = splhi();
		ssm_init_wdt((u_long)(ssm_wdt_timeout * 1000)); /* wdt_timeout milliseconds */
		splx(s_ipl);
	} else {
		wdtsmptr = &wdtsm;
		if (wdt_timeout <= 0)
			return;
		wdtsmptr->sm_status = 0;
		wdtsmptr->sm_un.sm_wdt_mode = wdt_timeout;

		wdtcib->cib_inst = SINST_SETMODE;
		wdtcib->cib_status = (int *)&wdtsm;
		s_ipl = splhi();
		mIntr(cons_scsi, 7, SDEV_WATCHDOG);
		splx(s_ipl);

		while ((wdtsmptr->sm_status & SINST_INSDONE) == 0)
			continue;

		if (wdtsmptr->sm_status != SINST_INSDONE) {
			panic("Initializing Watchdog");
                	/*
                 	 *+ An error occurred when the kernel tried to initialize the
                 	 *+ watchdog timer.
                 	 */
		}
	}
	/*
	 * Adjust poll time to give five tries before timeout; this will
	 * generally guarantee that the SCED will be hit before it times out,
	 * but will be much less frequent than the default, twice per second
	 */
        if ((CD_LOC->c_cons->cd_type != SLB_SSMBOARD)  &&
	   (CD_LOC->c_cons->cd_type != SLB_SSM2BOARD)) 
		wdt_polltime = wdt_timeout*hz/5;
	timeout(wdtreset, (caddr_t)0, wdt_polltime);
}

wdtreset()
{
	register struct	sec_smode *wdtsmptr;
	spl_t s_ipl;

        if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
           (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD))
		ssm_poke_wdt();
	else {
		wdtsmptr = &wdtsm;
		wdtsmptr->sm_status = 0;
		wdtcib->cib_inst = SINST_STARTIO;
		wdtcib->cib_status = (int *)&wdtsm;

		/*
	 	 * Tell SCED about the command.  Bin 3 is sufficient, helps avoid
	 	 * SLIC-bus saturation/lockup (since SCED interrupts Dynix mostly on
	 	 * bins 4-7, using bin 3 to interrupt SCED gives SCED -> Dynix priority
	 	 * over Dynix -> SCED, thus SCED won't deadlock against Dynix).
	 	 */

		s_ipl = splhi();
		mIntr(cons_scsi, 3, SDEV_WATCHDOG);
		splx(s_ipl);

		while ((wdtsmptr->sm_status & SINST_INSDONE) == 0)
			continue;

		if (wdtsmptr->sm_status != SINST_INSDONE) {
			panic("Resetting Watchdog");
                	/*
                 	 *+ An error occurred when the kernel tried to reset the
                 	 *+ watchdog timer.
                 	 */
		}
	}
	timeout(wdtreset, (caddr_t)0, wdt_polltime);
}

/*
 * access_error()
 *	Access Error Reporting:
 *		Bus timeouts
 *		ECC Uncorrectable
 *		Processor fatal error (SGS only)
 *
 * Called from NMI handler, SEC_error and MBAd_error with
 * copy of Access error register.
 *
 * Called at SPLHI.
 */

access_error(errval)
	u_char errval;
{
	register int io_access;
	register char *s;
	u_char acctype;
	extern	memerr();

	printf("Access Error Register = 0x%x\n", errval);
        /*
         *+ An access error has occurred.  More information, such as
         *+ the type of access error, follows.
         */
	errval = ~errval;
	acctype = errval & SLB_ATMSK;
	io_access = errval & SLB_AEIO;

	switch (acctype) {
	case SLB_AEFATAL:
			s = "Fatal";
		break;
	case SLB_AENONFAT:
		if (io_access)
			s = "Non-Fatal";
		else
			s = "Ecc Correctable";
		break;
	case SLB_AETIMOUT:
		s = "Timeout";
		break;
	default:
		s = "Unknown";
		break;
	}
	CPRINTF("%s error on %s %s.\n", s,
		(errval & SLB_AEIO) ? "I/O" : "memory",
		(errval & SLB_AERD) ? "read" : "write");

	/*
	 * If memory error get more data...
	 */

	if ((acctype == SLB_AEFATAL) && (io_access != SLB_AEIO)) {
		if (upyet) {
			/*
			 * Avoid races with memory polling.
			 */
			untimeout(memerr, (caddr_t) 0);
			/*
			 * If concurrent access errors, the loser of the
			 * race commits suicide.
			 *
			 * Since we are about to die, do not bother releasing
			 * lock.
			 */
			if (cp_lock(&uncmem_lock, SPLHI) == CPLOCKFAIL) {
				printf("Concurrent Access Error\n");
                                /*
                                 *+ A concurrent access error has occurred.
                                 *+ That is, another processor received a prior
                                 *+ access error and
                                 *+ the system is already on the way down.
                                 */

				pause_self();
				/*NOTREACHED*/
			}
		}
		memlog();
	}
}

#ifdef i386
/*
 * Same as above, for modelD.
 */
sgs2_access_error(flag)
	int flag;
{
	register char *s;
	extern	memerr();

	/*
	 *+ An access error has occurred.  More information, such as
	 *+ the type of access error, follows.
	 */

	if (isfatal(flag))
		s = "Fatal";
	else if (isnonfatal(flag)) {
		if (isio(flag))
			s = "Non-Fatal";
		else
			s = "Ecc Correctable";
	} else if (istimeout(flag))
		s = "Timeout";
	else
		s = "Unknown";

	cmn_err(CE_CONT, "%s error on %s %s.\n", s,
		isio(flag) ? "I/O" : "memory",
		isread(flag) ? "read" : "write");

	/*
	 * If memory error get more data...
	 */

	if (isfatal(flag) && !isio(flag)) {
		if (upyet) {
			/*
			 * Avoid races with memory polling.
			 */
			untimeout(memerr, (caddr_t) 0);

			/*
			 * If concurrent access errors, the loser of the
			 * race commits suicide.
			 *
			 * Since we are about to die, do not bother releasing
			 * lock.
			 */
			if (cp_lock(&uncmem_lock, SPLHI) == CPLOCKFAIL) {
				cmn_err(CE_WARN, "Concurrent Access Error\n");
				/*
				 *+ a concurrent access error has ocurred.
				 *+ i.e., another processor took an
				 *+ access error before this one and
				 *+ the system's already on the way down.
				 */
				pause_self();
				/*NOTREACHED*/
			}
		}
		memlog();
	}
}

#endif /* i386 */

/*
 * Procedures to verify integrity of system.
 *   check_sys() is called early in main() by the init process.
 *   good_sys() is called by the *REAL* /etc/init.
 */

#define	INIT_GRACE	25*60*hz

check_sys(firsttime)
{
	if (!firsttime) {
		printf("Bogus init detected, returning to single user mode.\n");
                /*
                 *+ The init process has not followed the protocol
                 *+ used by the kernel to identify it as a valid
                 *+ init process.  The system is being returned to
                 *+ single user mode.  Corrective action:
                 *+ replace the /etc/init binary with the version distributed
                 *+ with the operating system.
                 */
		(void) kill1(0, SIGTERM, 1);
	}
	timeout(check_sys, (caddr_t)0, INIT_GRACE);
	bootname = CD_LOC->c_boot_name;	 	/* for get_vers() */
}

good_sys()
{
	untimeout(check_sys, (caddr_t)0);
}

#ifndef KLINT
/*
 * cmn_err (for compability with ptx).
 *
 */

/*VARARGS2*/
/*ARGSUSED*/
cmn_err(level, fmt, ARGS)
	int level;
	char *fmt;
	int ARGS;
{
	switch (level) {
	case CE_TO_USER:
		uprintf(fmt, ARGS);
		break;

	case CE_CONT :
		printf(fmt, ARGS);
		break;

	case CE_NOTE :
		printf("\nNOTICE: ");
		printf(fmt, ARGS);
		printf("\n");
		break;

	case CE_WARN :
		printf("\nWARNING: ");
		printf(fmt, ARGS);
		printf("\n");
		break;

	case CE_PANIC : 
		printf("\nPANIC: ");
		printf(fmt, ARGS);
		printf("\n");
		panic("");

	default :
		cmn_err(CE_PANIC,
		  "unknown level in cmn_err (level=%d, msg=\"%s\")",
		  level, fmt, &ARGS);
		/*
		 *+ an invalid level argument has been passed to the
		 *+ cmn_err routine.
		 */
	}
}
#endif
