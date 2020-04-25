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

	.file	"setjmp.s"
	.text
	.asciz	"$Header: setjmp.s 1.4 86/06/19 $"

/*
 * setjmp(a) 
 * longjmp(a, v) 
 *   jmp_buf a;
 *   int v;
 *
 * 	longjmp(a,v)
 *  will generate a "return(v)" from the last call to setjmp(a)
 *  by restoring %edi, %esi, %ebx, fp, sp, pc, old signal mask,
 *  and onsigstack from 'a' and doing a return.
 *  Longjmp never returns 0.
 */


#include "DEFS.h"
#include "Setjmp.h"
.text
	.globl	_sigblock
	.globl	_sigstack
	.globl	_sigsetmask

ENTRY(setjmp)
	ENTER
	#addl	$-8,%esp		# 8 bytes for struct sigstack
/*
 * block all (blockable) signals
 */
	pushl	$SIGALL
	CALL	_sigblock		# sigblock(SIGALL)
	addl	$4,%esp			# old mask returned in %eax

	movl	FPARG0,%ecx		# get 'a'
	movl	%edi,REGEDI(%ecx)	# save	%edi
	movl	%esi,REGESI(%ecx)	#	%esi
	movl	%ebx,REGEBX(%ecx)	#	%ebx
	movl	0(%ebp),%edx
	movl	%edx,FPTR(%ecx)		# save	fp
	leal	4(%ebp),%edx
	movl	%edx,SPTR(%ecx)		#	user sp
	movl	4(%ebp),%edx
	movl	%edx,PCTR(%ecx)		#	pc
	movl	%eax,SMSK(%ecx)		#	old signal mask
	movl	%ecx,%edi		# save 'a'  since can't assume 
					# scratch regs are saved across syscalls
/*
 * Get onsigstack and save it
 */
	#leal	-8(%ebp),%eax
	#pushl	%eax
	#pushl	$0
	#CALL	_sigstack		# sigstack(0, oss);
	#addl	$8,%esp
	#movl	-4(%ebp),%eax
	#movl	%eax,SSTK(%edi)		# save oss->ss_onstack

	movl	$0,SSTK(%edi)		# for now just use 0

	movl	$MAGIC,MGIC(%edi)	# validate setjmp buffer
/*
 * unblock signals by restoring old signal mask
 */
	pushl	SMSK(%edi)
	CALL	_sigsetmask		# sigsetmask(oldmask)
	addl	$4,%esp

        xorl	%eax,%eax		# always returns zero when set
	movl	REGEDI(%edi),%edi	# restore %edi on return
	leave			
	RETURN
