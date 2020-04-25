/* $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
 */

/* $Log: exp.s,v $
 */
	.ascii	"$Header: exp.s 1.5 1991/05/21 23:48:19 $"
	.file	"exp.s"

/       Copyright (c) 1984 AT&T
/         All Rights Reserved

/       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/       The copyright notice above does not evidence any
/       actual or intended publication of such source code.

	.ident	"@(#)exp.s	1.3"


/	Double precision exponetial routine.

	
/	.string "(c) Copyright Weitek 1987.  All rights reserved."

#include "1167defs.h"
#include "PROF.h"

ENTRY(exp)
	wstctx  %eax
	orl     $0x2000000,%eax
	wldctx  %eax
	movl	4(%esp), %ecx
	movl	%ecx, LOADD|[[FP2|1]<<2]
	movl	8(%esp), %eax
	movl	%eax, LOADD|[FP2<<2]
	orl	%eax, %eax
	jl	neg_arg

	cmpl	$ 0x40862e42, %eax
	jge	maybe_too_big
not_too_big:
	movb	%al, ABSD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0x652b82fe, LOADD|[FP1<<2]
	movl	$ 0x40471547, MULD|[FP4<<2]
	cmpl	$ 0x3e400000, %eax
	jl	could_underflow
	movb	%al, FIXD|[[FP4&0x1c]<<5]|[FP4&3]|[FP1<<2]
	movl	STORS|[FP1<<2], %eax
	movl	%eax, FLOATD|[FP4<<2]
	movl	$ 0xFEFA0000, LOADD|[FP1<<2]
	movl	$ 0x3F962E42, MULD|[FP4<<2]
	movb	%al, SUBD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	%eax, FLOATD|[FP2<<2]
	movl	$ 0xBC9E3B3A, LOADD|[FP1<<2]
	movl	$ 0x3D2CF79A, MULD|[FP2<<2]
	movl	$ 0x1f, %ecx
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x652b82fe, LOADD|[FP1<<2]
	movl	$ 0x40471547, MULD|[FP2<<2]	

	movl	$ 0xaf69a588, LOADD|[[FP4|1]<<2]
	movl	$ 0x3dc6156a, LOADD|[FP4<<2]	
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0xc55011d1, LOADD|[FP1<<2]
	movl	$ 0x3e43b266, ADDD|[FP4<<2]
	andl	%eax, %ecx
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0x1e520f4c, LOADD|[FP1<<2]
	movl	$ 0x3ebc6b09, ADDD|[FP4<<2]
	subl	%ecx, %eax
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0xff614cb7, LOADD|[FP1<<2]
	movl	$ 0x3f2ebfbd, ADDD|[FP4<<2]
	movl	table(, %ecx, 8), %edx
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0xfefa3fae, LOADD|[FP1<<2]
	movl	$ 0x3f962e42, ADDD|[FP4<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	table+4(, %ecx, 8), %ecx
	movl	%edx, LOADD|[FP1<<2]
	movl	%ecx, MULD|[FP2<<2]
	sall	$ [52-32-5], %eax
	movl	%edx, LOADD|[FP1<<2]
	movl	%ecx, ADDD|[FP2<<2]
	addl	$ 0x3ff00000, %eax
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	%eax, MULD|[FP2<<2]
	ret

	.align	2
neg_arg:
	cmpl	$ 0xc086232b, %eax
	jge	maybe_too_small
not_too_small:
	movb	%al, LOADD|[[[FP2|1]&0x1c]<<5]|[[FP2|1]&3]|[[FP4|1]<<2]
	movb	%al, LOADD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0x652b82fe, LOADD|[FP1<<2]
	movl	$ 0x40471547, MULD|[FP4<<2]
	cmpl	$ 0x3e400000+0x80000000, %eax
	jl	could_underflow
	movb	%al, FIXD|[[FP4&0x1c]<<5]|[FP4&3]|[FP1<<2]
	movl	STORS|[FP1<<2], %eax
	decl	%eax
	movl	%eax, FLOATD|[FP4<<2]
	movl	$ 0xFEFA0000, LOADD|[FP1<<2]
	movl	$ 0x3F962E42, MULD|[FP4<<2]
	movb	%al, SUBD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	%eax, FLOATD|[FP2<<2]
	movl	$ 0xBC9E3B3A, LOADD|[FP1<<2]
	movl	$ 0x3D2CF79A, MULD|[FP2<<2]
	movl	$ 0x1f, %ecx
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x652b82fe, LOADD|[FP1<<2]
	movl	$ 0x40471547, MULD|[FP2<<2]	

	movl	$ 0xaf69a588, LOADD|[[FP4|1]<<2]
	movl	$ 0x3dc6156a, LOADD|[FP4<<2]	
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0xc55011d1, LOADD|[FP1<<2]
	movl	$ 0x3e43b266, ADDD|[FP4<<2]
	andl	%eax, %ecx
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0x1e520f4c, LOADD|[FP1<<2]
	movl	$ 0x3ebc6b09, ADDD|[FP4<<2]
	subl	%ecx, %eax
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0xff614cb7, LOADD|[FP1<<2]
	movl	$ 0x3f2ebfbd, ADDD|[FP4<<2]
	movl	table(, %ecx, 8), %edx
	movb	%al, MULD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	$ 0xfefa3fae, LOADD|[FP1<<2]
	movl	$ 0x3f962e42, ADDD|[FP4<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	table+4(, %ecx, 8), %ecx
	movl	%edx, LOADD|[FP1<<2]
	movl	%ecx, MULD|[FP2<<2]
	sall	$ [52-32-5], %eax
	movl	%edx, LOADD|[FP1<<2]
	movl	%ecx, ADDD|[FP2<<2]
	addl	$ 0x3ff00000, %eax
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	%eax, MULD|[FP2<<2]
	ret

	.align	2
could_underflow:
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	ret

	.align	2
maybe_too_small:
	jg	too_small
	cmpl	$ 0xdd7abcd2, %ecx
	jna	not_too_small
too_small:
	wsubl	%fp2,%fp2
	ret

	.align	2
maybe_too_big:
	jg	too_big
	cmpl	$ 0xfefa39ee, %ecx
	jna	not_too_big
too_big:
	pushl	$bigcode
	leal	4(%esp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	ret

smallcode:	.long 5
bigcode:	.long 6

	.align	2
table:
	.long	0x00000000, 0x3ff00000, 0xd3158574, 0x3ff059b0
	.long	0x6cf9890f, 0x3ff0b558, 0xd0125b51, 0x3ff11301
	.long	0x3c7d517b, 0x3ff172b8, 0x3168b9aa, 0x3ff1d487
	.long	0x6e756238, 0x3ff2387a, 0xf51fdee1, 0x3ff29e9d
	.long	0x0a31b715, 0x3ff306fe, 0x373aa9cb, 0x3ff371a7
	.long	0x4c123422, 0x3ff3dea6, 0x6061892d, 0x3ff44e08
	.long	0xd5362a27, 0x3ff4bfda, 0x569d4f82, 0x3ff5342b
	.long	0xdd485429, 0x3ff5ab07, 0xb03a5585, 0x3ff6247e
	.long	0x667f3bcd, 0x3ff6a09e, 0xe8ec5f74, 0x3ff71f75
	.long	0x73eb0187, 0x3ff7a114, 0x994cce13, 0x3ff82589
	.long	0x422aa0db, 0x3ff8ace5, 0xb0cdc5e5, 0x3ff93737
	.long	0x82a3f090, 0x3ff9c491, 0xb23e255d, 0x3ffa5503
	.long	0x995ad3ad, 0x3ffae89f, 0xf2fb5e47, 0x3ffb7f76
	.long	0xdd85529c, 0x3ffc199b, 0xdcef9069, 0x3ffcb720
	.long	0xdcfba487, 0x3ffd5818, 0x337b9b5f, 0x3ffdfc97
	.long	0xa2a490da, 0x3ffea4af, 0x5b6e4540, 0x3fff5076
