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

/* $Header: acos.s 1.5 87/08/13 $
 *
 *		ARCCOSINE FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the arc cosine function for the C language.
 *
 * Input:	Argument is located at 4(%esp)
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Created:
 *	06/03/86	GS - Quantitative Technology Corporation for
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
 *
 * Method:
 *			        _____________
 *			  \    /     1 - x
 *	acos(x) = 2*atan2( \  /   ----------    , 1).
 *			    \/       1 + x
 *
 *	Special cases checked:
 *	     			|x| > 1  = Error
 * 	      			acos(-1) = PI
 */

#include "PROF.h"

ENTRY(acos)

	fldl	4(%esp)			#put argument onto stack
	fld1				#put 1.0 onto stack
	fld	%st(1)
	fabs				#|x| --> st(0)

	subl	$16,%esp		#make room for temp storage

	#Assert Registers have the following values
	# St(0) = |x|
	# St(1) = 1
	# St(2) = x

	fcom				#compare 1.0 and |x|
	fstsw	%ax			#store status of compare
	sahf
	fxch	%st(2)			#replace the argument on top
	fst	%st(2)
	jz	acos_test_for_neg_one	#if |x| == 1 then check for neg one
	jc 	acos_continue		#if |x| < 1 then continue the calc

	pushl	$ecode
	leal	4+16(%esp), %eax
	pushl	%eax
	fstp	%st(0)
	fstp	%st(0)
	fstp	%st(0)
	call	_MATHERR
	addl	$8+16,%esp		#reset stack -- local storage
	ret

ecode:	.long	18

acos_test_for_neg_one:
	#Assert |x| = 1
	ftst				#Compare x to 0.0
	fstsw	%ax			#Store the outcome of the test
	sahf
	jnc	acos_continue		#If x = 1 then continue


					#Else load PI to return as answer
	fstp	%st			#       Clear the stack 
	fstp	%st	
	fstp	%st
	fldz
	fldpi				#	load result (acos(-1) = PI)
	jmp 	acos_return

acos_continue:

/*
 *    Compute 	sqrt((1-x)/(1-x))
 */
	fsubr	%st(1),%st   		#st(0) = 1 - x
	fxch	%st(1)			#st(0) = 1, st(1) = 1-x
	fadd	%st(2),%st   		#st(0) = 1 + x
	fdivr	%st(1),%st   		#st(0) = (1-x)/(1+x)
	fsqrt

	fstp	(%esp)			#save first arg for atan2
	fld1				#load 1 on stack
	fstl	8(%esp)			#save second arg for atan2
	fwait
 	call	_atan2			#form atan2(sqrt((1-x)/(1+x)),1)
	fst	%st(2)			#st(2) = atan2(sqrt((1-x)/(1+x)),1)
	fadd	%st,%st(2)		#st(2) = 2 * atan2(sqrt((1-x)/(1+x)),1)
	fstp	%st
	fstp	%st

acos_return:
	fstp	%st(1)			#stack model requires empty stack
	addl	$16,%esp		#reset stack -- local storage
	ret
