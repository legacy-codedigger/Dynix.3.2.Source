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

	.asciz	"@(#)$Header: sinhcosh.s 1.8 1991/05/21 23:50:28 $"

  # "Double Precision Hyperbolic Sine, Cosine, and Tangent Functions"
  #
  # 
  # Functional Description:
  # 	These functions are the top level entry points for the double
  # 	precision hyperbolic sine, cosine, and tangent functions. 
  #	For hyperbolic sine and cosine, the routines set the sin/cos flag 
  # 	to specify which value should be computed and then call the routine 
  # 	which actually does the computation. For the hyperbolic tangent, 
  # 	the following algorithm was used. It was obtained from the original
  # 	PASCAL library that these routines replace.
  # 
  # 	IF (abs(arg) > 19.1)
  # 		return error and NAN
  #	ENDIF
  # 	IF (arg == 0.0)
  # 		result = 1.0 and return
  #	ENDIF
  #	IF (arg < 0.0)
  #	    signf = 1
  #	ELSE
  #	    signf = 0
  #	ENDIF
  #	x = abs(arg)
  #	IF (x < 0.75)
  #	    xsq = x * x
  #	    result =  x + ((((-0.96445016240077) * xsq
  # 			+    -99.41819453693073) * xsq
  #			+  -1617.38247611662225) * xsq * x /
  #		   ((( xsq + 112.95299835936588) * xsq
  #			+   2239.11355495022451) * xsq
  #			+   4852.14742834988424)
  #	ELSE  /* x >= 0.75 */
  #	    call exp_reduce(x + x)
  #	    temp = (q+p) * 2^mult
  #	    result = 1.0 - 2.0 * (q-p)/(temp + (q-p))
  #	ENDIF
  #	IF (signf == 1)
  #	    result = - result
  # 	ENDIF
  # 
  # Created:
  # 	12/19/86 	BS - Quantitative Technology Corporation for
  # 				Sequent Computer Systems
  #
  #
	.text
  #Constants
tanh_a:	.double	0Dxbfeedcc696444324	#      -0.96445016240077
tanh_b:	.double	0Dxc058dac3b304deef 	#     -99.41819453693073
tanh_c:	.double	0Dxc0994587a7d1b193	#   -1617.38247611662225
tanh_d:	.double	0Dx405c3cfdecd4a78f 	#     112.95299835936588
tanh_e:	.double	0Dx40a17e3a23dfdb06 	#    2239.11355495022451
tanh_f:	.double	0Dx40b2f425bddd4541 	#    4852.14742834988424
  #
tanhmx:	.double 0Dx4033800000000000	#    19.5 for Cody tests
  #tanhmx:	.double 0Dx4033199999999999	#    19.1
schmax:	.double 0Dx4086300000000000	#   710.0
cmpval:	.double 0Dx3fe8000000000000	#     0.75
negone:	.double 0Dxbff0000000000000	#    -1.0
zero:	.double 0Dx0000000000000000	#     0.0
two:	.double 0Dx4000000000000000	#     2.0
one:	.double 0Dx3ff0000000000000	#     1.0
half:	.double 0Dx3fe0000000000000	#     0.5
pos_inf:	.double 0Dx7ff0000000000000
neg_inf:	.double 0Dxfff0000000000000
  #
  #       signf	%bh		 sign of argument
  #     sc_flag	%bl		 sine/cosine flag
  #	 mult	%edx		 multiple
  #      result	%fp2		 holds original argument, final result
  #	    x	%fp4		 holds abs(arg) 
  #	  xsq	%fp6		 holds square of argument
  #	    p	%fp8		 reduction result
  #           q	%fp10		 reduction result
  #        temp	%fp12		 temporary register
  #
#include "PROF.h"
ENTRY(sinh)
	pushl	%ebx			#save ebx
	movl	$1,%ebx			#set flag to perform sine
	jmp	dsinhcosh		#go perform sine

ENTRY(cosh)
	pushl	%ebx			#save ebx
	xorl	%ebx,%ebx		#set flag to perform cosine
	jmp	dsinhcosh		#go perform cosine
  #	
  # "Compute Double Hyberbolic Sine/Cosine Functions"
  #
  # Functional Description:
  # 	This routine actually computes the hyperbolic sine/cosine function. 
  # 	It checks to see that the argument does not exceed the maximum value
  # 	If it does, an error is issued. If not, a check is made to see if
  # 	the input argument needs to be reduced. If so, the reduction
  # 	routine is called. Finally, the hyperbolic sine/cosine is computed. 
  # 	The following algorithm is used. It was obtained from the 
  # 	original PASCAL library that these routines replace.
  # 
  #	IF (sinh) 
  #	    sc_flag = 1
  #	ELSE
  #	    sc_flag = 0
  #	ENDIF
  #	IF (abs(arg) > 710.0)
  #	    IF (sc_flag == 1)
  #		result = arg
  #	    ELSE
  #		return error and INF
  #	    ENDIF
  #	ENDIF
  #	IF (arg < 0.0)
  #	    signf = 1
  #	ENDIF
  # 
  # 	x = abs(arg)
  # 	IF (x < 0.75)
  # 	    xsq = x * x
  # 	    IF (sc_flag ==1)
  # 		result = arg + arg *
  # 		(((((( 0.000000000162095726) * xsq
  # 		    +  0.000000025050948590) * xsq
  # 		    +  0.000002755732364785) * xsq
  # 		    +  0.000198412698326202) * xsq
  # 		    +  0.008333333333341372) * xsq
  # 		    +  0.166666666666666387) * xsq
  #	    ELSE
  # 		result = 1.0 +
  # 		(((((( 0.000000002110133500) * xsq
  # 		    +  0.000000275556007731) * xsq
  # 		    +  0.000024801593801248) * xsq
  # 		    +  0.001388888887631267) * xsq
  # 		    +  0.041666666666782032) * xsq
  #		    +  0.499999999999996056) * xsq
  #	    ENDIF
  #	ELSE		/* x >= 0.75 */
  #	    call exp_reduce
  # 	    x = p/(q-p)
  #	    IF (mult < 28)
  #		temp = (0.5/(x+0.5))*2^(-2*mult-1)
  #		IF (sc_flag == 1)
  #		    x = x - temp
  #		ELSE
  #		    x = x + temp
  #		ENDIF
  #	    ENDIF
  #	    result = (0.5 + x) * 2^mult
  #	    IF (sc_flag == 1)
  #		IF (signf == 1)
  #		    result = -result 
  #		ENDIF
  #	    ENDIF
  #	ENDIF
  #
  # Input:
  # 	4(%esp) - argument
  # 
  # Output:
  # 	%fp2 - double precision sine/cosine of argument
  # 
  # Created:
  # 	12/20/86	BS - Quantitative Technology Corporation for
  # 				Sequent Computer Systems
  #
sinh_a:	.double	0Dx3de6473c809eda4b	# 0.000000000162095726
sinh_b:	.double	0Dx3e5ae5f3caf0d8a6	# 0.000000025050948590
sinh_c:	.double	0Dx3ec71de3e3996da7	# 0.000002755732364785
sinh_d:	.double	0Dx3f2a01a019d0eea7	# 0.000198412698326202
sinh_e:	.double	0Dx3f8111111111232b	# 0.008333333333341372
sinh_f:	.double	0Dx3fc555555555554b	# 0.166666666666666387
cosh_a:	.double	0Dx3e22203b8e37b8d4	# 0.000000002110133500
cosh_b:	.double 0Dx3e927e04236f2b8d	# 0.000000275556007731
cosh_c:	.double	0Dx3efa01a08c597eb1	# 0.000024801593801248
cosh_d:	.double	0Dx3f56c16c1668ecc9	# 0.001388888887631267
cosh_e:	.double 0Dx3fa5555555559647	# 0.041666666666782032
cosh_f:	.double	0Dx3fdfffffffffffb8	# 0.499999999999996056
  #
  #

dsinhcosh:
	wstctx  %eax
	orl     $0x2000000,%eax
	wldctx  %eax
	wloadl	8(%esp),%fp2		#get argument
	wabsl	%fp2,%fp4		#take absolute value
 	wcmpl	schmax,%fp4		#is abs(argument) <= maximum value? 
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	jae	goodarg			#(jge) if valid continue

  #######################################################
  # Error return for sine cosine maximum value exceeded #
  #######################################################
	wloadl	pos_inf,%fp2		#return pos infinity 
	testb	$1,%bl			#check if sinh
	jz	quit			#cosh
sinh_err:
	wloadl  0x8(%esp),%fp4		#load argument
	wtstl   %fp4
	wstctx  %eax			#save contents of control reg.
	sahf
	jnc	quit			#positive, no work to do
	wloadl  neg_inf,%fp2 		#return negative infinity.
quit:
	pushl	$ecode		/* address of error code to stack */
	leal	8(%esp), %eax
	pushl	%eax		/* address of pc to stack */
	call	_MATHERR	/* print message and location */
	addl	$8, %esp	/* clean up stack */
	popl	%ebx		/* restore ebx */
	ret

ecode:	.long	20

goodarg:
	wtstl	%fp2			#check for zero
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	setb	%bh			#set signf if negative
	movl	$0,%edx			#initialize multiple
	wcmpl	cmpval,%fp4		#abs(agrument) < .75?
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	jbe	sch_reduce		#(jle) 

 # noreduce:
	wloadl	%fp4,%fp6		#
	wmull	%fp6,%fp6		#form xsq
	wloadl	%fp6,%fp4		#save xsq
	testb	$1,%bl			#check sc_flag
	jz	cosh_comp		#
  #
  # sinh computation
  #
	wmull	sinh_a,%fp4		#A*xsq
	waddl	sinh_b,%fp4		#B+(A*xsq)
	wmull	%fp6,%fp4		#xsq*(B+(A*xsq))
	waddl	sinh_c,%fp4		#C+(xsq*(B+(A*xsq)))
	wmull	%fp6,%fp4		#xsq*(C+(xsq*(B+(A*xsq))))
	waddl	sinh_d,%fp4		#D+(xsq*(C+(xsq*(B+(A*xsq)))))
	wmull	%fp6,%fp4		#xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))
	waddl	sinh_e,%fp4		#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	wmull	%fp6,%fp4		#xsq*(E+
					#(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	waddl	sinh_f,%fp4		#F+(xsq*(E+
					#(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))))
	wmull	%fp6,%fp4		#xsq*(F+xsq*(E+
					#(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))))
	wmull	%fp2,%fp4		#arg*xsq*(F+(xsq*(E+
					#(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))))
	waddl	%fp4,%fp2		#result = arg + arg*xsq*(F+(xsq*(E+
					#(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))))
	jmp	sch_retn		#
  #
  # cosh computation
  #
cosh_comp:
	wmull	cosh_a,%fp4		#A*xsq
	waddl	cosh_b,%fp4		#B+(A*xsq)
	wmull	%fp6,%fp4		#xsq*(B+(A*xsq))
	waddl	cosh_c,%fp4		#C+(xsq*(B+(A*xsq)))
	wmull	%fp6,%fp4		#xsq*(C+(xsq*(B+(A*xsq))))
	waddl	cosh_d,%fp4		#D+(xsq*(C+(xsq*(B+(A*xsq)))))
	wmull	%fp6,%fp4		#xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))
	waddl	cosh_e,%fp4		#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq))))))
	wmull	%fp6,%fp4		#xsq*(
					#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	waddl	cosh_f,%fp4		#F+(xsq*(
					#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	wmull	%fp6,%fp4		#xsq*(F+(xsq*(
					#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	waddl	one,%fp4		#1.0 +xsq*(F+(xsq*(
					#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	wloadl	%fp4,%fp2		#result = 1.0 + xsq*(F+(xsq*(
					#E+(xsq*(D+(xsq*(C+(xsq*(B+(A*xsq)))))))
	jmp 	sch_retn
  #
  # sinh - cosh reduction (x >= 0.75)
  #
sch_reduce:
	pushl	%fp8			#save user regs fp8-17, which we clobber
	pushl	%fp9			#a string move would be faster
	pushl	%fp10
	pushl	%fp11
	pushl	%fp12
	pushl	%fp13
	pushl	%fp14
	pushl	%fp15
	pushl	%fp16
	pushl	%fp17
	call	exp_reduce		#
	wloadl	%fp8,%fp12		#save p 
	wsubl	%fp10,%fp8		#form (q-p)
	wdivl	%fp12,%fp8		#x = p/(q-p)
	cmpw	$28,%dx			#is (mult < 28)?
	jge	cont			#if not, goto cont
	wloadl	%fp8,%fp10		#save x
	waddl	half,%fp10		#form x + 0.5
	wdivl	half,%fp10		#0.5 / (x + 0.5)
	movl 	%edx,%eax		#save mult
	shll	$1,%eax			# 2 * mult
	negw	%ax			# -2 * mult
	decw	%ax			# -2 * mult -1
	wstors	%fp10,%ecx		#get exponent of 0.5 / (x + 0.5)
	rorl	$20,%ecx		#rotate exponent to low portion
	addw	%ax,%cx			#update exponent and
	roll	$20,%ecx		#put it back
	wloads	%ecx,%fp10		#temp = (0.5 / (x + 0.5)) 
					#            * 2^(-2 * mult -1)
	testb	$1,%bl			#check sc_flag
	jz	cosh_add		#if cosh add temp
	wsubl	%fp8,%fp10		#x = x - temp
	jmp	arnd			#
cosh_add:
	waddl	%fp8,%fp10		#x = x + temp
arnd:
	wloadl	%fp10,%fp8		#save x
cont:
	waddl	half,%fp8		#form (0.5 + x)
	wstors	%fp8,%ecx		#get exponent of 0.5 + x 
	rorl	$20,%ecx		#rotate exponent to low portion
	addw	%dx,%cx			#update exponent and
	roll	$20,%ecx		#put it back
	wloads	%ecx,%fp8		#result = (x + 0.5) * 2^mult
	wloadl	%fp8,%fp2		#
	testb	$1,%bl			#sinh?
	jz	sch_restore		#if cosh return

	testb	$1,%bh			#check signf
	je	sch_restore			#sign is positive
	wnegl 	%fp2,%fp2		#result = -result

sch_restore:
	popl	%fp17			#restore user regs
	popl	%fp16
	popl	%fp15
	popl	%fp14
	popl	%fp13
	popl	%fp12
	popl	%fp11
	popl	%fp10
	popl	%fp9
	popl	%fp8

sch_retn:
	popl	%ebx			#restore ebx
	ret
	
  # "Double Precision Exponential Reduction for reduction of DTANH, DSINH, DCOSH"
  #
  # 
  # Functional Description:
  # 	This routine computes the reduction portion of the exponential 
  # 	function. The algorithm that is used was obtained from 
  # 	the original PASCAL library that this routine replaces.
  # 
  # 	single_temp = arg
  # 	upper = single_temp
  # 	lower = arg - upper
  # 	x = upper * LOG_2_UPPER
  # 	mult = round(x)
  # 	x = (x - mult) + (lower * LOG_2_UPPER) + (arg * LOG_2_LOWER)
  # 	xsq = sqr(x)
  # 	p = x * (P0 + xsq * (P1 + xsq * P2))
  # 	q = Q0 + xsq * (Q1 + xsq)
  # 
  # Created:
  # 	12/18/86 	BS - Quantitative Technology Corporation for
  # 				Sequent Computer Systems
	.text
  # 
log2low: .double 0Dx3e8295c17f0bbbe7 	# 1.384689194620474e-07
p0:	.double 0Dx4097a774e9c773d2 	# 1513.86417304653562
p1:	.double 0Dx403433a29c957777	# 20.20170000695313
p2:	.double 0Dx3f97a609aa5cd008	# 0.02309432127295
q0:	.double 0Dx40b11016b314dfb0	# 4368.08867006741699
q1:	.double 0Dx406d25b413b3ffd9	# 233.17823205143104
log2up:	.double	0Dx3FF7154740000000	# 1.44269490242004395
  #
  #		
	.text
  #
exp_reduce:
	wcvtsl	%fp4,%fp12		#compute upper portion
	wcvtls	%fp12,%fp14		#  of argument
	wloadl	%fp14,%fp16		#compute lower portion
	wsubl	%fp4,%fp16		#  of argument
 	wmull	log2up,%fp14		#x = upper * LOG_2_UPPER
	wstctx  %eax
	movl	%eax,%edx
	andl    $ 0x1ffffff,%eax        # round to nearest.
	wldctx  %eax
	wfixl	%fp14,%fp10		#ASSUME rounding mode 
	wldctx  %edx                    #restore  "round to zero" mode
	wstors	%fp10,%edx		#mult = round(x)
	wmull	log2up,%fp16		#lower * LOG_2_UPPER
	wloadl  %fp4,%fp12		#
	wmull	log2low,%fp12		#arg * LOG_2_LOWER
	waddl	%fp16,%fp12		#add the two quantities
	wfloatl	%fp10,%fp10		# form (x - mult)
	wnegl	%fp10,%fp10		#
	waddl	%fp14,%fp10		#
	waddl	%fp12,%fp10		#x = (x - mult) +
	wloadl	%fp10,%fp12		#  (lower * LOG_2_UPPER) +
					#  (arg * LOG_2_LOWER)
	wmull	%fp12,%fp12		#xsq = x * x
	wloadl	%fp12,%fp8		#get xsq
	wmull	p2,%fp8			#xsq*P2
	waddl	p1,%fp8			#P1+(xsq*P2)
	wmull	%fp12,%fp8		#xsq*(P1+(xsq*P2))
	waddl	p0,%fp8			#P0+(xsq*(P1+(xsq*P2)))
	wmull	%fp10,%fp8		#p = x*(P0+(xsq*(P1+(xsq*P2))))
	wloadl	%fp12,%fp10		#get xsq
	waddl	q1,%fp10		#Q1+xsq
	wmull	%fp12,%fp10		#xsq*(Q1+xsq)
	waddl	q0,%fp10		#q = Q0 + (xsq*(Q1+xsq))
	ret

ENTRY(tanh)
	wloadl	4(%esp),%fp2		#get argument
	pushl	%ebx			#save %ebx
	pushl	%fp8			#save user regs fp8-11, which we clobber
	pushl	%fp9
	pushl	%fp10
	pushl	%fp11
	wabsl	%fp2,%fp4		#take absolute value
 	wcmpl	tanhmx,%fp4		#is abs(argument) <= maximum value?
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	jae	zerochk			#(jge) if valid continue

  ##############################################################
  # Error return for hyperbolic tangent maximum value exceeded #
  ##############################################################
	
	wcmpl	%fp2,%fp4		#Return 1.0 or -1.0, depending
	wstctx	%eax			#on sign of arg
	sahf
	jne	ret_negone
	wloadl one,%fp2			#Cody tests only
	jmp tanhret			#Cody tests only
ret_negone:
	wloadl	negone,%fp2
	jmp	tanhret

zerochk:
 	wtstl	%fp2			#is arg = 0?
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	setb	%bh			#set signf
	jne	goodtanh		#if not continue computation
	wloadl	zero,%fp2		#if arg = 0, return a  zero
	jmp 	tanhret			#

  #
  # valid parameter input
  #
goodtanh:
	xorl	%edx,%edx		#initialize multiple
	wcmpl	cmpval,%fp4		#abs(agrument) < .75
	wstctx	%eax			#transfer contents of control reg
	sahf				#receive to flags
	jbe	tanh_reduce		#

tanh_noreduce:	
	wloadl	%fp4,%fp6		#form square
	wmull	%fp6,%fp6		# of x
	wloadl	%fp6,%fp8		#get tan factor A
	wmull	tanh_a,%fp8		#A * xsq
	waddl	tanh_b,%fp8		#(A * xsq) + B
	wmull	%fp6,%fp8		#((A * xsq) + B) * xsq
	waddl	tanh_c,%fp8		#(((A * xsq) + B) * xsq) + C
	wmull	%fp6,%fp8		#((((A*xsq)+B)*xsq)+C)*xsq
	wmull	%fp4,%fp8		#((((A*xsq)+B)*xsq)+C)*xsq*x
	wloadl	%fp6,%fp10		#get xsq
	waddl	tanh_d,%fp10		#xsq + D
	wmull	%fp6,%fp10		#(xsq + D) * xsq
	waddl	tanh_e,%fp10		#((xsq + D) * xsq) + E
	wmull	%fp6,%fp10		#(((xsq + D) * xsq) + E) * xsq
	waddl	tanh_f,%fp10		#((((xsq + D) * xsq) + E) * xsq)+F
	wdivl	%fp8,%fp10		#divide two quantities
	waddl	%fp10,%fp4		#add in x
	wloadl	%fp4,%fp2		#move result 
	jmp 	signchk			#

tanh_reduce:
	pushl	%fp12			#This section of code (including the
	pushl	%fp13			#call to exp_reduce) also clobbers
	pushl	%fp14			#fp12-17
	pushl	%fp15
	pushl	%fp16
	pushl	%fp17
	waddl	%fp4,%fp4		#x + x
	call	exp_reduce		#reduce to compute tanh
	wloadl	%fp8,%fp12		#move p
	waddl	%fp10,%fp12		#p + q
	wstors	%fp12,%ecx		#get exponent of q + p
	rorl	$20,%ecx		#rotate exponent to low portion
	addw	%dx,%cx			#update exponent and
	roll	$20,%ecx		#put it back
	wloads	%ecx,%fp12		#temp = (q+p) * 2^mult
	wsubl	%fp10,%fp8		#form q-p
	waddl 	%fp8,%fp12		#temp + (q-p)
	wmull	two,%fp8		#2.0*(q-p)
	wdivl	%fp8,%fp12		#2.0*(q-p)/(temp + (q-p))
	wnegl	%fp12,%fp2		#-(2.0*(q-p)/(temp + (q-p)))
	waddl	one,%fp2		#1 - (2.0*(q-p)/(temp + (q-p)))
	popl	%fp17			#restore fp12-17
	popl	%fp16
	popl	%fp15
	popl	%fp14
	popl	%fp13
	popl	%fp12

signchk:
	testb	$1,%bh			#check signf
	je	tanhret			#sign is positive
	wnegl 	%fp2,%fp2		#result = -result

tanhret:
	popl	%fp11			#restore fp8-11
	popl	%fp10
	popl	%fp9
	popl	%fp8
	popl	%ebx			#restore %ebx
	ret
