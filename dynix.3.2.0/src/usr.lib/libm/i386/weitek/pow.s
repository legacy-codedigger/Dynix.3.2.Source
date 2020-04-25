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

	.asciz	"@(#)$Header: pow.s 1.11 1991/09/19 20:33:22 $"

/*		POWER FUNCTION
 *
 * Functional Description:
 *
 *	This routine implements the power function for the C language.
 *
 *	This routine is a total rewrite to improve accuracy and speed
 *	over the current version. The numerous accuracy issues are
 *	confirmed by the Cody & Waite "elefunt" test "dpower".
 *
 *	Basic strategy: use double-to-integer exponentiation if
 *	appropriate. Otherwise use the 387 base 2 power and log
 *	instructions in 80-bit precision. The extra precision is
 *	useful in that it allows a more naive implementation than
 *	would otherwise be required to preserve the accuracy.
 *	Indeed, the power function requires a very sophisiticated
 *	implementation when extra precision is not available via
 *	hardware (see Cody & Waite, A Software Manual for the Elementary
 *	Functions, Prentice Hall, 1980).
 */

#include "PROF.h"
#ifdef ATT
#define ATTSYSV
#endif
#ifdef ATTSYSV
#include <sys/errno.h>
#include "matherr.h"
#endif
#define w1167

ENTRY(pow)
	pushl	%ebp
	movl	%esp,%ebp
#ifdef ATTSYSV
	subl	$44,%esp
#else 
	subl	$16,%esp
#endif
	fstcw	-4(%ebp)	/* save old exceptions, rounding modes */
	fclex
	movl	$0x0f3f,-8(%ebp) /* 80 bit round towards zero, no exceptions */
	fldcw	-8(%ebp)
/*
 *	check for integral exponent
 */
	fldl	16(%ebp)	/* get exponent */
	fld	%st
	frndint
	fcomp	%st(1)
	fstsw	%ax
	sahf
	je	dblpowint	
/*	
 *	let's do some error checking
 */
	movl	12(%ebp),%eax	/* get most significant word of base */
	testl	%eax,%eax
	jle	negzerobase	/*  negative base to non-integral power */
/*
 *	compute y*log2(x) to 80-bits (64-bit mantissa)
 */
	fldl	8(%ebp)
dbltodbl:
	fyl2x
	fld	%st(0)
	frndint
	fxch	%st(1)
	fsub	%st(1),%st
/*
 *	compute 2^fractional part
 */
	f2xm1	
	fadds	one
/*
 *	combine parts
 */
	fscale
chkoverflow:
	fstp	%st(1)		/* get rid of extra variable */
#ifdef w1167
	fstpl	-16(%ebp)
	wloadl	-16(%ebp),%fp2
#else
	fstl	-16(%ebp)
#endif
/*
 *	check for overflow
 */
	fstsw	-8(%ebp)
#ifdef LIBFVERSION
	testb	$0x18,-8(%ebp)	/* check underflow and overflow */
#else /* !LIBFVERSION */
	testb	$0x8,-8(%ebp)	/* just check overflow */
#endif /* !LIBFVERSION */
	jne	overflow
bye:	fldcw	-4(%ebp)	/* restore control word */
	leave
	ret

	ALIGN
one:	
	.long	0x3f800000

negzerobase:
	jne	negbase
zerobase:
        movl    $0x7fffffff,%eax        /* test for zero (-0 included) */
        andl    20(%ebp),%eax
        orl     16(%ebp),%eax
	je	zerozero
        testl   $0x80,23(%ebp)
        jne     zerozero        /* zero ** negative is also an error */
#ifdef w1167
	fstp	%st
	wsubl	%fp2,%fp2
#else
	fsub	%st
#endif
	jmp	bye
negbase:                                /* check for -0 base */
        movl    $0x7fffffff,%eax        /* test for zero (-0 included) */
        andl    12(%ebp),%eax
        orl     8(%ebp),%eax
        je      zerobase
        movl    $0x7fffffff,%eax        /* test for zero (-0 included) */
        andl    20(%ebp),%eax
        orl     16(%ebp),%eax
	jne	complex
	fstp	%st 
#ifdef w1167
	wcvtls	one,%fp2
#else
	flds	one
#endif
	jmp	bye
#ifdef ATTSYSV
powname:	.asciz	"pow"
zerozero:
complex:
	movl	$DOMAIN,-44(%ebp)
	movl	$0,-20(%ebp)		/* zero is result */
	movl	$0,-16(%ebp)	
	jmp	reporterr

overflow:
	testb	$0x10,-8(%ebp)
	jne	underflow
	movl	$OVERFLOW,-44(%ebp)
	movl	$-1,-20(%ebp)		/* +-HUGE is result */
	movl	$0x7fefffff,-16(%ebp)	
	fstsw	%ax
	sahf
	ja	reporterr
	movl	$0xffefffff,-16(%ebp)	/* make it -HUGE */
	jmp	reporterr
overint:
	movl	$OVERFLOW,-44(%ebp)
	movl	$-1,-20(%ebp)		/* +-HUGE is result */
	movl	$0x7fefffff,-16(%ebp)	
	jmp	reportint

underflow:
	movl	$UNDERFLOW,-44(%ebp)
	movl	$0,-20(%ebp)		/* zero is result */
	movl	$0,-16(%ebp)		
	jmp	reporterr
	
reporterr:
	fstp	%st
	fstp	%st
reportint:
	fclex
	fldcw	-4(%ebp)
	movl	$powname,-40(%ebp)
	movl	8(%ebp),%eax
	movl	%eax,-36(%ebp)
	movl	12(%ebp),%eax
	movl	%eax,-32(%ebp)
	movl	16(%ebp),%eax
	movl	%eax,-28(%ebp)
	movl	20(%ebp),%eax
	movl	%eax,-24(%ebp)
	movl	%esp,%eax
	pushl	%eax
#ifdef SYSV
	call	matherr
	popl	%ecx
	testl	%eax,%eax
	je	noerrno
	movl	$ERANGE,errno
#else
	call	_matherr
	popl	%ecx
	testl	%eax,%eax
	je	noerrno
	movl	$ERANGE,_errno
#endif
noerrno:
#ifdef w1167
	wloadl	-20(%ebp),%fp2
#else
	fldl	-20(%ebp)
#endif
#else /* !ATTSYSV */
zerozero:
	movl	$12,%eax	/* 0^-x or 0^0 */
	jmp	reporterr
overint:
	movl	$6,%eax		/* just too large */
	jmp	reportint
overflow:
	movl	$6,%eax		/* just too large */
	jmp	reporterr
complex:
	movl	$11,%eax	/* negative to non-int powr */
reporterr:
	fstp	%st
reportint:
	fclex
	fldcw	-4(%ebp)
	movl	%eax,-8(%ebp)	/* save error code */
	leal	-8(%ebp),%eax
	pushl	%eax
	leal	4(%ebp),%eax
	pushl	%eax
	call	_MATHERR
	addl	$8,%esp
#endif /* !ATTSYSV */
	leave
	ret

/* 	real to exact integer--allow negative bases */
dblpowint:
	movl	20(%ebp),%eax
	andl	$0x7ff00000,%eax
	cmpl	$0x43e00000,%eax	/* 2 ** 63 */
	jl	dbltomedium	/* it will fit in a 64-bit integer */
/* 
 *	check extreme cases for very large integral exponent 
 */
dbltohuge:
	fstp	%st		/* get rid of it anyway */
	movl	$0x7fffffff,%eax
	andl	12(%ebp),%eax	/* +- zero ** huge is zero */
	jne	dbltry1
	cmpl	%eax,8(%ebp)		/* check for full zero */
	je	zbasecheck		/* may be denormalized */
smallbase:				/* |base| < 1.0 */
	testb	$0x80,23(%ebp)		/* check sign of exponent */
	je	zbaseok			/* underflow to zero */
	jmp	overint			/* overflow */
dbltry1:
	cmpl	$0x3ff00000,%eax	/* compare abs(x) to one */
	jg	bigbase
	jl	smallbase
	cmpl	$0,8(%ebp)		/* check for base exactly one */
	je	itsone
bigbase:				/* |base| > 1  */
	testb	$0x80,23(%ebp)		/* check sign of exponent */
	jne	zbaseok			/* underflow to zero */
	jmp	overint			/* overflow */
zbasecheck:
        testb   $0x80,23(%ebp)
        je      zbaseok                 /* 0.0 ^ positive is zero */
        jmp     zerozero                /* 0.0 ^ negative is undefined */
/*
 *	check for really small--else use regular method
 *	but allow negative results for negative bases with odd exponents
 */
dbltomedium:
	fistpll	-16(%ebp)	/* store integral exponent */
	movl	-16(%ebp),%eax
	cdq
	cmpl	%edx,-12(%ebp)
	je	dbltosmall
	testl	$0x7fffffff,12(%ebp)	/* check for zero base */
	jne	dblmed
	cmpl	$0,8(%ebp)
	je	zbasecheck
dblmed: fldl	16(%ebp)	/* reload exponent */
	fldl	8(%ebp)
	cmpb	$0,8+7(%ebp)	/* check for negative base */
	jge	dbltodbl	/* just use normal algorithm */
	fabs
	testb	$1,-16(%ebp)	/* check for odd integral exponent */
	je	dbltodbl	/* even exponent--return (abs(x))**y */
/*
 *	compute y*log2(x) to 80-bits (64-bit mantissa) as above
 */
	fyl2x
	fld	%st(0)
	frndint
	fxch	%st(1)
	fsub	%st(1),%st
	f2xm1	
	fadds	one
	fscale
	fchs			/* negate due to negative base to odd power */
	jmp	chkoverflow
/*
 *	use fast bit-test algorithm if exponent is really small
 */
dbltosmall:
	movl	%eax,%edx
	testl	%edx,%edx
	jl	negint	
	flds	one
	fldl	8(%ebp)
	jne	intloop1	/* note: still checking the EXPONENT */
/* 
 *	check for zero ** zero
 */
	fxam
	fstsw	%ax
	fstp	%st		/* pop base and return one */
	sahf
	je	zerozero
#ifdef w1167
	fstp	%st
	wloads	one,%fp2
	wcvtls	%fp2,%fp2
#endif
	jmp	bye
	
/* 
 *	square and form product
 */
intloop:
	fmul	%st
intloop1:
	shrl	$1,%edx
	jnc	intskip
	fmul	%st,%st(1)
intskip:
	jnz	intloop
	fxch	%st(1)
	jmp	chkoverflow

negint:	negl	%edx
	testl	$0x7fffffff,12(%ebp)	/* - zero is still zero */
	jne	recip
	cmpl	$0,8(%ebp)		/* check for denormalized */
        je      zerozero                /* zero ^ negative is error, too */
recip:
	flds	one
	fldl	8(%ebp)
	fdivr	%st(1)
	jmp	intloop1

zbaseok:	
#ifdef w1167
	wsubl	%fp2,%fp2
#else
	fldz			/* return plus zero */
#endif
	jmp	bye

itsone:				/* return plus one */
#ifdef w1167
	wcvtls	one,%fp2
#else
	fld1
#endif
	jmp	bye
