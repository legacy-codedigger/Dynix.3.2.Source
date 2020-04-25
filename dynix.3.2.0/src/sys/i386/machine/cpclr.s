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

.data
.asciz	"$Header: cpclr.s 2.6 86/09/29 $"

/*
 * cpclr.s
 *	Various routines to copy and clear memory.
 *
 * TODO:
 *	Look into more optimal copy and zero routines: worry about alignment.
 *
 * i386 version.
 */

/* $Log:	cpclr.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

/*
 * bcopy(from, to, count)
 *	char *from, to;
 *	unsigned count;
 *
 * Copy data.  Initially simple byte-string-copy algorithm.
 *
 * Assumes non-overlapped strings, forward copy direction is ok.
 */

ENTRY(bcopy)
	movl	%edi, %eax	# save registers
	movl	%esi, %edx
	movl	SPARG0, %esi	# source
	movl	SPARG1, %edi	# destination
	movl	SPARG2, %ecx	# count
	shrl	$2, %ecx	# Figure # words to do.
	rep;	smovl		# do # longs (NOP if count==0).  Leaves ECX=0.
	movb	SPARG2, %cl	# ECX = low-order byte of count.
	andb	$3, %cl		# # remaining bytes to transfer
	rep;	smovb		# move the bytes (NOP if count == 0).
	movl	%edx, %esi	# restore registers
	movl	%eax, %edi
	RETURN

/*
 * ovbcopy(from, to, size)
 *	char *from, to;
 *	unsigned size;
 *
 * OVerlay bcopy. Only used in symbolic links.
 */

ENTRY(ovbcopy)
	movl	%edi, %eax	# save registers
	movl	%esi, %edx
	movl	SPARG0, %esi	# source
	movl	SPARG1, %edi	# destination
	movl	SPARG2, %ecx	# count
	cmpl	%edi, %esi	# Source < Destination?
	jl	ovback		# yes,  so copy backwards
	je	ovbret		# if == strings, nothing to do.
	rep;	smovb		# move the bytes
ovbret:	movl	%edx, %esi	# restore registers
	movl	%eax, %edi
	RETURN
ovback:
	addl	%ecx, %esi	# source += count...
	decl	%esi		#	... -1
	addl	%ecx, %edi	# destination += count...
	decl	%edi		#	... -1
	std			# set direction flag for backwards
	rep;	smovb		# move the bytes (backwards)
	cld			# re-set direction flag for forwards.
	movl	%edx, %esi	# restore registers
	movl	%eax, %edi
	RETURN

/*
 * bzero(base, length)
 *	char *base;
 *	unsigned length;
 *
 * Clear memory.  Currently simple 1-byte zap -- needs update.
 */

ENTRY(bzero)
	movl	%edi, %edx	# save register
	movl	SPARG0, %edi	# base
	movl	SPARG1, %ecx	# length
	shrl	$2, %ecx	# # of longs to do.
	xorl	%eax, %eax	# write zeroes.
	rep;	sstol		# clear longs first (leaves ECX == 0).
	movb	SPARG1, %cl	# ECX = low byte of lentgh.
	andb	$3, %cl		# # bytes left to move.
	rep;	sstob		# clear remaining bytes.
	movl	%edx, %edi	# restore register
	RETURN

/*
 * pgcopy(from, to)
 *	char *from, to;
 *
 * Fast block copy an aligned page of data.
 */

ENTRY(pgcopy)
	movl	%esi, %eax		# save esi (register variable).
	movl	%edi, %edx		# save edi, too.
	movl	SPARG0, %esi		# source address (from).
	movl	SPARG1, %edi		# target address (to).
	movl	$[NBPG/4], %ecx		# count == NBPG/sizeof(long).
	rep;	smovl			# move'em quick!
	movl	%eax, %esi		# restore esi.
	movl	%edx, %edi		# restore edi, too.
	RETURN

/*
 * ptefill(pte, val, cnt)
 *	struct pte *pte; int val, cnt;
 *
 * Fill a set of pte's with a given value.
 * Motivated by performance -- much faster than C for() loop.
 */

ENTRY(ptefill)
	movl	%edi, %edx		# save register variable.
	movl	SPARG0, %edi		# pte == destination.
	movl	SPARG1, %eax		# value to store.
	movl	SPARG2, %ecx		# count.
	rep;	sstol			# store 'em quickly!
	movl	%edx, %edi		# restore register variable.
	RETURN

/*
 * copyseg(vpn, paddr)
 *	register unsigned vpn;
 *	register unsigned paddr;
 *
 * Called by vmdup() to copy a page of data from parent to child process
 * during a fork().  "vpn" is virtual page number of source page,
 * guaranteed to be legit page.
 *
 * Assumes physical memory is mapped 1-1 virtually (ie, all physical
 * memory is accessible using virt=phys addresses).
 *
 * Assumes source page is not physical-mapped memory (vmdup() guarantees this).
 *
 * Assumes kernel page-fault handler can tollerate a page-fault for user
 * address in kernel mode.
 */

ENTRY(copyseg)
	movl	%esi, %eax		# save esi (register variable).
	movl	%edi, %edx		# save edi, too.
	movl	SPARG0, %esi		# source address (vpn).
	shll	$PGSHIFT, %esi		# vpn << PGSHIFT...
	addl	$VA_USER, %esi		# + VA_USER = kernel address of page.
	movl	SPARG1, %edi		# target address (to).
	movl	$[CLBYTES/4], %ecx	# count == CLBYTES/sizeof(long).
	rep;	smovl			# move'em quick!
	movl	%eax, %esi		# restore esi.
	movl	%edx, %edi		# restore edi, too.
	RETURN

/*
 * clearseg(base)
 *	char *base;
 *
 * Fast clear a system page of memory.  base is on a page boundary.
 *
 * Assumes physical memory is mapped 1-1 virtually (ie, all physical
 * memory is accessible using virt=phys addresses).
 */

ENTRY(clearseg)
	movl	%edi, %edx		# save edi -- register variable.
	movl	SPARG0, %edi		# target address.
	movl	$[CLBYTES/4], %ecx	# count = CLBYTES/sizeof(long).
	subl	%eax, %eax		# want to store a zero.
	rep;	sstol			# zap the page fastly!
	movl	%edx, %edi		# restore register variable.
	RETURN

/*
 * copyphys(sphys, tphys)
 *	unsigned sphys, tphys;
 *
 * Copy a system page from one physical address to another.
 *
 * Assumes physical memory is mapped 1-1 virtually (ie, all physical
 * memory is accessible using virt=phys addresses).
 */

ENTRY(copyphys)
	movl	%esi, %eax		# save esi (register variable).
	movl	%edi, %edx		# save edi, too.
	movl	SPARG0, %esi		# source address (sphys).
	movl	SPARG1, %edi		# target address (tphys).
	movl	$[CLBYTES/4], %ecx	# count == CLBYTES/sizeof(long).
	rep;	smovl			# move'em quick!
	movl	%eax, %esi		# restore esi.
	movl	%edx, %edi		# restore edi, too.
	RETURN
