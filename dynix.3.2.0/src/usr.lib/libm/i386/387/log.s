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

#ifdef	KXX
/* $Header: log.s 1.4 87/06/19 $
 *
 *		LOGARITHM FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the logarithm function for the C language.
 *	Both the natural log and log base 10 are computed in this routine.
 *	It uses the implicit fsqrt instruction of the 287 NPX. No error checking
 *	is performed since it is assumed that the 287 will correctly handle
 *	an illegal argument.
 *
 * Input:	Argument is located at 4(%esp)
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Method:	IF (x != 0)
 *		logn(X) = logn(2) * log2(X)
 *		   where for small arguments (< 1-(sqrt(2)/2)
 *			use argument of (x-1) & the fy12xp1 instruction
 *
 * Created:
 *	05/23/86	MF - Quantitative Technology Corporation for
 *				Sequent Computer Systems
 *	09/04/86	Phil Hochstetler - converted to C library conventions
 *
 *	05/22/87	Gary Tracy - converted to stack model C interface
 *			  under this model, the caller is responsible for
 *			  saving and restoring any required 387 state.
 *			  this need not be done if:
 *				1.  the callee does no calls.
 *				2.  the sum of 387 registers used by the caller
 *				    and callee does not exceed 8.
 *
 */

#include "PROF.h"

	.set	maskC0C3,0x4100		#mask for C0 and C3 in status word

ENTRY(log)
	movw	$0,%cx			#set flag for natural log
	jmp	do_log			#go perform log function
ENTRY(log10)
	movw	$1,%cx			#set flag for natural log
do_log:
	fldz				#create the %st(3) used below

	testw	$1,%cx			#natural log?
	je	log_load_ln2		#if so, go load ln2
	fldlg2				#if not, put LOG2 onto stack
	jmp	log_check_eps		#go check for value around 1.0

log_load_ln2:
	fldln2				#put LN2 onto stack
log_check_eps:
	fldl	4(%esp)			#put argument onto stack

	ftst				#compare the argument to zero
	fstsw	%ax			#save the result of the test
	sahf				#load result of the test in flags
	jbe	log_error		#If argument > 0 then ok
	jmp	log_ok
/*
 *    Error code for negative argument
 */
log_error:
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	fstp	%st(1)
	fstp	%st(1)
	jmp	log_continue
ecode:	.long	8

log_ok:
	fld1				#put 1.0 onto stack
	fsubr	%st(1),%st		#compute arg - 1
	fst	%st(3)			# and store it into st(3)
	fabs				#abs(arg-1) is now in st(0)
	fldl	libm_epsilon		#put (1-(sqrt(2)/2)) onto stack
	fcompp				#compare abs(arg-1) and (1-(sqrt(2)/2))
					# and pop (arg-1) and (1-(sqrt(2)/2))
	fstsw	%ax			#get status word
	testw	$maskC0C3,%ax		#are both C0 and C3 clear?
	je	log_small_num		#if so, abs(arg-1) < (1-(sqrt(2)/2))
	fyl2x				#compute ln2 * log base 2 (arg)
	jmp	log_continue		#go restore stack

log_small_num:
	fxch	%st(2)			#put (arg-1) into st(0)
	fyl2xp1				#compute ln2 * log base 2 (arg+1)
log_continue:
	fstp	%st(1)			#stack model requires empty %st(1)
	ret

#else

#include "PROF.h"

ENTRY(log)
	movl	8(%esp), %eax
	testl	$ 0x80000000, %eax
	jnz	negative_or_zero_arg
	orl	4(%esp), %eax
	jz	negative_or_zero_arg
	fldln2
	fldl	4(%esp)
	fyl2x
	ret

ENTRY(log10)
	movl	8(%esp), %eax
	testl	$ 0x80000000, %eax
	jnz	negative_or_zero_arg
	orl	4(%esp), %eax
	jz	negative_or_zero_arg
	fldlg2
	fldl	4(%esp)
	fyl2x
	ret

negative_or_zero_arg:
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	ret

ecode:	.long	8
	

#endif
