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
static	char	rcsid[] = "$Header: tlb.c 1.5 91/03/12 $";
#endif

/*
 * tlb.c
 * 	TLB (translation lookaside buffer) manipulation routines.
 *
 * Architecture specific, since mechanism to broadcast interrupts (or whatever)
 * is architecture dependent.
 *
 * TODO:
 *
 *	Move atomic_dec_zero() definition to more appropriate place.  Move
 *	lint version to lintasm.c
 *
 *	consider making tmo_onoff a lock_t instead of a sema_t.  See sys_tmp.c.
 */

/* $Log:	tlb.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/SGSproc.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"
#include "../machine/gate.h"

#ifdef	i386
#ifndef	lint
/*
 * atomic_dec_zero()
 *	Atomic decrement an integer variable, returns "true" if this
 *	decrement caused it to hit zero.
 */
asm	atomic_dec_zero(var)
{
%mem var; lab done;
	movl	var, %eax
	lock
	decl	(%eax)
	jz	done
	xorl	%eax, %eax
done:
}
#else	/* lint */
atomic_dec_zero(intp) int *intp; { return --*intp == 0; }
#endif	/* lint */
#endif	/* i386 */

extern	sema_t	tmp_onoff;	/* coordination of online(s) and offline(s) */

/*
 * Struct TLBflush contains necessary information for processors to flush
 * their TLB's.
 */

struct	TLBflush {
	lock_t	mutex;			/* mutex nengine */
	int	nengine;		/* # engines to go */
	caddr_t	start;			/* starting kernel virtual address */
	int	len;			/* length (HW pages) */
	int	initiator;		/* who started it? */
	sema_t	wait;			/* initiator waits here */
};

static	struct	TLBflush tlb_flush;	/* mutex'd via tmp_onoff */

#if defined(DEBUG) && defined(i386)
#define TLB_DEBUG
#endif
#ifdef	TLB_DEBUG
/*
 * Statistics on time to flush TLB's.
 */
static	int	num_tlb_flush;
static	u_long	tlb_flush_time;
#endif	/* TLB_DEBUG */

/*
 * InitFlushTLB()
 *	Called at boot time to initialize Coherent TLB functions.
 */

InitFlushTLB()
{
	init_lock(&tlb_flush.mutex, G_ENGINE);
	init_sema(&tlb_flush.wait, 0, 0, G_ENGINE);
}

/*
 * FlushTLB()
 *	Insure all processor TLB's are coherent for given address range.
 *
 * Called after some kernel mapping has changed to insure all processors have
 * up to date TLB -- no stale mappings.
 *
 * Must be called in process context, holding no locks -- uses tmp_onoff sema
 * to mutex against concurrent online/offline.
 *
 * Assumes calling processor did own TLB.
 *
 * Each processor "checks in" to the TLBflush "barrier".  Last in wakes waiting
 * process.  Note: could keep waiting process spinning, if this is faster: need
 * to measure time this takes.  Also, not truly a barrier, since each process
 * checks in and leaves immediately.
 */

FlushTLB(start, len)
	caddr_t	start;			/* start kernel virtual address */
	int	len;			/* length (bytes) */
{
	spl_t  s;
#ifdef	TLB_DEBUG
	u_long	start_etc, end_etc;
#endif	/* TLB_DEBUG */

	/*
	 * Coordinate with concurrent online or offline.  This also mutex's
	 * the TLBflush structure.
	 *
	 * If only one processor online, nothing to do (since caller is
	 * running on it ;-)
	 */

	p_sema(&tmp_onoff, PZERO);

	if (nonline == 1) {
		v_sema(&tmp_onoff);
		return;
	}
#ifdef	TLB_DEBUG
	start_etc = *(int*)(PHYS_ETC);
#endif	/* TLB_DEBUG */

	/*
	 * Set up TLBflush structure.
	 */

	tlb_flush.nengine = nonline - 1;	/* don't include self */
	tlb_flush.start = start;		/* starting kernel address */
	tlb_flush.len = clrnd(btoc(len));	/* length (HW pages) */
	tlb_flush.initiator = l.me;		/* processor who started this */

	/*
	 * Get all processors interested in updating their TLB's.
	 * Done by sending SW interrupt to all processors (using SLIC
	 * group interrupt).  This also sends to calling processor.
	 */

	s = splhi();
	sendsoft(SL_GROUP|TMPOS_GROUP, SWINT_TLB_FLUSH);
	splx(s);

	/*
	 * Wait for all processors to flush their TLB's.
	 */

	p_sema(&tlb_flush.wait, PSWP);
#ifdef	TLB_DEBUG
	end_etc = *(int*)(PHYS_ETC);
	num_tlb_flush++;
	tlb_flush_time += (end_etc - start_etc);
#endif	/* TLB_DEBUG */

	/*
	 * Done!
	 */

	v_sema(&tmp_onoff);
}

/*
 * FlushTLBnowait()--like FlushTLB(), but does not sleep; uses spin-waiting
 * instead.  Otherwise essentially the same algorithm.  This routine is useful
 * for those cases where you can't afford to sleep.
 */
FlushTLBnowait(start, len)
	caddr_t	start;			/* start kernel virtual address */
	int	len;			/* length (bytes) */
{
	spl_t  s;
#ifdef	DEBUG
	ulong	start_etc, end_etc;
#endif	/* DEBUG */

	/*
	 * Coordinate with concurrent online or offline.  This also mutex's
	 * the TLBflush structure.
	 *
	 * If only one processor online, nothing to do (since caller is
	 * running on it ;-)
	 */
	while (!cp_sema(&tmp_onoff))
		;

	if (nonline == 1) {
		v_sema(&tmp_onoff);
		return;
	}
#ifdef	TLB_DEBUG
	start_etc = *(int*)(PHYS_ETC);
#endif	/* TLB_DEBUG */

	/*
	 * Set up TLBflush structure.
	 */
	tlb_flush.nengine = nonline - 1;	/* don't include self */
	tlb_flush.start = start;		/* starting kernel address */
	tlb_flush.len = clrnd(btoc(len));	/* length (HW pages) */
	tlb_flush.initiator = l.me;		/* processor who started this */

	/*
	 * Get all processors interested in updating their TLB's.
	 * Done by sending SW interrupt to all processors (using SLIC
	 * group interrupt).  This also sends to calling processor.
	 */
	s = splhi();
	sendsoft(SL_GROUP|TMPOS_GROUP, SWINT_TLB_FLUSH);
	splx(s);

	/*
	 * Wait for all processors to flush their TLB's.
	 */

	while (!cp_sema(&tlb_flush.wait))
		;
#ifdef	TLB_DEBUG
	end_etc = *(int*)(PHYS_ETC);
	num_tlb_flush++;
	tlb_flush_time += (end_etc - start_etc);
#endif	/* TLB_DEBUG */

	/*
	 * Done!
	 */
	v_sema(&tmp_onoff);
}

/*
 * FlushTLBIntr()
 *	SW interrupt handler called as result of SWINT_TLB_FLUSH.
 *
 * Flush relevant TLB, then check into TLBflush barrier.
 * Last one in signals initiator.
 *
 * Initiator of the flush operation is assumed to have taken care of self,
 * thus does a NOP here.  Takes interrupt in interest of using SLIC to
 * broadcast a SW interrupt (much lower overhead to send the interrupts -- one
 * SLIC message).
 *
 * On i386, could use "atomic-decrement" barrier and not use the lock.
 */

FlushTLBIntr()
{
#ifdef	ns32000
	spl_t	s;
#endif	/* ns32000 */

	/*
	 * If this is initiating processor, no need to sync TLB nor check
	 * into barrier.
	 */

	if (tlb_flush.initiator == l.me)
		return;

	/*
	 * Flush relevant parts of TLB.  Machine dependent.
	 */

#ifdef	i386
	/*
	 * i386 can't flush individually; must flush whole TLB.
	 */
	FLUSH_TLB();

	if (atomic_dec_zero(&tlb_flush.nengine))
		v_sema(&tlb_flush.wait);
#endif	/* i386 */
#ifdef	ns32000 

	/*
	 * flush whole tlb
	 */

	flush_tlb();

	/*
	 * Check into the barrier.  Last one in gooses initiator.
	 */

	s = p_lock(&tlb_flush.mutex, SPLHI);

	if (--tlb_flush.nengine == 0)
		v_sema(&tlb_flush.wait);

	v_lock(&tlb_flush.mutex, s);
#endif	/* ns32000 */
}
