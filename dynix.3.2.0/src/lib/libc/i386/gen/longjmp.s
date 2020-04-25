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

	.file	"longjmp.s"
	.text
	.asciz	"$Header: longjmp.s 1.11 1992/03/16 23:18:56 $"

/*
 * setjmp(a) 
 * longjmp(a, v) 
 * jmp_buf a;
 * int v;
 *
 * 	longjmp(a,v)
 *  will generate a "return(v)" from the last call to setjmp(a)
 *  by restoring %esi, %edi, %ebx, fp, sp, pc, old signal mask,
 *  and onsigstack from 'a' and doing a return.
 *  Longjmp never returns 0.
 */

/*
 * NOTE:  Unlike the vax version, this implementation of longjmp call restores 
 *	 all register values to the value at the time of the setjmp call instead
 *	 of trying to crawl up the frame (like the vax) because our linkage is 
 *	 insufficient to do so. An improved version that could do so would 
 *	 require the register save mask from the enter instruction be saved 
 *	 on the stack for each frame. If we are in the same frame when the 
 *	 longjmp occurs, do not touch register variables. This is compatible 
 *	 with SYS V Release 2, but is not necessarily optiminal. In all cases, 
 *	 register variables after a longjmp should never be relied on!
 */

#include "Setjmp.h"
#include "SYS.h"
#undef	ENTRY
#include "DEFS.h"

/*
 * Longjmp is tricky business.  Look at sigvec.s for more info.
 */

	.text
	.globl	_write
	.globl	_abort

ENTRY(longjmp)
	ENTER
	cld				# assure direction flag is correct!
	movl    FPARG0,%edx		# get 'a'
	cmpl	$MAGIC,MGIC(%edx)	# sanity check
	jne	longjump_botch

	movl	FPARG1,%eax		# get 'v'
	cmpl	$0,%eax			# assure longjmp can't return 0
	jne	longjmp_nzero

	movl	$1,%eax			# return 1 if v == 0
longjmp_nzero:
	movl	0(%ebp),%ecx		# fetch parents frame pointer
	cmpl	%ecx,FPTR(%edx)		# save frame?
	je	longjmp_same_frame 
/*
 * restore regs if in different frame (to values at time of setjmp)
 */
	movl	REGEDI(%edx),%edi	# restore %edi
	movl	REGESI(%edx),%esi	#	  %esi
	movl	REGEBX(%edx),%ebx	#	  %ebx
longjmp_same_frame:
/*
 * setup sigcontext and sigframe on stack
 */
	movl    FPTR(%edx),%ebp		#	  fp
	movl	SPTR(%edx),%esp		# restore old stack pointer
	addl	$4,%esp			# pop off setjmp return addr
	pushl	PCTR(%edx)		# ip
	pushl	$0			# flags
	pushl	%esp			# sp before return
	pushl	SMSK(%edx)		# signal mask
	pushl	SSTK(%edx)		# signal stack (start of sigcontext)
					# fpusave & fpasave space is not
					#   needed, since u_uflags does
					#   not indicate that float context
					#   has been saved.
	pushl	$0			# u_uflags
	pushl	%eax			# %eax	<- return value
	pushl	%ecx			# %ecx    <- (don't cares)
	pushl	%edx			# %edx    <- (don't cares)
	leal	4*4(%esp),%eax		# get address of sigcontext structure
	pushl	%eax			# *scp (start of sigclean)
	SVC0(sigcleanup)		# do signal cleanup system call
/*
 * Now on sigcontext stack.  So we now return.
 */
	popfl			# pop off flags
	cld			# assure direction flag is correct!
	ret			# and back to where setjmp was set

longjump_botch:
	pushl	$msize
	pushl	$msgstr
	pushl	$STDERR
	call	_write		# write(2, "longjmp botch", sizeof "longjmp botch")
	addl	$12,%esp
	call	_abort		# generate core dump for user

msgstr:	.asciz	"longjmp botch\n"
msgend:
	.set	msize, msgend-msgstr
