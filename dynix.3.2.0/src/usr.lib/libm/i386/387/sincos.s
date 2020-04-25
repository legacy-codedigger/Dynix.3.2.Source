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

#ifdef	KXX
/* $Header: sincos.s 1.5 87/08/13 $
 *
 *		SINE & COSINE FUNCTIONS
 *
 * Functional Description:
 *
 *	This routine implements the sine & cosine functions for the C language.
 *	It uses the fptan instruction of the 287 NPX. No error checking
 *	is performed since it is assumed that the 287 will correctly handle
 *	an illegal argument.
 *
 *
 * Input:	Argument is located at 4(%esp)
 *	        Assumed that the argument is normal.
 *
 * Output:	Double Precision value is returned in ST(0)
 *
 * Assumptions:  ST(0) & ST(1) are scratch registers
 *		Input argument is normal.
 *
 * Created:
 *	05/27/86	GS - Quantitative Technology Corporation for
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
 * Method:
 *
 *
 *	The Sine and Cosine functions are calculated using the fptan
 *	instruction.  The fptan requires an argument of 0 <= x <= pi/4.
 *	The calculation is dependant upon the octant that the angle
 *	lies in.
 *	
 *	The sine & cosine functions are calculated like this:
 *
 *		 where   k = angle mod PI/4
 *
 *
 *	Octant		Sine			Cosine
 *  	        (Negate if angle < 0)
 *
 *	  0	       sin(K)		        cos(K)
 *	  1	    cos(pi/4 - K)	     sin(pi/4 - K)
 *	  2	       cos(K)			-sin(K)
 *	  3	    sin(pi/4 - K) 	    -cos(pi/4 - K)
 *	  4	      -sin(K)			-cos(K)
 *	  5	    -cos(pi/4 - K)	    -sin(pi/4 - K)
 *	  6	      -cos(K)	  	        sin(K)
 *	  7	    -sin(pi/4 - K)	     cos(pi/4 - K) 
 *
 *	In all cases the sine is calculated and then converted to the 
 *	cosine function by  the identity:
 *
 *			sin(|A| + pi/2) = cos(A)
 */

#include "PROF.h"

 	.set mask_cond0, 0x01
 	.set mask_cond1, 0x02
 	.set mask_cond2, 0x04
 	.set mask_cond3, 0x40
 	.set mask_cond13, 0x42
 	.set not_mask_cond1, 0xFD

ENTRY(cos)
	movw 	$1,%cx			# Set flag to show cosine operation
	jmp 	sincos
ENTRY(sin)
	movw	$0,%cx			# Set flag to show sine operation
sincos:

	fldl	libm_quarter_pi		#put argument onto stack
	fldl	4(%esp)
       		
        fxam				# Examine the operand
	fstsw 	%ax			# Save S.W. to determine sign of output
	sahf
	jnz	calc_sin		# If Angle !=0 & Normal then
					#	Continue the calculation  
	jp	calc_sin		# If Angle != 0 & Denormal then
					#       Continue the calculation
					# Else  Assert angle is zero or unnorm.
					#       Angle = 0 (ignoring unnormal)
	fstp	%st(0)			#       Pop stack to balance load
	testw	$1,%cx			#   If operation = sin then
	jz	its_sin			#       so sin(0) = 0 
	fld1				#   Else  load cos(0) = 1
	jmp 	sincos_return		#	so return(1)

its_sin:
	fldz				#	load sin(0) = 0 
	jmp	sincos_return		#	so return(0)


/*
 *    Now we have  	ST(0) = Angle  (Angle != 0)
 *    			ST(1) = PI/4
 *			ax  = status from fxam of arg
 */
calc_sin:
	fprem				# Angle mod PI/4

        xchgw 	%dx,%ax			# Save status word in dx
	fstsw	%ax			                    
        xchgw 	%dx,%ax

	testb	$mask_cond2,%dh		# If Angle < PI/4
	jz 	sincos_angle_ok		#    then Angle OK
/*
 *    Error code is placed here for the angle too big
 */
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	fstp	%st(0)
	fstp	%st(0)
	call	_MATHERR
	addl	$8, %esp
	ret
ecode:	.long	4

sincos_angle_ok:
	fabs				# Now 0 < ST(0) < PI/4
/*
 *    Test for sin or cos operation
 */
        testb	$1,%cl			# If cl = 0 then
        jz 	sin_select		#    do the sin operation
					# Else do the cos operation
/*
 *    Cos Operation - Ignore the original sign of the angle
 *    and add a quarter revolution to the octant id from the
 *    fprem instruction cos(A)=sin(A+PI/2) and cos(A)=cos(|A|)
 */
	andb 	$not_mask_cond1,%ah	# Turn off sign of argument
	orb 	$0x80,%dh		# Prepare to add 010 to C0,C3,C1 status
					# value in ax 
	addb 	$mask_cond3,%dh		# Set busy bit so carry out from C3 will
					# go into the carry flag
	movb 	$0,%al			# Extract carry flag
	rclb 	$1,%al			# Put carry flag into low bit
	xorb 	%al,%dh			# Add carry to C0 not changing C1 flag
					# Now process as if it is a sin op.




 		# Sin Operation
sin_select:

	testb 	$mask_cond1,%dh		# If Angle in Octants 0,2,4,6 then
      	jz  	no_sine_reverse		#      don't reverse the angle

             				# Else Angle in octants 1,3,5,7
        fsubr 				#      reduce the argument by pi/4
	jmp 	do_sine_fptan		#      reverse the angle

no_sine_reverse:
	
	ftst				# If Angle != zero then
        xchgw 	%ax,%cx
	fstsw 	%ax			#     find sin(Angle)
        xchgw 	%cx,%ax
	fstp 	%st(1)			# Remove PI/4 from stack
	testb 	$mask_cond3,%ch		# Else
	jz 	do_sine_fptan     	#     can't use fptan 
	

sine_argument_zero:

	fld1				# Simulate fptan(0) call 
	jmp 	after_sine_fptan

do_sine_fptan:

	fptan				# Tan(ST(0)) = ST(1)/ST(1) = y/x

after_sine_fptan:

    	testb 	$mask_cond13,%dh	# if angle in octant 1,2,5,6 then
	jpo 	X_numerator             #       calculate the cosine

	fld 	%st(1)			# Else  calculate the sine
	jmp 	finish_sine



	
X_numerator:

        # Calculate the cosine of the angle.
	#     cos(A) = 1/sqrt(1+tan(A)**2)      
	#     cos(A) = x/sqrt(x*x + y*y)

	fld 	%st(0)			# Copy x value
	fxch 	%st(2)			# Put x in numerator

finish_sine:

	fmul 	%st,%st(0)		# Form x*x + y*y
	fxch
	fmul 	%st,%st(0)
	fadd				# st(0) = x*x + y*y
	fsqrt				# st(0) = sqrt(x*x + y*y)
        
	# Sign the result

	andb 	$mask_cond0,%dh    	# If sign should be negative then
	andb 	$mask_cond1,%ah
	orb 	%dh,%ah
	jpe 	positive_sign

	fchs				#    	negate the result

positive_sign:

	fdivr	%st(1),%st		# Else  form the result in st(0)

sincos_return: 
	fstp	%st(1)		#stack model conversion requires empty stack
	ret

#else

#include "PROF.h"

ENTRY(cos)
	fldl	4(%esp)
	.byte	0xd9, 0xff	# ASM won't do FCOS
	fstsw	%ax		# get status
	testw	$ 0x0400, %ax	# check for C2 bit set
	jnz	too_large_arg	# incomplete reduction if set
	ret

ENTRY(sin)
	fldl	4(%esp)
	.byte	0xd9, 0xfe	# ASM won't do FSIN
	fstsw	%ax		# get status
	testw	$ 0x0400, %ax	# check for C2 bit set
	jnz	too_large_arg	# incomplete reduction if set
	ret

too_large_arg:
	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	fstp	%st(0)
	call	_MATHERR
	addl	$8, %esp
	ret
ecode:	.long	4
	
#endif
