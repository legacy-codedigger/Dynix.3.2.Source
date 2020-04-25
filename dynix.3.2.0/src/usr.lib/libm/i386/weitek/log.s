/* $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
 */

/* $Log:	log.s,v $
 */
	.ascii	"$Header: log.s 1.3 90/02/13 $"
	.file	"log.s"

/       Copyright (c) 1984 AT&T
/         All Rights Reserved

/       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/       The copyright notice above does not evidence any
/       actual or intended publication of such source code.

	.ident	"@(#)log.s	1.3"


/	Double precision logarithm routines (both natural and base 10).

	
/	.string "(c) Copyright Weitek 1987.  All rights reserved."

#include "1167defs.h"
#include "PROF.h"

ENTRY(log)
	movl    8(%esp), %eax
	andl    $0x7ff00000, %eax
	cmpl    $[0x7ff << 20], %eax
	je      nanorinf
	movl	4(%esp), %eax		
	movl	%eax, LOADD|[[FP4|1]<<2]
	movl	8(%esp), %eax
	andl	$ 0x000fffff, %eax
	addl	$ 0x3ff00000, %eax
	movl	%eax, LOADD|[FP4<<2]
	andl	$ 0xffff8000, %eax
	addl	$ 0x00008000, %eax
	andl	$ 0xffff7fff, %eax
	movl	$ 0x00000000, LOADD|[[FP2|1]<<2]
	movl	%eax, LOADD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	%eax, ADDD|[FP4<<2]
	movb	%al, ADDD|[[FP2&0x1c]<<5]|[FP2&3]|[FP2<<2]		
	movb	%al, DIVD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]

	movl	8(%esp), %eax		
	movl	%eax, %ecx
	andl	$ 0x000f8000, %ecx
	addl	$ 0x00008000, %ecx
	shrl	$ 16, %ecx

	andl	$ 0x7ff00000|0x80000000, %eax
	jle	not_positive
	shrl	$ 20, %eax
	cmpl	$ 8, %ecx
	sbbl	$ 1023-1, %eax

	pushl	%edx
	movl	table(, %ecx, 8), %edx
	movl	table+4(, %ecx, 8), %ecx

	movb	%al, ABSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]		
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ 0x1a5d9e7f, LOADD|[FP1<<2]
	movl	$ 0x3f624adf, MULD|[FP2<<2]
	movl	$ 0x88c97f94, LOADD|[FP1<<2]
	movl	$ 0x3f899999, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ 0x55556f94, LOADD|[FP1<<2]
	movl	$ 0x3fb55555, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movb	%al, ADDD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]

	movl	%edx, LOADD|[FP1<<2]
	movl	%ecx, ADDD|[FP2<<2]	

	movl	%eax, FLOATD|[FP4<<2]		
	movl	$ 0xfefa39ef, LOADD|[FP1<<2]
	movl	$ 0x3fe62e42, MULD|[FP4<<2]
	popl	%edx
	movb	%al, ADDD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	ret

nanorinf:
	movl    8(%esp), %eax
	andl    $0x000fffff, %eax
	orl     4(%esp), %eax
	jz      infinity
	wloadl  4(%esp), %fp2
	pushl   $nancode
	leal    4(%esp), %eax
	pushl   %eax
	call    _MATHERR
	addl    $8, %esp
	ret

infinity:
	wloadl  4(%esp), %fp2
	pushl   $infcode
	leal    4(%esp), %eax
	pushl   %eax
	call    _MATHERR
	addl    $8, %esp
	ret

not_positive:
	pushl	%edx		/* save scratch register for return */
	pushl	$ecode		/* address of error code to stack */
	leal	8(%esp), %eax
	pushl	%eax		/* address of pc to stack */
	call	_MATHERR	/* print message and location */
	addl	$8, %esp	/* clean up stack */
	popl	%edx		/* restore saved register */
	ret
	
infcode:.long	7
ecode:	.long	8
nancode:.long	9

	.align	2
table:
	.long	0x00000000, 0x00000000, 0xc01162a6, 0x3faf0a30
	.long	0x6e2af2e6, 0x3fbe2707, 0x70a793d4, 0x3fc5ff30
	.long	0xc79a9a22, 0x3fcc8ff7, 0xababa60e, 0x3fd1675c
	.long	0xc21c5ec2, 0x3fd4618b, 0xf6bbd007, 0x3fd739d7
	.long	0x1134db92, 0xbfd26962, 0x6cb3b379, 0xbfcf991c
	.long	0x3c8ad9e3, 0xbfca93ed, 0x6b543db2, 0xbfc5bf40
	.long	0x8227e47c, 0xbfc1178e, 0x5d594989, 0xbfb9335e
	.long	0xb59e3a07, 0xbfb08598, 0x89e74444, 0xbfa0415d
	.long	0x00000000, 0x00000000

ENTRY(log10)
	popl	%edx
	call	_log
	movl	$ 0x1526e50e, LOADD|[FP1<<2]
	movl	$ 0x3fdbcb7b, MULD|[FP2<<2]
	jmp	* %edx
