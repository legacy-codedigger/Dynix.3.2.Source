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

/* $Header: brk.s 2.5 87/06/22 $
 *
 * $Log:	brk.s,v $
 */

#include "SYS.h"

	.globl	__minbrk
	.globl	__curbrk

	.data
__minbrk: .long	_end
__curbrk: .long	_end

ENTRY(brk)
	movl	SPARG0, %ecx
	cmpl	__minbrk, %ecx
	jl	fixbrk
ok:	SVC(1,brk)
	jc	err
	movl	SPARG0, %eax
	movl	%eax, __curbrk
	xorl	%eax, %eax
	ret
fixbrk:	movl	__minbrk, %ecx
	movl	%ecx, SPARG0
	jmp	ok
CERROR
