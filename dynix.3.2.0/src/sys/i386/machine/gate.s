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
rcsid:	.asciz	"$Header: gate.s 2.8 87/03/12 $"

/*
 * gate.s
 *	p_gate, cp_gate routines to request and release gates.
 *
 * In the eventual kernel, most references to these procedures will be
 * in-line expanded.
 *
 * v_gate() is a macro in machine/mutex.h
 */

/* $Log:	gate.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

#if	defined(KXX) || defined(SLIC_GATES)

/*
 * void p_gate(gate)
 *	gate_t *gate;
 *
 * Acquire a gate.  Disable processor interrupts.
 */

ENTRY(p_gate)
	movl	SPARG0, %ecx		# ecx -> gate.
	cli				# block processor interrupts.
	P_GATE_ASM((%ecx))		# grab gate.
#ifdef	DEBUG				/* Check for nested gates */
	cmpb	$0, _upyet		# don't check if not yet "up".
	je	9f			#
	cmpl	$0, VA_PLOCAL+L_HOLDGATE # already holding gate?
	je	8f			# nope.
	movl	SPARG0, %ecx		# dump hoser address.
	pushl	%ecx			#
	pushl	VA_PLOCAL+L_HOLDGATE	# held address
	movl	$0, VA_PLOCAL+L_HOLDGATE # don't iterate!
	pushl	$which			#
	CALL	_printf			# complain
	addl	$12, %esp		#
	pushl	$pgmsg			# yup -- blammo.
	CALL	_panic			# die in nasty ways.
.data
which:	.asciz	"Holding 0x%x going for 0x%x\n"
pgmsg:	.asciz	"p_gate: nested gate"
.text
8:	movl	SPARG0, %ecx		# gate we now hold.
	movl	%ecx, VA_PLOCAL+L_HOLDGATE # now holding gate.
9:
#endif	DEBUG
	RETURN				# done.

/*
 * bool_t cp_gate(gate)
 *	gate_t *gate;
 *
 * Acquire a gate.  Block processor interrupts.
 * Return CPGATESUCCEED or CPGATEFAIL.
 */

ENTRY(cp_gate)
	movl	SPARG0, %ecx		# ecx -> gate.
	cli				# block processor interrupts.
	CP_GATE_ASM((%ecx),1f)		# try for gate.
	movl	$CPGATESUCCEED, %eax	# return success.
#ifdef	DEBUG				/* Check for nested gates */
	cmpb	$0, _upyet		# don't check if not yet "up".
	je	9f			#
	cmpl	$0, VA_PLOCAL+L_HOLDGATE # already holding gate?
	je	8f			# nope.
	movl	$0, VA_PLOCAL+L_HOLDGATE # don't iterate!
	pushl	$cpgmsg			# yup -- blammo.
	CALL	_panic			# die in nasty ways.
.data
cpgmsg:	.asciz	"cp_gate: nested gate"
.text
8:	movl	SPARG0, %ecx		# now holding gate.
	movl	%ecx, VA_PLOCAL+L_HOLDGATE # now holding gate.
9:
#endif	DEBUG
	RETURN				# Got it!
1:	sti				# Didn't get gate.  Re-enable int's.
	movl	$CPGATEFAIL, %eax	# sorry.
	RETURN				# Maybe next time!

#ifdef	DEBUG				/* Check for nested gates */
ENTRY(v_gate_dbg)
	cmpb	$0, _upyet		# don't check if not yet "up".
	je	9f			#
	movl	$0, VA_PLOCAL+L_HOLDGATE # no longer holding gate.
9:	RETURN
#endif	DEBUG

#else	Real HW

#ifdef	DEBUG
ENTRY(v_gate_dbg)			/* NOP on Real HW */
	RETURN
#endif	DEBUG

#endif	KXX
