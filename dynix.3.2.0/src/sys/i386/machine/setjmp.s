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
.asciz	"$Header: setjmp.s 2.5 86/07/16 $"

/*
 * setjmp.s
 *	Setjmp/longjump and push longjmp routines.
 *
 * i386 version.
 */

/* $Log:	setjmp.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

/*
 * setjmp(jmp_buf)
 *	label_t *jmp_buf;
 *
 * Save process state in label_t buffer.
 * Return 0.
 *
 * Since longjmp() does a jmp* (instead of a return), the saved stack
 * pointer is that just prior to the call to setjmp() (ie, args pushed).
 * Thus, longjmp behaves as a return to setjmp().
 *
 * This also lets in-line expanded fsetjmp() save %esp instead of &-4(%esp)
 * (ie, faster) -- if in-line'd fsetjmp() did the save, there is no argument
 * at the new %esp.
 */

ENTRY(setjmp)
	popl	%ecx			# ECX = return PC, ESP -> arg.
	movl	(%esp), %eax		# EAX -> label_t buffer.
	movl	%esp, LT_ESP(%eax)	# saved SP -> where setjmp() returns.
	movl	%ebp, LT_EBP(%eax)	# save invokers EBP.
	movl	%ecx, LT_EIP(%eax)	# save return address for longjmp.
	movl	%ebx, LT_EBX(%eax)	# save...
	movl	%esi, LT_ESI(%eax)	#	...register
	movl	%edi, LT_EDI(%eax)	#		...variables.
	subl	%eax, %eax		# 0 ==> return from setjmp().
	jmp	*%ecx			# "return"

/*
 * longjmp(jmp_buf)
 *	label_t *jmp_buf;
 *
 * Restore process state from jmp_buf.
 * Return non-zero.
 */

ENTRY(longjmp)
	movl	SPARG0, %eax		# EAX -> label_t buffer.
	movl	LT_ESP(%eax), %esp	# restore stack pointer.
	movl	LT_EBP(%eax), %ebp	# restore frame pointer.
	movl	LT_EIP(%eax), %ecx	# return address.
	movl	LT_EBX(%eax), %ebx	# restore...
	movl	LT_ESI(%eax), %esi	#	...register
	movl	LT_EDI(%eax), %edi	#		...variables.
	jmp	*%ecx			# "longjmp" -- EAX non-zero from above.

/*
 * pushlongjmp(uarea, save-area)
 *	Push a resume context into the stack of the argument uarea such that
 *	the resume will actually cause execution of longjmp(save-area).
 *
 * Used in procdup() to arrange for the longjmp without requiring
 * swtch()/resume()/etc to test flags.
 *
 * Uses current SP since this is below "setjmp" frame, and caller need not
 * worry about what SP to use.
 *
 * Doesn't save "ebp" for return from resume context -- not used by longjmp().
 */

ENTRY(pushlongjmp)
	movl	SPARG0, %ecx		# ECX -> child Uarea.
	movl	%esp, %eax		# assume current SP.
	subl	$VA_UAREA, %eax		# stack offset from Uarea (SP - &u).
	addl	%ecx, %eax		# EAX -> where in child stack to push.
	movl	SPARG1, %edx		# argument for the longjmp...
	movl	%edx, (%eax)		#	... "push".
					# skip -4(%eax): unused longjmp return.
	leal	_longjmp, %edx		# address of longjmp...
	movl	%edx, -8(%eax)		#	... "push".  Skip unused ret.
					# skip -12(%eax): unused frame (ebp).
	leal	-12(%esp), %edx		# addr of "save" state...
	movl	%edx, U_SP(%ecx)	#	... set for resume().
	RETURN				# Done.
