/*
 * $Copyright:	$
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

	.file	"_setjmp.s"
	.text
	.asciz	"$Header: _setjmp.s 1.4 86/06/19 $"

/*
 * _setjmp(a)
 * _longjmp(a, v)
 *	jmp_buf a;
 *	int v;
 *
 * 	_longjmp(a,v)
 *  will generate a "return(v)" from the last call to _setjmp(a)
 *  by restoring %edi, %esi, %ebx, fp, sp, pc, from 'a' and doing a return.
 */

/*
 * NOTE: _setjmp and _longjmp don't manipulate signals (or the signal stack).
 *	Also, taking a signal in the critical code followed by a longjmp WILL 
 *	result in a _longjmp botch.  So block signals around calls.  Alternative
 *	is just let signal race with updating setjmp buffer but this may
 *	lead to a later illegal instruction or worse instead of a _longjmp
 *	botch.  The real setjmp routine internally blocks signals.
 */

#include "DEFS.h"
#include "Setjmp.h"

ENTRY(_setjmp)
	ENTER
	movl	FPARG0,%eax		# get 'a'
	movl	%edi,REGEDI(%eax)	# save	%edi
	movl	%esi,REGESI(%eax)	#	%esi
	movl	%ebx,REGEBX(%eax)	#	%ebx

	movl	$0,MGIC(%eax)		# invalidate setjmp buffer
					# around critical code so
					# if we take signal and call
					# _longjmp, we _longjmp botch
	movl	0(%ebp),%edx
	movl	%edx,FPTR(%eax)		# save	fp
	leal	4(%ebp),%edx
	movl	%edx,SPTR(%eax)		#	user sp
	movl	4(%ebp),%edx
	movl	%edx,PCTR(%eax)		#	pc

	movl	$MAGIC,MGIC(%eax)	# validate setjmp buffer

        xorl  	%eax,%eax		# return zero when set
	EXIT
	RETURN
