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

#ident	"$Header: wemulate.c 1.3 90/11/08 $"

/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms
 *	of a license agreement or nondisclosure
 *	agreement with Intel Corporation and may
 *	not be copied nor disclosed except in
 *	accordance with the terms of that agreement.
 */

/* This file emulates the 1167 system,
*
*   There are 3 external interface routines that represent the 3 types of
*   bus transfers that can be made to the 1167.
*
*   wtl1167_rr(address)
*      This routine represents a "register-to-register" operation. It is
*      caused by a byte move to the address specified by the specific
*      address. Note the address is only the offset portion of the physical
*      address (the low order 16 bits).  This routine primarily manages
*      the registers where things come from/go to.  "wemulate.h" is where
*      the code emulating the operations resides (for the most part).
*
*   wtl1167_mr(address,val)
*      This routine represents a "memory-to-register" operation. It is
*      cased by a word move to the specified address. The val parameter
*      represents the 32-bit word that is moved (the data bus value) to
*      the 1167.
*
*   long wtl1167_read(address)
*      This routine represents an unload operation. It is caused by a word
*      read operation from the specified address. It returns the value that
*      the read operation would yield (the data bus value).
*
*/

#include "../h/types.h"
#include "../h/signal.h"
#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../machine/mftpr.h"
#include "../machine/wemulate.h"

/*
 * All opcodes.
 */
#define	ADDS		0x00
#define	LOADS		0x01
#define	MULS		0x02
#define	STORS		0x03
#define	SUBS		0x04
#define	DIVS		0x05
#define	MULNS		0x06
#define	FLOATS		0x07
#define	CMPTS		0x08
#define	TSTS		0x09
#define	NEGS		0x0A
#define	ABSS		0x0B
#define	CMPS		0x0C
#define	TSTTS		0x0D
#define	AMULS		0x0E
#define	FIXS		0x0F
#define	CVTSD		0x10		/* Double to Single */
#define	CVTDS		0x11		/* Single to Double */
#define	MACS		0x12

#define	ADDD		0x20
#define	LOADD		0x21
#define	MULD		0x22
#define	STORD		0x23
#define	SUBD		0x24
#define	DIVD		0x25
#define	MULND		0x26
#define	FLOATD		0x27
#define	CMPTD		0x28
#define	TSTTD		0x29
#define	NEGD		0x2A
#define	ABSD		0x2B
#define	CMPD		0x2C
#define	TSTD		0x2D
#define	AMULD		0x2E
#define	FIXD		0x2F
#define	LDCTX		0x30
#define	STCTX		0x31
#define	MACD		0x32

#define	FAST		0x40

#define	NONE		0	/* Operand Precision and Types */
#define	SINGLE		1
#define	DOUBLE		2
#define	INTEGER		3

#ifndef lint
static int source_precision[] = {
	SINGLE,		/*  ADD.S    0 */
	INTEGER,	/*  LOAD     1 */
	SINGLE,		/*  MUL.S    2 */
	INTEGER,	/*  STOR     3 */
	SINGLE,		/*  SUB.S    4 */
	SINGLE,		/*  DIV.S    5 */
	SINGLE,		/*  MULN.S   6 */
	INTEGER,	/*  FLOAT.S  7 */
	SINGLE,		/*  CMPT.S   8 */
	SINGLE,		/*  TSTT.S   9 */
	SINGLE,		/*  NEG.S    A */
	SINGLE,		/*  ABS.S    B */
	SINGLE,		/*  CMP.S    C */
	SINGLE,		/*  TST.S    D */
	SINGLE,		/*  AMUL.S   E */
	SINGLE,		/*  FIX.S    F */
	DOUBLE,		/*  CVTS.D  10 */
	SINGLE,		/*  CVTD.S  11 */
	SINGLE,		/*  MAC.S   12 */
	NONE,		/*  ?       13 */
	NONE,		/*  ?       14 */
	NONE,		/*  ?       15 */
	NONE,		/*  ?       16 */
	NONE,		/*  ?       17 */
	NONE,		/*  ?       18 */
	NONE,		/*  ?       19 */
	NONE,		/*  ?       1A */
	NONE,		/*  ?       1B */
	NONE,		/*  ?       1C */
	NONE,		/*  ?       1D */
	NONE,		/*  ?       1E */
	NONE,		/*  ?       1F */
	DOUBLE,		/*  ADD.D   20 */
	INTEGER,	/*  LOAD    21 */
	DOUBLE,		/*  MUL.D   22 */
	INTEGER,	/*  STOR    23 */
	DOUBLE,		/*  SUB.D   24 */
	DOUBLE,		/*  DIV.D   25 */
	DOUBLE,		/*  MULN.D  26 */
	INTEGER,	/*  FLOAT.D 27 */
	DOUBLE,		/*  CMPT.D  28 */
	DOUBLE,		/*  TSTT.D  29 */
	DOUBLE,		/*  NEG.D   2A */
	DOUBLE,		/*  ABS.D   2B */
	DOUBLE,		/*  CMP.D   2C */
	DOUBLE,		/*  TST.D   2D */
	DOUBLE,		/*  AMUL.D  2E */
	DOUBLE,		/*  FIX.D   2F */
	INTEGER,	/*  LDCTX   30 */
	NONE,		/*  STCTX   31 */
	SINGLE		/*  MAC.D   32 */
};
#endif

/*
 * Types of PCR update.
 */
#define	ARITHMETIC		1
#define	COMPARE			2
#define	COMPARETRP		3

static int pcr_update[] = {
	ARITHMETIC,	/*  ADD.S    0 */
	NONE,		/*  LOAD     1 */
	ARITHMETIC,	/*  MUL.S    2 */
	NONE,		/*  STOR     3 */
	ARITHMETIC,	/*  SUB.S    4 */
	ARITHMETIC,	/*  DIV.S    5 */
	ARITHMETIC,	/*  MULN.S   6 */
	ARITHMETIC,	/*  FLOAT.S  7 */
	COMPARETRP,	/*  CMPT.S   8 */
	COMPARETRP,	/*  TSTT.S   9 */
	ARITHMETIC,	/*  NEG.S    A */
	ARITHMETIC,	/*  ABS.S    B */
	COMPARE,	/*  CMP.S    C */
	COMPARE,	/*  TST.S    D */
	ARITHMETIC,	/*  AMUL.S   E */
	NONE,		/*  FIX.S    F */
	ARITHMETIC,	/*  CVTS.D  10 */
	ARITHMETIC,	/*  CVTD.S  11 */
	ARITHMETIC,	/*  MAC.S   12 */
	NONE,		/*  ?       13 */
	NONE,		/*  ?       14 */
	NONE,		/*  ?       15 */
	NONE,		/*  ?       16 */
	NONE,		/*  ?       17 */
	NONE,		/*  ?       18 */
	NONE,		/*  ?       19 */
	NONE,		/*  ?       1A */
	NONE,		/*  ?       1B */
	NONE,		/*  ?       1C */
	NONE,		/*  ?       1D */
	NONE,		/*  ?       1E */
	NONE,		/*  ?       1F */
	ARITHMETIC,	/*  ADD.D   20 */
	NONE,		/*  LOAD    21 */
	ARITHMETIC,	/*  MUL.D   22 */
	NONE,		/*  STOR    23 */
	ARITHMETIC,	/*  SUB.D   24 */
	ARITHMETIC,	/*  DIV.D   25 */
	ARITHMETIC,	/*  MULN.D  26 */
	ARITHMETIC,	/*  FLOAT.D 27 */
	COMPARETRP,	/*  CMPT.D  28 */
	COMPARETRP,	/*  TSTT.D  29 */
	ARITHMETIC,	/*  NEG.D   2A */
	ARITHMETIC,	/*  ABS.D   2B */
	COMPARE,	/*  CMP.D   2C */
	COMPARE,	/*  TST.D   2D */
	ARITHMETIC,	/*  AMUL.D  2E */
	NONE,		/*  FIX.D   2F */
	NONE,		/*  LDCTX   30 */
	NONE,		/*  STCTX   31 */
	ARITHMETIC	/*  MAC.D   32 */
};

#define	regs	u.u_fpaesave->fpae_regs
#define	pcr	u.u_fpaesave->fpae_pcr

void wtl1167_rr();

/*
 * This subroutine handles all Memory to Register Operations.
 */
void
wtl1167_mr(address, value)
	unsigned address;
	long value;
{
	regs[0^1] = value;
	wtl1167_rr(address);
}

static int rounding_conversion[] = {
	FPU_CW_RN,
	FPU_CW_RZ,
	FPU_CW_RP,
	FPU_CW_RM,
};

#undef	tstvec
int	wtl1167_rr_vec[3];
#define	tstvec(n)	(void)(wtl1167_rr_vec[(unsigned)(n)>>5] |= 1<<(n&31))

/*
 * This subroutine handles all Register to Register operations.
 */
void
wtl1167_rr(address)
	unsigned address;
{
	register long *srcp, *dstp;
	long *accp;
	int opcode;
	int dst;
	int src;
	int new_mask;			/* New 8087 context */
	int status;			/* New 8087 status */
	int rm;
tstvec(0);
	opcode = (address >> 10) & 0x3F;
tstvec(1);
tstvec(2);
	dst = (address >> 2) & 0x1F;
	src =  ((address >> 5) & 0x1C) | (address & 3);

	src ^= 1;
	dst ^= 1;

	srcp = &regs[src];
	dstp = &regs[dst];

	/*
	 * Initialize the 8087.
	 */
	rm = rounding_conversion[(pcr & FPA_PCR_RND) >> 26];
	new_mask = FPU_CW_EM | FPU_CW_AFFINE | rm | FPU_CW_P53;

	/*
	 * Initialize the i387.  This clears all exceptions.
	 */
	init_i387();
	set_cw(new_mask);

	switch (pcr & FPA_PCR_FAST? opcode+FAST : opcode) {
	case ADDS:
tstvec(3);
		adds(srcp, dstp);
		break;
	case ADDS+FAST:
tstvec(4);
		adds_fast(srcp, dstp);
		break;

	case ADDD:
tstvec(5);
		addd(srcp, dstp);
		break;
	case ADDD+FAST:
tstvec(6);
		addd_fast(srcp, dstp);
		break;

	case LOADS:
	case LOADS+FAST:
tstvec(7);
		loads(srcp, dstp);
		break;

	case LOADD:
	case LOADD+FAST:
tstvec(8);
		loadd(srcp, dstp);
		break;

	case FLOATS:
	case FLOATS+FAST:
tstvec(9);
		floats(srcp, dstp);
		break;

	case FLOATD:
	case FLOATD+FAST:
tstvec(10);
		floatd(srcp, dstp);
		break;

	case FIXS:
		if (pcr & FPA_PCR_IRND) {
			new_mask = new_mask & ~FPU_CW_RND | FPU_CW_RZ;
			set_cw(new_mask);
		}

		fixs(srcp, dstp);
		break;

	case FIXS+FAST:
tstvec(11);
		if (pcr & FPA_PCR_IRND) {
tstvec(12);
			new_mask = new_mask & ~FPU_CW_RND | FPU_CW_RZ;
			set_cw(new_mask);
		}
tstvec(13);
		fixs_fast(srcp, dstp);
		break;

	case FIXD:
		if (pcr & FPA_PCR_IRND) {
			new_mask = new_mask & ~FPU_CW_RND | FPU_CW_RZ;
			set_cw(new_mask);
		}

		fixd(srcp, dstp);
		break;

	case FIXD+FAST:
tstvec(14);
		if (pcr & FPA_PCR_IRND) {
tstvec(15);
			new_mask = new_mask & ~FPU_CW_RND | FPU_CW_RZ;
			set_cw(new_mask);
		}
tstvec(16);
		fixd_fast(srcp, dstp);
		break;

	case CVTSD:
tstvec(17);
		cvtsd(srcp, dstp);
		break;
	case CVTSD+FAST:
tstvec(18);
		cvtsd_fast(srcp, dstp);
		break;

	case CVTDS:
tstvec(19);
		cvtds(srcp, dstp);
		break;
	case CVTDS+FAST:
tstvec(20);
		cvtds_fast(srcp, dstp);
		break;

	case MULS:
tstvec(21);
		muls(srcp, dstp);
		break;
	case MULS+FAST:
tstvec(22);
		muls_fast(srcp, dstp);
		break;

	case MULD:
tstvec(23);
		muld(srcp, dstp);
		break;
	case MULD+FAST:
tstvec(24);
		muld_fast(srcp, dstp);
		break;

	case SUBS:
tstvec(25);
		subs(srcp, dstp);
		break;
	case SUBS+FAST:
tstvec(26);
		subs_fast(srcp, dstp);
		break;

	case SUBD:
tstvec(27);
		subd(srcp, dstp);
		break;
	case SUBD+FAST:
tstvec(28);
		subd_fast(srcp, dstp);
		break;

	case DIVS:
tstvec(29);
		divs(srcp, dstp);
		break;
	case DIVS+FAST:
tstvec(30);
		divs_fast(srcp, dstp);
		break;

	case DIVD:
tstvec(31);
		divd(srcp, dstp);
		break;
	case DIVD+FAST:
tstvec(32);
		divd_fast(srcp, dstp);
		break;

	case MULNS:
tstvec(33);
		mulns(srcp, dstp);
		break;
	case MULNS+FAST:
tstvec(34);
		mulns_fast(srcp, dstp);
		break;

	case MULND:
tstvec(35);
		mulnd(srcp, dstp);
		break;
	case MULND+FAST:
tstvec(36);
		mulnd_fast(srcp, dstp);
		break;

	case CMPTS:
	case CMPS:
tstvec(37);
		cmps(srcp, dstp);
		break;
	case CMPTS+FAST:
	case CMPS+FAST:
tstvec(38);
		cmps_fast(srcp, dstp);
		break;

	case CMPTD:
	case CMPD:
tstvec(39);
		cmpd(srcp, dstp);
		break;
	case CMPTD+FAST:
	case CMPD+FAST:
tstvec(40);
		cmpd_fast(srcp, dstp);
		break;

	case TSTS:
	case TSTTS:
tstvec(41);
		tsts(srcp);
		break;
	case TSTS+FAST:
	case TSTTS+FAST:
tstvec(42);
		tsts_fast(srcp);
		break;

	case TSTD:
	case TSTTD:
tstvec(43);
		tstd(srcp);
		break;
	case TSTD+FAST:
	case TSTTD+FAST:
tstvec(44);
		tstd_fast(srcp);
		break;

	case NEGS:
tstvec(45);
		negs(srcp, dstp);
		break;
	case NEGS+FAST:
tstvec(46);
		negs_fast(srcp, dstp);
		break;

	case NEGD:
tstvec(47);
		negd(srcp, dstp);
		break;
	case NEGD+FAST:
tstvec(48);
		negd_fast(srcp, dstp);
		break;

	case ABSS:
tstvec(49);
		abss(srcp, dstp);
		break;
	case ABSS+FAST:
tstvec(50);
		abss_fast(srcp, dstp);
		break;

	case ABSD:
tstvec(51);
		absd(srcp, dstp);
		break;
	case ABSD+FAST:
tstvec(52);
		absd_fast(srcp, dstp);
		break;

	case AMULS:
tstvec(53);
		amuls(srcp, dstp);
		break;
	case AMULS+FAST:
tstvec(54);
		amuls_fast(srcp, dstp);
		break;

	case AMULD:
tstvec(55);
		amuld(srcp, dstp);
		break;
	case AMULD+FAST:
tstvec(56);
		amuld_fast(srcp, dstp);
		break;

	case MACS:
tstvec(57);
		accp = &regs[2^1];
		macs(srcp, dstp, accp);
		break;
	case MACS+FAST:
tstvec(58);
		accp = &regs[2^1];
		macs_fast(srcp, dstp, accp);
		break;

	case MACD:
tstvec(59);
		accp = &regs[2^1];
		macd(srcp, dstp, accp);
		break;
	case MACD+FAST:
tstvec(60);
		accp = &regs[2^1];
		macd_fast(srcp, dstp, accp);
		break;

	case LDCTX:
	case LDCTX+FAST:
tstvec(61);
		pcr = *srcp;
		break;
#if 0
	/*
	 * Up in "emula_fpa".
	 */
	case STCTX:
	case STCTX+FAST:
		*dstp = pcr;
		break;
#endif
	default:
tstvec(62);
		pcr |= FPA_PCR_AE_UOE;
		goto pcr_check;
	}

tstvec(63);
	/*
	 * Check the result and correct it to reflect the 1164 and 1165.
	 */
	status = get_sw();

	/*
	 * The 1167 and i387 exception bits match up fairly well.
	 * The only difference is the i387 has a denormalized indicator
	 * in bit 2 were as the 1167 has a exception indicator at bit 2.
	 */
	pcr |= status & (FPU_SW_INVALID|FPU_SW_ZERODIVIDE|FPU_SW_OVERFLOW|
		         FPU_SW_UNDERFLOW|FPU_SW_INEXACT);

	/*
	 * Calculate new PCR.
	 */
	switch (pcr_update[opcode]) {
	case COMPARETRP:
tstvec(64);
		if ((status & (FPU_SW_C3|FPU_SW_C2|FPU_SW_C0)) ==
		     (FPU_SW_C3|FPU_SW_C2|FPU_SW_C0))
tstvec(65),
			pcr |= FPA_PCR_AE_IE;
	case COMPARE:
tstvec(66);
		pcr = pcr & ~(FPA_PCR_CC_Z|FPA_PCR_CC_C2|FPA_PCR_CC_C0);
		if (status & FPU_SW_C3)
tstvec(67),
			pcr |= FPA_PCR_CC_Z;
		if (status & FPU_SW_C2)
tstvec(68),
			pcr |= FPA_PCR_CC_C2;
		if (status & FPU_SW_C0)
tstvec(69),
			pcr |= FPA_PCR_CC_C0;
tstvec(70);
		break;

	case NONE:
	case ARITHMETIC:
	default:
tstvec(71);
		break;
	}
tstvec(72);

	/*
	 * Check for an unmasked exception.
	 */
pcr_check:
tstvec(73);
	if ((pcr & FPA_PCR_AE) & ~(pcr >> 16))
tstvec(74),
		/*
		 * Generate Interrupt.
		 */
		pcr |= FPA_PCR_AE_EE;
}

/*
 * This subroutine handles read operations, the returned value is the
 * result of the read operation.
 */
wtl1167_read(address)
	unsigned address;
{
	int opcode,dst;

tstvec(75),
	opcode = (address >> 10) & 0x3F;
	dst = (address >> 2) & 0x1F;
	if (opcode == 3 || opcode == 0x23)
{
tstvec(76);
		return(regs[dst^1]);
}
	else if (opcode == 0x31)
{
tstvec(77);
		return(pcr);
}
	else
{
tstvec(78);
		return (0xFFFFFFFF);
}
}
