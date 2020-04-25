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
static	char	rcsid[] = "$Header: slic.c 2.6 89/06/30 $";
#endif

/*
 * slic.c
 *	Slic functions.
 *
 * wrslave/rdslave/wrSubslave/rdSubslave are mutexed by a global lock.
 * All other routines assume caller arranged mutex of SLIC usage (splhi()
 * or disable processor interrupts) -- SLIC registers are not saved or
 * restored across interrupts.
 *
 * Conditionals:
 *	-DCHECKSLIC	check slic results (parity, etc) for sanity
 *
 * Gates are handled in gate.c; interrupt acceptence/etc in locore.s
 */

/* $Log:	slic.c,v $
 */

#include "../h/types.h"
#include "../h/mutex.h"

#include "../balance/cfg.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"

#include "../machine/gate.h"
#include "../machine/vmparam.h"
#include "../machine/intctl.h"
#include "../machine/hwparam.h"

#ifdef	ns32000

/*
 * Balance has no special SLIC synchronization requirements.
 */

#define	LOCK_SLIC
#define	UNLOCK_SLIC

#else	!ns32000		/* Eg, Symmetry */

/*
 * In order to use the performance counters on the CMCs and BICs, it is
 * necessary to access the SLICs of other processors. Since multiple SLIC
 * commands are required for wrslave/rdslave/wrSubslave/rdSubslave, we need to
 * ensure their atomicity by mutexing on the same lock.  This also is important
 * during access error processing, and while panic'ing.  This is due to
 * asymmetry of (eg) BIC's -- one per processor board, accessed thru only one
 * SLIC.
 *
 * Other uses of slic slave addressing is mutually excluded at higher levels;
 * upper level code only online/offline's one processor at a time, memory
 * error polling guaranteed single-thread, etc.
 */

static	gate_t	slic_mutex = G_UNLOCKED;

#define	LOCK_SLIC	p_gate(&slic_mutex)
#define	UNLOCK_SLIC	v_gate(&slic_mutex)

#endif	ns32000

/*
 * wrslave()
 *	Write to a slave port.
 */

wrslave(destination, reg, data)
	unsigned char destination, reg, data;
{
	LOCK_SLIC;
	wrAddr(destination, reg);
	wrData(destination, data);
	UNLOCK_SLIC;
}

/*
 * lwrslave()
 *	Write to a slave port. Don't acquire slic_mutex.
 *
 * On panics, engines pausing themselves call lwrslave to hold
 * themselves without acquiring slic_mutex. Otherwise the lock would
 * never get released (wrslave doesn't return) and all other engines
 * would block behind the lock.
 */

lwrslave(destination, reg, data)
	unsigned char destination, reg, data;
{
	wrAddr(destination, reg);
	wrData(destination, data);
}

/*
 * rdslave()
 *	Read a slave port.
 */

rdslave(destination, reg)
	unsigned char destination, reg;
{
	int val;
	LOCK_SLIC;
	wrAddr(destination, reg);
	val = rdData(destination);
	UNLOCK_SLIC;
	return (val);
}

/*
 * lrdslave()
 *	Read a slave port. Don't acquire slic_mutex.
 *
 * See comments on lwrslave().
 */

lrdslave(destination, reg)
	unsigned char destination, reg;
{
	wrAddr(destination, reg);
	return(rdData(destination));
}

/*
 * wrSubslave()
 *	Write a register that responds to SLIC slave sub-register addressing.
 *
 * For compatibility with diagnostic usage, if slave==0 don't do the wrAddr().
 */

wrSubslave(slic, slave, subreg, val)
	u_char slic, slave, subreg, val;
{
	LOCK_SLIC;

	if (slave != 0)
		wrAddr(slic, slave);
	wrData(slic, subreg);
	wrData(slic, val);

	UNLOCK_SLIC;
}

/*
 * rdSubslave()
 *	Read a register that responds to SLIC slave sub-register addressing.
 *
 * For compatibility with diagnostic usage, if slave==0 don't do the wrAddr().
 */

rdSubslave(slic, slave, subreg)
	u_char slic, slave, subreg;
{
	int	val;

	LOCK_SLIC;

	if (slave != 0)
		wrAddr(slic, slave);
	wrData(slic, subreg);
	val = rdData(slic);

	UNLOCK_SLIC;

	return (val);
}

/*
 * wrAddr()
 *	Write address to slave.
 */

wrAddr(destination, address)
	unsigned char destination, address;
{
	register struct cpuslic *sl = (struct cpuslic *)va_slic;

	sl->sl_dest = destination;
	sl->sl_smessage = address;
	sl->sl_cmd_stat = SL_WRADDR;
	while (sl->sl_cmd_stat & SL_BUSY)
		continue;
#ifdef	CHECKSLIC
	check_slic("wrAddr");
#endif	CHECKSLIC
}

/*
 * wrData()
 *	Write data to previously addressed slave register.
 */

static
wrData(destination, data)
	unsigned char destination, data;
{
	register struct cpuslic *sl = (struct cpuslic *)va_slic;

	sl->sl_dest = destination;
	sl->sl_smessage = data;
	sl->sl_cmd_stat = SL_WRDATA;
	while (sl->sl_cmd_stat & SL_BUSY)
		continue;
#ifdef	CHECKSLIC
	check_slic("wrData");
#endif	CHECKSLIC
}

/*
 * rdData()
 *	Read data from previously addressed slave register.
 */

static
rdData(destination)
	unsigned char destination;
{
	register struct cpuslic *sl = (struct cpuslic *)va_slic;

	sl->sl_dest = destination;
	sl->sl_cmd_stat = SL_RDDATA;
	while (sl->sl_cmd_stat & SL_BUSY)
		continue;
#ifdef	CHECKSLIC
	check_slic("rdData");
#endif	CHECKSLIC
	return(sl->sl_sdr & 0xff);
}

/*
 * sendsoft()
 *	Post SW interrupt to somebody.
 *
 * Used to post resched "nudge", net handler interrupts, pff calc, softclock.
 *
 * Caller assures mutex of SLIC usage (splhi() or holding gate is sufficient).
 */

sendsoft(dest, bitmask)
	unsigned char dest;
	unsigned char bitmask;
{
	register struct cpuslic *sl = (struct cpuslic *)va_slic;

	sl->sl_dest = dest;
	sl->sl_smessage = bitmask;

	sl->sl_cmd_stat = SL_MINTR | 0;		/* 0 ==> bin 0 */
	while (sl->sl_cmd_stat & SL_BUSY)
		continue;

#ifdef	CHECKSLIC
	check_slic("sendsoft");
#endif	CHECKSLIC
}

/*
 * nmIntr()
 *	Post NMI interrupt to somebody.
 *
 * Used to post send NMI to a processor to have it shut down.
 *
 * Caller assures mutex of SLIC usage (splhi() or holding gate is sufficient).
 */

nmIntr(dest, message)
	unsigned char dest;
	unsigned char message;
{
	register struct cpuslic *sl = (struct cpuslic *)va_slic;

	sl->sl_dest = dest;
	sl->sl_smessage = message;

	do {
		sl->sl_cmd_stat = SL_NMINTR;
		while (sl->sl_cmd_stat & SL_BUSY)
			continue;
	} while ((sl->sl_cmd_stat & SL_OK) == 0);
#ifdef	CHECKSLIC
	check_slic("nmIntr");
#endif	CHECKSLIC
}

/*
 * mIntr()
 *	Post HW interrupt to somebody.
 *
 * Used to send commands to MBAd's.
 *
 * Caller assures mutex of SLIC usage (splhi() or holding gate is sufficient).
 *
 * Implementation acquires gate before sending message, to avoid SLIC bus
 * saturation.
 */

mIntr(dest, bin, data)
	unsigned char dest;
	unsigned char bin;
	unsigned char data;
{
	register struct cpuslic *sl = (struct cpuslic *)va_slic;
	register unsigned stat;
#ifndef	ns32000
	static	gate_t	mIntr_mutex = G_UNLOCKED;
#endif	ns32000

	/*
	 * Get mIntr "gate" -- serializes mIntr requests, avoiding
	 * SLIC bus saturation.
	 *
	 * Semantics insist caller is at splhi(); thus no need to re-do this.
	 */

#ifdef	ns32000
	/*
	 * In-line expansion of p_gate() to avoid overhead of call, and
	 * since already have interrupts masked and hold a pointer to SLIC.
	 */
	sl->sl_dest = GATE_GROUP;		/* group of gate access */
	sl->sl_smessage = G_MINTR;		/* gate # we want */
	do {
		sl->sl_cmd_stat = SL_REQG;
		while((stat = sl->sl_cmd_stat) & SL_BUSY)
			continue;
	} while ((stat & SL_OK) == 0);		/* spin until get gate */
#ifdef	CHECKSLIC
	check_slic("mIntr: p_gate");
#endif	CHECKSLIC

#else	!ns32000				/* Symmetry */
	/*
	 * p_gate() is in-line expanded on real SGS HW.  On K20, this
	 * is out-of-line.
	 */
	p_gate(&mIntr_mutex);			/* single-thread mIntr's */
#endif	ns32000

	/*
	 * Send message.  Spin forever until sent.
	 */

	sl->sl_dest = dest;
	sl->sl_smessage = data;

	do {
		sl->sl_cmd_stat = SL_MINTR | bin;
		while ((stat = sl->sl_cmd_stat) & SL_BUSY)
			continue;
	} while ((stat & SL_OK) == 0);


#ifdef	CHECKSLIC
	check_slic("mIntr");
#endif	CHECKSLIC

	/*
	 * Release mIntr gate.
	 */

#ifdef	ns32000
	sl->sl_dest = GATE_GROUP;		/* group of gate access */
	sl->sl_smessage = G_MINTR;		/* gate # we want to release */
	sl->sl_cmd_stat = SL_RELG;		/* want to release it */
	while (sl->sl_cmd_stat & SL_BUSY)	/* while command busy... */
		continue;
#ifdef	CHECKSLIC
	check_slic("mIntr: v_gate");
#endif	CHECKSLIC
#else	!ns32000				/* Symmetry */
	v_gate(&mIntr_mutex);
#endif	ns32000
}

/*
 * setgm()
 *	Set group mask in destination slic.
 *
 * Caller assures mutex of SLIC usage (splhi() or holding gate is sufficient).
 */

setgm(dest, mask)
	unsigned char dest;
	unsigned char mask;
{
	register struct cpuslic *sl = va_slic;

	sl->sl_dest = dest;			/* set this guy's... */
	sl->sl_smessage = mask;			/* group mask to "mask" */
	sl->sl_cmd_stat = SL_SETGM;		/* set the group-mask */
	while (sl->sl_cmd_stat & SL_BUSY)	/* and wait for cmd done */
		continue;

#ifdef	CHECKSLIC
	check_slic("setgm");
#endif	CHECKSLIC
}

#ifdef	CHECKSLIC
/* 
 * check_slic()
 *	Check status from SLIC for parity, exists, and ok bits.
 */

check_slic(procname)
	char	*procname;
{
	register unsigned stat;

	stat = va_slic->sl_cmd_stat;

	if ((stat & SL_PARITY) == 0) {
		printf("%s: slic parity error\n", procname);
		panic("slic");
		/*NOTREACHED*/
	}
	if ((stat & SL_EXISTS) == 0) {
		printf("%s: slic(s) don't exist\n", procname);
		panic("slic");
		/*NOTREACHED*/
	}
	if ((stat & SL_OK) == 0) {
		printf("%s: slic not ok\n", procname);
		panic("slic");
		/*NOTREACHED*/
	}
}
#endif	CHECKSLIC
