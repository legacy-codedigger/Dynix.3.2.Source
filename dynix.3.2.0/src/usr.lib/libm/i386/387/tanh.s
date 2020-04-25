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

/* $Header: tanh.s 1.4 87/06/19 $
 *
 *		TANH FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the hyperbolic tangent function for 
 *	the C language. Error checking is performed for overflow errors.
 *
 *
 * Input:	Argument is located at 4(%esp)
 *	        Assumed that the argument is normal.
 *		A check is done to be sure that it will not overflow.
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Assumptions:  ST(0) & ST(1) are scratch stack registers
 *		Input argument is normal.
 *		Registers ax,cx, and dx are scratch registers.
 *
 * Created:
 *	06/19/86	GS - Quantitative Technology Corporation for
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
 * Method:
 *	Tanh:
 *
 *	1. Replace x by |x|  since tanh(-x) = - tanh(x)
 *
 *	2. For 0 < x <= 1e-10    : tanh(x) = x
 *
 *          For 1e-10 < x <  45.0 : tanh(x) = exp(x) - 1/exp(x)
 *			  		     -----------------
 *					     exp(x) + 1/exp(x)	
 *
 *	   For 45.0 < x < INF.	 : tanh(x) = 1
 *
 *	   Note: 45 was chosen since exp(45) + exp(-45) == exp(45)
 *
 *	Special case checked: tanh(0) = 0 exact for finite arguments
 */

#include "PROF.h"

ENTRY(tanh)

	fldl	4(%esp)			#Put argument on top of stack
	fld	%st(0)			#Copy argument to top of stack
	fabs				#st(0) = |x|
	fldl	libm_lo_range		#place lower range const. on stack

	subl	$8,%esp			#make room for temp storage

	fcomp				#compare lo_range to |x|
	fstsw	%ax			#set flags with test results
	sahf
	jnc	tanh_return		#If (x < lo_range) then return(x)
	fldl	libm_hi_range		#place high range const. on stack
	fcomp				#compare hi_range to |x|
	fstsw	%ax			#set flags with test results
	sahf
	jc	tanh_return_one		#If (x > lo_range) then return(1)
					#Else  do the brute force calc.
/*
 *	Stack values
 *	  st(0) = |x|
 *	  st(1) = x
 *	  for stack restoration   st = original st(1)
 */
	fld	%st(0)			#copy |x| onto the top of the stack
	fstpl	(%esp)			#save argument in memory for the exp
	call	_exp			#call the exp routine
/*
 *	Stack values
 *	  st(0) = exp(x)
 *	  st(1) = ??
 *	  st(2) = x
 *	  for stack restoration st = original st(0)
 */
	fxch	%st(2)			#st(0) = x , st(2) = exp(x)
	ftst				#compare x to 0.0
	fstsw	%ax			#save results of the comparison
	fstp	%st(0)			#pop the stack
	fstp	%st(0)			#pop the stack
	fld	%st(0)			#copy exp(x) onto the stack
	fld1				#place 1.0 onto the stack
	fdiv	%st(1),%st		#st(0) = exp(-x) == 1/exp(x)
	fadd	%st,%st(1)		#st(1) = exp(x) + exp(-x)
	fsubr	%st(2),%st		#st(0) = exp(x) - exp(-x)
	fdiv				#st(1)=exp(x)-exp(-x)/exp(x)+exp(-x),pop
	
	fxch	%st(1)			#st(1) = tanh(x)
	jmp	correct_sign		#now correct the sign of the result
	
	
tanh_return_one:
	#Stack values
	#st(0) = |x|
	#st(1) = x
	#For stack restoration st = original st(1)

	fxch	%st(1)			#move x to top of stack
	ftst				#compare x to 0.0
	fstsw	%ax			#save results of comparison
	fld1				#load 1 onto the stack
	fxch	%st(2)			#st(2) = tanh(x)
	fstp	%st(0)			#pop the stack

correct_sign:
	sahf				#set flags from compare x to 0.0 
	jnc	tanh_return		#if (x>0) then don't change sign
	fxch	%st(1)
	fchs				#else reverse the sign of result
	fxch	%st(1)

tanh_return:
	fstp	%st(0)			#stack model requires empty stack
	addl	$8,%esp			#flush local storage
	ret
