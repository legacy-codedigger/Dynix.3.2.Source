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

/*
 * $Header: fpu.h 2.22 90/11/08 $
 *
 * fpu.h
 *	Floating-point unit related constants and structures.
 *
 * This information is derived from the 80387 target spec, Rev 1.1, 1/07/86.
 */

/* $Log:	fpu.h,v $
 */

/*
 * Signal code values for SIGFPE.  The first group match the 287/387 hardware.
 */

#define	FPE_FLTINV_TRAP		0x01	/* floating INValid operation */
#define	FPE_FLTDEN_TRAP		0x02	/* floating DENormal exception */
#define	FPE_FLTDIV_TRAP		0x04	/* floating DIVide by zero */
#define	FPE_FLTOVF_TRAP		0x08	/* floating OVerFlow exception */
#define	FPE_FLTUND_TRAP		0x10	/* floating UNDerflow exception */
#define	FPE_FLTPRE_TRAP		0x20	/* floating PREcision exception */
#define	FPE_FLTSTK_TRAP		0x40	/* floating stack overflow/underflow */

#define	FPE_INTDIV_TRAP		0x80	/* integer DIVide by zero */
#define	FPE_INTOVF_TRAP		0x81	/* integer OVerFlow (into) */
#define	FPE_NOFPA_AVAIL		0x82	/* No FPA's in the system */

#define	FPE_B1_BUG21		0x8F	/* 386 B1 Errata # 21 (9/1/87 Errata) */

#ifdef	KERNEL
#define	FSRTT			0x7F	/* Mask to extract Trap Type */
#define	FSRTM			0x3F	/* Mask to extract Trap Mask */
#define FPA_EM_OK               0x00    /* No problems with emulation */
#define FPA_EM_ILLINSTR         0x01    /* Didn't understand instruction  */
					/* accessing FPA. 		  */
#define FPA_EM_EXCEPTION        0x02    /* FPA exception of some sort */
#define FPA_EM_SEG              0x03    /* Detected a SEG error */
#endif	KERNEL

/*
 * 80387 Floating Point Unit registers save area.
 * This structure used for both 287 or 387.
 */

struct	fpusave	{
	u_short	fpu_control, fpu_rsvd1;	/* control word */
	u_short	fpu_status, fpu_rsvd2;	/* status word */
	u_short	fpu_tag, fpu_rsvd3;	/* tag word */
	u_long	fpu_ip;			/* IP offset */
	u_short	fpu_cs, fpu_rsvd4;	/* CS selector */
	u_long	fpu_data_offset;	/* data offset */
	u_short	fpu_op_sel, fpu_rsvd5;	/* operand selector */
	u_short	fpu_stack[8][5];	/* 8 80-bit temp-reals from FPU stack */
};

/*
 * The following definitions determine the state of the FPU after an exec.
 *
 * FPU_CONTROL_INIT:	rounding=nearest, precision=53-bit, mask denormalized, 
 *			underflow, and precision exceptions.
 * FPU_STATUS_INIT:	stack top = 0, no exceptions present.
 * FPU_TAG_INIT:	all entries are marked empty.
 */

#define	FPU_CONTROL_INIT	0x0232	/* PC=53,RC=nearest, exception mask  */
#define	FPU_STATUS_INIT		0x0000	/* stack-top = 0, no exceptions */
#define	FPU_TAG_INIT		0xFFFF	/* all stack entries empty */

/*
 * User Control Word Mask and Bit Definitions.
 * These definitions match the 8087/80287
 */
#define	FPU_CW_EM		0x003F		/* Exception Masks */
#define	FPU_CW_INVALID		0x0001		/* Invalid */
#define	FPU_CW_DENORMAL 	0x0002		/* Denormal */
#define	FPU_CW_ZERODIVIDE	0x0004		/* Zero divide */
#define	FPU_CW_OVERFLOW 	0x0008		/* Overflow */
#define	FPU_CW_UNDERFLOW	0x0010		/* Underflow */
#define	FPU_CW_INEXACT		0x0020		/* Inexact (precision) */

#define	FPU_CW_AFFINE		0x1000		/* Affine */

#define	FPU_CW_RND		0x0c00		/* Rounding Control */
#define	FPU_CW_RZ		0x0c00		/* Round to Zero (truncate) */
#define	FPU_CW_RP		0x0800		/* Round to Plus infinity (round up) */
#define	FPU_CW_RM		0x0400		/* Round to Minus infinity (round down) */
#define	FPU_CW_RN		0x0000		/* Round to Nearest */

#define FPU_CW_P24		0x0000		/* single */
#define FPU_CW_P53		0x0200		/* double */
#define	FPU_CW_P64		0x0300		/* extended */

/*
 * Fields in the i387 status word.
 */
#define	FPU_SW_BUSY		0x8000		/* FPU busy */
#define	FPU_SW_C3		0x4000		/* Condition code 3 */
#define	FPU_SW_TOP		0x3800		/* Top of stack */
#define	FPU_SW_C2		0x0400		/* Condition code 2 */
#define	FPU_SW_C1		0x0200		/* Condition code 1 */
#define	FPU_SW_C0		0x0100		/* Condition code 0 */

#define	FPU_SW_EF		0x003F		/* Exception masks */
#define	FPU_SW_INVALID		0x0001		/* Invalid */
#define	FPU_SW_DENORMAL 	0x0002		/* Denormal */
#define	FPU_SW_ZERODIVIDE	0x0004		/* Zero divide */
#define	FPU_SW_OVERFLOW 	0x0008		/* Overflow */
#define	FPU_SW_UNDERFLOW	0x0010		/* Underflow */
#define	FPU_SW_INEXACT		0x0020		/* Inexact (precision) */

/*
 * Floating-point accelerator (FPA) definitions.
/*
 * Floating-point accelerator (FPA) definitions.
 * The hardware definitions correspond to the WTL 1163 Floating Point
 * Controller specification dated July 1986.
 */

#define	FPA				/* SGS supports FPA */

/*
 * VA_FPA defines system virtual address (level-1 mapping granularity)
 * where FPA lives.  FPA_START, FPA_SIZE define where in that window the
 * actual FPA hardware lives.
 */

#define	FPA_START	0		/* FPA at start of VA_FPA */
#define	FPA_SIZE	(64*1024)	/* FPA is 64K of address space */

/*
 * Important FPA offsets.  Kernel only needs a few.  See WTL spec for others.
 */

#define	FPA_NREGS	31		/* 31 registers */
#define	FPA_LOAD_R1	0x0404		/* R1 write offset (single precision) */
#define	FPA_STOR_R1	0x0C04		/* R1 read offset (single precision) */
#define	FPA_LDCTX	0xC000		/* write context register */
#define	FPA_STCTX	0xC400		/* read context register */

/*
 * Software save area for fpa context.
 */

struct	fpasave	{
	long		fpa_pcr;		/* context register */
	long		fpa_regs[FPA_NREGS];	/* register contents */
};


struct  fpaesave {
	long            fpae_pcr;               /* emulation pcr */
	long            fpae_regs[32];          /* all the necessary registers*/
	long            fpae_extra_status;      /* extra status bits */
};


/*
 * Process Context Register field definitions.
 *
 * Initial FPA PCR value (after first reference in a process) sets
 * mode = round to nearest, integer round to nearest, exception mask
 * analogous to FPU_CONTROL_INIT.
 *
 * Registers are all zero on first reference.
 *
 * SIGFPE for FPA exception places accumulated exceptions in u_code.
 */

#define	FPA_PCR_MODE	0xFF000000	/* mode select field */

#define FPA_PCR_RND     0x0C000000      /* Rounding Field */
#define FPA_PCR_IRND    0x02000000      /* Integer Rounding toward Zero */
#define FPA_PCR_FAST    0x01000000      /* FAST mode (flush denorm's to zero) */

#define	FPA_PCR_EM	0x00FF0000	/* exception mask; 1 = mask exception */
#define	FPA_PCR_EM_DM	0x00800000	/* data chain exception mask */
#define	FPA_PCR_EM_UOM	0x00400000	/* unimplemented op-code excep. mask */
#define	FPA_PCR_EM_PM	0x00200000	/* precision exception mask */
#define	FPA_PCR_EM_UM	0x00100000	/* underflow exception mask */
#define	FPA_PCR_EM_OM	0x00080000	/* overflow exception mask */
#define	FPA_PCR_EM_ZM	0x00040000	/* zero divide exception mask */
#define	FPA_PCR_EM_IM	0x00010000	/* invalid operation exception mask */
#define	FPA_PCR_20MHZ	0x00008000	/* 20MHz 1163 */
#define	FPA_PCR_EM_ALL	0x00FF0000	/* mask all exceptions */
#define	FPA_PCR_EM_SHIFT	16	/* right-justify shift count */

#define	FPA_PCR_CC	0x0000FF00	/* condition code field */
#define	FPA_PCR_CC_Z	0x00004000
#define	FPA_PCR_CC_C2	0x00000400
#define FPA_PCR_CC_C0   0x00000100
#define	FPA_PCR_CC_C1	0x00000100

#define	FPA_PCR_AE	0x000000FF	/* accumulated exceptions */
#define	FPA_PCR_AE_DE	0x00000080	/* data chain exception */
#define	FPA_PCR_AE_UOE	0x00000040	/* unimplemented op-code exception */
#define	FPA_PCR_AE_PE	0x00000020	/* precision exception */
#define	FPA_PCR_AE_UE	0x00000010	/* underflow exception */
#define	FPA_PCR_AE_OE	0x00000008	/* overflow exception */
#define	FPA_PCR_AE_ZE	0x00000004	/* zero divide exception */
#define	FPA_PCR_AE_EE	0x00000002	/* enabled exception (1 == interrupt) */
#define	FPA_PCR_AE_IE	0x00000001	/* invalid operation exception */

#define	FPA_FPE		0x100		/* distinguish FPA exception from FPU */

#ifdef	KERNEL
#define	FPA_INIT_PCR	(0x01020000|FPA_PCR_EM_PM|FPA_PCR_EM_UM)
#endif	KERNEL
