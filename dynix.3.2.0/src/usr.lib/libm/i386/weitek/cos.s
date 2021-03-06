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

/* $Log:	cos.s,v $
 */
	.ascii	"$Header: cos.s 1.2 90/04/19 $"
	.file	"cos.s"

/       Copyright (c) 1984 AT&T
/         All Rights Reserved

/       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/       The copyright notice above does not evidence any
/       actual or intended publication of such source code.

	.ident	"@(#)cos.s	1.1"


/	Double precision cosine routine.

	
/	.string "(c) Copyright Weitek 1987.  All rights reserved."

#include "1167defs.h"
#include "PROF.h"

ENTRY(cos)
	wstctx  %eax
	orl     $0x2000000,%eax
	wldctx  %eax
	movl	4(%esp), %eax
	movl	%eax, LOADD|[FP1<<2]
	movl	8(%esp), %eax
	movl	%eax, ABSD|[FP4<<2]
	andl	$ 0x7fffffff, %eax
	cmpl	$ 0x3fe921fb, %eax
	jg	cos1
	cmpl	$ 0x3e400000, %eax
	jl	small_arg
	
cos_mod_0:
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movb	%al, CVTSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movl	$ 0x85a1feff, LOADD|[[FP2|1]<<2]
	movl	$ 0xbe927df1, LOADD|[FP2<<2]
	movl	$ 0x310dc01e, MACDS|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x78f60d09, LOADD|[FP1<<2]
	movl	$ 0x3efa019f, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x16338b22, LOADD|[FP1<<2]
	movl	$ 0xbf56c16c, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x5554ddb9, LOADD|[FP1<<2]
	movl	$ 0x3fa55555, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0xffffff68, LOADD|[FP1<<2]
	movl	$ 0xbfdfffff, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	ret

	.align	2
cos1:
	cmpl	$ 0x4002d97c, %eax
	jg	cos2

	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x3ff921fb+0x80000000, ADDD|[FP4<<2]
	movl	$ 0x33145c07, LOADD|[FP1<<2]
	movl	$ 0x3c91a626+0x80000000, ADDD|[FP4<<2]
	
cos_mod_1:
	movb	%al, NEGD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movl	$ 0xb2d72f15, LOADS|[FP2<<2]
	movb	%al, CVTSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP1<<2]
	movl	$ 0x2f2ec1ee, MACS|[FP1<<2]
	movb	%al, MULS|[[FP1&0x1c]<<5]|[FP1&3]|[FP2<<2]
	movb	%al, CVTDS|[[FP2&0x1c]<<5]|[FP2&3]|[FP2<<2]
	movl	$ 0x51e84127, LOADD|[FP1<<2]
	movl	$ 0x3ec71de3, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x19b92304, LOADD|[FP1<<2]
	movl	$ 0xbf2a01a0, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x1110f306, LOADD|[FP1<<2]
	movl	$ 0x3f811111, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x55555543, LOADD|[FP1<<2]
	movl	$ 0xbfc55555, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, ADDD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	ret
	
	.align	2
cos2:
	cmpl	$ 0x400f6a7a, %eax
	jle	cos3
	cmpl	$ 0x4015fdbb, %eax
	jle	cos4
	cmpl	$ 0x401c463a, %eax
	jg	cos5

	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x401921fb+0x80000000, ADDD|[FP4<<2]
	movl	$ 0x33145c07, LOADD|[FP1<<2]
	movl	$ 0x3cb1a626+0x80000000, ADDD|[FP4<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movb	%al, CVTSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movl	$ 0x85a1feff, LOADD|[[FP2|1]<<2]
	movl	$ 0xbe927df1, LOADD|[FP2<<2]
	movl	$ 0x310dc01e, MACDS|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x78f60d09, LOADD|[FP1<<2]
	movl	$ 0x3efa019f, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x16338b22, LOADD|[FP1<<2]
	movl	$ 0xbf56c16c, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x5554ddb9, LOADD|[FP1<<2]
	movl	$ 0x3fa55555, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0xffffff68, LOADD|[FP1<<2]
	movl	$ 0xbfdfffff, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000, ADDD|[FP2<<2]
	ret
	
	.align	2
cos3:
	movl	$ 0x54442d18, LOADD|[FP1<<2]
	movl	$ 0x400921fb+0x80000000, ADDD|[FP4<<2]
	movl	$ 0x33145c07, LOADD|[FP1<<2]
	movl	$ 0x3ca1a626+0x80000000, ADDD|[FP4<<2]
	
cos_mod_2:
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movb	%al, CVTSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movl	$ 0x85a1feff, LOADD|[[FP2|1]<<2]
	movl	$ 0xbe927df1, LOADD|[FP2<<2]
	movl	$ 0x310dc01e, MACDS|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x78f60d09, LOADD|[FP1<<2]
	movl	$ 0x3efa019f, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x16338b22, LOADD|[FP1<<2]
	movl	$ 0xbf56c16c, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x5554ddb9, LOADD|[FP1<<2]
	movl	$ 0x3fa55555, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0xffffff68, LOADD|[FP1<<2]
	movl	$ 0xbfdfffff, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3ff00000+0x80000000, SUBD|[FP2<<2]
	ret
	
	.align	2
cos4:
	movl	$ 0x7f3321d2, LOADD|[FP1<<2]
	movl	$ 0x4012d97c+0x80000000, ADDD|[FP4<<2]
	movl	$ 0x4c9e8a0a, LOADD|[FP1<<2]
	movl	$ 0x3caa7939+0x80000000, ADDD|[FP4<<2]
	
cos_mod_3:
	movb	%al, LOADD|[[[FP4|1]&0x1c]<<5]|[[FP4|1]&3]|[[FP6|1]<<2]
	movb	%al, LOADD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movl	$ 0xb2d72f15, LOADS|[FP2<<2]
	movb	%al, CVTSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP1<<2]
	movl	$ 0x2f2ec1ee, MACS|[FP1<<2]
	movb	%al, MULS|[[FP1&0x1c]<<5]|[FP1&3]|[FP2<<2]
	movb	%al, CVTDS|[[FP2&0x1c]<<5]|[FP2&3]|[FP2<<2]
	movl	$ 0x51e84127, LOADD|[FP1<<2]
	movl	$ 0x3ec71de3, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x19b92304, LOADD|[FP1<<2]
	movl	$ 0xbf2a01a0, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x1110f306, LOADD|[FP1<<2]
	movl	$ 0x3f811111, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	$ 0x55555543, LOADD|[FP1<<2]
	movl	$ 0xbfc55555, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movb	%al, MULD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movb	%al, ADDD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	ret

	.align	2
cos5:
	movb	%al, ABSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	cmpl	$ 0x41900000, %eax
	movl	$ 0x6dc9c883, LOADD|[FP1<<2]
	movl	$ 0x3ff45f30, MULD|[FP4<<2]
	jge	out_of_range
	movb	%al, FIXD|[[FP4&0x1c]<<5]|[FP4&3]|[FP4<<2]
	movl	STORS|[FP4<<2], %eax
	incl	%eax
	sarl	$ 1, %eax
	movl	%eax, FLOATD|[FP4<<2]
	movl	$ 0x54000000, LOADD|[FP1<<2]
	movl	$ 0x3ff921fb, MULD|[FP4<<2]
	movl	%eax, %ecx
	movb	%al, SUBD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	andl	$ 3, %ecx
	movl	%eax, FLOATD|[FP2<<2]
	movl	$ 0x00000000, LOADD|[FP1<<2]
	movl	$ 0x3e110b46, MULD|[FP2<<2]
	sall	$ 2, %ecx
	movb	%al, SUBD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	%eax, FLOATD|[FP4<<2]
	movl	$ 0x33145c07, LOADD|[FP1<<2]
	movl	$ 0x3c91a626, MULD|[FP4<<2]
	movl	cos_table(%ecx), %ecx
	movb	%al, SUBD|[[FP2&0x1c]<<5]|[FP2&3]|[FP4<<2]
	jmp	* %ecx

	.align	2
cos_table:
	.long	cos_mod_0, cos_mod_1, cos_mod_2, cos_mod_3

small_arg:
	movl	$ 0x3f800000, CVTDS|[FP2<<2]
	ret

ecode: .long	4

out_of_range:
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	ret
