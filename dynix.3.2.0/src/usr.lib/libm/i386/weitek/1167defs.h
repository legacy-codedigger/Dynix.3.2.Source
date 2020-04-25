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

/* $Header: 1167defs.h 1.2 87/06/16 $ */

/* $Log:	1167defs.h,v $
 */

/*	Define the starting address of the WTL-1167. */

#define	w1167		0xffc00000

/*	Define the floating point registers. */

#define	FP0		0
#define	FP1		1
#define	FP2		2
#define	FP3		3
#define	FP4		4
#define	FP5		5
#define	FP6		6
#define	FP7		7
#define	FP8		8
#define	FP9		9
#define	FP10		10
#define	FP11		11
#define	FP12		12
#define	FP13		13
#define	FP14		14
#define	FP15		15
#define	FP16		16
#define	FP17		17
#define	FP18		18
#define	FP19		19
#define	FP20		20
#define	FP21		21
#define	FP22		22
#define	FP23		23
#define	FP24		24
#define	FP25		25
#define	FP26		26
#define	FP27		27
#define	FP28		28
#define	FP29		29
#define	FP30		30
#define	FP31		31

/*	Define the operation-code symbols. */

#define	ADDS		0x00<<10|w1167
#define	LOADS		0x01<<10|w1167
#define	MULS		0x02<<10|w1167
#define	SUBS		0x04<<10|w1167
#define	DIVS		0x05<<10|w1167
#define	MULNS		0x06<<10|w1167
#define	FLOATS		0x07<<10|w1167
#define	CMPTS		0x08<<10|w1167
#define	NEGS		0x0a<<10|w1167
#define	ABSS		0x0b<<10|w1167
#define	CMPS		0x0c<<10|w1167
#define	AMULS		0x0e<<10|w1167
#define	FIXS		0x0f<<10|w1167
#define	CVTDS		0x11<<10|w1167
#define	MACS		0x12<<10|w1167
#define	FLOATD		0x27<<10|w1167
#define	MACDS		0x32<<10|w1167
#define	TSTTS		0x09<<10|w1167
#define	TSTS		0x0d<<10|w1167
#define	STORS		0x03<<10|w1167
#define	LDCTX		0x30<<10|w1167
#define	STCTX		0x31<<10|w1167
#define	LOADD		[0x21<<10]|w1167
#define	CVTSD		0x10<<10|w1167
#define	ADDD		0x20<<10|w1167
#define	MULD		0x22<<10|w1167
#define	SUBD		0x24<<10|w1167
#define	DIVD		0x25<<10|w1167
#define	MULND		0x26<<10|w1167
#define	CMPTD		0x28<<10|w1167
#define	NEGD		0x2a<<10|w1167
#define	ABSD		0x2b<<10|w1167
#define	CMPD		0x2c<<10|w1167
#define	AMULD		0x2e<<10|w1167
#define	FIXD		0x2f<<10|w1167
#define	TSTTD		0x29<<10|w1167
#define	TSTD		0x2d<<10|w1167
#define	STORD		0x23<<10|w1167
