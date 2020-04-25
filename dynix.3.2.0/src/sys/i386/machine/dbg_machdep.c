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
static	char	rcsid[] = "$Header: dbg_machdep.c 2.9 1991/07/19 17:01:22 $";
#endif

/*
 * dbg_machdep.c
 *	Machine dependent debugger support routines.
 *
 * Conditionals:
 *	-DKWATCHPT:	turn on kernel watchpoint code.
 *	-DWATCHPT:	turn on watchpoint code.
 *
 * i386 version.
 */

#define KWATCHPT

/* $Log: dbg_machdep.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/cmn_err.h"

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/plocal.h"

/*
 * Order of registers must match struct pt_regset.
 */

int	ipcreg[] = {EAX,EBX,ECX,EDX,ESI,EDI,EBP,ESP,EIP};
int	nipcreg	= (sizeof(ipcreg)/sizeof(ipcreg[0]));


/* 
 * kernel watchpoint actions 0 is no or all watchpoints 1,2,3 and4 are for
 * dbg0-3.
 */
int	kw[5] = {CE_PANIC,CE_PANIC,CE_PANIC,CE_PANIC,CE_PANIC};	

/*
 * dbg_watchpt_set()
 *	Set a watchpoint.
 */

/*ARGSUSED*/
dbg_watchpt_set(wpt)
	struct	pt_watchpt *wpt;
{
#ifdef	WATCHPT
	/*
	 * Must insure address is within legit user space
	 * (real problems could happen if certain kernel addresses
	 * were given).  Need to pick a free watchpoint (if any)
	 * and set it up.  No need to program up debug registers,
	 * this process necessarily context switches before
	 * runs user code again.
	 *
	 * May need to not turn on until return from mpt_stop??
	 * Think is ok; worst case is user asks to write a variable
	 * that a watchpoint is set in -- gets another trap, but
	 * that's what he asked for.
	 */
#else	!WATCHPT
	return(EINVAL);				/* for now */
#endif	WATCHPT
}

/*
 * dbg_watchpt_clear()
 *	Clear a watchpoint.
 */

/*ARGSUSED*/
dbg_watchpt_clear(wpt)
	struct	pt_watchpt *wpt;
{
#ifdef	WATCHPT
	/*
	 * See if currently have this address watchpoint'd.
	 * If so, disable it by clearing appropriate wp_control
	 * Gi bit.  If last just went off, set wp_control == DCR_OFF.
	 *
	 * Again, no need to turn off since process will context
	 * switch before really continuing.
	 */
#else	!WATCHPT
	return(EINVAL);				/* for now */
#endif	WATCHPT
}

/*
 * watchpoint()
 *	A watchpoint() debug trap occurred (in user or kernel mode).
 *
 * Turn off watchpoints() and enter mpt_stop().
 *
 * Caller already checked for and handled single-step trap.
 */

#ifdef KWATCHPT
struct watchpt kw_watchpt = { 0, 0, 0, 0, 0 };
#endif

/*ARGSUSED*/
watchpoint(dsr, kernel, eip)
	int	dsr;			/* Debug Status Register (DR6) */
	bool_t	kernel;			/* trap came from kernel mode? */
	unsigned int	eip;		/* pc when watchpoint traped */
{
#ifdef KWATCHPT
	int	wp;

	/*
	 * If came from kernel mode, CE_PANIC or CE_NOTE
	 */
	if (kernel) {
		kwatch_regs(0, 0);
		if (wp = ffs(dsr & 0xf)) {
			cmn_err(kw[wp-1], "watchpoint %d at eip=0x%x", wp-1, 
				eip);
			/*
			 *+ A watchpoint was encountered while a process was executing
			 *+ in the kernel.  
			 */
		} else {
			cmn_err(kw[4], "watchpoint eip=0x%x", eip);
			/*
			 *+ A watchpoint was encountered while a process was executing
			 *+ in the kernel.  
			 */
		}
		WRITE_DSR(DSR_CLEAR);
		return;
	}
#else	/* !KWATCHPT */

	if (kernel) {
		kwatch_regs(0, 0);
		cmn_err(CE_PANIC, "watchpoint: kernel mode not supported");
		/*
		 *+ A watchpoint was encountered while a process was executing
		 *+ in the kernel; however, setting watchpoints in the kernel
		 *+ is not yet supported.  This indicates a probable
		 *+ kernel software bug.
		 */
		/*NOTREACHED*/
	}
#endif
#ifdef	WATCHPT
	register struct proc *p = u.u_procp;

	ASSERT(dsr & DSR_WATCHPT, "watchpoint: dsr");
        /*
         *+ The kernel's watchpoint-handling routine was entered,
         *+ but the processor's debug status register doesn't indicate
         *+ that a watchpoint has been encountered.  This indicates a
         *+ probable kernel software bug.
         */
	ASSERT(l.watchptON, "watchpoint: !watchptON");
        /*
         *+ The kernel's watchpoint-handling routine has been entered,
         *+ but the software status of that processor indicates that
         *+ watchpoints are not enabled on that processor.  This
         *+ indicates a probable kernel software bug.
         */


	/*
	 * Disable watchpoints in HW and arrange SW doesn't turn on again.
	 */

	WRITE_DCR(DCR_OFF);			/* turn OFF HW watchpoints */
	u.u_watchpt.wp_control = DCR_OFF;	/* turn OFF SW shadow */
	l.watchptON = 0;			/* really OFF */

	/*
	 * Lock proc-state and stop for debugger.
	 * Could race with debugger exit, thus only stop if still supposed to.
	 */

	(void) p_lock(&p->p_state, SPLHI);

	if (p->p_flag & SMPTRC)
		mpt_stop(PTS_WATCHPT_AFTER);

	v_lock(&p->p_state, SPL0);
#else	!WATCHPT
	panic("watchpoint");
        /*
         *+ The user watchpoint handler was entered, but
         *+ watchpoints are not yet supported in this kernel.
         *+ This indicates a probable kernel software bug.
         */
#endif	WATCHPT
}

/*
 * set kernel mode watchpoints
 */
/*ARGSUSED*/
int
kwatchpt(arg1,arg2,arg3)
	int arg1;
	int arg2;
	int arg3;
{
#ifdef lint
	extern void WRITE_DB6();
#endif
	/*
	 * arg1 is which register.
	 * arg2 is the watchpoint mask.
	 * arg3 is the action value.
	 */
#ifdef KWATCHPT
	cmn_err(CE_CONT, "setting kernel watch point #%d to 0x%x action=0x%x\n",
			arg1, arg2, arg3);
	switch (arg1) {
	case 0:
		kw_watchpt.wp_vaddr[0] = arg2 + KSOFF;
		kw[0] = arg3;
		break;
	case 1:
		kw_watchpt.wp_vaddr[1] = arg2 + KSOFF;
		kw[1] = arg3;
		break;
	case 2:
		kw_watchpt.wp_vaddr[2] = arg2 + KSOFF;
		kw[2] = arg3;
		break;
	case 3:
		kw_watchpt.wp_vaddr[3] = arg2 + KSOFF;
		kw[3] = arg3;
		break;
	case 6:
		WRITE_DB6(arg2);
		break;
	case 7:			/* set db7 */
		kw_watchpt.wp_control = arg2;
		kw[4] = arg3;
		WRITE_DB0(kw_watchpt.wp_vaddr[0]);
		WRITE_DB1(kw_watchpt.wp_vaddr[1]);
		WRITE_DB2(kw_watchpt.wp_vaddr[2]);
		WRITE_DB3(kw_watchpt.wp_vaddr[3]);
		WRITE_DB7(kw_watchpt.wp_control);
		break;
	case -1:		/* just list the options */
		break;
	default:
		return (EINVAL);
	}
	kwatch_regs(1, 1);
	return (0);
#else
	return (EINVAL);
#endif
}

/*ARGSUSED*/
kwatch_regs(type, all)
	int 	type;
	int	all;
{
	int	i;

#ifdef KWATCHPT
	if (type == 1) {	/* display global copies of registers, too */
		cmn_err(CE_CONT, "kw_watchpt: db0:0x%x db1:0x%x db2:0x%x db3:0x%x\n",
			kw_watchpt.wp_vaddr[0], kw_watchpt.wp_vaddr[1],
			kw_watchpt.wp_vaddr[2], kw_watchpt.wp_vaddr[3]);
		cmn_err(CE_CONT, "kw_watchpt: db7:0x%x\n", 
			kw_watchpt.wp_control);
		cmn_err(CE_CONT, "local regs: ");
	}
#endif

	cmn_err(CE_CONT, "db0:0x%x db1:0x%x db2:0x%x db3:0x%x\n",
		READ_DB0(), READ_DB1(),
		READ_DB2(), READ_DB3());
	cmn_err(CE_CONT, "db6:0x%x db7:0x%x\n", 
		READ_DB6(), READ_DB7());
	if (all) {
		cmn_err(CE_CONT, "actions: ");
		for (i=0; i<5; i++ ) {
			if (i==4)
				cmn_err(CE_CONT, " multiple=");
			else
				cmn_err(CE_CONT, " %d=", i);
			switch (kw[i]) {
			case CE_TO_USER:
				cmn_err(CE_CONT, "CE_TO_USER");
				break;
			case CE_CONT:
				cmn_err(CE_CONT, "CE_CONT   ");
				break;
			case CE_NOTE:
				cmn_err(CE_CONT, "CE_NOTE   ");
				break;
			case CE_PANIC:
				cmn_err(CE_CONT, "CE_PANIC  ");
				break;
			case CE_WARN:
				cmn_err(CE_CONT, "CE_WARN   ");
				break;
			default:
				cmn_err(CE_CONT, "0x%x ", i, kw[i]);
				break;
			}
		}
		cmn_err(CE_CONT, "\n");
	}
}
