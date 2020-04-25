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

/* $Log:	atan2.s,v $
 */
	.ascii	"$Header: atan2.s 1.1 87/06/16 $"
	.file	"atan2.s"

/       Copyright (c) 1984 AT&T
/         All Rights Reserved

/       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/       The copyright notice above does not evidence any
/       actual or intended publication of such source code.

	.ident	"@(#)atan2.s	1.1"


/	Double precision 2-argument arctangent routine.

	
/	.string "(c) Copyright Weitek 1987.  All rights reserved."

#include "1167defs.h"
#include "PROF.h"

ENTRY(atan2)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	%edx, LOADD|[[FP2|1]<<2]
	movl	%eax, LOADD|[FP2<<2]
	movl	12(%esp), %edx
	movl	16(%esp), %ecx
	movl	%edx, LOADD|[[FP4|1]<<2]
	movl	%ecx, LOADD|[FP4<<2]
	orl	%eax, %eax
	jle	y_non_pos
	orl	%ecx, %ecx
	jl	y_pos_x_neg
	je	y_pos_x_zero
	cmpl	%eax, %ecx
	jl	upper

	subl	%eax, %ecx			
	cmpl	$ 0x7fe00000, %ecx
	jge	right_1
	movb	%al, DIVD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	STORD|[[FP4|1]<<2], %eax
	movl	STORD|[FP4<<2], %ecx
	pushl	%ecx
	pushl	%eax
	call	_atan
	addl	$ 8, %esp
	ret

	.align	2
right_1:
	movl	$ 0x00000000, LOADD|[[FP2|1]<<2]
	movl	$ 0x00000000, LOADD|[FP2<<2]
	ret

y_pos_x_neg:
	subl	$ 0x80000000, %ecx
	cmpl	%eax, %ecx
	jg	left_upper

upper:
	subl	%ecx, %eax			
	cmpl	$ 0x7fe00000, %eax
	jge	upper_1
	movb	%al, DIVD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	STORD|[[FP2|1]<<2], %eax
	movl	STORD|[FP2<<2], %ecx
	pushl	%ecx
	pushl	%eax
	call	_atan
	addl	$ 8, %esp
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x3ff921fb, SUBD|[FP2<<2]
	ret

	.align	2
upper_1:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x3ff921fb, LOADD|[FP2<<2]
	ret

	.align	2
y_pos_x_zero:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x3ff921fb, LOADD|[FP2<<2]
	ret

	.align	2
left_upper:
	subl	%eax, %ecx			
	cmpl	$ 0x7fe00000, %ecx
	jge	left_upper_1
	movb	%al, DIVD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	STORD|[[FP4|1]<<2], %eax
	movl	STORD|[FP4<<2], %ecx
	pushl	%ecx
	pushl	%eax
	call	_atan
	addl	$ 8, %esp
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x400921fb, ADDD|[FP2<<2]
	ret

	.align	2
left_upper_1:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x400921fb, LOADD|[FP2<<2]
	ret

	.align	2
y_non_pos:
	je	y_zero
	orl	%ecx, %ecx
	jg	y_neg_x_pos
	je	y_neg_x_zero
	cmpl	%eax, %ecx
	jg	left_lower

lower:
	subl	%ecx, %eax			
	cmpl	$ 0x7fe00000, %eax
	jge	lower_1
	movb	%al, DIVD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	STORD|[[FP2|1]<<2], %eax
	movl	STORD|[FP2<<2], %ecx
	pushl	%ecx
	pushl	%eax
	call	_atan
	addl	$ 8, %esp
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x3ff921fb+0x80000000, SUBD|[FP2<<2]
	ret

	.align	2
lower_1:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x3ff921fb+0x80000000, LOADD|[FP2<<2]
	ret

	.align	2
y_neg_x_zero:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x3ff921fb+0x80000000, LOADD|[FP2<<2]
	ret

	.align	2
y_zero:
	orl	%ecx, %ecx
	jl	minus_pi
	je	bad_arguments

	movl	$ 0x00000000, CVTDS|[FP2<<2]
	ret

	.align	2
minus_pi:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x400921fb, LOADD|[FP2<<2]
	ret

	.align	2
left_lower:
	subl	%eax, %ecx			
	cmpl	$ 0x7fe00000, %ecx
	jge	left_lower_1
	movb	%al, DIVD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	STORD|[[FP4|1]<<2], %eax
	movl	STORD|[FP4<<2], %ecx
	pushl	%ecx
	pushl	%eax
	call	_atan
	addl	$ 8, %esp
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x400921fb+0x80000000, ADDD|[FP2<<2]
	ret

	.align	2
left_lower_1:
	movl	$ 0x54442d18, LOADD|[[FP2|1]<<2]
	movl	$ 0x400921fb+0x80000000, LOADD|[FP2<<2]
	ret

	.align	2
y_neg_x_pos:
	subl	$ 0x80000000, %eax
	cmpl	%eax, %ecx
	jl	lower

	subl	%eax, %ecx			
	cmpl	$ 0x7fe00000, %ecx
	jge	right_2
	movb	%al, DIVD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	movl	STORD|[[FP4|1]<<2], %eax
	movl	STORD|[FP4<<2], %ecx
	pushl	%ecx
	pushl	%eax
	call	_atan
	addl	$ 8, %esp
	ret

	.align	2
right_2:
	movl	$ 0x00000000, LOADD|[[FP2|1]<<2]
	movl	$ 0x00000000, LOADD|[FP2<<2]
	ret

bad_arguments:
	movl	$ 0x00000000, CVTDS|[FP2<<2]
	ret
