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

/* $Log: sqrt.s,v $
 */
	.ascii	"$Header: sqrt.s 1.4 1991/05/21 23:36:43 $"
	.file	"sqrt.s"

/       Copyright (c) 1984 AT&T
/         All Rights Reserved

/       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/       The copyright notice above does not evidence any
/       actual or intended publication of such source code.

	.ident	"@(#)sqrt.s	1.3"


/	Double precision square-root routine.

	
/	.string "(c) Copyright Weitek 1987.  All rights reserved."

#include "1167defs.h"
#include "PROF.h"

ENTRY(sqrt)
	movl	8(%esp), %eax
	andl	$0x7ff00000, %eax
	cmpl	$[0x7ff << 20], %eax
	je	nanorinf
	movl	4(%esp), %eax
	movl	%eax, LOADD|[[FP4|1]<<2]
	movl	8(%esp), %eax
	andl	$ 0x001fffff, %eax
	orl	$ 0x3fe00000, %eax
	movl	%eax, LOADD|[FP4<<2]
	andl	$ 0x001f0000, %eax
	shrl	$ 13, %eax

	movb	%al, ABSD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]		
	movl	p3(%eax), %ecx
	movl	p3+4(%eax), %edx
	movl	%ecx, LOADD|[FP1<<2]
	movl	%edx, MULD|[FP2<<2]
	movl	p2(%eax), %ecx
	movl	p2+4(%eax), %edx
	movl	%ecx, LOADD|[FP1<<2]
	movl	%edx, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	p1(%eax), %ecx
	movl	p1+4(%eax), %edx
	movl	%ecx, LOADD|[FP1<<2]
	movl	%edx, ADDD|[FP2<<2]
	movb	%al, MULD|[[FP4&0x1c]<<5]|[FP4&3]|[FP2<<2]
	movl	p0(%eax), %ecx
	movl	p0+4(%eax), %edx
	movl	%ecx, LOADD|[FP1<<2]
	movl	%edx, ADDD|[FP2<<2]
	movb	%al, LOADD|[[[FP2|1]&0x1c]<<5]|[[FP2|1]&3]|[[FP6|1]<<2]
	movb	%al, LOADD|[[FP2&0x1c]<<5]|[FP2&3]|[FP6<<2]
	movb	%al, DIVD|[[FP4&0x1c]<<5]|[FP4&3]|[FP6<<2]		

	movl	8(%esp), %eax
	sarl	$ 1, %eax
	jle	not_positive
	andl	$ 0xffe00000>>1, %eax
	addl	$ 0x1ff00000, %eax
/	movl	$ 0, %ecx
	xorl	%ecx,%ecx

	movb	%al, ADDD|[[FP6&0x1c]<<5]|[FP6&3]|[FP2<<2]
	movl	%ecx, LOADD|[FP1<<2]
	movl	%eax, MULD|[FP2<<2]
	ret

nanorinf:
	movl    8(%esp), %eax
	andl	$0x000fffff, %eax
	orl	4(%esp), %eax
	jz	infinity
	wloadl	4(%esp), %fp2
	pushl	$nancode
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
	testl	$0x7fffffff, 8(%esp)
	jne	negative
	wloadl	zero,%fp2
	ret

negative:
	pushl	$ecode		/* address of error code to stack */
	leal	4(%esp), %eax
	pushl	%eax		/* address of pc to stack */
	call	_MATHERR	/* print message and location */
	addl	$8, %esp	/* clean up stack */
	ret

nancode:.long	1
ecode:	.long	2
infcode:.long	3
zero:	.double	0Dx0000000000000000

	.align	2
p0:
	.long	0x187e1686, 0x3fccb721, 0x726d4674, 0x3fcd92d5
	.long	0x2010b101, 0x3fce6854, 0xef1a35d1, 0x3fcf381c
	.long	0xb5c5a333, 0x3fd0014f, 0xd17b0929, 0x3fd0641e
	.long	0x1f4eacda, 0x3fd0c4a7, 0x312ae752, 0x3fd1230f
	.long	0x81d802e4, 0x3fd17f79, 0x0e600129, 0x3fd1da05
	.long	0xcab1337f, 0x3fd232cd, 0x13a3d394, 0x3fd289ed
	.long	0xf316e90b, 0x3fd2df79, 0x74d4c0f7, 0x3fd33389
	.long	0xd63ab71d, 0x3fd3862e, 0xd70988b9, 0x3fd3d77b
	.long	0x5dcb33cc, 0x3fd44e08, 0x206ca1ea, 0x3fd4e963
	.long	0xd103f0fd, 0x3fd58059, 0xd19465ef, 0x3fd61346
	.long	0xff2573f3, 0x3fd6a278, 0xafc025e7, 0x3fd72e35
	.long	0x2a9c64c8, 0x3fd7b6ba, 0xfdbd977e, 0x3fd83c3c
	.long	0xe782acaa, 0x3fd8beee, 0xbbaa9b27, 0x3fd93efb
	.long	0x05a36206, 0x3fd9bc8b, 0xa215aeb1, 0x3fda37c0
	.long	0x29bd6377, 0x3fdab0bd, 0x54189d11, 0x3fdb279e
	.long	0x68618bd6, 0x3fdb9c7f, 0x6b5793a3, 0x3fdc0f79

	.align	2
p1:
	.long	0xd499ec2a, 0x3ff4e4d9, 0x6de150c0, 0x3ff449a6
	.long	0xd6a5acae, 0x3ff3bb39, 0x6ad013df, 0x3ff337ea
	.long	0x40ef62df, 0x3ff2be58, 0x8324a43e, 0x3ff24d5d
	.long	0xb4d874d3, 0x3ff1e402, 0xa588dc6f, 0x3ff18175
	.long	0x8b45d396, 0x3ff12502, 0x9045edb1, 0x3ff0ce0e
	.long	0x8f022c49, 0x3ff07c13, 0x9b45cd42, 0x3ff02e9c
	.long	0xa2419559, 0x3fefca86, 0x19bae5f5, 0x3fef3f5b
	.long	0x4476f902, 0x3feebb17, 0x87e245fe, 0x3fee3d2d
	.long	0x21744694, 0x3fed8c6b, 0x6eaa68e1, 0x3fecb0ee
	.long	0x5cb90169, 0x3febe783, 0xf528570e, 0x3feb2dcf
	.long	0x8e5a2a76, 0x3fea81e2, 0x9bc1c0c9, 0x3fe9e21b
	.long	0x1db83fdb, 0x3fe94d1d, 0xd13aac79, 0x3fe8c1bd
	.long	0x78810efb, 0x3fe83eff, 0x1a387a76, 0x3fe7c407
	.long	0xfcd38f78, 0x3fe75016, 0xc75b3621, 0x3fe6e289
	.long	0xa9406ec2, 0x3fe67ace, 0x329d9b84, 0x3fe61866
	.long	0xa82ce9c7, 0x3fe5badf, 0xf9b644ef, 0x3fe561d6

	.align	2
p2:
	.long	0x1a7b0576, 0xbfeb05f5, 0x4e02d53c, 0xbfe8bd27
	.long	0x131a89bf, 0xbfe6c25b, 0xd0ab3f5a, 0xbfe507bb
	.long	0x210361ea, 0xbfe38275, 0x77b83d6a, 0xbfe229f0
	.long	0x76edbac3, 0xbfe0f74a, 0xd8dc441c, 0xbfdfc9db
	.long	0x4786a43f, 0xbfdddc93, 0x95195124, 0xbfdc1f31
	.long	0xe718cc31, 0xbfda8b71, 0x9299268e, 0xbfd91c13
	.long	0x972d05c2, 0xbfd7cca8, 0x7586fda6, 0xbfd6996e
	.long	0xae2c4fe4, 0xbfd57f2f, 0xf5361efc, 0xbfd47b2a
	.long	0x8df4dfff, 0xbfd31bbc, 0xb0d142ed, 0xbfd17e37
	.long	0x74c308a3, 0xbfd017db, 0x7c338dbf, 0xbfcdbdc0
	.long	0x64976a11, 0xbfcb973b, 0x6800f832, 0xbfc9b002
	.long	0xbf6bbd5b, 0xbfc7fe57, 0x33f7e45a, 0xbfc67a56
	.long	0x510ecbb8, 0xbfc51d88, 0x9c3e3ed5, 0xbfc3e299
	.long	0x3f720e66, 0xbfc2c51b, 0x6bcfc03b, 0xbfc1c156
	.long	0x3da8cbb5, 0xbfc0d429, 0x69dc418a, 0xbfbff5d6
	.long	0x9cb9d7aa, 0xbfbe66ae, 0xeabe3161, 0xbfbcf6f5

	.align	2
p3:
	.long	0x3ec37eb8, 0x3fd4f799, 0x4e96efe4, 0x3fd218f8
	.long	0x5b45e41e, 0x3fcf7f89, 0x0b74e8a6, 0x3fcb9cc2
	.long	0x215d92b8, 0x3fc85daf, 0xfa2cd105, 0x3fc5a13c
	.long	0x155467c5, 0x3fc34e29, 0xf5a166fb, 0x3fc150e9
	.long	0x42ab83d7, 0x3fbf346d, 0x87c044a4, 0x3fbc3bf2
	.long	0x1a8fa1f8, 0x3fb9a518, 0x22fb3617, 0x3fb76053
	.long	0xaa052fe6, 0x3fb56104, 0xb1cbd37b, 0x3fb39cd8
	.long	0x77a5f2bc, 0x3fb20b4c, 0x3aed7d0c, 0x3fb0a550
	.long	0x8caa539f, 0x3fada6ef, 0x5e5c1c63, 0x3fa99803
	.long	0xf7650e5a, 0x3fa645c8, 0xa58845aa, 0x3fa3865e
	.long	0x14561682, 0x3fa13ab6, 0x72d23f72, 0x3f9e96d7
	.long	0x23e99590, 0x3f9b4d46, 0x0236413b, 0x3f987d17
	.long	0x5bf82ab1, 0x3f9610ac, 0xd28f4b6c, 0x3f93f6ee
	.long	0x7f568e10, 0x3f922239, 0xf13350f8, 0x3f90878e
	.long	0xf33823f0, 0x3f8e3c04, 0x48ee1d25, 0x3f8bbc8d
	.long	0x48ed9c22, 0x3f8984ad, 0xcc016f8d, 0x3f878a68
