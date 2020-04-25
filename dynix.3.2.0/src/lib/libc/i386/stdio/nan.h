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

/* $Header: nan.h 1.2 87/01/25 $
 *
 * Structures and macros to deal with detecting NaNs, infinities, and
 * denormals in the IEEE format.
 */

#if i386 || ns32000
typedef union 
{
	struct 
	{
		unsigned frac_low  : 32;
		unsigned frac_high : 20;
		unsigned exponent  : 11;
		unsigned sign_bit  : 1;
	} ieee_parts;
	double d;
} dnan;
#endif

#define NaN(X)		(((dnan *)&(X))->ieee_parts.exponent == 0x7ff)
#define INF(X)		(((dnan *)&(X))->ieee_parts.frac_low == 0 && \
			 ((dnan *)&(X))->ieee_parts.frac_high == 0)
#define DeN(X)		(((dnan *)&(X))->ieee_parts.exponent == 0 && \
			 ( ((dnan *)&(X))->ieee_parts.frac_low | \
			   ((dnan *)&(X))->ieee_parts.frac_high ) != 0)
#define SIGN(X)		(((dnan *)&(X))->ieee_parts.sign_bit)
#define GETVAL(X) 	(((dnan *)&(X))->ieee_parts.frac_low >> 20 | \
			 ((dnan *)&(X))->ieee_parts.frac_high << 12)
