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
 * $Header: inline.h 2.5 87/09/18 $
 *
 * inline.h
 *	Various in-line expansion asm functions.  i386 version.
 *
 * These don't deal with reading/writing particular registers.
 */

/* $Log:	inline.h,v $
 */

/*
 * Fast in-line version of setjmp().
 * This code is very dependent on order of fields in types.h
 *
 * NOTE: saves "esp" instead of esp-4 for stack pointer since in-line
 * expanded call has no argument on the stack.
 */

#ifndef	lint				/* lint can't handle asm fcts yet */

asm fsetjmp(jmpbuf)
{
%mem jmpbuf;
	movl	jmpbuf, %eax
	movl	%esp, 0(%eax)
	movl	%ebp, 4(%eax)
	movl	$9f, 8(%eax)
	subl	%eax, %eax
9:
}

/*
 * ip_hdr_cksum(iphdr)
 *	struct ip *iphdr;
 *
 * ip_hdr_cksum is the fast algorithm to calculate an ip header checksum.
 * This algoritm computes the 16-bit 1's complement of the 1's complement
 * sum of exactly 20 bytes of an ip header. The calculated checksum is returned.
 *
 * This is only used by ku_fastsend() and assumes the ip_sum field is zero.
 */

asm	ip_hdr_cksum(iphdr)
{
%	mem	iphdr;
	movl	iphdr,%ecx		/* address of iphdr */
	movl	(%ecx),%eax		/* do 5 32-bit adds */
	addl	4(%ecx),%eax
	adcl	8(%ecx),%eax		/* assumes ip_sum is ZERO */
	adcl	12(%ecx),%eax
	adcl	16(%ecx),%eax
	adcl	$0,%eax			/* add leftover carry */
	movl	%eax,%edx
	shrl	$16,%edx		/* Fold upper half onto lower 16-bits */
	addw	%dx,%ax
	adcw	$0,%ax			/* add leftover carry */
	notl	%eax			/* do 1's complement */
	andl	$0xffff,%eax		/* leave only 16-bit checksum */
}

#endif	lint
