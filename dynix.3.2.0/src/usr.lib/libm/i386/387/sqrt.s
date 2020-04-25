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

/* $Header: sqrt.s 1.4 87/06/19 $
 *
 *
 *		SQUARE ROOT FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the square root function for the C language.
 *	It uses the implicit fsqrt instruction of the 287 NPX. No error checking
 *	is performed since it is assumed that the 287 will correctly handle
 *	an illegal argument.
 *
 * Input:	Argument is located at 4(%esp)
 *
 * Output:	Double Precision value is returned in ST(0)
 *		Integer value is returned in %eax
 *
 * Created:
 *	05/20/86	MF - Quantitative Technology Corporation for
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

ENTRY(sqrt)
	testb	$ 0x80, 11(%esp)
	jnz	sqrt_negative_arg
	fldl	4(%esp)			#put argument into st(0)
	fsqrt				#compute sqrt(arg)
	ret

sqrt_negative_arg:
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	ret
ecode:	.long	2
