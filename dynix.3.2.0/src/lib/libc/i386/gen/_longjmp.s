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

	.file	"_longjmp.s"
	.text
	.asciz	"$Header: _longjmp.s 1.9 87/05/29 $"

/*
 * _setjmp(a)
 * _longjmp(a, v)
 *	jmp_buf a;
 *	int v;
 *
 * 	_longjmp(a,v)
 *  will generate a "return(v)"  from the last call to _setjmp(a)
 *  by restoring %edi, %esi, %ebx, fp, sp, pc, old signal mask,
 *  and onsigstack from 'a' and doing a return.
 *
 * NOTE: _setjmp and _longjmp don't manipulate signals (or the signal stack)
 */

#include "DEFS.h"
#include "Setjmp.h"

	.text
ENTRY(_longjmp)
	ENTER
	cld				# assure direction flag is correct!
	movl	FPARG0,%edx		# get 'a'
	cmpl	$MAGIC,MGIC(%edx)	# sanity check
	jne	_longjmp_botch

	movl	FPARG1,%eax		# get 'v'
	cmpl	$0,%eax			# assure _longjmp doesn't return 0
	jne	_longjmp_nzero

	movl	$1,%eax			# return 1 if called with 0
_longjmp_nzero:
	movl	0(%ebp),%ecx		# fetch parents frame pointer
	cmpl	%ecx,FPTR(%edx)		# same frame?
	je	_longjmp_same_frame
/*
 * restore all registers (to value at time of _setjmp)
 */
	movl	REGEDI(%edx),%edi	# restore %edi
	movl	REGESI(%edx),%esi	#         %esi
	movl	REGEBX(%edx),%ebx	#         %ebx
_longjmp_same_frame:
	movl	FPTR(%edx),%ebp		#	  fp
	movl	SPTR(%edx),%esp		#	  sp
	addl	$4,%esp			# clear PC (since no ret to do it)
	movl	PCTR(%edx),%edx		# get return address
	jmp	*%edx			# return

_longjmp_botch:
	pushl	$msize
	pushl	$msgstr
	pushl	$STDERR
	CALL	_write			# write error message to fd 2
	addl	$12,%esp
	CALL	_abort			# generate core dump for user to debug

msgstr:	.asciz	"_longjmp botch\n"
msgend:
	.set	msize, msgend-msgstr
