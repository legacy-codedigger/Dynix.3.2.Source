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

	.asciz	"@(#)$Header: tan.s 1.8 1991/05/21 23:54:08 $"

  # "Double Precision Sine, Cosine, and Tangent Functions"
  #
  # Functional Description:
  #	These functions are the top level entry points for the double
  #	precision sine, cosine, and tangent functions. For sine and cosine,
  #	the routines set the sin/cos flag to specify which value should be
  #	computed and then call the routine which actually does the computation.
  #	For the tangent routine, the following algorithm was used. It was
  #	obtained from the original PASCAL library that these routines replace.
  #
  #	IF (abs(arg) .LT. 0.7854)
  #		multiple = 0
  #	ELSE
  #		arg = pi_reduce(arg,multiple)
  #	ENDIF
  #	argsq = arg * arg
  #	result = arg + ((( -0.9640986146520d0) * argsq
  #			  +98.5333590598011d0) * argsq
  #			-1599.1344569510355d0) * argsq * arg /
  #		(((argsq -111.9944155426216d0) * argsq
  #			+2214.5614255216536d0) * argsq
  #			-4797.4033708531468d0)
  #	IF (odd(multiple))
  #		result = -1.0 / result
  #	ENDIF
  #
  # Created:
  #	11/26/86	BS - Quantitative Technology Corporation for
  #				Sequent Computer Systems
  #
  #
	.data
  #Constants
tan_a:	.double	0Dxbfeed9e55681936d	#    -0.9640986146520
tan_b:	.double	0Dx4058a2228e09b7bf	#   +98.5333590598011
tan_c:	.double	0Dxc098fc89af153dab	# -1599.1344569510355
tan_d:	.double	0Dxc05bffa481168c68	#  -111.9944155426216
tan_e:	.double	0Dx40a14d1f732a7d48	# +2214.5614255216536
tan_f:	.double	0Dxc0b2bd67434fee6c	# -4797.4033708531468
argmax:	.double 0Dx41dfffffffc00000	#  2147483647.0
cmpval:	.double 0Dx3fe921ff2248e8a7	#     0.7854
negone:	.double 0Dxbff0000000000000	#     -1.0
zero:	.double 0Dx0000000000000000	#     0.0
one:	.double 0Dx3ff0000000000000	#
  #
  #	sc_flag	%bl			# sine/cosine flag
  #	mult	%edx			# multiple
  #	arg	%fp2			# holds original argument, final result
  #	temp1	%fp4			# temporary register
  #	argsq	%fp6			# holds square of argument
  #	temp12	%fp8			# second temporary register
  #
#include "PROF.h"
ENTRY(tan)
	wstctx  %eax
	orl     $0x2000000,%eax
	wldctx  %eax
	wloadl	4(%esp),%fp2		#get argument
	wabsl	%fp2,%fp4		#take absolute value
	wcmpl	argmax,%fp4		#is abs(argument) <= maximum value?
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	jae	goodtan			#(jge) if valid continue

	pushl	$ecode		/* address of error code to stack */
	leal	4(%esp), %eax
	pushl	%eax		/* address of pc to stack */
	call	_MATHERR	/* print message and location */
	addl	$8, %esp	/* clean up stack */
	ret

ecode:	.long	16
  #
  # valid parameter input
  #
goodtan:
	pushl	%fp8			#save user registers fp8/9
	pushl	%fp9
	movl	$0,%edx			#initialize multiple
	wcmpl	cmpval,%fp4		#abs(agrument) < .7854?
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	ja	tan_noreduce		#(jg) if valid continue
	call	pi_reduce		#go reduce arg.

tan_noreduce:
	wloadl	%fp2,%fp6		#form square
	wmull	%fp6,%fp6		# of arg
	wloadl	%fp6,%fp4		#get tan factor A
	wmull	tan_a,%fp4		#A * argsq
	waddl	tan_b,%fp4		#(A * argsq) + B
	wmull	%fp6,%fp4		#((A * argsq) + B) * argsq
	waddl	tan_c,%fp4		#(((A * argsq) + B) * argsq) + C
	wmull	%fp6,%fp4		#((((A*argsq)+B)*argsq)+C)*argsq
	wmull	%fp2,%fp4		#((((A*argsq)+B)*argsq)+C)*argsq*arg
	wloadl	%fp6,%fp8		#get argsq
	waddl	tan_d,%fp8		#argsq + D
	wmull	%fp6,%fp8		#(argsq + D) * argsq
	waddl	tan_e,%fp8		#((argsq + D) * argsq) + E
	wmull	%fp6,%fp8		#(((argsq + D) * argsq) + E) * argsq
	waddl	tan_f,%fp8		#((((argsq + D) * argsq) + E) * argsq)+F
	wdivl	%fp4,%fp8		#divide two quantities
	waddl	%fp8,%fp2		#add in arg
	testb	$1,%dl			#test for odd multiple
	jz	tanret			#if not odd, return
	wdivl	negone,%fp2		#-1.0 / arg

tanret:
	popl	%fp9			#restore user registers fp8/9
	popl	%fp8
	ret

  # Reduction of input value for double sine/cosine/tangent
  #
  #
  # Functional Description:
  #	This routine reduces the double precision input argument for the
  #	sine/cosine functions. The following algorithm
  #	is used. The algorithm was obtained from the original PASCAL library
  #	that this routine replaces.
  #
  #	single_temp = input argument  "convert double to single precision
  #	upper = single_temp
  #	lower = arg - upper
  #	result = pi_tbl[1] * upper
  #	cutoff = log_base_2(result) - (HALF_WORD_SIZE - 2)
  #	dmultiple = round (result)
  #	result = result - dmultiple
  #	multiple = trunc(dmultiple - trunc(dmultiple/4)*4)
  #	r_lower = 0.0
  #	i = 1
  #	DO WHILE ((log_base_2(result) < cutoff) .AND. (i + 2 < PRECISION))
  #		r_lower = r_lower + (lower * pi_tbl[i] + upper * pi_tbl[i+1])
  #		r_temp = result + r_lower
  #		IF (log_base_2(r_temp) >= (-1))
  #			dmultiple = round(r_temp)
  #			result = result - dmultiple
  #			mod4 = trunc(dmultiple - trunc(dmultiple/4)*4)
  #			multiple = multiple + mod4
  #			r_temp = result +r_lower
  #		ENDIF
  #		r_lower = (result - r_temp) + r_lower
  #		result = r_temp
  #		cutoff = cutoff - HALF_WORD_SIZE
  #		i = i + 1
  #	ENDDO
  #	result = 1.57079632679489662 * (((lower * pi_tbl[i] +
  #					  arg * (pi_tbl[i+1] + pi_tbl[i+2])) +
  #					  r_lower) + result)
  #
  # Inputs:
  # Argument to reduce IN FP2
  #
  # Outputs:
  #	result - Reduced argument
  #	mult - Multiple of reduction
  #
  # Created:
  #	11/26/86	BS - Quantitative Technology Corporation for
  #				Sequent Computer Systems
  #
  #	index	si		index into PI table
  #	cutoff	bx		value used to see if result has been
  #				sufficiently reduced
  #	itemp	ecx		temporary integer register
  #	result	fp2		result of reduction
  #	dmult	fp4		double precision multiple
  #	rtemp	fp6		temporary flt pt register
  #	r_lower	fp8		reduction factor
  #	upper   fp10		upper portion of argument
  #	lower	fp12		lower portion of argument
  #	temp1	fp14		temporary flt pt register
  #	temp12	fp16		temporary flt pt register
  #	argsav	fp18		save the argument
  #
  #	BIAS		1023	exponent bias
  #	HALF_SIZE	23	number of bits in half word
  #	PRECISION	6	number of bits of precision for reduction
  #
	.data

pidiv2:	.double	0Dx3FF921FB54442D18	# PI divided by 2 (1.57079632679489662)

  #ctxrnd	0x013a0000		 rounding mode for context register
  #ctxtrnc	0x073a0000		 truncate mode for context register

four:	.double 0Dx4010000000000000	# 4.0
quart:	.double 0Dx3fd0000000000000	# 0.25
pi_tbl:	.double	0Dx3FE45F3040000000	# 0.636619687080383301e+00
	.double	0Dx3E76E4E440000000	# 8.528719774858473100e-08
	.double	0Dx3CB529F7FFFFFFFF	# 2.937086744604286430e-16
	.double	0Dx3B909D5F3FFFFFFF	# 8.795767286342583120e-22
	.double	0Dx39FF534BFFFFFFFF	# 2.471136637518957480e-29
	.double	0Dx38BDC0DB3FFFFFFF	# 2.238392930622293410e-35
  #
  #
	.text

  #	.globl pi_reduce	(avoid name conflict with dtan's pi_reduce)
pi_reduce:
	pushl	%fp10			#save user registers fp10-19
	pushl	%fp11			#(a string move would be faster)
	pushl	%fp12
	pushl	%fp13
	pushl	%fp14
	pushl	%fp15
	pushl	%fp16
	pushl	%fp17
	pushl	%fp18
	pushl	%fp19
	wloadl	%fp2,%fp18		#save arg in argsav
	pushl	%ebx			#save sine/cosine flag
	pushl	%esi			#esi = index into PI table
	wstctx	%ecx			#save CTX reg contents
	pushl	%ecx
	wcvtsl	%fp2,%fp6		#compute upper portion
	wcvtls	%fp6,%fp10		# of argument
	wloadl	%fp10,%fp12		#compute lower portion
	wsubl	%fp2,%fp12		# of argument
	wloadl	%fp10,%fp2		#initialize result
	xorl	%esi,%esi		#initialize PI table index
	wmull	pi_tbl,%fp2		#result = result*PI_table[1]
	wabsl	%fp2,%fp14		#get exponent of result
	wstors	%fp14,%ebx		#
	shrl	$20,%ebx		#and form cutoff
	subw	$1044,%bx		#cutoff = cutoff - (BIAS+(HALF_SIZE-2))
	movl	$0x013a0000,%ecx	#round context word
	wldctx	%ecx			#load round context word
	wfixl	%fp2,%fp16		#round result
	wstors	%fp16,%edx		#
	wfloatl %fp16,%fp4		#dmult = round(result)
	wnegl	%fp4,%fp16		#get -dmult
	waddl	%fp16,%fp2		#result = result - dmult
	wloadl	%fp4,%fp6		#
	wmull	quart,%fp6		#dmult/4
	movl	$0x073a0000,%ecx	#get truncate word
	wldctx	%ecx			#put truncate mode in CTX reg
	wfixl	%fp6,%fp16		#
	wfloatl %fp16,%fp6		#trunc(dmult/4)
	wmull	four,%fp6		#trunc(dmult/4) * 4
	wsubl	%fp4,%fp6		#dmult-(trunc(dmult/4) * 4)
	wfixl	%fp6,%fp16		#mult=trunc(dmult-(trunc(dmult/4) * 4))
	wstors	%fp16,%edx		#
	wloadl  zero,%fp8		#initialize r_lower
  #
  # begin main loop
  #
loopr:
	cmpw	$3,%si			#is reduction complete?
	jg	done			#if reduced enough exit
	wabsl	%fp2,%fp14		#get exponent of result
	wstors	%fp14,%ecx		#
	shrl	$20,%ecx		#
	subw	$1023,%cx		#remove BIAS
	cmpw	%cx,%bx			#have we reduced enough
	jl	done			#if so compute final result
  #
  # r_lower = r_lower + (lower * pi_table[i]) + (upper * pi_table[i+1])
  #
	wloadl	%fp12,%fp6		#compute pi_table[index] * lower
	wmull	pi_tbl(,%si,8),%fp6	#
	incw	%si			#increment index
	wloadl	%fp10,%fp4		#compute pi_table[index] * upper
	wmull	pi_tbl(,%si,8),%fp4	#
	waddl	%fp4,%fp6		#add results of multiplies
	waddl	%fp6,%fp8		# and update r_lower
	wloadl	%fp8,%fp6		#rtemp = r_lower + result
	waddl	%fp2,%fp6		#
	wabsl	%fp6,%fp14		#get exponent of rtemp
	wstors	%fp14,%ecx		#
	shrl	$20,%ecx		#
	subw	$1023,%cx		#remove BIAS
	cmpw	$-1,%cx			#exponent < -1?
	jle	continue		#if so don't update multiple,result
	movl	$0x013a0000,%ecx	#round context word
	wldctx	%ecx			#load round context word
	wfixl	%fp6,%fp16		#
	wfloatl %fp16,%fp4		#dmult = round(rtemp)
	wnegl	%fp4,%fp16		#get -dmult
	waddl	%fp16,%fp2		#result = result - dmult
	wloadl	%fp4,%fp6		#
	wmull	quart,%fp6		#dmult/4
	movl	$0x073a0000,%ecx	#truncate context word
	wldctx	%ecx			#load truncate
	wfixl	%fp6,%fp16		#
	wfloatl	%fp16,%fp6		#trunc(dmult/4)
	wmull	four,%fp6		#trunc(dmult/4) * 4
	wsubl	%fp4,%fp6		#dmult - (trunc(dmult/4) * 4)
	wfixl	%fp6,%fp16		#trunc(dmult - (trunc(dmult/4) * 4))
	wstors	%fp16,%ecx		#
	addl	%ecx,%edx		#mult = mult +
					# trunc(dmult - (trunc(dmult/4) * 4))
	wloadl	%fp8,%fp6		#rtemp = r_lower + result
	waddl	%fp2,%fp6		#
continue:
	wloadl	%fp6,%fp16		#save rtemp
	wsubl	%fp2,%fp6		#r_lower = r_lower + (result - rtemp)
	waddl	%fp6,%fp8		#
	wloadl	%fp16,%fp2		#result = rtemp
	subw	$23,%bx			#update boundary
	jmp	loopr			#

done:
	movl	$0x013a0000,%ecx	#round context word
	wldctx	%ecx			#put rounding mode back to normal
	wloadl	%fp12,%fp4		#get lower
	wmull	pi_tbl(,%si,8),%fp4	#compute pi_table[index] * lower
	incw	%si			#increment index
	wloadl	pi_tbl(,%si,8),%fp6	#compute pi_table[i+1] * pi_table[i+2]
	incw	%si			#increment index
	waddl	pi_tbl(,%si,8),%fp6	#
	wmull	%fp18,%fp6		#arg * (pi_table[i+1] + pi_table[i+2])
	waddl	%fp4,%fp6		#add pi_table[i] * lower
	waddl	%fp8,%fp6		#add r_lower
	waddl	%fp6,%fp2		#add result
	wmull	pidiv2,%fp2		#multiply by pi/2
	popl	%ecx			#get old value of CTX reg
	wldctx	%ecx			#restore old CTX value
	popl	%esi
	popl	%ebx			#restore sine/cosein flag
	popl	%fp19			#restore user registers fp10-19
	popl	%fp18
	popl	%fp17
	popl	%fp16
	popl	%fp15
	popl	%fp14
	popl	%fp13
	popl	%fp12
	popl	%fp11
	popl	%fp10
	ret				#
