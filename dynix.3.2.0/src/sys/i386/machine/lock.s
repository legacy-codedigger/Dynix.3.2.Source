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

.data
rcsid:	.asciz	"$Header: lock.s 2.6 87/03/12 $"

/*
 * lock.s
 *	init_lock, p_lock, _cp_lock routines to initialize and
 *	acquire locks.
 *
 * i386 version.
 *
 * Note that v_lock (to release a lock) is a macro in mutex.h.
 */

/* $Log:	lock.s,v $
 */

#if	defined(KXX) || defined(SLIC_GATES)

#include "../machine/asm.h"
#include "assym.h"

#ifdef	never			/* init_lock() is a macro */
/*
 * initlock(lockp)
 *	lock_t	*lockp;
 *
 * Initialize a lock to be L_UNLOCKED.
 */

ENTRY(init_lock)
	movl	SPARG0, %eax			# eax -> lock byte.
	movb	$L_UNLOCKED, (%eax)		# unlock lock.
	RETURN
#endif	never

/*
 * spl_t
 * p_lock(lockp, retipl)
 *	lock_t	*lockp;
 *	spl_t	retipl;
 *
 * Lock the lock and return at interrupt priority level "retipl".
 * Return previous interrupt priority level.
 */

ENTRY(p_lock)
	movl	SPARG0, %ecx			# ecx -> lock byte.
0:	movb	SPARG1, %al			# al = arg SPL
	SPL_ASM(%al,%dl)			# dl = spl(arg1).
	CP_GATE_ASM((%ecx),1f)			# try for the lock.
	movl	%edx, %eax			# return spl value in eax.
	RETURN					# Done.
1:	SPLX_ASM(%dl)				# Didn't get it.  Drop SPL.
2:	cmpb	$L_LOCKED, (%ecx)		# spin...
	je	2b				#	...until lock clears.
	jmp	0b				# then try again.

/*
 * spl_t
 * _cp_lock(lockp, retipl)
 *	lock_t	*lockp;
 *	spl_t	retipl;
 *
 * Conditionally acquire a lock.
 *
 * If lock is available, lock the lock and return at interrupt priority
 * level "retipl". Return previous interrupt priority level.
 * If lock is unavailable, return CPLOCKFAIL.
 */

ENTRY(_cp_lock)
	movl	SPARG0, %ecx			# ecx -> lock byte.
	movb	SPARG1, %al			# al = arg SPL
	SPL_ASM(%al,%dl)			# dl = spl(arg1).
	CP_GATE_ASM((%ecx),1f)			# try for the lock.
	movl	%edx, %eax			# return spl value in eax.
	movb	$0, %ah				# insure value != CPLOCKFAIL
	RETURN					# Done.
1:	SPLX_ASM(%dl)				# Didn't get it.  Drop SPL.
	movl	$CPLOCKFAIL, %eax		# sorry...
	RETURN					#	...didn't get the lock.

#endif	KXX
