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
static	char	rcsid[] = "$Header: clock.c 2.10 90/12/13 $";
#endif

/*
 * Machine-dependent clock routines.
 *
 * Included are the time-of-day clock initialization and
 * the per processor real-time clock initialization.
 */

/* $Log:	clock.c,v $
 */

#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/systm.h"
#include "../h/cmn_err.h"

#include "../balance/cfg.h"
#include "../balance/clock.h"
#include "../balance/slic.h"

#include "../machine/ioconf.h"
#include "../machine/vmparam.h"
#include "../machine/hwparam.h"
#include "../machine/intctl.h"

#include "../sec/sec.h"

#include "../ssm/ioconf.h"
#include "../ssm/ssm_misc.h"
#include "../ssm/ssm.h"
#include "../balance/slicreg.h"

extern u_char	cons_scsi;		/* console scsi slic address */

/* For time-of-day handling */
struct	sec_cib *todcib;
struct	sec_gmode todgm;	/* getmodes command */
struct	sec_smode todsm;	/* setmodes command */

/*
 * startrtclock()
 *	Start the real-time clock.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.  On Sequent HW, this
 * is one-time only per processor (eg, no restart, clock reprimes
 * itself).
 *
 * Called by localinit() during selfinit().
 * This turns on the processor-local SLIC timer.
 *
 * For testing/performance measurement convenience, enable_local_clock
 * allows the per-processor clock to be left OFF.  Need to patch the
 * kernel binary or system memory to effect this.
 */

static	bool_t	enable_local_clock = 1;		/* default ON */

startrtclock()
{
	register struct cpuslic *sl = va_slic;

	if (!enable_local_clock)
		return;

	sl->sl_trv = ((sys_clock_rate * 1000000) / (SL_TIMERDIV * hz)) - 1;
	/* clear prescaler, load reload value */
	sl->sl_tcont = 0;
	sl->sl_tctl = SL_TIMERINT | LCLKBIN;	/* timer on in given bin */
}

/*
 * Routines to manipulate the SCED based time-of-day register.
 * TOD clock interrupt handling done by todclock in kern_clock.c
 *
 *
 * Inittodr initializes the time-of-day hardware which provides
 * date functions. This starts the time-of-day clock.
 *
 * Base provides the time from the filesystem, and the time-of-day
 * clock provides the rest.
 */
inittodr(base)
	time_t base;
{
	register u_int todr;
	register struct	sec_gmode *todgmptr = &todgm;
	register int i;
	long deltat;
	spl_t s_ipl;

	if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
	    (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD)) {
		/* 
	 	 * Verify that TOD clock passed diagnostics.
	 	 */
		ASSERT(SSM_cons, "inittodr: can't find console SSM");
		/*
		 *+ ssm_inittodr was called to initialize the time of day
	   	 *+ hardware, but an SSM has not been selected as the
		 *+ controlling console.
		 */

		if (SSM_cons->ssm_diag_flags & CFG_SSM_TOD) {
			/*
		 	 * Clear todr if TOD failed powerup 
		 	 * diagnostics.  Will use file system
		 	 * time.
		 	 */
			CPRINTF("WARNING: TOD failed powerup diagnostics\n");
			todr = 0;
		} else {
			/*
		 	 * Fetch the current time-of-day from 
		 	 * the SSM tod clock.
		 	 */
			todr = ssm_get_tod(SM_NOLOCK);
		}
	} else {
		/*
	 	 * Find console SCED and check if the TOD clock has
	 	 * failed powerup diagnostics.
	 	 */
		for (i = 0; i < NSEC; i++) {
			/* is SEC there? */
			if ((SECvec & (1 << i)) == 0)
				continue;

			if (SEC_desc[i].sec_is_cons)
				break;
		}
		if (SEC_desc[i].sec_diag_flags & CFG_S_TOD) {
			/*
		 	 * Clear todr if TOD failed powerup diagnostics.
		 	 */
			CPRINTF("WARNING: TOD failed powerup diagnostics\n");
                	/*
                 	 *+ The time of day clock on the SCED controlling
                 	 *+ the system failed power-up diagnostics.  Check the
                 	 *+ system date (and reset if necessary) after
                 	 *+ the system is booted and the failure is repaired.
                 	 */
			todr = 0;
		} else {
			/*
		 	 * get the current time-of-day from the SCED tod clock.
		 	 */
			todgmptr->gm_status = 0;
			todcib->cib_inst = SINST_GETMODE;
			todcib->cib_status = (int *)&todgm;
			s_ipl = splhi();
			mIntr(cons_scsi, TODCLKBIN, SDEV_TOD);
			splx(s_ipl);

			while ((todgmptr->gm_status & SINST_INSDONE) == 0)
				continue;

			if (todgmptr->gm_status != SINST_INSDONE) {
				panic("Cannot get TOD value");
                        	/*
                         	 *+ The SCED didn't respond to a request
                         	 *+ for the current time of day.
                         	 */
			}

			todr = todgmptr->gm_un.gm_tod.tod_newtime;
		}
	}

	if (todr < 13*SECYR) {
		CPRINTF("WARNING: TOD value bad -- Setting TOD to file system time\n");
                /*
                 *+ An invalid time of day value was returned from the
                 *+ console controller.  The system time of day is
                 *+ being loaded from the filesystem.
                 *+ Corrective action:  use the date command to set the
                 *+ time of day to a correct value.
                 */
		if (base < 13*SECYR) {
			CPRINTF("WARNING: preposterous time in file system -- Setting TOD to default time\n");
                        /*
                         *+ The time value in the filesystem is wrong.
                         *+ The time of day is being set from a precalculated
                         *+ value.
                         *+ Corrective action:  use the date command to set the
                         *+ time of day to a correct value.
                         */

			time.tv_sec = 13*SECYR + 19*SECDAY + (2*SECDAY)/3;
			resettodr();
			goto check;
		}
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TOD.
		 */
		time.tv_sec = base;
		resettodr();
		goto check;
	}

	time.tv_sec = todr;

	/*
	 * See if we gained/lost two or more days;
	 * if so, assume something is amiss.
	 */
	deltat = time.tv_sec - base;
	if (deltat < 0)
		deltat = -deltat;
	resettodr();			/* start the tod clock */
	if (deltat < 2*SECDAY) {
		goto nocheck;
	}
	CPRINTF("WARNING: clock %s %d days",
	    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
check:
	CPRINTF("\nCHECK AND RESET THE DATE!\n");
	/*
	 *+ The time of day clock has gained or lost more than
	 *+ two days.  Corrective action:  use the date command
	 *+ to reset the time to the correct value.
	 */
nocheck:
	if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
	    (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD))
			ssm_tod_freq(SM_NOLOCK, TODFREQ, SL_GROUP | TMPOS_GROUP,
					     TODCLKVEC, SL_MINTR | TODCLKBIN);
}

/*
 * Resettodr restores the time-of-day hardware after a time change.
 * Also, also called via inittodr to start todclock interrupts.
 *
 * Reset the TOD based on the time value; used when the TOD
 * has a preposterous value and also when the time is reset
 * by the stime system call.
 */
resettodr()
{
	register struct sec_smode *todsmptr = &todsm;
	spl_t s_ipl;

	if ((CD_LOC->c_cons->cd_type == SLB_SSMBOARD) ||
	    (CD_LOC->c_cons->cd_type == SLB_SSM2BOARD))
		ssm_set_tod((u_long)time.tv_sec);
	else {
		todsmptr->sm_status = 0;
		todsmptr->sm_un.sm_tod.tod_freq = TODFREQ;
		todcib->cib_inst = SINST_SETMODE;
		todcib->cib_status = (int *)&todsm;
		s_ipl = splhi();
		todsmptr->sm_un.sm_tod.tod_newtime = time.tv_sec;
		/*
	 	* Bin 3 is sufficient, helps avoid SLIC-bus lockup.
	 	*/
		mIntr(cons_scsi, 3, SDEV_TOD);
		splx(s_ipl);

		while ((todsmptr->sm_status & SINST_INSDONE) == 0)
			continue;

		if (todsmptr->sm_status != SINST_INSDONE) {
			panic("Cannot set TOD value");
                	/*
                 	*+ The time of day clock didn't accept the
                 	*+ request to reset the time of day.
                 	*/
		}
	}
}
