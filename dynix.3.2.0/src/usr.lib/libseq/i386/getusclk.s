/* $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
 */

/* $Header: getusclk.s 1.1 87/03/23 $
*/

.file	"getusclk.s"
/*
 *	unsigned long getusclk();
 *		Accumulates a 32-bit microsecond clock value from two
 *		16-bit clk words maintained on the usclk device.  Since
 *		the host reads asynchronously with the usclk device
 *		update, there is a potential race condition.  To solve
 *		this race, we use a derivative of Lamport's multidigit
 *		update algorithm -- Lamport, L., "Concurrent Reading
 *		and Writing," pp 806-811, Comm ACM, Nov 1977, Vol 20,
 *		Num 11.  The two values of clk_hi serve as the two
 *		sequence counters.  This ensures that getusclk32 can
 *		return a consistent 32-bit value.
 *
 *	References: extern usclk16_t *usclk_p
 *	Uses: r0, r1, r2 .  Normally scratch by C calling conventions.
 *	Returns: r0 .  Per normal C conventions.
 */

/* $Log:	getusclk.s,v $
 */

#define USCLK_HI2	-2	/* byte offset from usclk_p */
#define USCLK_LO	0
#define USCLK_HI1	2

#ifdef KXX

.text
.data
.text
/*  Two entry points, one for Fortran, one for C */
.globl	_getusclk /* C language */
.globl	getusclk  /* Fortran, Pascal with -e */
.globl	_getusclk_ /* New Fortran conveion */
LL0:
	.align	2
.data
.text
_getusclk:
getusclk:
_getusclk_:
L17:
	movl	_usclk_p,%edx		# Pointer to mapped usclk device.
	movw	USCLK_HI2(%edx),%eax		# Get HI2.
	movw	%eax, %ecx
	shll	16,%eax			# Assemble usclk32 timval, using LO.
	orw	USCLK_LO(%edx),%eax
	cmpw	%ecx,USCLK_HI1(%edx)		# Algorithm: if HI2 != HI1, then retry.
	jne	L17
	ret				# Return consistent 32-bit timval in r0.

#endif KXX
