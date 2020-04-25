  #
  # Double Precision Arcsine and Arccosine Functions
  #
  #
  # Functional Description:
  #	This routine implements the double precision arcsine/cosine functions.
  #	This routine contains two entry points: one for ASIN and one for ACOS.
  #	The algorithms used for these routines were obtained from the
  #	original PASCAL library that this replaces.
  #
  #	The algorithm for ASIN/ACOS is as follows:
  #
  #	x = abs(ARG)
  #	IF (x .GT. 1.0)
  #		return error
  #	ELSE
  #		IF (x .LT. 0.7071)
  #			reduced = FALSE
  #		ELSE
  #			x = sqrt((1.0-x) * (1.0+x))
  #			reduced = TRUE
  #		ENDIF
  #		xsq = sqr(x)
  #		result = x +
  #			(((((( +0.004751624376434203) * xsq
  #			       -0.592413491400993952) * xsq
  #			       +5.031785600674824666) * xsq
  #			      -14.350406308005313946) * xsq
  #			      +16.647746863878756233) * xsq
  #			       -6.773695835107418535) * xsq * x /
  #		    ((((( xsq -13.925210930636176476) * xsq
  #			      +63.723096250580563708) * xsq
  #			     -128.395097929241385370) * xsq
  #			     +118.175459937604538160) * xsq
  #			      -40.642175010638165982)
  #		IF (ASIN)
  #			IF (reduced)
  #				result = PI/2 - result
  #			ENDIF
  #			IF (ARG .LT. 0)
  #				result = -result
  #			ENDIF
  #		ELSE
  #			IF (ARG .LT. 0)
  #				IF (reduced)
  #					result = PI - result
  #				ELSE
  #					result = PI/2 + result
  #				ENDIF
  #			ELSEIF (.NOT. reduced)
  #				result = PI/2 - result
  #			ENDIF
  #		ENDIF
  #	ENDIF
  #
  # Created:
  #	12/17/86 	BS - Quantitative Technology Corporation for
  #				Sequent Computer Systems
  #
	.data
  #
  #	sign	dh			 holds sign flag 
  #	reduce	ecx			 holds reduction flag 
  #	sc_flag	dl			 holds sine/cosine flag 
  #	x	fp2			 computation register and result 
  #	arg	fp2			 holds argument 
  #	xsq	fp4			 square of x 
  #	templ	fp6			 temporary reg 
  #	templ1	fp8			 temporary reg 
  #
pi:	.double	0Dx400921fb54442d18	#   3.141592654
pidiv2:	.double	0Dx3ff921fb54442d18	#   1.570796327 (pi/2)
cnsta:	.double	0Dx3f73767074cb69a4	#  +0.004751624376434203
cnstb:	.double 0Dxbfe2f50d2368d8a5	#  -0.592413491400993952
cnstc:	.double	0Dx4014208c678d8742	#  +5.031785600674824666
cnstd:	.double	0Dxc02cb36874a263cd	# -14.350406308005313946
cnste:	.double	0Dx4030a5d2bd0c7221	# +16.647746863878756233
cnstf:	.double 0Dxc01b1843b89359e0	#  -6.773695835107418535
cnstg:	.double	0Dxc02bd9b53f41f7dc	# -13.925210930636176476
cnsth:	.double	0Dx404fdc8e6afe0d47	# +63.723096250580563708
cnsti:	.double	0Dxc0600ca4a46999e3	#-128.395097929241385370
cnstj:	.double	0Dx405d8b3abc517142	#+118.175459937604538160
cnstk:	.double	0Dxc0445232ca6e7feb	# -40.642175010638165982
one:	.double	0Dx3ff0000000000000	#   1.0
sqrt2div2: .double 0Dx3fe6a0902de00d1b	#   sqrt(2) / 2
  #
  #

#include "PROF.h"

ENTRY(asin)
	xorl	%edx,%edx		#set flag for ASIN
	wloadl	4(%esp),%fp2		#get argument
	wtstl	%fp2			#argument < 0?
	wstctx  %eax			#transfer contents of control reg
	sahf				#receive to flags
	setb	%dh			#if argument < 0, set sign flag 
	wabsl	%fp2,%fp2		# get abs(arg)
	wcmpl	one,%fp2		# x <= 1.0?
	wstctx  %eax			#transfer contents of control reg
	sahf				#receive to flags
	jae	cont			#if argument <= 1.0, continue (jge)

 # |arg| > 1 is an input error

	pushl	$ecode
	leal	4(%esp), %eax
	pushl	%eax
	call	_MATHERR
	addl	$8, %esp
	ret

ecode:	.long	18

cont:
	wcmpl	sqrt2div2,%fp2		#x < 0.7071?
	wstctx  %eax			#transfer contents of control reg
	sahf				#receive to flags
	movl	$0,%ecx			#init reduce to FALSE
	ja	noredu			#if x < 0.7071, don't reduce (jg)
					#
	wnegl	%fp2,%fp6		# form 1.0 - x
	waddl	one,%fp6		#
	waddl	one,%fp2		# form 1.0 + x
	wmull	%fp6,%fp2		# form (1.0-x) * (1.0+x)

	pushl	%edx			#save sign and sc_flag
	subl	$8,%esp
	wstorl	%fp2,(%esp)		#compute sqrt((1.0-x) * (1.0+x))
	call	_sqrt
	addl	$8,%esp
	popl	%edx			#restore sign and sc_flag
	movl	$1,%ecx			# set reduce to TRUE

noredu:
	wloadl	%fp2,%fp4		# form sqr(x)
	wmull	%fp4,%fp4		#
	wloadl	%fp4,%fp6		# templ gets xsq
	wmull	cnsta,%fp6		# A * xsq
	waddl	cnstb,%fp6		# (A*xsq) + B)
	wmull	%fp4,%fp6		# ((A*xsq)+B) * xsq
	waddl	cnstc,%fp6		# (((A*xsq)+B)*xsq) + C
	wmull	%fp4,%fp6		# ((((A*xsq)+B)*xsq)+C) * xsq
	waddl	cnstd,%fp6			# (((((A*xsq)+B)*xsq)+C)*xsq) + D
	wmull	%fp4,%fp6		# ((((((A*xsq)+B)*xsq)+C)*xsq)+D) * xsq
	waddl	cnste,%fp6		# (((((((A*xsq)+B)*xsq)+C)*xsq)+D)*xsq)
					#  + E
	wmull	%fp4,%fp6		# ((((((((A*xsq)+B)*xsq)+C)*xsq)+D)*xsq)
					#  +E) * xsq
	waddl	cnstf,%fp6			# (((((((((A*xsq)+B)*xsq)+C)*xsq)+D)*xsq)
					#  +E)*xsq) + F
	wmull	%fp4,%fp6		# ((((((((((A*xsq)+B)*xsq)+C)*xsq)+D)
					#  *xsq)+E)*xsq)+F) * xsq
	wmull	%fp2,%fp6			# ((((((((((A*xsq)+B)*xsq)+C)*xsq)+D)
					#  *xsq)+E)*xsq)+F) * xsq * x
	pushl	%fp8			# Save user registers fp8/9
	pushl	%fp9			#  so we can use them.
	wloadl	%fp4,%fp8		# get xsq
	waddl	cnstg,%fp8		# xsq + G
	wmull	%fp4,%fp8		# (xsq+G) * xsq
	waddl	cnsth,%fp8		# ((xsq+G)*xsq) + H
	wmull	%fp4,%fp8		# (((xsq+G)*xsq)+H) * xsq
	waddl	cnsti,%fp8		# ((((xsq+G)*xsq)+H)*xsq) + I
	wmull	%fp4,%fp8		# (((((xsq+G)*xsq)+H)*xsq)+I) * xsq
	waddl	cnstj,%fp8		# ((((((xsq+G)*xsq)+H)*xsq)+I)*xsq) + J
	wmull	%fp4,%fp8		# (((((((xsq+G)*xsq)+H)*xsq)+I)*xsq)+J)
					#  * xsq
	waddl	cnstk,%fp8		# ((((((((xsq+G)*xsq)+H)*xsq)+I)*xsq)+J)
					# *xsq) + K
	wdivl	%fp6,%fp8		# divide two quantities
	waddl	%fp8,%fp2		# add in x
	popl	%fp9			# Restore user registers fp8/9
	popl	%fp8
  #
  #	ASIN				
  #
	cmpl	$0,%ecx			# reduction occur?
	jz	asin1			# if not, continue
	wnegl	%fp2,%fp2		# if so, form -x
	waddl	pidiv2,%fp2		# x = PI/2 - x
asin1:
	cmpb	$0,%dh			# ARG >= 0?
	jz	retn			# if so, go return
	wnegl	%fp2,%fp2		# if not, result = -x
retn:
	ret
