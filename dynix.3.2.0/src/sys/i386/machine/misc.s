/* $Copyright: $
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

.data
.asciz	"$Header: misc.s 2.8 1991/04/17 23:03:46 $"

/*
 * misc.s
 *	Miscellaneous Assembly routines.
 */

/* $Log: misc.s,v $
 *
 *
 */

#include "assym.h"
#include "../machine/asm.h"

/*
 * void
 * addupc(upc, u_prof, ticks)
 *	caddr_t	upc;		# saved user pc on stack
 *	struct uprof *u_prof	# usually &u.u_prof
 *	int ticks;		# number of clock ticks 
 *
 * Update user profiling buffer. If pc out of range, ignore and return.
 * If user buffer not accessable, turn off profiling and return.
 * There is no return value.
 */

ENTRY(addupc)
	movl	SPARG0, %eax		# user pc
	movl	SPARG1, %edx		# &u.u_prof
	subl	PR_OFF(%edx), %eax	# corrected pc: pc - u.u_prof.pr_off
	orl	%eax, %eax		# result less than 0?
	jl	addone			# yes,  done.
	shrl	$1, %eax		# right shift corrected pc.
	movl	PR_SCALE(%edx), %ecx	# u.u_prof.pr_scale.
	shrl	$1, %ecx		# same for pr_scale.
	mull	%ecx			# edx:eax = corrected pc * scale.
	testl	$0xffffc000, %edx	# high-order non-zero?
	jnz	addone			# yes -- value too big.
	shrdl	$14, %edx, %eax		# eax = (edx:eax) >> 14
	andb	$0xfe, %al		# zero low-order eax bit.
	movl	SPARG1, %edx		# &u.u_prof
	cmpl	%eax, PR_SIZE(%edx)	# length
	jbe	addone			# out of range
	addl	PR_BASE(%edx), %eax	# eax = bucket address in user space
	/*
	 * Verify user can read and write the bucket.
	 * If process has phys-maps, use C version of useracc().
	 */
	movl	$adderr, VA_UAREA+U_FLTADDR	# in case *bad* fault.
	cmpb	$0, VA_UAREA+U_PMAPCNT	# any phys-mapped stuff?
	jne	addpmap			# yup -- use C version.
addpmok:addl	$VA_USER, %eax		# relocate to kernel virtual address.
	movl	%eax, %edx		# fetch from here (later).
	shrl	$PGSHIFT, %eax		# >>= PGSHIFT (== pte idx)
	movb	VA_PT(,%eax,4), %cl	# is page...
	andb	$RW, %cl		#	...writable ??
	cmpb	$RW, %cl		# eh?
	jne	adderr			# nope -- user can't write it.
	leal	1(%edx), %eax		# &last-byte
	shrl	$PGSHIFT, %eax		# >>= PGSHIFT (== pte idx)
	movb	VA_PT(,%eax,4), %cl	# is page...
	andb	$RW, %cl		#	...writable ??
	cmpb	$RW, %cl		# eh?
	jne	adderr			# nope -- user can't write it.
	/*
	 * Bump tick count.
	 */
	movl	SPARG2, %eax		# tick count
	pushl	%ebx			# use ebx as scratch register
	movl	(%edx), %ebx		# fetch current count
	addw	%ax, %ebx		# bump the short
	jc	ofl			# carry in on, it overflowed
	movl	%ebx, (%edx)		# No overflow, write new count
ofl:	popl	%ebx			# Restore old value of ebx
addone:	movl	$0, VA_UAREA+U_FLTADDR	# reset fault address.
	RETURN				# Done.
	/*
	 * Process has some phys-maps ==> use __useracc() to check.
	 * Assumes __useracc() doesn't modify 1st argument.
	 */
addpmap:pushl	$B_WRITE		# check for write access...
	pushl	$2			#	...for two bytes
	pushl	%eax			#		...at this user address.
	CALL	__useracc		# see if it's ok.
	popl	%edx			# recover user address (Note: diff reg).
	addl	$8, %esp		# clear rest of stack.
	testl	%eax, %eax		# was it an ok return?
	movl	%edx, %eax		# restore user base address.
	jnz	addpmok			# yup -- go for it (redundant check ok).
	/*
	 * Some kind of error -- turn off profiling.
	 */
adderr:	movl	$0, VA_UAREA+U_FLTADDR	# reset fault address.
	movl	SPARG1, %eax		# &u.u_prof
	movl	$0, PR_SCALE(%eax)	# bad - turn off profiling.
	RETURN				# Done.

/*
 * bcmp(s1, s2, n)
 *	char *s1,s2;
 *	int n;
 *
 * Compare strings of length n. If s1=s2 return 0, else return non-zero.
 */

ENTRY(bcmp)
	movl	%edi, %eax	# save registers
	movl	%esi, %edx
	movl	SPARG0, %esi	# s1
	movl	SPARG1, %edi	# s2
	movl	SPARG2, %ecx	# length
	jcxz	bcmp_done	# nothin' to compare.
	repz;	scmpb		# do byte by byte comparison
	jne	bcmp_notequal	# quit due to count or cmp?
bcmp_done:
	movl	%edx, %esi	# restore registers
	movl	%eax, %edi
	movl	%ecx, %eax	# return value == 0 (ecx is zero from above)
	RETURN
bcmp_notequal:
	movb	$1, %cl		# insure return value != 0
	jmp	bcmp_done

/*
 * bit = ffs(mask)
 *	unsigned int mask;
 *
 * Return first found set bit position (1-32) or zero if none set.
 */

ENTRY(ffs)
	movl	$-1, %eax	# in case no bits set
	bsfl	%eax, SPARG0	# check all 32 bits of arg
	incl	%eax		# incr by one for 0-32 (0 if SPARG==0)
	RETURN

/*
 * struct list {
 *	struct list *link;
 *	struct list *rlink;
 * }
 *
 * insque( entry, predecessor )
 *	struct list *entry, *predecessor;
 *
 * Insert entry into a doubly-linked list. Similar rules apply as in remque.
 */

ENTRY(insque)
	movl	SPARG0, %eax	# entry
	movl	SPARG1, %ecx	# predecessor
	movl	(%ecx), %edx
	movl	%edx, (%eax)	# foward link of entry
	movl	%ecx, 4(%eax)	# backward link of entry
	movl	%eax, 4(%edx)	# backward link of successor
	movl	%eax, (%ecx)	# forward link of predecessor
	RETURN

/*
 * struct list {
 *	struct list *link;
 *	struct list *rlink;
 * }
 *
 * remque(linkptr)
 *	struct list *linkptr;
 *
 * Unlink item from doubly-linked list. Assumes forward pointer is first item
 * in structure. Back pointer is second item. Thus prochd, proc, sema, and
 * engine structures MUST have linkages first.
 */

ENTRY(remque)
	movl	SPARG0, %eax
	movl	(%eax), %edx		# %edx has successor
	movl	4(%eax), %ecx		# %ecx has predecessor
	movl	%edx, (%ecx)		# forward link of predecessor
	movl	%ecx, 4(%edx)		# backward link of successor
	RETURN

/*
 * strlen(string)
 *	char *string;
 *
 * Return length of string.
 */

ENTRY(strlen)
	movl	%edi, %edx		# save register var
	movl	SPARG0, %edi		# string
	movl	$-1, %ecx		# large count
	xorb	%al, %al		# match null in %al
	repnz;	scab			# scan forward till null byte
	leal	2(%ecx), %eax		# eax = -len
	negl	%eax			# eax = len
	movl	%edx, %edi		# restore register var
	RETURN

#ifdef	NOT_INLINE
/*
 * Byte swapping routines used by the network - although they are
 *	generally applicable if byte swapping is an issue
 */

/*
 * netorder = htonl(hostorder)
 *
 * Rotates words then bytes in words
 */

ENTRY(htonl)
	movl	SPARG0, %eax	# 3!2!1!0
	xchgb	%ah, %al	# 3!2!0!1
	roll	$16, %eax	# 0!1!3!2
	xchgb	%ah, %al	# 0!1!2!3
	RETURN

/*
 * netorder = htons(hostorder)
 *
 * Rotates two lower bytes and clears upper two
 */

ENTRY(htons)
	movzwl	SPARG0, %eax	# zero!zero!1!0
	xchgb	%ah, %al	# zero!zero!0!1
	RETURN

/*
 * hostorder = ntohl(netorder)
 *
 * Rotates words then bytes in words
 */

ENTRY(ntohl)
	movl	SPARG0, %eax	# 3!2!1!0
	xchgb	%ah, %al	# 3!2!0!1
	roll	$16, %eax	# 0!1!3!2
	xchgb	%ah, %al	# 0!1!2!3
	RETURN

/*
 * hostorder = ntohs(netorder)
 *
 * Rotates two lower bytes and clears upper two
 */

ENTRY(ntohs)
	movzwl	SPARG0, %eax	# zero!zero!1!0
	xchgb	%ah, %al	# zero!zero!0!1
	RETURN
#endif	NOT_INLINE	

/*
 * enable_nmi: NMI's are enabled at the processor by
 * an iret. This routine enables interrupts and does
 * an iret back to the caller, thus enabling interrupts
 * and NMI's. Only called by trap().
 *	On entry, stack looks like
 *		<return_eip>
 *	Change to (stack grows down)
 *		<flags>
 *		<kernel_cs>
 *		<return_eip>
 *	so the iret works
 */

ENTRY(enable_nmi)
	popl	%eax		# return addr
	sti
	pushfl
	pushl	$KERNEL_CS
	pushl	%eax
	iret
