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

/* $Id: mac2defs.h,v 2.8 88/09/02 11:46:15 ksb Exp $ */
/*
 *	Berkeley Pascal Compiler	(mac2defs.h)
 */

/*
 * NS32032 Registers
 */

/*
 * Scratch registers (we need them all because of the special purpose
 * munging we have to do)
 */
#define EAX		0
#define EBX		1
#define ECX		2
#define EDX		3
#define ESI		4
#define EDI		5

/*
 * Special purpose registers
 */
#define EBP		6		/* frame pointer		*/
#define ESP		7		/* stack pointer		*/
#define TMPREG		EBP		/* allocate temp regs off this	*/

/*
 * Float (double really) registers
 */
#define FP0		8		/* useless to us		*/
#define FP1		9		/* floating point return value	*/
#define FP2		10
#define FP3		11
#define FP4		12
#define FP5		13
#define FP6		14
#define FP7		15
#define REGSZ		(FP7+1)

extern	int fregs;
extern	int maxargs;

#define BYTEOFF(x)	((x)&03)
#define wdal(k)		(BYTEOFF(k)==0)		/* ZZ (long?) word align */
#define BITOOR(x)	((x)>>3)		/* bit offset to oreg offset */

/*
 * Some macros used in store():
 *	just evaluate the arguments, and be done with it...
 */
#define STOARG(p)
#define STOFARG(p)
#define STOSTARG(p)
#define genfcall(a,b)	gencall((a), (b))

#define MYREADER(p) myreader(p)

extern int optim2();
