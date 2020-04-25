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

/* $Log: atan.s,v $
 */
	.ascii	"$Header: atan.s 1.3 1991/05/22 00:08:04 $"
	.file	"atan.s"

/       Copyright (c) 1984 AT&T
/         All Rights Reserved

/       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/       The copyright notice above does not evidence any
/       actual or intended publication of such source code.

	.ident	"@(#)atan.s	1.5"

/	Double precsion arctangent routine.

	
/	.string "(c) Copyright Weitek 1987.  All rights reserved."

#include "1167defs.h"
#include "PROF.h"

ENTRY(atan)
	movl	4(%esp), %ecx
	movl	%ecx, LOADD|[FP1<<2]
	movl	8(%esp), %eax
	movl	%eax, ABSD|[FP4<<2]
	andl	$ 0x7fffffff, %eax

	cmpl	$ 0x3ff6cc63, %eax	
	jge	atan_11_16
	cmpl	$ 0x3fde193b, %eax	
	jl	atan_1_4
	cmpl	$ 0x3fe67e67, %eax	
	jl	atan_5_6
	cmpl	$ 0x3feae94e, %eax	
	jl	atan_7
	cmpl	$ 0x3ff00346, %eax	
	jl	atan_8
	cmpl	$ 0x3ff30ded, %eax	
	jge	atan_10

	movl	$ 0x3f8bb292, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_9_t, %edx
	jmp	common_evaluate

	.align	2
atan_9_t:
	.long	0x03c112e8, 0x3fea87b8

	.align	2
atan_8a:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]	
	movl	8(%esp), %eax
	subl	$ 0x3ff00000-0x3fe921fb, %eax
	movl	%eax, LOADD|[FP2<<2]
	ret

	.align	2
atan_1_4:
	cmpl	$ 0x3fd79557, %eax	
	jge	atan_4
	cmpl	$ 0x3fd178d4, %eax	
	jge	atan_3
	cmpl	$ 0x3e539885, %eax	
	jl	atan_1

	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movl	$ 0xcb6ce108, LOADD|[[FP2|1]<<2]
	movl	$ 0x402268f9, LOADD|[FP2<<2]
	movb	%al, ADDD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0xa323f893, LOADD|[FP1<<2]
	movl	$ 0x40348b0d, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x1e60b845, LOADD|[FP1<<2]
	movl	$ 0x4029afee, ADDD|[FP2<<2]

	movl	$ 0xf88731fc, LOADD|[[FP6|1]<<2]
	movl	$ 0x3fca4d7d, LOADD|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movl	$ 0xa7519496, LOADD|[FP1<<2]
	movl	$ 0x4013b440, ADDD|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movl	$ 0x9e13dc01, LOADD|[FP1<<2]
	movl	$ 0x40304310, ADDD|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movl	$ 0x1e60b844, LOADD|[FP1<<2]
	movl	$ 0x4029afee, ADDD|[FP6<<2]

	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	8(%esp), %eax
	movl	%ecx, LOADD|[FP1<<2]
	movl	%eax, MULD|[FP2<<2]
	ret

	.align	2
atan_1:
	movl	%ecx, LOADD|[[FP2|1]<<2]
	movl	8(%esp), %eax
	movl	%eax, LOADD|[FP2<<2]
	ret

	.align	2
atan_5_6:
	cmpl	$ 0x3fe29581, %eax	
	jl	atan_5

	movl	$ 0x3f23e072, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_6_t, %edx
	jmp	common_evaluate

	.align	2
atan_6_t:
	.long	0xcc435f26, 0x3fe238a5

	.align	2
atan_11_16:
	cmpl	$ 0x40010b78, %eax	
	jl	atan_11_12
	cmpl	$ 0x4005c2f8, %eax	
	jl	atan_13
	cmpl	$ 0x400d648e, %eax	
	jl	atan_14
	cmpl	$ 0x43345f0a, %eax	
	jg	atan_16

	movb	%al, ABSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP6<<2]
	movl	$ 0x5d81c684, LOADD|[[FP2|1]<<2]
	movl	$ 0x3ff9977e, LOADD|[FP2<<2]
	movb	%al, ADDD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ 0xf32ef8a6, LOADD|[FP1<<2]
	movl	$ 0x3fe6ef36, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ 0x22a140d6, LOADD|[FP1<<2]
	movl	$ 0x3fb3ee9d, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x082c73da, LOADD|[[FP4|1]<<2]
	movl	$ 0x3ff44229, LOADD|[FP4<<2]
	movb	%al, ADDD|[[FP6&0x1c]<<5]|[FP6&3]|[FP4<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP4<<2]
	movl	$ 0x8bc9c6cc, LOADD|[FP1<<2]
	movl	$ 0x3fd88be7, ADDD|[FP4<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP4<<2]
	movl	$ 0x4d219f8e, LOADD|[FP1<<2]
	movl	$ 0x3f906224, ADDD|[FP4<<2]
	movb	%al, DIVD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	8(%esp), %eax
	andl	$ 0x80000000, %eax
	orl	$ 0x3ff921fb, %eax
	jl	atan_15a
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	%eax, SUBD|[FP2<<2]
	ret

	.align	2
atan_15a:
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	%eax, ADDD|[FP2<<2]
	ret

	.align	2
atan_11_12:
	cmpl	$ 0x3ffb9999, %eax	
	jge	atan_12

	movl	$ 0x3fc84a8e, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_11_t, %edx
	jmp	common_evaluate

	.align	2
atan_11_t:
	.long	0xb6acb768, 0x3ff008c7

	.align	2
atan_16:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	8(%esp),%ecx
	andl	$ 0x80000000, %ecx		
	orl     $ 0x3ff921fb, %ecx
	movl	%ecx, LOADD|[FP2<<2]
	ret

	.align	2
atan_3:
	movl	$ 0x3ea3e29a, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_3_t, %edx
	jmp	common_evaluate

	.align	2
atan_3_t:
	.long	0xe507b9d6, 0x3fd3d37b

	.align	2
atan_4:
	movl	$ 0x3ed640c8, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_4_t, %edx
	jmp	common_evaluate

	.align	2
atan_4_t:
	.long	0xa9b3c692, 0x3fd95d51

	.align	2
atan_5:
	movl	$ 0x3f063622, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_5_t, %edx
	jmp	common_evaluate

	.align	2
atan_5_t:
	.long	0x526fde38, 0x3fdee755

	.align	2
atan_7:
	movl	$ 0x3f450894, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_7_t, %edx
	jmp	common_evaluate

	.align	2
atan_7_t:
	.long	0xa075668c, 0x3fe4fdad

	.align	2
atan_8:
	movl	$ 0x3f6aee86, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	cmpl	$ 0x3ff00000, %eax
	jne	eval_1
	cmpl	$ 0x00000000, %ecx
	je	atan_8a
eval_1:
	movl	$ atan_8_t, %edx

common_evaluate:
	movb	%al, ABSD|[[FP2&0x1c]<<5]|[FP2&3]|[FP6<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP6<<2]
	movl	$ 0x1c43e0be, LOADD|[[FP4|1]<<2]
	movl	$ 0x40240367, LOADD|[FP4<<2]
	movb	%al, ADDD|[[FP6&0x1c]<<5]|[FP6&3]|[FP4<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP4<<2]
	movl	$ 0x60a95cb6, LOADD|[FP1<<2]
	movl	$ 0x40275b01, ADDD|[FP4<<2]

	movl	$ 0x4d6e1288, LOADD|[FP1<<2]
	movl	$ 0x401874cd, MULD|[FP6<<2]
	movl	$ 0x60a95cb2, LOADD|[FP1<<2]
	movl	$ 0x40275b01, ADDD|[FP6<<2]

	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP4<<2]
	movl	(%edx), %ecx
	movl	4(%edx), %edx
	movl	8(%esp), %eax
	addl	%eax, %eax
	jl	eval_negative

	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	%ecx, LOADD|[FP1<<2]
	movl	%edx, ADDD|[FP2<<2]
	ret

	.align	2
eval_negative:
	xorl	$ 0x80000000, %edx
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	%ecx, LOADD|[FP1<<2]
	movl	%edx, SUBD|[FP2<<2]
	ret

	.align	2
atan_8_t:
	.long	0x873b4988, 0x3fe7c2a3

	.align	2
atan_10:
	movl	$ 0x3fa69356, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_10_t, %edx
	jmp	common_evaluate

	.align	2
atan_10_t:
	.long	0xa9fd6ebe, 0x3fed4cad

	.align	2
atan_12:
	movl	$ 0x3ff49b1a, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_12_t, %edx
	jmp	common_evaluate

	.align	2
atan_12_t:
	.long	0x5ea9d1ce, 0x3ff16b46

	.align	2
atan_13:
	movl	$ 0x4019441a, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_13_t, %edx
	jmp	common_evaluate

	.align	2
atan_13_t:
	.long	0xe18ed09e, 0x3ff2cdbf

	.align	2
atan_14:
	movl	$ 0x40487a02, CVTDS|[FP6<<2]
	movb	%al, ABSD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, DIVD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	$ atan_14_t, %edx
	jmp	common_evaluate

	.align	2
atan_14_t:
	.long	0x01f54b46, 0x3ff43044
