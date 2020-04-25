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

#ifndef lint
static char rcsid[] = "$Header: ssm_misc.c 1.11 90/11/13 $";
#endif lint

/*
 * ssm_misc.c
 *	Routines for manipulating misc 
 *	pieces of the SSM.
 */

/* $Log:	ssm_misc.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/cmn_err.h"
#include "../machine/intctl.h"
#include "../balance/cfg.h"
#include "../ssm/ioconf.h"
#include "../ssm/ssm_misc.h"
#include "../ssm/ssm.h"
#include "../ssm/sc.h"
#include "../ssm/sp.h"

#ifdef i386
/*
 * Scan interface is only present on i386 Model-D and E  Machines.
 */
#include "../machine/scan.h"
#define SCAN	1
#endif /* i386 */

#include "../balance/slic.h"

static int ssm_gen_cmd();

/*
 * ssm_send_misc_addr() 
 *	Send address of ssm_misc structure.
 *
 * 	Assumes that no outstanding requests are 
 *	active using the current SSM message-passing 
 *	structure.
 */
void
ssm_send_misc_addr(addr, dest)
	struct ssm_misc *addr;
	u_char dest;
{
	u_char *addr_bytes = (u_char *)&addr;

	mIntr(dest, SM_BIN, SM_ADDRVEC);
	mIntr(dest, SM_BIN, addr_bytes[0]);	/* low byte first */
	mIntr(dest, SM_BIN, addr_bytes[1]);
	mIntr(dest, SM_BIN, addr_bytes[2]);
	mIntr(dest, SM_BIN, addr_bytes[3]);	/* high byte last */
}

/*
 * ssm_get_fpst()
 *	Return the state of the front-panel.
 *
 * 	'context' is either SM_LOCK or SM_NOLOCK, 
 *	depending on whether this routine is called 
 *	in a context where locking of the common SSM 
 *	communication area is required.
 *
 * 	Returns a bit-vector defining the front-panel 
 *	state.
 *
 * 	Assumes this command can never fail.
 */
u_long
ssm_get_fpst(context)
	int	context;
{	
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;
	u_long val;

	if (context == SM_LOCK) {
		s = p_lock(&ssm_mc->sm_lock, SM_SPL);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_FP, SM_GET);

	/* suck out value (while ssm_mc still locked) */
	val = ssm_mc->sm_fpst;

	if (context == SM_LOCK)
		v_lock(&ssm_mc->sm_lock, s);

	return (val);
}

/*
 * ssm_set_fpst()
 *	Set the state of the front-panel.
 *
 * 	'fpst' is a bit-vector defining the new 
 *	front-panel state. Not all bits are 
 *	writable (as they correspond to physical 
 *	switches on the front panel).
 *	
 * 	'context' is either SM_LOCK or SM_NOLOCK, 
 *	depending on whether this routine is 
 *	called in a context where locking of the 
 * 	common SSM communication area is required.  
 *
 *	Assumes this command can never fail.
 */
void
ssm_set_fpst(fpst, context)
	u_long	fpst;
	int	context;
{	
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;

	if (context == SM_LOCK)
		s = p_lock(&ssm_mc->sm_lock, SM_SPL);
		ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_fpst = fpst;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_FP, SM_SET);

	if (context == SM_LOCK)
		v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_init_wdt()
 *	Set up watchdog timer.
 *
 * 	'ms' is the number of milliseconds before the 
 *	RUN light is extinguished.  Dynix must poke 
 *	the watchdog timer at least this often to 
 *	keep the error light on.
 *
 * 	Called when locking is unnecessary.
 */
static u_long wdt_loc;

void
ssm_init_wdt(ms)
	u_long	ms;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_wdtst.wdt_addr = (u_long)&wdt_loc;
	ssm_mc->sm_wdtst.wdt_intvl = ms;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_WDT, SM_SET);
}

/*
 * ssm_poke_wdt
 *	Poke the watchdog timer to show SSM we're still alive.
 *
 * 	Doesn't require locking, as the SSM message 
 *	area is not used.
 */
void
ssm_poke_wdt()
{
	++wdt_loc;
}


/*
 * ssm_set_vme_mem_window( ssm_mc, ssm_slic, window )
 *	set the dest ssm to respond to the window for the VME
 *
 *      'ssm_mc' is the address of the misc cb.
 * 	'ssm_slic' is the SLIC id of the target SSM.
 *	'window' is the VME window to assign to this SSM
 *
 *	windonw 0 = IO BASE + 32Mb
 *		1 = IO BASE + 64Mb
 *		etc
 *
 * 	Assumes no errors are possible on this command, 
 *	and only called in a context where locking is 
 *	unnecessary (e.g. system initialization time).
 */
void
ssm_set_vme_mem_window(ssm_mc, ssm_slic, window )
	struct ssm_misc *ssm_mc;
	u_char ssm_slic;
	u_char window;
{
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_vme_imap.vme_window = window;
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_SET_WINDOW );
}

#ifdef notyet
/*
 * ssm_set_vme_mem_off()
 *      turns off accessiability to the VME, by turning off the PIC
 *      thus the SSM will not respond nor generate accesses, etc
 */
void
ssm_set_vme_mem_off(ssm_mc, slic)
	struct ssm_misc *ssm_mc;
	u_char slic;
{
	bzero((char*)&ssm_mc->sm_un, sizeof (ssm_mc->sm_un) );
	ssm_gen_cmd(ssm_mc, slic, SM_VME, SM_SET_OFF);
}
#endif notyet


/*
 * ssm_set_vme_imap()
 *	Add an entry to SSM's VME interrupt mapping.
 *
 *      'ssm_mc' is the address of the misc cb.
 * 	'ssm_slic' is the SLIC id of the target SSM.
 * 	'vlev' is the VMEbus interrupt level.
 * 	'vvec' is the VMEbus interrupt vector.
 * 	'dest' is the SLIC destination.
 * 	'svec' is the SLIC interrupt vector.
 * 	'cmd' is the SLIC command to issue (e.g. SL_MINTR | 7)
 *
 * 	Assumes no errors are possible on this command, 
 *	and only called in a context where locking is 
 *	unnecessary (e.g. system initialization time).
 */
void
ssm_set_vme_imap(ssm_mc, ssm_slic, vlev, vvec, dest, svec, cmd)
	struct ssm_misc *ssm_mc;
	u_char	ssm_slic, vlev, vvec, dest, svec, cmd;
{
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_vme_imap.vi_vlev = vlev;
	ssm_mc->sm_vme_imap.vi_vvec = vvec;
	ssm_mc->sm_vme_imap.vi_dest = dest;
	ssm_mc->sm_vme_imap.vi_svec = svec;
	ssm_mc->sm_vme_imap.vi_cmd = cmd;
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_SET);
}

/*
 * ssm_clr_vme_imap()
 *	Clear all VME interrupt mapping for an SSM.
 *
 * 	'ssm_slic' is the SLIC id of the target SSM.
 *
 * 	Assumes called in a context where no locking 
 *	is required, and that there are no possible 
 *	errors from this command.
 */
void
ssm_clr_vme_imap(ssm_mc, ssm_slic)
	struct ssm_misc *ssm_mc;
	u_char	ssm_slic;
{
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_CLR_MAP);
}
/*
 * ssm_vme_imap_ready()
 *      Notify the SSM that the interrupt mapping is
 *      complete and all SSM VME devices have been
 *      booted.  The SSM needs to know this to
 *      correctly control SYSFAIL and BUSERR.
 *
 *      'ssm_slic' is the SLIC id of the target SSM.
 *
 *      Assumes called in a context where no locking
 *      is required, and that there are no possible
 *      errors from this command.
 */
void
ssm_vme_imap_ready(ssm_mc, ssm_slic)
 	struct ssm_misc *ssm_mc;
        u_char  ssm_slic;
{
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
    	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_MAP_RDY);
}

#ifdef notyet
/*
 * ssm_gen_vme_intr()
 *	Generate a VME interrupt.
 *
 * 	'ssm_slic' is the SLIC id of the target SSM.
 * 	'virq' is the VMEbus interrupt request line.
 *
 * 	Assumes there are no possible errors from 
 *	this command, and it is always called in 
 *	a context where locking is needed.
 */
void
ssm_gen_vme_intr(ssm_mc, ssm_slic, virq, vvec)
	struct ssm_misc *ssm_mc;
	u_char	ssm_slic, virq, vvec;
{
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_vme_gen.vg_irq = virq;
	ssm_mc->sm_vme_gen.vg_vvec = vvec;
	ssm_gen_cmd(ssm_mc, ssm_slic, SM_VME, SM_GEN_VME);
	v_lock(&ssm_mc->sm_lock, s);
}
#endif notyet

/*
 * ssm_get_tod()
 *	Retrieve the current time of day from the SSM.
 *
 * 	'context' is either SM_LOCK, or SM_NOLOCK, and 
 *	determines whether we are called from a context 
 *	where locking is necessary.
 *
 * 	Time returned is in seconds since Midnight Jan 1, 1970.
 */
u_long
ssm_get_tod(context)
	int context;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;

	if (context == SM_LOCK) {
		s = p_lock(&ssm_mc->sm_lock, SM_SPL);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_TOD, SM_GET);

	if (context == SM_LOCK)
		v_lock(&ssm_mc->sm_lock, s);
	
	return(ssm_mc->sm_todval);
}

/*
 * ssm_set_tod()
 *	Set the current time of day on the SSM.
 *
 * 	't' is the current time, measured in seconds 
 *	from midnight Jan1, 1970.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_set_tod(t)
	u_long	t;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_todval = t;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_TOD, SM_SET);

	v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_tod_freq()
 *	Setup time-of-day interrupts from SSM
 *
 * 	'context' is either SM_LOCK or SM_NOLOCK,
 * 	depending on whether locking is required.
 * 	'freq' is the interval in milliseconds
 *	at which interrupts from SSM are generated.
 *	'freq' == 0 turns off the time-of-day interrupts.
 * 	'dest' is the SLIC interrupt destination.
 * 	'vec' is the SLIC interrupt vector for the 
 *	SSM to use.
 * 	'cmd' is the SLIC command for the SSM to use.
 */
void
ssm_tod_freq(context, freq, dest, vec, cmd)
	int	context, freq;
	u_char	dest, vec, cmd;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;

	if (context == SM_LOCK) {
		s = p_lock(&ssm_mc->sm_lock, SM_SPL);
		ssm_chk_intr(ssm_mc);
	}

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_todfreq.tod_freq = freq;
	ssm_mc->sm_todfreq.tod_dest = dest;
	ssm_mc->sm_todfreq.tod_vec = vec;
	ssm_mc->sm_todfreq.tod_cmd = cmd;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_TOD, SM_SET_FREQ);

	if (context == SM_LOCK)
		v_lock(&ssm_mc->sm_lock, s);
}

#ifdef notyet
/*
 * ssm_set_errst()
 *	Tell the SSM how to poll the bus and report errors.
 *
 * 	'time' is how often the SSM should poll the 
 *	bus (in milliseconds).  If 'time' is zero, 
 *	the SSM stops polling the bus.
 * 	'dest' is the SLIC destination to use when 
 *	reporting a failure to access the bus.
 * 	'vecbase' is the base SLIC vector to use in 
 *	the report.  Errors may be reported with SLIC 
 *	vectors ranging from 'vecbase', through 
 *	'vecbase' + ERRST_NERRS - 1.
 * 	'cmd' is the SLIC command to use in the report.  
 *	If 'cmd' is zero, the SSM will poll the bus, 
 *	but will not attempt to report errors.
 *
 * 	Assumed to be called when locking is never required.
 */
void
ssm_set_errst(time, dest, vecbase, cmd)
	u_long	time;
	u_char	dest, vecbase, cmd;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_errst.err_time = time;
	ssm_mc->sm_errst.err_dest = dest;
	ssm_mc->sm_errst.err_vecbase = vecbase;
	ssm_mc->sm_errst.err_cmd = cmd;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_ERR, SM_SET);
}

/*
 * ssm_get_errst()
 *	Retrieve how the SSM polls the bus and reports errors.
 *
 * 	'time' is how often the SSM is polling the 
 *	bus (in milliseconds).
 * 	'dest' is the SLIC destination used when 
 *	reporting a failure to access the bus.
 * 	'vecbase' is the base SLIC vector being 
 *	used to report errors.
 * 	'cmd' is the SLIC command being used in 
 *	the reports.
 *
 * 	Assumes called when locking is never required.
 */
void
ssm_get_errst(time, dest, vecbase, cmd)
	u_long	*time;
	u_char	*dest, *vecbase, *cmd;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_ERR, SM_GET);

	*time = ssm_mc->sm_errst.err_time;
	*dest = ssm_mc->sm_errst.err_dest;
	*vecbase = ssm_mc->sm_errst.err_vecbase;
	*cmd = ssm_mc->sm_errst.err_cmd;
}

/*
 * ssm_set_powerst()
 *      Set power supply state.
 *
 *      'mask' is a bit-vector describing which
 *      power supply state bits are interesting;
 *      any combination of PSST_AC_FAIL, PSST_UPS_ON,
 *      or PSST_DC_FAIL may be set.
 *      'vec' is the vector that will be used to send
 *      the interrupt to Dynix when the bits specified
 *      by 'mask' change.
 *
 *      Whenever any of the power supply status bits
 *      defined by mask changes, the SSM will send the
 *      specifid interrupt to Dynix.
 *
 *      Assumes called when locking is required.
 */
void
ssm_set_powerst(mask, vec)
	u_long mask;
	u_char vec;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
 	int s;

 	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

 	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_powerst.pow_flags = mask;
	ssm_mc->sm_powerst.pow_dest = SL_GROUP | TMPOS_GROUP;
	ssm_mc->sm_powerst.pow_vec = vec;
	ssm_mc->sm_powerst.pow_cmd = SL_MINTR | PSST_BIN;
											ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_POWER, SM_SET);
	v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_get_powerst()
 *      Return the status of the power supply
 *      (a bit-vector of status bits).
 *
 *      Assumes called when locking is required.
 */
u_long
ssm_get_powerst()
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;
	u_long val;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_POWER, SM_GET);

	/* suck out value (while ssm_mc still locked) */
	val = ssm_mc->sm_powerst.pow_flags;

	v_lock(&ssm_mc->sm_lock, s);

	return (val);
}

/*
 * psstintr()
 *	Power supply state interrupt service routine
 *
 *	Get the current state of the power supply signals.
 *	If the UPS state changed send init a SIGPWR with
 *	an indication of the state change.  For Now ignore
 *	the rest of the power supply signal bits.
 */
/*ARGSUSED*/
void
psstintr(vector)
	int vector;
{
	u_long ups_vector;
	int state;

	ups_vector = ssm_get_powerst();
	state = (ups_vector & PSST_UPS_ON) ? PWR_FAIL : PWR_OK;
	if (ups_state != state) {
		ups_state = state;
		/* signal init */
		psignal(&proc[1], SIGPWR);
	}
	if (ups_vector & PSST_AC_FAIL) {
		/*
		 * We will know if 4 usec if AC is really going away
		 * Try to send a message to the console.  If we make it
		 * it must have just been a glitch.
		 */
		cmn_err(CE_WARN, "AC POWER FAIL");
		/*
		 *+ The power supply has indicated that the AC power
		 *+ failed.  This message can be caused by a
		 *+ power line transient or by the failure of the power supply.
		 *+ Corrective action:  check AC power, call service.
		 */
		if (ssm_get_powerst() & PSST_AC_FAIL) {
			/*
			 * We should really never get here.  We have a
			 * AC power fail indication, but the power has
			 * not gone away.  The power supply could never
			 * hold up the DC voltages this long if AC was
			 * really gone.
			 */
			cmn_err(CE_WARN, "Bad AC power fail signal");
			/*
			 *+ Either the power supply AC power fail
			 *+ circuit has failed or AC power is unstable.
			 *+ Corrective action:  check AC power, call service.
			 */
		}
	}
	if (ups_vector & PSST_DC_FAIL) {
		/*
		 * We should never get here.  The system should have
		 * crashed before now.  This is not recoverable
		 */
		cmn_err(CE_PANIC, "DC POWER FAIL");
		/*
		 *+ The power supply has indicated that one of the DC
		 *+ voltages has gone out of regulation.  Corrective
		 *+ action:  call service.
		 */
	} 
}

/*
 * ssm_init_powerst()
 *	Set up the power supply interrupt.
 */
void
ssm_init_powerst()
{
	u_char vector;
	u_long ups_vector;

	vector = ivecall(PSST_BIN);
	ssm_set_powerst(PSST_AC_FAIL | PSST_UPS_ON | PSST_DC_FAIL, vector);
	ups_vector = ssm_get_powerst();
	ups_state = (ups_vector & PSST_UPS_ON) ? PWR_FAIL : PWR_OK;
	ivecinit(PSST_BIN, vector, psstintr);
}
#endif notyet

/*
 * ssm_reboot()
 *	Reboot with these flags and string.
 *
 * 	'flags' is the boot flags to reboot with.
 * 	'size' is the number of bytes in 'str'.
 * 	'str' is a character buffer with the boot string.
 *
 * 	Assumed called when no locking is required.
 */
void
ssm_reboot(flags, size, str)
	u_int size;
	u_short	flags;
	char *str;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_flags = flags;
	if (size)
		bcopy(str, ssm_mc->sm_boot.boot_str,
	      		(size > BNAMESIZ)? BNAMESIZ: size);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_BOOT, SM_REBOOT);
}

/*
 * ssm_get_boot()
 *	Returns boot info.
 *
 * 	'which' specifies which default boot string to get.
 *		It is one of SM_DYNIX or SM_DUMPER.
 *	'cmd' specifies whech set of boot strings to get.  It
 *		is SM_SET_DFT for the NOVRAM copy, or SM_SET for
 *		volatile copy.
 * 	'flags' is a pointer to the location that gets the current
 *		current boot flags.
 * 	'size' is the number of bytes that can be written in 'str'.
 * 	'str' is a character buffer for placing the boot string.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_get_boot(which, cmd, flags, size, str)
	int which, cmd;
	u_short	*flags;
	u_int size;
	char *str;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;
	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_which = which;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_BOOT, cmd);
	*flags = ssm_mc->sm_boot.boot_flags;
	bzero(str, size);		/* Clear the buffer to begin with */
	bcopy(ssm_mc->sm_boot.boot_str, str, (size > ssm_mc->sm_boot.boot_size)?
		 ssm_mc->sm_boot.boot_size : size);
	v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_set_boot()
 *	Set current boot info.
 *
 * 	'which' specifies which default boot string is to be
 *		set.  It is one of SM_DYNIX or SM_DUMPER.
 *	'cmd' specifies whech set of boot strings are to be
 *		set.  It is SM_SET_DFT for NOVRAM copy, or SM_SET
 *		for volatile copy.
 * 	'flags' is the new default boot flags.
 * 	'size' is the number of bytes in 'str'.
 * 	'str' is a character buffer with the boot string.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_set_boot(which, cmd, flags, size, str)
	int which, cmd;
	u_short	flags;
	u_int size;
	char *str;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;
	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_which = which;
	ssm_mc->sm_boot.boot_flags = flags;
	bcopy(str, ssm_mc->sm_boot.boot_str,
	      (size > BNAMESIZE)? BNAMESIZE: size);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_BOOT, cmd);
	v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_get_log_info()
 *	Returns location and total size of the
 *	SSM message log.   Since the log is
 *	a queue, it also returns the location
 *	of the next free character.
 *
 * 	Assumes called when locking is required.
 */
void
ssm_get_log_info(unit, buf, next, size)
	int unit;
	char **buf, **next;
	short *size;
{
	register struct ssm_misc *ssm_mc = SSM_desc[unit].ssm_mc;
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_desc[unit].ssm_slicaddr, SM_LOG, SM_GET);
	*buf = ssm_mc->sm_log.log_buf;
	*next = ssm_mc->sm_log.log_next;
	*size = ssm_mc->sm_log.log_size;

	v_lock(&ssm_mc->sm_lock, s);
	return;
}

/*
 * ssm_get_ram()
 *	Obtain the contents of the SSM RAM specified.
 *
 *	'unit' is the index of the applicable SSM.
 * 	'buf' is the host physical address to which
 *	'size' bytes of data from SSM RAM will be
 *	copied starting at 'ram'.  This buffer must 
 *	not cross a megabyte boundary.
 *
 * 	Assumes called when locking of the SSM message 
 *	area is required.
 */
void
ssm_get_ram(unit, size, buf, ram)
	int unit;
	int size;
	char *buf, *ram;
{
	register struct ssm_misc *ssm_mc = SSM_desc[unit].ssm_mc;
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_ram.ram_buf = buf;
	ssm_mc->sm_ram.ram_loc = ram;
	ssm_mc->sm_ram.ram_size = size;
	ssm_gen_cmd(ssm_mc, SSM_desc[unit].ssm_slicaddr, SM_RAM, SM_GET);
	v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_set_ram()
 *	Set the contents of the SSM RAM specified.
 *
 *	'unit' is the index of the applicable SSM.
 * 	'buf' is the host physical address from which
 *	'size' bytes of data are copied to SSM RAM 
 *	starting at 'ram'.  This buffer must not cross 
 *	a megabyte boundary.
 *
 * 	Assumes called when locking of the SSM message 
 *	area is required.
 */
void
ssm_set_ram(unit, size, buf, ram)
	int unit;
	int size;
	char *buf, *ram;
{
	register struct ssm_misc *ssm_mc = SSM_desc[unit].ssm_mc;
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_ram.ram_loc = ram;
	ssm_mc->sm_ram.ram_buf = buf;
	ssm_mc->sm_ram.ram_size = size;
	ssm_gen_cmd(ssm_mc, SSM_desc[unit].ssm_slicaddr, SM_RAM, SM_SET);
	v_lock(&ssm_mc->sm_lock, s);
}

#ifdef notyet
/*
 * ssm_get_nvram_size()
 *	Return size of SSM Dynix-accessible non-volatile RAM.
 *
 * 	Assumes called when locking is required.
 */
int
ssm_get_nvram_size()
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s, size;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_NVRAM,
		    SM_GET_NVRAMSIZ);
	size = ssm_mc->sm_nvram.nv_size;
	v_lock(&ssm_mc->sm_lock, s);
	return (size);
}

/*
 * ssm_get_nvram()
 *	Obtain the contents of Dynix-accessible NVRAM.
 *
 * 	'buf' is the physical address of a buffer that 
 *	will receive 'size' bytes of non-volatile RAM 
 *	data.  This buffer must not cross a megabyte 
 *	boundary.
 *
 * 	Assumes called when locking of the SSM message 
 *	area is required.
 */
void
ssm_get_nvram(size, buf)
	char *buf;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_nvram.nv_size = size;
	ssm_mc->sm_nvram.nv_buf = buf;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_NVRAM, SM_GET);
	v_lock(&ssm_mc->sm_lock, s);
}

/*
 * ssm_set_nvram()
 *	Set the contents of Dynix-accessible NVRAM.
 *
 * 	'buf' is the physical address of a buffer that 
 *	contains 'size' bytes of non-volatile RAM data.  
 *	This buffer must not cross a megabyte boundary.
 *
 * 	Assumes called when locking of the SSM message 
 *	area is required.
 */
void
ssm_set_nvram(size, buf)
	char *buf;
{
	register struct ssm_misc *ssm_mc = SSM_cons->ssm_mc;
	int s;

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);
	ssm_chk_intr(ssm_mc);

	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_nvram.nv_size = size;
	ssm_mc->sm_nvram.nv_buf = buf;
	ssm_gen_cmd(ssm_mc, SSM_cons->ssm_slicaddr, SM_NVRAM, SM_SET);
	v_lock(&ssm_mc->sm_lock, s);
}
#endif notyet

/*
 * ssm_gen_cmd()
 *	Send a generic command to the SSM
 *
 * 	'dest' is the SLIC id of the SSM (for when 
 *	there are multiple SSM's in a system).
 *	'who' is who on the SSM gets the message.
 * 	'cmd' is the command to send.
 *
 * 	Returns the contents of 'sm_stat' after the 
 *	command completes.
 *
 * 	Assumes that the rest of the 'ssm_mc' has 
 *	been filled in, and that we have exclusive 
 *	control of it.
 */
static int
ssm_gen_cmd(ssm_mc, dest, who, cmd)
	struct ssm_misc *ssm_mc;
	u_char dest;
	int who, cmd;
{
	spl_t s;

	/* build command to ssm */
	ssm_mc->sm_who = who;
	ssm_mc->sm_cmd = cmd;
	ssm_mc->sm_stat = SM_BUSY;

	/* send it and wait for completion */
	s = splhi();
	mIntr(dest, SM_BIN, (u_char)who);
	splx(s);
	while (ssm_mc->sm_stat == SM_BUSY)
		;
}


/*
 * SGS2 introduces SSM based scan library access.
 * Configuration and status registers are no longer accessible via
 * the slic; they must be accessed via a scan interface gained via
 * requests to the SSM.
 *
 * As with all previous SSM messages, certain access to these
 * command chains can be gotten using a "poll on completion" interface.
 * For performance software, a "interrupt on completion" interface is
 * required.  Thus, "poll on completion" routines must be aware of
 * "interrupt on completion" requests and take special precautions.
 *
 * In general, pre-SGS2 routines are only available in "poll on completion"
 * versions.  Post-SGS2 routines are available in both versions.
 *
 * The following variables allow sharing of the SSM mailbox by both the
 * poll on completion and the "interrupt on completion" routines.
 */
static int ssm_interrupt_active = 0;
static int ssm_copied = 0;
static sema_t ssm_interrupt, ssm_done;
#ifdef SCAN
static caddr_t ssm_scanbuf[2];
static struct ssm_misc scan_misc_copy;
static ulong scan_vec = 0xffffffff;	/* out of bounds value */
static u_char scan_dest = TMPOS_GROUP;
static u_char scan_bin = SCAN_BIN;
#endif

/*
 * The scan library must communicate with the console SSM long before
 * autoconfigure (when the ssm misc routines are initialized).  For this
 * reason, we keep a relatively independent ssm_misc structure for the
 * scan library and then attach this to the console SSM at autoconfig
 * time.
 */
#ifdef SCAN
static struct ssm_misc *scan_misc = 0;
static int scan_slicaddr = 0;
int setup_scan_intr = 0;
int ssm_misc_intr();
#endif /* SCAN */


extern int nsgs2;

#define	round2long(x)	(((x) + sizeof(long)+1)/sizeof(long))

static struct ssm_misc ssm_misc_copy;

struct ssm_misc *
ssm_misc_init(iscons, slicaddr)
	int	iscons;
	u_char	slicaddr;
{
	register struct ssm_misc *mc;

	if (!iscons) {
		/* 
		 * Allocate the message passing structure 
		 * for misc. SSM commands and notify the 
		 * SSM of its location.
		 */
		mc = (struct ssm_misc *)
			ssm_alloc(sizeof(struct ssm_misc), 
				  SSM_ALIGN_XFER, SSM_BAD_BOUND);
		init_lock(&mc->sm_lock, ssmgate);
		ssm_send_misc_addr(mc, slicaddr);
		return(mc);
	}

#ifdef SCAN
	if (scan_misc) {
		setup_scan_intr = 1;
		

		/*
		 * Console interface misc interface already allocated.
		 */
		return(scan_misc);
	}
#endif /* SCAN */
	/*
	 * Initialize the console scan interface.  This is used by the
	 * scan library access routines early in startup (before autoconf).
	 */
	mc = (struct ssm_misc *) ssm_alloc(sizeof(struct ssm_misc),
			  SSM_ALIGN_XFER, SSM_BAD_BOUND);
	init_lock(&mc->sm_lock, ssmgate);
	ssm_send_misc_addr(mc, slicaddr);

	/*
	 * Initialize the other aspects of the scan interface.
	 */
	init_sema(&ssm_interrupt, 1, 0, ssmgate);
	init_sema(&ssm_done, 0, 0, ssmgate);

#ifdef SCAN
	/*
	 * Allocate 2 properly aligned buffers to send/receive the
	 * scan data.
	 */
	ssm_scanbuf[0] = (caddr_t)ssm_alloc(SCAN_MAXSIZE, SSM_ALIGN_XFER,
					    SSM_BAD_BOUND);
	ssm_scanbuf[1] = (caddr_t)ssm_alloc(SCAN_MAXSIZE, SSM_ALIGN_XFER,
					    SSM_BAD_BOUND);
	scan_misc = mc;
	scan_slicaddr = slicaddr;
#endif /* SCAN */
	return(mc);
}

#ifdef SCAN
scan_init(slicaddr)
	int slicaddr;
{
	(void)ssm_misc_init(1, (u_char)slicaddr);
}
#endif

/*
 * Check to see if the interrupt interface to the system is active.
 * If so, we must wait for the command to complete and copy the results
 * off to the side.
 */
static
ssm_chk_intr(ssm_mc)
	struct ssm_misc *ssm_mc;
{
	if (nsgs2 <= 0 || !ssm_interrupt_active)
		return;

	/*
	 * The SSM's busy with a "interrupt on completion" command.
	 * we must wait for this command to complete before issueing
	 * our own.
	 */
	while (ssm_mc->sm_stat == SM_BUSY)
		;

	/*
	 * The command's complete.  Copy the results off to the
	 * side and let the interrupt routine know that it's been
	 * copied.
	 */
	ssm_misc_copy = *ssm_mc;
	ssm_copied = 1;
	ssm_interrupt_active = 0;
	return;
}

#ifdef SCAN

/*
 * bicscan(sic, tap, chain, mask, ...)
 * bisscan(sic, tap, chain, mask, ...)
 * bitscan(sic, tap, chain, mask, ...)
 *
 * BIt Clear SCAN chain, BIt Set SCAN chain and BIt Test SCAN chain.
 * All operate on the command chain <sic, tap, chain>.
 * Bicscan clears the bits of the chain specified by "mask".
 * Bisscan sets the bits of the chain specified by "mask".
 * Bitscan tests the bits of the chain specified by "mask", returning
 * non-zero if any of the specified bits are set.
 * All three operations are polled only.
 */
#define	SCAN_BIC	0
#define	SCAN_BIS	1
#define	SCAN_BIT	2
#define	SCAN_READ	3
#define	SCAN_WRITE	4
#define	SCAN_FUNC	5

#ifdef notyet
/*
 * These are probably obsolete due to our choice to go with a functional
 * interface to the scan library on the SSM... but the code's here, and
 * could be turned on again if needed... Andy
 */
/* VARARGS */
void
bicscan(sic, tap, chain, mask)
	int sic, tap, chain;
	long mask;
{
	(void)biopscan(SCAN_BIC, sic, tap, chain, &mask, 0);
}

/* VARARGS */
void
bisscan(sic, tap, chain, mask)
	int sic, tap, chain;
	long mask;
{
	(void)biopscan(SCAN_BIS, sic, tap, chain, &mask, 0);
}

/* VARARGS */
bitscan(sic, tap, chain, mask)
	int sic, tap, chain;
	long mask;
{
	return(biopscan(SCAN_BIT, sic, tap, chain, &mask, 0));
}

/*
 * readscan(sic, tap, chain, addr)
 * writescan(sic, tap, chain, addr)
 *
 * Read and write the scan chain specified by <sic, tap, chain> to/from
 * the memory pointed to by "addr".
 */
void
readscan(sic, tap, chain, addr)
	int sic, tap, chain;
	long *addr;
{
	(void)biopscan(SCAN_READ, sic, tap, chain, addr, 0);
}

void
writescan(sic, tap, chain, addr)
	int sic, tap, chain;
	long *addr;
{
	(void)biopscan(SCAN_WRITE, sic, tap, chain, addr, 0);
}

/*
 * ibicscan(sic, tap, chain, mask, ...)
 * ibisscan(sic, tap, chain, mask, ...)
 * ibitscan(sic, tap, chain, mask, ...)
 * ireadscan(sic, tap, chain, addr);
 * iwritescan(sic, tap, chain, addr);
 *
 * Same as the above, only interrupt driven.
 */
/* VARARGS */
void
ibicscan(sic, tap, chain, mask)
	int sic, tap, chain;
	long mask;
{
	(void)biopscan(SCAN_BIC, sic, tap, chain, &mask, 1);
}

/* VARARGS */
void
ibisscan(sic, tap, chain, mask)
	int sic, tap, chain;
	long mask;
{
	(void)biopscan(SCAN_BIS, sic, tap, chain, &mask, 1);
}

/* VARARGS */
ibitscan(sic, tap, chain, mask)
	int sic, tap, chain;
	long mask;
{
	return(biopscan(SCAN_BIT, sic, tap, chain, &mask, 1));
}

void
ireadscan(sic, tap, chain, addr)
	int sic, tap, chain;
	long *addr;
{
	(void)biopscan(SCAN_READ, sic, tap, chain, addr, 1);
}

void
iwritescan(sic, tap, chain, addr)
	int sic, tap, chain;
	long *addr;
{
	(void)biopscan(SCAN_WRITE, sic, tap, chain, addr, 1);
}
#endif notyet

scan_func(sic, cmd, intr)
	int sic, cmd, intr;
{
	return(biopscan(SCAN_FUNC, sic, cmd, 0, (long *)0, intr));
}

/*
 * Perform one of the above operations, doing the necessary
 * interlocking depending on interrupt or polled operations.
 */
biopscan(op, sic, tap, chain, mask, intr)
	int op, sic, tap, chain;
	long *mask;
{
	register struct ssm_misc *ssm_mc = scan_misc;
	register long *src, *dst;
	long result = 0;
	long *buf;
	int size = ssm_chain_size(sic, tap, chain);
	int s;

	if (!ssm_mc) {
		CPRINTF("biopscan before init.\n");
		return(-1);
	}

	if (intr) {
		buf = (long *)ssm_scanbuf[1];
		(void)p_sema(&ssm_interrupt, PZERO);
	} else
		buf = (long *)ssm_scanbuf[0];

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);

	if (!intr)
		ssm_chk_intr(ssm_mc);

	bzero((caddr_t)buf, SCAN_MAXSIZE);

	if (op != SCAN_WRITE) {
#define	SICEXT_INTERCEPT
#ifdef	SICEXT_INTERCEPT
		if (tap == SIC_TAP && (chain == SICEXT0 || chain == SICEXT1)) {
			ssm_sic_intercept(sic, (caddr_t)buf, chain, 0);
			goto read_next;
		}
#endif	/* SICEXT_INTERCEPT */
		/*
		 * Read the scan chain.
		 */
		ssm_scan_cmd(SM_GET, (caddr_t)buf, size, sic, tap, chain, intr);

		if (intr) {
			(void)p_sema_v_lock(&ssm_done, PZERO, &ssm_mc->sm_lock, s);
			(void)p_lock(&ssm_mc->sm_lock, SM_SPL);
		} else {
			while (ssm_mc->sm_stat == SM_BUSY)
				;
		}
#ifdef	SICEXT_INTERCEPT
		read_next:;
#endif	/* SICEXT_INTERCEPT */
	}

	switch (op) {
	case SCAN_BIC:
		/*
		 * Bic in the data.
		 */
		size = round2long(size);
		src = mask;
		dst = buf;
		while (size-- > 0)
			*dst++ &= ~*src++;
		break;

	case SCAN_BIS:
		/*
		 * Bis in the data.
		 */
		size = round2long(size);
		src = mask;
		dst = buf;
		while (size-- > 0)
			*dst++ |= *src++;
		break;

	case SCAN_BIT:
		/*
		 * Test the data.
		 */
		size = round2long(size);
		src = mask;
		dst = buf;
		result = 0;
		while (size-- > 0)
			result |= *src++ & *dst++;
		goto out;

	case SCAN_READ:
		/*
		 * Read the data.
		 */
		size = round2long(size);
		bcopy((caddr_t)buf, (caddr_t)mask, (unsigned)size);
		goto out;

	case SCAN_WRITE:
		/*
		 * Write the data.
		 */
		size = round2long(size);
		bcopy((caddr_t)mask, (caddr_t)buf, (unsigned)size);
		break;

	case SCAN_FUNC:
		/*
		 * New functional interface.  Eventually, this will
		 * be the only interface and all the above will be
		 * rewritten.
		 * We already sent the command down above, we just
		 * need to fetch the return code (if any) and return.
		 */
		result = buf[0];
		goto out;
	}

#ifdef	SICEXT_INTERCEPT
	if (tap == SIC_TAP && (chain == SICEXT0 || chain == SICEXT1)) {
		ssm_sic_intercept(sic, (caddr_t)buf, chain, 1);
		goto out;
	}
#endif	/* SICEXT_INTERCEPT */
	/*
	 * Write the data back.
	 */
	ssm_scan_cmd(SM_SET, (caddr_t)buf, size, sic, tap, chain, intr);

	if (intr) {
		(void)p_sema_v_lock(&ssm_done, PZERO, &ssm_mc->sm_lock, s);
		(void)p_lock(&ssm_mc->sm_lock, SM_SPL);
	} else {
		while (ssm_mc->sm_stat == SM_BUSY)
			;
	}
out:
	v_lock(&ssm_mc->sm_lock, s);
	if (intr)
		v_sema(&ssm_interrupt);

	return(result);
}


#ifdef	SICEXT_INTERCEPT
/****
*	Defines the Register Numbers on the Sic when addressed over
*	the SLIC bus.
***/

#define SLIC_SIC_BASE   32
#define	SL_SIC_CHIP_ID 		(SLIC_SIC_BASE + 0)
#define	SL_SIC_VERSION 		(SLIC_SIC_BASE + 1)
#define	SL_SIC_EXT0 		(SLIC_SIC_BASE + 2)
#define	SL_SIC_EXT1 		(SLIC_SIC_BASE + 3)
#define	SL_SIC_STATUS 		(SLIC_SIC_BASE + 4)
#define	SL_SIC_CONFIG 		(SLIC_SIC_BASE + 5)
#define	SL_SIC_INT_EN 		(SLIC_SIC_BASE + 6)
#define	SL_SIC_INT 		(SLIC_SIC_BASE + 7)
#define	SL_SIC_XMIT0 		(SLIC_SIC_BASE + 8)
#define SL_SIC_INIT 		(SLIC_SIC_BASE + 8)
#define	SL_SIC_XMIT0_0		(SLIC_SIC_BASE + 8)
#define	SL_SIC_DATA 		(SLIC_SIC_BASE + 9)
#define	SL_SIC_XMIT0_1		(SLIC_SIC_BASE + 9)
#define	SL_SIC_XMIT0_2		(SLIC_SIC_BASE + 10)
#define	SL_SIC_INSTR_RD 	(SLIC_SIC_BASE + 10)
#define	SL_SIC_XMIT0_3 		(SLIC_SIC_BASE + 11)
#define	SL_SIC_XMIT0_4 		(SLIC_SIC_BASE + 12)
#define	SL_SIC_ADDR 		(SLIC_SIC_BASE + 12)
#define	SL_SIC_XMIT0_5 		(SLIC_SIC_BASE + 13)
#define	SL_SIC_DUMMY 		(SLIC_SIC_BASE + 13)
#define	SL_SIC_INSTR_WR 	(SLIC_SIC_BASE + 14)
#define	SL_SIC_XMIT0_6 		(SLIC_SIC_BASE + 14)
#define	SL_SIC_XMIT0_7 		(SLIC_SIC_BASE + 15)
#define	SL_SIC_XMIT1 		(SLIC_SIC_BASE + 16)
#define	SL_SIC_TYPE 		(SLIC_SIC_BASE + 17)
#define	SL_SIC_SYNC_DEPTH 	(SLIC_SIC_BASE + 18)
#define SL_SIC_RING_CONFIG 	(SLIC_SIC_BASE + 19)
#define	SL_SIC_RING_SELECT 	(SLIC_SIC_BASE + 20)
#define	SL_SIC_TMS 		(SLIC_SIC_BASE + 21)
#define SL_SIC_PRE_CNT		(SLIC_SIC_BASE + 22)
#define SL_SIC_POST_CNT 	(SLIC_SIC_BASE + 23)
#define	SL_SIC_LENGTH0 		(SLIC_SIC_BASE + 24)
#define	SL_SIC_LENGTH1 		(SLIC_SIC_BASE + 25)
#define	SL_SIC_START_CFG 	(SLIC_SIC_BASE + 26)
#define	SL_SIC_RUN_CFG 		(SLIC_SIC_BASE + 27)
#define	SL_SIC_COUNT0 		(SLIC_SIC_BASE + 28)
#define	SL_SIC_COUNT1 		(SLIC_SIC_BASE + 29)
#define	SL_SIC_START 		(SLIC_SIC_BASE + 30)
#define	SL_SIC_JTAG_STAT 	(SLIC_SIC_BASE + 31)
#define SL_SIC_SICADDR 		(SLIC_SIC_BASE + 31)

ssm_sic_intercept(sic, buf, chain, wr)
	int sic, chain, wr;
	register caddr_t buf;
{
	register int reg;

	if (chain == SICEXT0) {
		/*
		 * Must write the outputs down.
		 */
		buf[0] |= SICEXT0_INPUTS;
		reg = SL_SIC_EXT0;
	} else {
		buf[0] |= SICEXT1_INPUTS;
		reg = SL_SIC_EXT1;
	}

	if (wr)
		wrslave((u_char)sic, (u_char)reg, (u_char)buf[0]);
	else
		buf[0] = rdslave((u_char)sic, (u_char)reg);
}
#endif	/* SICEXT_INTERCEPT */

/*
 * Initiate a request to the scan library without waiting for
 * it to finish.
 */
ssm_scan_cmd(cmd, buf, size, sic, tap, chain, intr)
	caddr_t buf;
	int cmd, size, sic, tap, chain, intr;
{
	register struct ssm_misc *ssm_mc = scan_misc;

	bzero((caddr_t)ssm_mc, sizeof(*ssm_mc));
	ssm_mc->sm_who = SM_SCANLIB;
	ssm_mc->sm_stat = SM_BUSY;
	ssm_mc->sm_scanst.buf = buf;
	ssm_mc->sm_scanst.size = size;
	ssm_mc->sm_scanst.sic = sic;
	ssm_mc->sm_scanst.tap = tap;
	ssm_mc->sm_scanst.chain = chain;
	if (intr) {
		ssm_interrupt_active = 1;
		ssm_mc->sm_dest = scan_dest | SL_GROUP;
		ssm_mc->sm_bin = scan_bin | SL_MINTR;
		ssm_mc->sm_mesg = scan_vec;
		cmd |= SM_INTR;
	}
	ssm_mc->sm_cmd = cmd;

	mIntr((unsigned char)scan_slicaddr, SM_BIN, (u_char)SM_SCANLIB);

	return;
}

/*
 * Process a message passing interrupt from the ssm.  After we get
 * the ssm_mc lock, this implies that there's a process waiting on
 * "ssm_done" for completion.  We should copy the results of the
 * operation off to the side (assuming a poller didn't do so) and
 * wake the process up off of ssm_Done.
 */

ssm_misc_intr()
{
	register struct ssm_misc *ssm_mc = scan_misc;
	int s;

	if (!ssm_mc) {
		CPRINTF("ssm_misc_intr before init.\n");
		return;
	}

	s = p_lock(&ssm_mc->sm_lock, SM_SPL);

	if (!ssm_copied)
		scan_misc_copy = *ssm_mc;

	ssm_copied = 0;
	ssm_interrupt_active = 0;

	v_lock(&ssm_mc->sm_lock, s);

	v_sema(&ssm_done);
}

/*
 * Determine what the size of a scan chain is (rounded up to the
 * nearest byte).
 */
ssm_chain_size(sic, tap, chain)
	int sic, tap, chain;
{
	register int size;
#define	f(tap, chain)	((tap)<<16 | (chain))
#ifdef lint
	size = sic;
#endif

	/*
	 * Should probably do this with a table lookup.
	 * Do a switch statement for now.
	 */
	switch f(tap, chain) {
	case f(SIC_TAP, SICEXT0):
		size = SICEXT0_SIZE;
		break;
	case f(SIC_TAP, SICEXT1):
		size = SICEXT1_SIZE;
		break;
	case f(BICD_TAP, BCDSES):
		size = BCDSES_SIZE;
		break;
	case f(BICD_TAP, BCDTACH):
		size = BCDTACH_SIZE;
		break;
	case f(BICD_TAP, BCDTC):
		size = BCDTC_SIZE;
		break;
	case f(CIC0_TAP, CICTACH):
		size = CICTACH_SIZE;
		break;
	case f(CIC0_TAP, CICTCM):
		size = CICTCM_SIZE;
		break;
	default:
		return(16);
/*		cmn_err(CE_PANIC, "scan_chain_size: unknown chain"); */
		/* not reached */
	}

	size = (size+7) >> 3;
	return(size);
}
#endif /* SCAN */

/*
 * ssm_alloc()
 *	Allocate a properly-aligned chunk of memory.
 *
 * 	'nbytes' is the number of bytes to allocate.
 * 	'align' is the byte multiple at which the memory 
 *	is to be aligned (e.g. 2 means align to two-byte 
 *	boundary).  
 *	'badbound' is a boundary which cannot be crossed 
 *	(usually one megabyte for the SSM); it must be a 
 *	power of two and a multiple of 'align'.
 */
char *
ssm_alloc(nbytes, align, badbound)
	unsigned nbytes, align, badbound;
{
	long addr;
	caddr_t	calloc();

	callocrnd((int)align);
	addr = (long)calloc(0);
	if ((addr & ~(badbound - 1)) != ((addr + nbytes) & ~(badbound - 1))) {
		/*
		 * It would have crossed a 'badbound' boundary,
		 * so bump past this boundary.
		 */
		callocrnd((int)badbound);
	}
	return (calloc((int)nbytes));
}

/* 
 * init_ssm_cons - allocate SSM console CBs     
 *
 * Allocate properly aligned console CBs and notify 
 * the SSM by sending the address a byte at a time
 * to it over the slic.  Return the base CB address
 * to the caller.
 * Assumes that mIntr() retries messages until they
 * succeed.
 */
struct cons_cb *
init_ssm_cons(slic)
	u_char slic;
{
	struct cons_cb *cons_cbs;
	u_char *addr_bytes = (u_char *)&cons_cbs;
	u_int nbytes = sizeof(struct cons_cb) * NCONSDEV << NCBCONSHFT;

	cons_cbs = (struct cons_cb *)
		ssm_alloc(nbytes, SSM_ALIGN_BASE, SSM_BAD_BOUND);	
	
 	/* Notify the SSM of the CB'a location. */
	mIntr(slic, CONS_BIN, CONS_ADDRVEC);
	mIntr(slic, CONS_BIN, addr_bytes[0]);	/* low byte first */
	mIntr(slic, CONS_BIN, addr_bytes[1]);
	mIntr(slic, CONS_BIN, addr_bytes[2]);
	mIntr(slic, CONS_BIN, addr_bytes[3]);	/* high byte last */

	return (cons_cbs);
}

/* 
 * init_ssm_prnt - allocate printer CBs     
 *
 * Allocate properly aligned printer CBs and notify 
 * the SSM by sending the address a byte at a time
 * to it over the slic.  Return the base CB address
 * to the caller.
 * Assumes that mIntr() retries messages until they
 * succeed.
 */
struct print_cb *
init_ssm_prnt(slic)
	u_char slic;
{
	struct print_cb *cbs;
	u_char *addr_bytes = (u_char *)&cbs;
	u_int nbytes = sizeof(struct print_cb) * NPRINTDEV * NCBPERPRINT;
	spl_t s;

	cbs = (struct print_cb *)
		ssm_alloc(nbytes, SSM_ALIGN_BASE, SSM_BAD_BOUND);	
 	/* Notify the SSM of the CB's location. */
	s = splhi();
	mIntr(slic, PRINT_BIN, PCB_ADDRVEC);
	mIntr(slic, PRINT_BIN, addr_bytes[0]);	/* low byte first */
	mIntr(slic, PRINT_BIN, addr_bytes[1]);
	mIntr(slic, PRINT_BIN, addr_bytes[2]);
	mIntr(slic, PRINT_BIN, addr_bytes[3]);	/* high byte last */
	splx(s);

	return (cbs);
}
