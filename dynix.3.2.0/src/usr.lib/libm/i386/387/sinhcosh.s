/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

	.asciz	"@(#)$Header: sinhcosh.s 1.9 91/01/17 $"

/*		SINH & COSH FUNCTIONS
 *
 * Functional Description:
 *
 *	This routine implements the hyperbolic sine and cosine function for
 *	the C language. Error checking is performed for overflow errors.
 *
 *
 * Input:	Argument is located at 8(%ebp)
 *	        Assumed that the argument is normal.
 *		A check is done to be sure that it will not overflow.
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Assumptions:  ST(0) & ST(1) are scratch stack registers
 *		Input argument is normal.
 *		Registers ax,cx, and dx are scratch registers.
 *
 * Created:
 *	05/29/86	GS - Quantitative Technology Corporation for
 *				Sequent Computer Systems
 *	09/04/86	Phil Hochstetler - converted to C library conventions
 *
 *	05/22/87	Gary Tracy - converted to stack model C interface
 *			  under this model, the caller is responsible for
 *			  saving and restoring any required 387 state.
 *			  this need not be done if:
 *				1.  the callee does no calls.
 *				2.  the sum of 387 registers used by the caller
 *				    and callee does not exceed 8.
 *
 *	08/08/88	Dorsey Drane - check for small values of sinh(x) and
 *			use a polynomial for computing sinh(x) for x < 0.5.
 *			(from 02/23/88 Gary Graunke's fix to plib/dsinhcosh.s)
 *
 *	01/11/89	Dorsey Drane - fix constant used for 2^-27, and fix the
 *			explanation of the algorithm in the comments.
 *
 *	7/18/90		garyg - add tanh function with rational approximation
 *			for small values. Use in-line 80-bit code for exp
 *			as well. Using the polynomial from Cody and Waite,
 *			"A Software Manual for the Elementary Functions",
 *			Prentice Hall, 1980.
 *			Beef up sinh polynomial using 80-bit mode.
 *			Build a stack frame and put in ATT error handling.
 *
 * Method:
 *	Cosh:
 *
 *	1. Replace x by |x|
 *
 *	2. For 2^-27 < x <= ln2/2 : cosh(x) =      [exp(x) - 1]^2
 *					   1 + -------------------
 *						2 * exp(x)
 *
 *          For ln2/2 < x <  ln(2*ovfl) : cosh(x) = exp(x) + 1/exp(x)
 *					     -----------------
 *						     2
 *
 *	3. for x <= 2^-27 : cosh(x) = 1
 *
 *	Sinh:
 *
 *	1. Replace x = |x| since sinh(-x) = - sinh(x)
 *
 *	2. 0.5 <= x <= ln(2*ovfl) : sinh(-x) = (exp(x)-1) + (exp(x)-1)/exp(x)
 *					     ------------------------------
 *							   2
 *	3. x < 2^-10 : sinh(x) = polynomial(x)
 */
#undef	ATTSYSV
#undef	SYSV

#include "PROF.h"

	.set	maskC0,0x0100
one:	.long	0x3f800000
two:	.long	0x40000000
ln_2_times_ovfl:	.double	0Dx408633CE8FB9F87E

sinhp0:	.long	0x99a0c68a,0xabc89ab2,0xc011 # -0.35181283430177117881d6
sinhp1:	.long	0xa0a47d86,0xb4ae15b4,0xc00c # -0.11563521196851768270e5
sinhp2:	.long	0x2df25310,0xa3c20b1c,0xc006 # -0.16375798202630751372e3
sinhp3:	.long	0x7e40d67c,0xca273dc3,0xbffe # -0.78966127417357099479e0
sinhq0:	.long	0xf33895cc,0x80d67405,0xc014 # -0.21108770058106271242e7
sinhq1:	.long	0xb2f63780,0x8d42b91d,0x400e # +0.36162723109421836460e5
sinhq2:	.long	0x4bff9009,0x8ade1c0e,0xc007 # -0.27773523119650701667e3
	

tanhp0:	.long	0xf0f0c35f,0xc9ad2e4d,0xc009 # -0.16134119023996228053d+4
tanhp1:	.long	0xd2e66361,0xc673ad0d,0xc005 # -0.99225929672236083313d+2
tanhp2:	.long	0xdd3bf3f6,0xf6e14677,0xbffe # -0.96437492777225469787d+0
tanhq0:	.long	0x74b49693,0x9741e2ba,0x400b # 0.48402357071988688686d+4
tanhq1:	.long	0xf8795e6,0x8b9c5a68,0x400a # 0.22337720718962312926d+4
tanhq2:	.long	0x5fbfd3bc,0xe17d4f0f,0x4005 # 0.11274474380534949335d+3

ENTRY(cosh)
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8, %esp		#set up temperary space
	fstcw	-4(%ebp)	/* save old exceptions, rounding modes */
	fclex
	movl	$0x033f,-8(%ebp) /* 80 bit round nearest, no exceptions */
	fldcw	-8(%ebp)
	movb	$1,%cl			# Set the operation flag to cosh
	fldl	8(%ebp)			# Put argument onto stack
	movw	6+8(%ebp),%ax		# get exponent
	andw	$0x7ff0,%ax		# mask sign out, mantissa
	cmpw	$0x3ff0-27*16,%ax	# compare |x| with 2^-27
	jg	sinhcosh
	fstp	%st(0)
	fld1
	fldcw	-4(%ebp)
	leave
	ret
ENTRY(sinh)
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8, %esp		#set up temperary space
	fstcw	-4(%ebp)	/* save old exceptions, rounding modes */
	fclex
	movl	$0x033f,-8(%ebp) /* 80 bit round nearest, no exceptions */
	fldcw	-8(%ebp)
	movb	$0,%cl			# Set the operation flag to sinh
	fldl	8(%ebp)			# Put argument onto stack
	movw	6+8(%ebp),%ax		# get exponent
	andw	$0x7ff0,%ax		# mask sign out, mantissa
	cmpw	$0x3fe0,%ax		# compare |x| with 0.5
	jge	sinhcosh
	fld	%st(0)			# replicate x
	fmul	%st(0)			# square to get g = x^2
	fldt	sinhp3			# evalute P(g)
	fmul	%st(1)
	fldt	sinhp2
	fadd
	fmul	%st(1)
	fldt	sinhp1
	fadd
	fmul	%st(1)
	fldt	sinhp0
	fadd	
	fldt	sinhq2			# evaluate Q(g)
	fadd	%st(2)			# note sinhq3 was 1.0
	fmul	%st(2)
	fldt	sinhq1
	fadd
	fmul	%st(2)
	fldt	sinhq0
	fadd
	fdivrp				/* p(g) / q(g) */ 
	fmulp				/* R(g) = g*p(g)/q(g) */
	fmul	%st(1)			/* x * R(g) */
	fnop
	fldcw	-4(%ebp)		/* round function result */
	faddp				/* x + x * R(g) */
	leave
	ret				# done--st(0) has answer--rest clean
sinhcosh:
	fldz				# Put zero on stack for space
	fld	%st(1)			# copy argument on the stack
	fldl	ln_2_times_ovfl		# Load maximum range for argument


	fcomp	%st(1)			# Check the argument's range
	fstsw	%ax			# Store the status word
	sahf
	jnc	cont_sinhcosh		# If argument <= libm_lnovfl continue
/*
 *    Error code for argument too big #
 */
	fclex
	fldcw	-4(%ebp)
	pushl	$ecode
	leal	4(%ebp), %eax
	pushl	%eax
	fstp	%st(0)
	fstp	%st(0)
	fstp	%st(0)
	call	_MATHERR
	addl	$8,%esp
	cmpb	$0,8+7(%ebp)		# check sign of orginal argument
	jge	1f
	fchs	
1:	
	leave
	ret
ecode:	.long	20

/*
 *    This would be the place to add a funny parameter test
 */

cont_sinhcosh:
	fabs				# x = |x|
	fldln2
	fxch	%st(1)
/*
 *    Beginning of exp for hypers
 *	Registers
 *		st(0) = arg
 *		st(1) = ln2
 *		st(2) = open for use.
 *	For restoration st = original st(1)
 */
	jmp	exph			#call exp - assume exp(x) is in st(0)

/*
 *    End of exp for hypers
 *	Registers
 *	st(0) = exp(arg)
 *		For restoration st = st(3)
 */
expdone:
	fld	%st
	fld	%st
	fld	%st(3)
	testw	$1,%cl			# If the operation is sinh then
	jz	calc_sinh		#    calculate sinh
					# Else
					#    calculate cosh
cont_cosh:
	fabs				# x = |x|

	fxch	%st(1)
	fstp	%st(0)			# Pop to balance the load of the stack
	fldl	libm_log_tot
	fxch	%st(1)

	# Stack Registers have the following values
	# st(0) = x
	# st(1) = ln2/2
	# st(2) = exp(x)

	fcom				#compare |x| and ln2/2
	fstsw	%ax			#store flags from compare
	fxch	%st(2)
	fxch	%st(1)
	fstp	%st(0)			# Pop to balance the stack load
	fld1
	fxch	%st(1)

	sahf				# If (x < ln2/2) then
					#     use function 0
	jc	function0		# Else
					#     use function 1

	# Stack Registers have the following values
	# st(0) = exp(operand)
	# st(1) = 1
	# st(2) = x



function1:	# for x >= ln2/2
		#	where cosh(x) = exp(x) + 1/exp(x)
		#	                ------------------
		#		           2 * exp(x)

	fdivr	%st,%st(1)		# st(1) = 1/exp(x)
	fadd	%st,%st(1)		# st(1) = exp(x) + 1/exp(x)
	fstp	%st(0)			# Pop to balance the stack load
	fld1				# st(0)	= 1
	fst	%st(2)			# st(2) = 1
	fadd	%st(2),%st		# st(0) = 1 + 1 = 2
	fdivr	%st(1),%st		# st(0) = (exp(x) + 1/exp(x))/2

        jmp sinhcosh_return

function0:	# for 0 <= x < ln2/2
		#	where cosh(x) =       (exp(x) - 1)^2
		#			1 + ------------------
		#			      2 * exp(x)

	fst	%st(2)			# st(2) = exp(x)
	fsub	%st(1),%st		# st(0) = exp(x) - 1
	fmul	%st,%st			# st(0) = (exp(x) - 1)^2
	fxch	%st(2)			# st(0) = exp(x), st(2)=(exp(x) - 1)^2
	fadd	%st,%st			# st(0) = 2 * exp(x)
	fdivr	%st(2),%st		# st(0) = ((exp(x) - 1)^2)/ (2 *exp(x))
	fadd	%st(1),%st		# Add a 1 so that st(0) = cosh(x)

	jmp sinhcosh_return

calc_sinh:
	# Registers have the following values
	# st(0) = x
	# st(1) = ?
	# st(2) = exp(x)

	ftst				# Compare the opperand to zero
	fstsw	%ax
	sahf
	jc	neg_opperand		#   If x > 0 then
	movb	$0,%cl			#       Set the sign flag = positive
	jmp	cont_sinh

neg_opperand:				#   Else
	movb	$1,%cl			#       Set the sign flag = negative

cont_sinh:

	# Registers have the following values
	# st(0) = x
	# st(1) = ?
	# st(2) = exp(x)
	# cl    = sign flag (1 = neg, 0 = pos)

	fstp	%st(0)			# Pop to balance the stack load
	fld1				# ST(0) = 1
	fst	%st(1)			# ST(1) = 1
	fsubr	%st(2),%st		# st(0) = exp(x) - 1
	fdiv	%st,%st(2)		# ST(2) = (exp(x) - 1)/exp(x)
	fadd	%st,%st(2)		# ST(2) = (exp(x)-1)/exp(x) + (exp(x)-1)
	fstp	%st(0)			# Pop to balance the stack load
	fld1				# Form 2 by adding 1's
	fadd	%st(1),%st		# ST(0) = 1 + 1
	fdivr	%st(2),%st		# ST =(((exp(x)-1)/exp(x))+(exp(x)-1))/2

		# Correct the sign of the outcome

	btw	$0,%cl			# If opperand sign = pos then
	jnc	sinhcosh_return		#       Return
	fchs				# Else   change the sign

	        # clear the stack, and return the answer
sinhcosh_return:
	fstp	%st(1)			#Pop and move answer down
	fstp	%st(1)			#Pop and move answer down
	fstp	%st(1)			#Pop and move answer down
	fstp	%st(1)			#Pop and move answer down
	fldcw	-4(%ebp)
	leave
	ret

/*
 *    Beginning of exp for hypers
 *	Registers
 *		st(0) = arg
 *		st(1) = ln2
 *		st(2) = open for use.
 */

exph:	fldl	ln_2_times_ovfl		#load in maximum range
	fcomp	%st(1)			#Compare maximum with argument
	fstsw	%ax			#Save the result of the test
	sahf				#Set the flags
	jc	big_err			#if argument > maximum, issue error

	fldl	libm_minexp		#load in minimum range
	fcomp	%st(1)			#Compare minimum with argument
	fstsw	%ax			#Save the result of the test
	sahf				#Set the flags
	jc	cont_calc		#if argument > minimum, continue
	jz	cont_calc		#if argument = minimum, continue
	fstp	%st(0)			#point to st(2)
	fstp	%st(0)			#point to st(3)
	fstp	%st(0)			#point to st(4)
	fldz				#return a 0.0 in st(3)
	jmp	sinhcosh_return		#go return
/*
 *    Error code for overflow argument
 */
big_err:
	fstp	%st(0)
	fstp	%st(0)
	fstp	%st(0)
	fstp	%st(0)
	fclex
	fldcw	-4(%ebp)
	pushl	$ecode
	leal	4(%ebp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8,%esp
	cmpb	$0,8+7(%ebp)		# check sign of orginal argument
	jge	1f
	fchs	
1:	
	leave
	ret

cont_calc:
	fst	%st(2)			#save arg in st2
	fdiv	%st(1),%st		#form arg/ln2
	frndint				#form k = NINT(arg/ln2)
	fwait				#wait until store is complete
	fxch	%st(1)			#switch k and ln2
	fmul	%st(1),%st		#form k * ln2
	fsubr	%st(2),%st		#form x - (k * ln2)
	fldl2e				#load LOG2e
	fmul				#form LOG2e * (x - (k*ln2))
	ftst				#check if value < 0
	fstsw	%eax			#get status word from test
	fabs				#form absolute value
	f2xm1				#form (2 ** (LOG2e * x - (k*ln2))) - 1.0
	fld1				#put 1.0 onto stack
	fxch	%st(1)			#switch 1.0 and result of exp
	fadd	%st(1),%st		#add back in the 1.0 to form Y
	testw	$maskC0,%eax		#was value < 0?
	je	no_invert		#if not, don't invert result
	fdivr	%st(1),%st(0)		#if so, invert result (1.0/result)
no_invert:
	fxch	%st(3)			#switch arg and Y
	fstp	%st(0)			#pop arg
	fscale				#scale 1.0 by k
	fxch	%st(1)			#switch k and 2 ** k
	fstp	%st(0)			#pop k
	fmul				#form result = Y * 2 ** k
/*
 *    End of exp for hypers
 *	Registers
 *	    st(0) = exp(arg)
 *	    For restoration st = st(3)
 */
	jmp	expdone


ENTRY(tanh)
	pushl	%ebp
	movl	%esp,%ebp
	subl	$8,%esp
	fstcw	-4(%ebp)	/* save old exceptions, rounding modes */
	fclex
	movl	$0x033f,-8(%ebp) /* 80 bit round nearest, no exceptions */
	fldcw	-8(%ebp)
/*
 * 	check for argument too large
 */
	movl	12(%ebp),%eax		/* most signifcant word */
	addl	%eax,%eax		/* ignore sign (double number) */
	cmpl	$0x80670000,%eax	/* check abs(x) vs ~19.5 */
	ja	bigtanh
	cmpl	$0x7fc327d4,%eax	/* check abs(x) < .5493061443 */
	ja	tanhexp			/* use exp formula */
	cmpl	$0x7c800000,%eax	/* check abs(x) < 2^-27 */
	jb	tanhsmall
	fldl	8(%ebp)			/* evaluate p(x)/q(x) */
	fld	%st			/* form g = x*x */
	fmul	%st
	fldt	tanhp2			/* p(g) = (p2 * g + p1) * g + p0 */
	fmul	%st(1)
	fldt	tanhp1
	fadd
	fmul	%st(1)
	fldt	tanhp0
	fadd
	fld	%st(1)			/* q(g) = ((1.0*g + q2)*g+q1)*g+q0 */
	fldt	tanhq2
	fadd
	fmul	%st(2)
	fldt	tanhq1
	fadd
	fmul	%st(2)
	fldt	tanhq0
	fadd
	fdivrp				/* p(g) / q(g) */ 
	fmulp				/* R(g) = g*p(g)/q(g) */
	fmul	%st(1)			/* x * R(g) */
	fnop
	fldcw	-4(%ebp)		/* round function result */
	faddp				/* x + x * R(g) */
	leave
	ret				/* sign is already correct */
tanhexp:			/* tanh(x) = 1-2.0/(exp(x+x)+1.0) */
	flds	one
	flds	two
	fldl	8(%ebp)
	fabs	
	fadd	%st		/* double */
	fldl2e			/* Load log2(e) */
	fmulp			/* x' = 2*abs(x)*log2(e) */
	fld	%st(0)		/* Duplicate */
	frndint			/* Get integer part */
	fxch	%st(1)		/* Swap st(1) with st(0) */
	fsub	%st(1), %st	/* integer part of x' & fractional part of x' */
	f2xm1			/* Compute exp(x'frac)-1.0 */
	fadds	%st(3)		/* Now exp(x'frac) */
	fscale			/* now exp(x') */
	fstp	%st(1)		/* clean up register */
	fadd	%st(2)		/* exp(2*abs(x))+1 */
	fdivrp			/* 2 / (exp(2*abs(x))+1) */
	fnop
	fldcw	-4(%ebp)	/* round function result */
	fsubrp			/* 1 - 2 / (exp(2*abs(x))+1) */
tanhsign:
	cmpb	$0,15(%ebp)	/* check sign of argument */
	jge	1f
	fchs	
1:	
	fnop
	fldcw	-8(%ebp)
	leave
	ret
tanhsmall:
	fldl	8(%ebp)		/* tanh(x) == x for small x */
	fldcw	-4(%ebp)
	leave
	ret
bigtanh:
	flds	one
	fldcw	-4(%ebp)	/* round function result */
	jmp	tanhsign
