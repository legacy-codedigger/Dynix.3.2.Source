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

/* $Header: vfork.s 2.5 87/06/23 $
 *
 * $Log:	vfork.s,v $
 */

/*
 * pid = vfork();
 *
 * %ecx == 0 in parent process, %ecx == 1 in child process.
 * %eax == pid of child in parent, %eax == pid of parent in child.
 *
 * Original trickery here, due to keith sklower, uses ret to clear the stack,
 * and then returns with a jump indirect, since only one person can return
 * with a ret off this stack... we do the ret before we vfork!
 *
 * This assumes %edx is saved/restored across syscall.
 */

#include "SYS.h"

	.globl	_vfork
	.text
	.align	2
_vfork:
	popl	%edx			/* return PC */
	SVC(0,vfork)			/* do the vfork. */
	jc	err			/* error? */

	testl	%ecx, %ecx		/* no -- is parent? */
	jz	parent			/* yes */
	xorl	%eax, %eax		/* child returns 0 */
parent:
	jmp	*%edx			/* no -- parent, return. */
err:
	movl	%eax, _errno		/* save in _errno */
	movl	$-1, %eax		/* return -1 to indicate error */
	jmp	*%edx
