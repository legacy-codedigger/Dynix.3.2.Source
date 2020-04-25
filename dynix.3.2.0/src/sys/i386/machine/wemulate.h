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

#ident	"$Header: wemulate.h 1.4 91/01/29 $"

/*
 * This file contains the really, really local view of the world.
 * Each of the "asm"'s below deal with the emulation of one particular
 * aspect of the 1167.
 *
 * Certain of the code below should have probably been coded in "C"
 * (I got carried away).
 */

#ifndef	lint
/*
 * Fpa_extra_status (u.u_fpasave.fpae_extra_status) is used to compensate
 * for the inability to set bits in the status register of the i387.
 */
asm init_i387()
{
/PEEPOFF
	finit
	movl	$0,fpae_extra_status
/PEEPON
}

asm get_sw()
{
/PEEPOFF
	fstsw	%eax
	orl	fpae_extra_status,%eax
/PEEPON
}

asm set_cw(val)
{
%reg val;
/PEEPOFF
	fldcw	val
/PEEPON
%mem val;
/PEEPOFF
	fldcw	val
/PEEPON
}

/*
 * Load the operand, check for denormalized.  If denormalized, must flush
 * the operand to the appropriately signed zero (i.e. that's why the
 * "fmuls").
 *
 *#define	LDS_FAST(src)	\
 *	flds	(src);	\
 *	fstsw	%eax;	\
 *	andl	$FPU_SW_DENORMAL,%eax; \
 *	je	1f;	\
 *	fldz;		\
 *	fmul;		\
 *	fnclex;		\
 *1:
 *
 *#define	LDD_FAST(src)	\
 *	fldl	-4(src);\
 *	fstsw	%eax;	\
 *	andl	$FPU_SW_DENORMAL,%eax; \
 *	je	1f;	\
 *	fldz;		\
 *	fmul;		\
 *	fnclex;		\
 *1:
 */

/*
 * Store the result, checking for denormalized and NaN.  If denormalized,
 * must flush the result to the appropriately signed zero; if NaN, must
 * set the IE bit of the PCR and store the proper NaN.
 *
 *#define	STS_FAST(dst)	\
 *	fstps	(dst); /* must store and reload to check for denorm.  \
 *	flds	(dst);	\
 *	fxam;		\
 *	fstsw	%eax;	\
 *	testl	$FPU_SW_DENORMAL,%eax; \
 *	je	1f;	\
 *	andl	$0x80000000,(dst); \
 *	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status; \
 *	jmp	2f;	\
 *1:			\
 *	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax; \
 *	fstps	(dst);	\
 *	cmpl	$FPU_SW_C0,%eax; \
 *	jne	2f;	\
 *	orl	$FPU_SW_INVALID,fpae_extra_status; \
 *	movl	$0x7fffffff,(dst); \
 *2:
 *
 *#define	STD_FAST(dst)	\
 *	fstpl	-4(dst); \
 *	fldl	-4(dst); \
 *	fxam;		\
 *	fstsw	%eax;	\
 *	testl	$FPU_SW_DENORMAL,%eax; \
 *	je	1f;	\
 *	andl	$0x80000000,(dst); \
 *	movl	$0,-4(dst); \
 *	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status; \
 *	jmp	2f;	\
 *1:			\
 *	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax; \
 *	fstpl	-4(dst); \
 *	cmpl	$FPU_SW_C0,%eax; \
 *	jne	2f;	\
 *	orl	$FPU_SW_INVALID,fpae_extra_status; \
 *	movl	$0x7fffffff,(dst); \
 *	movl	$0xffffffff,-4(dst); \
 *2:
 */

/*
 * Examine the result and store it.  If a NaN is found, the appropriate
 * signaling NaN must be generated.
 *
 *#define	STD(dst)	\
 *	fstpl	-4(dst); \
 *	fldl	-4(dst); \
 *	fxam;		\
 *	fstsw	%eax;	\
 *	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax; \
 *	fstpl	-4(dst); \
 *	cmpl	$FPU_SW_C0,%eax; \
 *	jne	1f;	\
 *	orl	$FPU_SW_INVALID,fpae_extra_status; \
 *	movl	$0x7fffffff,(dst); \
 *	movl	$0xffffffff,-4(dst); \
 *1:
 *
 *#define	STS(dst)	\
 *	fstps	-4(dst); \
 *	flds	-4(dst); \
 *	fxam;		\
 *	fstsw	%eax;	\
 *	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax; \
 *	fstps	(dst);	\
 *	cmpl	$FPU_SW_C0,%eax; \
 *	jne	1f;	\
 *	orl	$FPU_SW_INVALID,fpae_extra_status; \
 *	movl	$0x7fffffff,(dst); \
 *1:
 */

asm adds(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds	(dst)
	fadds	(src)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm adds_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fadd
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
/PEEPON
}

asm addd(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl	-4(dst)
	faddl	-4(src)
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm addd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fadd
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
4:
/PEEPON
}

#define	loads(src, dst)	(*(int *)(dst) = *(int *)(src))
#define	loadd loads

asm floats(src, dst)
{
%reg src, dst;
/PEEPOFF
	fildl	(src)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm floatd(src, dst)
{
%reg src, dst;
/PEEPOFF
	fildl	(src)
	fstpl	-4(dst)
/PEEPON
}

/*
 * Fix up the results of the double/single to integer conversion.
 * I387 always sets INVALID on "out of range", NaN or empty (won't happen).
 * must distinguish between these two cases.
 *
 *#define	FIXUP(org, dst) \
 *	fstsw	%eax;	\
 *	testl	$FPU_SW_INVALID,%eax; \
 *	je	1f;	\
 *	fnclex;		\
 *	testl	$FPU_SW_DENORMAL,%eax; \
 *	je	0f;	\
 *	orl	$FPU_SW_INEXACT|FPU_SW_INVALID|FPU_SW_OVERFLOW,fpae_extra_status; \
 *	movl	$0x7fffffff,(dst); \
 *	jmp	1f;	\
 *0:			\
 *	orl	$FPU_SW_OVERFLOW|FPU_SW_INEXACT,fpae_extra_status; \
 *	movl	$0x7fffffff,%ecx; \
 *	testl	$0x80000000,org; \
 *	je	0f;	\
 *	movl	$0x80000000,%ecx; \
 *0:			\
 *	movl	%ecx,(dst); \
 *1:
 */

/*
 * I387 and FPA don't agree when a FIX operation encounters a NaN.
 *
 *#define	FIX_NAN(dst, lab) \
 *	fxam; \
 *	fstsw	%eax; \
 *	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax; \
 *	cmpl	$FPU_SW_C0,%eax; \
 *	jne	0f; \
 *	orl	$FPU_SW_INVALID,fpae_extra_status; \
 *	movl	$0x7fffffff,(dst); \
 *	jmp	lab; \
 *0:
 */

asm fixs(src, dst)
{
%reg src, dst;
/PEEPOFF
	movl	(src),%edx
	flds	(src)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	cmpl	$FPU_SW_C0,%eax
	jne	0f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
0:
	fistpl	(dst)
	fstsw	%eax
	testl	$FPU_SW_INVALID,%eax
	je	9f
	fnclex
	testl	$FPU_SW_DENORMAL,%eax
	je	2f
	orl	$FPU_SW_INEXACT|FPU_SW_INVALID|FPU_SW_OVERFLOW,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
2:
	orl	$FPU_SW_OVERFLOW|FPU_SW_INEXACT,fpae_extra_status
	movl	$0x7fffffff,%ecx
	testl	$0x80000000,%edx
	je	3f
	movl	$0x80000000,%ecx
3:
	movl	%ecx,(dst)
9:
/PEEPON
}

asm fixs_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	movl	(src),%edx
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	cmpl	$FPU_SW_C0,%eax
	jne	0f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
0:
	fistpl	(dst)
	fstsw	%eax
	testl	$FPU_SW_INVALID,%eax
	je	9f
	fnclex
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	orl	$FPU_SW_INEXACT|FPU_SW_INVALID|FPU_SW_OVERFLOW,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
3:
	orl	$FPU_SW_OVERFLOW|FPU_SW_INEXACT,fpae_extra_status
	movl	$0x7fffffff,%ecx
	testl	$0x80000000,%edx
	je	2f
	movl	$0x80000000,%ecx
2:
	movl	%ecx,(dst)
9:
/PEEPON
}

asm fixd(src, dst)
{
%reg src, dst;
/PEEPOFF
	movl	(src),%edx
	fldl	-4(src)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	cmpl	$FPU_SW_C0,%eax
	jne	0f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
0:
	fistpl	(dst)
	fstsw	%eax
	testl	$FPU_SW_INVALID,%eax
	je	9f
	fnclex
	testl	$FPU_SW_DENORMAL,%eax
	je	2f
	orl	$FPU_SW_INEXACT|FPU_SW_INVALID|FPU_SW_OVERFLOW,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
2:
	orl	$FPU_SW_OVERFLOW|FPU_SW_INEXACT,fpae_extra_status
	movl	$0x7fffffff,%ecx
	testl	$0x80000000,%edx
	je	3f
	movl	$0x80000000,%ecx
3:
	movl	%ecx,(dst)
9:
/PEEPON
}

asm fixd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	movl	(src),%edx
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	cmpl	$FPU_SW_C0,%eax
	jne	0f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
0:
	fistpl	(dst)
	fstsw	%eax
	testl	$FPU_SW_INVALID,%eax
	je	9f
	fnclex
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	orl	$FPU_SW_INEXACT|FPU_SW_INVALID|FPU_SW_OVERFLOW,fpae_extra_status
	movl	$0x7fffffff,(dst)
	jmp	9f
3:
	orl	$FPU_SW_OVERFLOW|FPU_SW_INEXACT,fpae_extra_status
	movl	$0x7fffffff,%ecx
	testl	$0x80000000,%edx
	je	4f
	movl	$0x80000000,%ecx
4:
	movl	%ecx,(dst)
9:
/PEEPON
}

asm cvtds(src, dst)		/* Single to Double (weitek terminology) */
{
%reg src, dst;
/PEEPOFF
	flds	(src)
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm cvtds_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	2f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
2:
/PEEPON
}

asm cvtsd(src, dst)		/* Double to Single */
{
%reg src, dst;
/PEEPOFF
	fldl	-4(src)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm cvtsd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
/PEEPON
}

asm muls(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds	(dst)
	fmuls	(src)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

/*
 * mul also botches the smallest non-denormalized number
 * from both directions (+ and -) treats it like a denormalized.
 *
 *#define	MULS_BOTCH(dst)	\
 *	movl	(dst),%eax; \
 *	andl	$0x7fffffff,%eax; \
 *	cmpl	$0x00800000,%eax; \
 *	jne	1f; \
 *	fstsw	%eax; \
 *	orl	fpae_extra_status,%eax; \
 *	andl	$FPU_SW_UNDERFLOW,%eax; \
 *	je	1f; \
 *	andl	$0x80000000,(dst); \
 *1:
 */

asm muls_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00800000,%eax
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
5:
/PEEPON
}

asm muld(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl	-4(dst)
	fmull	-4(src)
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

/*
 * Denormalized mul botch.  Probably the same for MAC.
 * When no underflow, thinks the smallest normalized is
 * denormalized and flushes to zero (only on no underflow).
 *
 *#define	MULD_BOTCH(dst) \
 *	movl	(dst),%eax; \
 *	andl	$0x7fffffff,%eax; \
 *	cmpl	$0x00100000,%eax; \
 *	jne	1f; \
 *	cmpl	$0x00000000,-4(dst); \
 *	jne	1f; \
 *	fstsw	%eax; \
 *	orl	fpae_extra_status,%eax; \
 *	andl	$FPU_SW_UNDERFLOW,%eax; \
 *	je	1f; \
 *	andl	$0x80000000,(dst); \
 *	movl	$0,-4(dst); \
 *1:
 */

asm muld_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00100000,%eax
	jne	5f
	cmpl	$0x00000000,-4(dst)
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
5:
/PEEPON
}

asm subs(src, dst)		/* is reversed subtract */
{
%reg src, dst;
/PEEPOFF
	flds	(src)
	fsubs	(dst)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm subs_fast(src, dst)		/* is reversed subtract */
{
%reg src, dst;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fsub
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
/PEEPON
}

asm subd(src, dst)		/* is reversed subtract */
{
%reg src, dst;
/PEEPOFF
	fldl	-4(src)
	fsubl	-4(dst)
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm subd_fast(src, dst)		/* is reversed subtract */
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fsub
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
4:
/PEEPON
}

asm divs(src, dst)		/* is reversed divide */
{
%reg src, dst;
/PEEPOFF
	flds	(src)
	fdivs	(dst)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm divs_fast(src, dst)		/* is reversed divide */
{
%reg src, dst;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fdiv
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00800000,%eax
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
5:
/PEEPON
}

asm divd(src, dst)		/* is reversed divide */
{
%reg src, dst;
/PEEPOFF
	fldl	-4(src)
	fdivl	-4(dst)
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm divd_fast(src, dst)		/* is reversed divide */
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fdiv
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00100000,%eax
	jne	5f
	cmpl	$0x00000000,-4(dst)
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
5:
/PEEPON
}

asm mulns(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds	(dst)
	fchs
	fmuls	(src)
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm mulns_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fchs
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00800000,%eax
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
5:
/PEEPON
}

asm mulnd(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl	-4(dst)
	fchs
	fmull	-4(src)
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm mulnd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fchs
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00100000,%eax
	jne	5f
	cmpl	$0x00000000,-4(dst)
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
5:
/PEEPON
}

asm cmps(src, dst)		/* reversed compare */
{
%reg src, dst;
/PEEPOFF
	flds	(src)
	fcomps	(dst)
/PEEPON
}

asm cmps_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fcomp
/PEEPON
}

asm cmpd(src, dst)		/* reversed compare */
{
%reg src, dst;
/PEEPOFF
	fldl	-4(src)
	fcompl	-4(dst)
/PEEPON
}

asm cmpd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fcomp
/PEEPON
}

asm tsts(src)
{
%reg src;
/PEEPOFF
	fldz
	flds	(src)
	fcomp
/PEEPON
}

asm tsts_fast(src)
{
%reg src;
/PEEPOFF
	fldz
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fcomp
/PEEPON
}

asm tstd(src)
{
%reg src;
/PEEPOFF
	fldz
	fldl	-4(src)
	fcomp
/PEEPON
}

asm tstd_fast(src)
{
%reg src;
/PEEPOFF
	fldz
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fcomp
/PEEPON
}

/*
 * Weitek handles negation as "0 - src".  Fails validation suite,
 * bigtime.
 */
asm negs(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds	(src)
	fldz
	fsub
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm negs_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldz
	fsub
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	2f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
2:
/PEEPON
}

asm negd(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl	-4(src)
	fldz
	fsub
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm negd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldz
	fsub
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	2f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	3f
2:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	3f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
3:
/PEEPON
}

asm abss(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds	(src)
	fabs
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm abss_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fabs
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	2f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	3f
2:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	3f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
3:
/PEEPON
}

asm absd(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl	-4(src)
	fabs
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm absd_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fabs
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	2f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	3f
2:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	3f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
3:
/PEEPON
}

asm amuls(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds	(dst)
	fmuls	(src)
	fabs
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
1:
/PEEPON
}

asm amuls_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fabs
	fstps	(dst)
	flds	(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00800000,%eax
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
5:
/PEEPON
}

asm amuld(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl	-4(dst)
	fmull	-4(src)
	fabs
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
1:
/PEEPON
}

asm amuld_fast(src, dst)
{
%reg src, dst;
/PEEPOFF
	fldl    -4(dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	fldl    -4(src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fabs
	fstpl	-4(dst)
	fldl	-4(dst)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	3f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	4f
3:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(dst)
	cmpl	$FPU_SW_C0,%eax
	jne	4f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(dst)
	movl	$0xffffffff,-4(dst)
4:
	movl	(dst),%eax
	andl	$0x7fffffff,%eax
	cmpl	$0x00100000,%eax
	jne	5f
	cmpl	$0x00000000,-4(dst)
	jne	5f
	fstsw	%eax
	orl	fpae_extra_status,%eax
	andl	$FPU_SW_UNDERFLOW,%eax
	je	5f
	andl	$0x80000000,(dst)
	movl	$0,-4(dst)
5:
/PEEPON
}

/*
 * Multiply always performed in single precision, but accumulate
 * done to ws2 in either single or double precision.
 */
asm macs(src, dst, acc)
{
%reg src, dst; mem acc;
/PEEPOFF
	movl	acc,%ecx
	flds	(dst)
	fmuls	(src)
	fadds	(%ecx)
	fstps	(%ecx)
	flds	(%ecx)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(%ecx)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(%ecx)
1:
/PEEPON
}

asm macs_fast(src, dst, acc)
{
%reg src, dst; mem acc;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fstsw	%eax
	andl	$FPU_SW_DENORMAL,%eax
	je	3f
	fldz
	fmul
3:
	movl	acc,%ecx
	flds    (%ecx)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      4f
	fldz
	fmul
	fnclex
4:
	fadd
	fstps	(%ecx)
	flds	(%ecx)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	5f
	andl	$0x80000000,(%ecx)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	6f
5:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstps	(%ecx)
	cmpl	$FPU_SW_C0,%eax
	jne	6f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(%ecx)
6:
/PEEPON
}

asm macd(src, dst, acc)
{
%reg src, dst; mem acc;
/PEEPOFF
	movl	acc,%ecx
	flds	(dst)
	fmuls	(src)
	faddl	-4(%ecx)
	fstpl	-4(%ecx)
	fldl	-4(%ecx)
	fxam
	fstsw	%eax
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(%ecx)
	cmpl	$FPU_SW_C0,%eax
	jne	1f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(%ecx)
	movl	$0xffffffff,-4(%ecx)
1:
/PEEPON
}

asm macd_fast(src, dst, acc)
{
%reg src, dst; mem acc;
/PEEPOFF
	flds    (dst)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      1f
	fldz
	fmul
	fnclex
1:
	flds    (src)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      2f
	fldz
	fmul
	fnclex
2:
	fmul
	fstsw	%eax
	andl	$FPU_SW_DENORMAL,%eax
	je	3f
	fldz
	fmul
3:
	movl	acc,%ecx
	fldl    -4(%ecx)
	fstsw   %eax
	andl    $FPU_SW_DENORMAL,%eax
	je      4f
	fldz
	fmul
	fnclex
4:
	fadd
	fstpl	-4(%ecx)
	fldl	-4(%ecx)
	fxam
	fstsw	%eax
	testl	$FPU_SW_DENORMAL,%eax
	je	5f
	andl	$0x80000000,(%ecx)
	movl	$0,-4(%ecx)
	orl	$FPA_PCR_AE_PE|FPA_PCR_AE_UE,fpae_extra_status
	jmp	6f
5:
	andl	$FPU_SW_C3|FPU_SW_C2|FPU_SW_C0,%eax
	fstpl	-4(%ecx)
	cmpl	$FPU_SW_C0,%eax
	jne	6f
	orl	$FPU_SW_INVALID,fpae_extra_status
	movl	$0x7fffffff,(%ecx)
	movl	$0xffffffff,-4(%ecx)
6:
/PEEPON
}
#else	/* lint */
void init_i387();
int get_sw();
void set_cw();
void adds();
void adds_fast();
void addd();
void addd_fast();
void floats();
void floatd();
void fixs();
void fixs_fast();
void fixd();
void fixd_fast();
void cvtds();
void cvtds_fast();
void cvtsd();
void cvtsd_fast();
void muls();
void muls_fast();
void muld();
void muld_fast();
void subs();
void subs_fast();
void subd();
void subd_fast();
void divs();
void divs_fast();
void divd();
void divd_fast();
void mulns();
void mulns_fast();
void mulnd();
void mulnd_fast();
void cmps();
void cmps_fast();
void cmpd();
void cmpd_fast();
void tsts();
void tsts_fast();
void tstd();
void tstd_fast();
void negs();
void negs_fast();
void negd();
void negd_fast();
void abss();
void abss_fast();
void absd();
void absd_fast();
void amuls();
void amuls_fast();
void amuld();
void amuld_fast();
void macs();
void macs_fast();
void macd();
void macd_fast();
void loads();
void loadd();
#endif	/* lint */
