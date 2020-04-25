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

/* $Header: asin.s 1.5 87/08/13 $
 *
 *		ARCSINE FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the arcsine function for the C language.
 *
 * Input:	Argument is located at 4(%esp)
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Method:
 *		for |x| <= 1
 *		
 *		asin(x) = atan2(x,sqrt(1-x*x))
 *
 *		    where for better accuracy when x >= 0.5
 *			  1-x*x = 2*(1-|x|)-(1-|x|)*(1-|x|)
 *
 * Created:
 *	05/28/86	MF - Quantitative Technology Corporation for
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
 */

#include "PROF.h"

	.text
half:	.double	0Dx3FE0000000000000	#0.5
	.set	maskC0C3,0x4100		#mask for C0 and C3
	.set	maskC0,0x0100		#mask for C0

ENTRY(asin)
	fldl	4(%esp)			#put argument onto stack [%st(2)]
	fld1				#put 1.0 onto stack
	subl	$16,%esp		#make room for temp storage
	fld	%st(1)			#put argument onto stack
	fabs				#|x| --> st(0)
	fcom				#compare 1.0 and |x|
	fstsw	%ax			#store status of compare
	testw	$maskC0C3,%ax		#|x| <= 1.0?
	jne	asin_continue		#if so, continue

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

asin_continue:
	fldl	half			#put 0.5 onto stack
	fcomp				#compare |x| and 0.5
	fstsw	%ax			#store status of compare
	testw	$maskC0,%ax		#|x| > 0.5?
	jne	asin_accuracy		#if so, do more accurate computation
	fmul	%st(0),%st(0)		#form sqr(x)
	fsubr				#form 1 - sqr(x)
	jmp	asin_do_sqrt		#go take sqrt

/*			|x| > 0.5
 *    Compute 1 - x*x by:
 *		2*(1-|x|) - (1-|x|) * (1-|x|)
 */

asin_accuracy:
	fsubr				#form 1 - |x|
	fld	%st(0)			#make copy of 1 - |x|
	fmul	%st(0),%st(0)		#form (1-|x|) * (1-|x|)
	fxch	%st(1)			#put 1 - |x| back on top of stack
	fadd	%st(0),%st(0)		#form 2 * (1-|x|)
	fsub				#form (2*(1-|x|) - (1-|x|)*(1-|x|)

asin_do_sqrt:
	fsqrt				#form sqrt(1-sqr(x))
	fstpl	8(%esp)			#get second arg for atan2
	fstl	(%esp)			#get first arg for atan2
	call	_atan2			#form atan2(x, sqrt(1-sqr(x)))
asin_return:
	fstp	%st(1)			#stack model requires empty stack
	addl	$16,%esp		#reset stack -- local storage
	ret
