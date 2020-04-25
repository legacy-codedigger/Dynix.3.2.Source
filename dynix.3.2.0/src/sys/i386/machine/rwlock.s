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
.asciz	"$Header: rwlock.s 2.4 86/05/01 $"

/*
 * rwlock.s
 *	Reader-writer lock implementations.
 *
 * v_writer() is a macro defined in machine/mutex.h
 *
 * Most/all of these should be in-line expanded.
 */

/* $Log:	rwlock.s,v $
 */

#include "../machine/asm.h"
#include "assym.h"

/*
 * p_reader(&lock, SPL)
 *	Don't return until hold reader lock @ SPL.
 *
 * This version tries reader first, then checks if there is a writer.
 * Should check which version is faster for system (assume reader gets
 * in vs test writer first).
 */

#ifdef	KXX

ENTRY(p_reader)
	movl	SPARG0, %edx			# edx -> rw lock
	movl	%edx, %ecx			# ecx -> rw lock, too.
	shrb	$2, %cl				# ecx/4
	andb	0x3f, %cl			# cl == SLIC gate # to use.
0:	cmpb	$0, RWL_WRITER(%edx)		# spin until...
	jne	0b				#	...no writers active.
	movb	SPARG1, %ah			# ah = argument SPL
	SPL_ASM(%ah,%al)			# al = spl(arg1).
	P_SLIC_GATE(%cl)			# mutex rwlock.
	cmpb	$0, RWL_WRITER(%edx)		# is there a writer?
	jne	1f				# yup - lost race.
	incb	RWL_READERS(%edx)		# Nope -- bump read count.
	V_SLIC_GATE(%cl)			# Drop SLIC gate.
	RETURN					# Got it!
1:	V_SLIC_GATE(%cl)			# release SLIC gate.
	SPLX_ASM(%al)				# Drop SPL.
	jmp	0b				# try again.

#else	Real HW

ENTRY(p_reader)
	movl	SPARG0, %edx			# edx -> rw lock
0:	movb	SPARG1, %ah			# ah = argument SPL
	SPL_ASM(%ah,%al)			# al = spl(arg1).
	lock;	incb RWL_READERS(%edx)		# Atomiclly bump read count.
	cmpb	$0, RWL_WRITER(%edx)		# is there a writer?
	jne	1f				# yup - lost race.
	RETURN					# No writer ==> got it.
1:	lock;	decb RWL_READERS(%edx)		# Atomicly un-do above bump.
	SPLX_ASM(%al)				# Drop SPL.
2:	cmpb	$0, RWL_WRITER(%edx)		# spin until...
	jne	2b				#	...no writers active.
	jmp	0b				# try again.

#endif	KXX

/*
 * v_reader(&lock, oldspl)
 *	Release reader lock and drop SPL.
 */

#ifdef	KXX

ENTRY(v_reader)
	movl	SPARG0, %edx			# edx -> rwlock
	movl	%edx, %ecx			# ecx -> rw lock, too.
	shrb	$2, %cl				# ecx/4
	andb	0x3f, %cl			# cl == SLIC gate # to use.
	P_SLIC_GATE(%cl)			# Atomic...
	decb	RWL_READERS(%edx)		#	decrement reader count.
	V_SLIC_GATE(%cl)			# release SLIC gate.
	movb	SPARG1, %al			# al = SPL argument.
	SPLX_ASM(%al)				# Drop SPL.
	RETURN					# Done.

#else	Real HW

ENTRY(v_reader)
	movl	SPARG0, %edx			# edx -> rwlock
	lock;	decb RWL_READERS(%edx)		# Atomicly fix reader count.
	movb	SPARG1, %al			# al = SPL arg.
	SPLX_ASM(%al)				# Drop SPL.
	RETURN					# Done.

#endif	KXX

/*
 * p_writer(&lock, SPL)
 *	Don't return until hold writer lock @ SPL.
 *
 * This version goes strait for lock; should measure "test-writers-first"
 * version in a system environment.
 */

#ifdef	KXX

ENTRY(p_writer)
	movl	SPARG0, %edx			# edx -> rwlock.
	movl	%edx, %ecx			# ecx -> rw lock, too.
	shrb	$2, %cl				# ecx/4
	andb	0x3f, %cl			# cl == SLIC gate # to use.
0:	cmpb	$0, RWL_WRITER(%edx)		# wait for...
	jne	0b				#	no writers.
	movb	SPARG1, %ah			# ah = argument SPL
	SPL_ASM(%ah,%al)			# al = spl(arg1).
	P_SLIC_GATE(%cl)			# Atomic...
	cmpb	$0, RWL_WRITER(%edx)		#	go for writer lock.
	jne	2f				# Nope.
	movb	$1, RWL_WRITER(%edx)		# Got it!
	V_SLIC_GATE(%cl)			# Drop SLIC gate.
1:	cmpb	$0, RWL_READERS(%edx)		# Wait for...
	jne	1b				#	...no readers.
	RETURN					# Done.  EAX holds entry SPL.
2:	V_SLIC_GATE(%cl)			# Drop SLIC gate.
	SPLX_ASM(%al)				# Didn't get lock.  Drop SPL.
	jmp	0b				# and try again.

#else	Real HW

ENTRY(p_writer)
	movl	SPARG0, %edx			# edx -> rwlock.
0:	movb	SPARG1, %ah			# ah = argument SPL
	SPL_ASM(%ah,%al)			# al = spl(arg1).
	movb	$1, %cl				# set up and...
	xchgb	%cl, RWL_WRITER(%edx)		#	go for writer lock.
	cmpb	$0, %cl				# Got it?
	jne	2f				# Nope.
1:	cmpb	$0, RWL_READERS(%edx)		# Yup; wait for...
	jne	1b				#	...no readers.
	RETURN					# Done.  EAX holds entry SPL.
2:	SPLX_ASM(%al)				# Didn't get lock.  Drop SPL.
3:	cmpb	$0, RWL_WRITER(%edx)		# wait for...
	jne	3b				#	no writers.
	jmp	0b				# and try again.

#endif	KXX
