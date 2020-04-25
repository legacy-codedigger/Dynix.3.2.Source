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
.asciz	"$Header: icode.s 2.5 87/06/02 $"
.text

/*
 * icode.s
 *	Init-code start-off.
 *
 * This is coded such that it is position independent; when loaded at
 * user virtual "LOWPAGES*NBPG" it will be correct.
 *
 * Actually, no need to load at LOWPAGES*NBPG -- this is an 032 vestige
 * due to need for fixed position module table.
 */

/* $Log:	icode.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

#include <syscall.h>

/*
 * The init code!  If this is successful, the system comes up.
 * Else, we get a BPT in user-mode.
 */

ENTRY(icode)
	jmp	start

etcinit:.asciz	"/etc/init"

start:
	movl	$[SYS_execv|[SYS_BSD<<16]], %eax # execv, release independent
	movl	$[[LOWPAGES*NBPG]+etcinit-_icode], %ecx # arg1 = "/etc/init"
	movl	$0, %edx		# arg2 = args == NULL.
	int	$T_SVC2			# execv() (2-arg syscall)
	int	$3			# just in case...

	.globl	_szicode
	.align	2
_szicode:
	.long	_szicode-_icode		# sizeof(icode)
