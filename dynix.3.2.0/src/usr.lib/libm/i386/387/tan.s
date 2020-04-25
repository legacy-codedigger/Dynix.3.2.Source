/* $Copyright: $
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

/* $Header: tan.s 1.7 1991/06/03 21:53:34 $
 *
 *		TAN FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the tangent function for the C language.
 *	It uses the fptan instruction of the 287 NPX. No error checking
 *	is performed since it is assumed that the 287 will correctly handle
 *	an illegal argument.
 *
 *
 * Input:	Argument is located at 4(%esp)
 *	        Assumed that the argument is normal.
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Assumptions:  ST(0) & ST(1) are scratch registers
 *		Input argument is normal.
 *
 * Created:
 *	05/28/86	GS - Quantitative Technology Corporation for
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
 *	This routine uses the fptan instruction to calculate the tangent.
 *	The fptan instruction requires arguments in the range 0 <= x <= pi/4.
 *	The relationship between fptan and the tangent depends on the 
 *	octant in which the angle lies and is as follows:
 *
 *		where K = angle mod pi/4
 *
 *	      Octant		        Relationship
 *				    (Negate if the angle < 0)
 *	
 *		0			    tan(K)	
 *		1			1/tan(pi/4 - K)
 *		2			  -1/tan(K)
 *		3			-tan(pi/4 - K)
 *		4			    tan(K)	
 *		5			1/tan(pi/4 - K)
 *		6			  -1/tan(K)
 *		7			-tan(pi/4 - K)
 *
 */

#include "PROF.h"

	.set mask_cond0,0x01
	.set mask_cond1,0x02
	.set mask_Scond13,0x0240
	.set mask_cond13,0x42

ENTRY(tan)

	fldl	4(%esp)			#put argument onto stack
       		
	/*
	 * Range checking
	 * The 387 tan will loose all percision if the argument is 
	 * too large.
	 */
	fld 	%st			#	Load arg to top of stack
	fabs				# 	Get the abs val
	fcompl	libm_half_tloss		#	Compare with max (and pop abs(arg))
	fstsw	%ax			#	Save status regs
	sahf				#	Check status regs
	ja	tan_tloss_err		#	Handle arg too large

	fptan				# tan(st(0)) = st(1)/st(0)
	fstp	%st(0)
	ret

tan_tloss_err:
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	fstp	%st(0)
	call	_MATHERR
	addl	$8, %esp
	ret

ecode:	.long	16
