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
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
 * Copyright (c) 1984, 1985, 1986, 1987 Sequent Computer Systems, Inc.
 * All rights reserved
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: atan.s 1.5 91/01/16 $
 *
 *		ARC TANGENT FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the arctangent function for the C language.
 *	Both atan and atan2 are computed in this routine.
 *	It uses the implicit fpatan instruction of the 287 NPX.
 *
 * Input:	Argument is located at 4(%esp)
 *		For ATAN2, second argument is located at 12(%esp)
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Method:
 *		Uses the FPATAN instruction to calculate atan & atan2.
 *		Check for errors where the arguments are zero.
 *
 *		where fpatan = arctan(st(1)/st)
 *		let x = st  & y = st(1)
 *		sign(y)	   sign(x)   |y| < |x|		final result
 *	       ________  __________  ____________      __________________
 *		  +	     +	       yes		 atan(y/x)
 *		  +	     +	       no		 Pi/2-atan(x/y)
 *		  +	     -	       yes		 Pi-atan(y/-x)
 *		  +	     -	       no		 Pi/2+atan(x/y)
 *		  -	     +	       yes		 atan(-y/x)
 *		  -	     +	       no		 -Pi/2+atan(x/-y)
 *		  -	     +	       yes		 -Pi+atan(-y/-x)
 *		  -	     +	       no		 -Pi/2-atan(-x/-y)
 *		
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
 *	07/23/90	garyg add error checking for atan2(0.0,0.0)
 */

#include "PROF.h"

ENTRY(atan)
	fldl	4(%esp)
	fld1
	fpatan
	ret

ENTRY(atan2)
	movl	8(%esp),%eax	/* check for atan2(0.0,0.0) */
	orl	16(%esp),%eax
	addl	%eax,%eax	/* check bits except sign */
	je	zerozero
ok:
	fldl	4(%esp)
	fldl	12(%esp)
	fpatan
	ret
zerozero:		/* check for both denormalized */
	movl	4(%esp),%eax
	orl	12(%esp),%eax
	jne	ok
	pushl	%ebp
	movl	%esp,%ebp
	pushl	$ecode
	leal	4(%ebp),%eax
	pushl	%eax
	call 	_MATHERR
	leave
	ret
ecode:	.long	14
