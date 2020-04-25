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
.asciz	"$Header: userio.s 2.13 90/12/19 $"
.text

/*
 * userio.s
 *	Routines to copy data between user and kernel space.
 *
 * i386 version.
 *
 * 80386 allows page-table to be self-mapped (see VA_PT).  This allows
 * fast code to check the pte's directly.  Accessing page-table as
 * virtually contiguous array can fault (if ref a hole), thus u_fltaddr
 * tells trap() where to return, instead of panic.
 *
 * TODO:
 *	improve copyin/copyout copy algorithms (when/if improve bcopy).
 */

/* $Log:	userio.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

/*
 * copyin(from, to, count)
 *	char *from, *to;
 *	unsigned int count;
 *
 * Copy data from user space to kernel space.
 *
 * Return 0 on success.  Return EFAULT on access failure.
 *
 * Assumes RO is single bit in low byte of pte (see machine/seg.h).
 */

ENTRY(copyin)
	cmpb	$0, VA_UAREA+U_PMAPCNT		# any phys-mapped stuff?
	jne	cpin_uacc			# yup -- use C useracc.
	/*
	 * Verify user address space is readable (clone from useracc()).
	 */
	movl	SPARG0, %eax			# from.
	movl	%eax, %ecx			# for count offset.
	addl	$VA_USER, %eax			# + User start vaddr
	movl	%eax, %edx			# save for copy later.
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	leal	VA_PT(,%eax,4), %eax		# eax -> 1st pte.
	andl	$CLOFSET, %ecx			# ecx = base offset in page.
	addl	SPARG2, %ecx			# ecx = count.
#ifdef	DEBUG
	cmpl	$128*1024, %ecx			# catch large/negative counts
	jna	cpin_count_ok			# unsigned compare
	pushl	$cpin_mesg			# failed so panic
	CALL	_panic
	.data
cpin_mesg:
	.asciz	"copyin: count > 128K"
	.text
cpin_count_ok:
#endif	DEBUG
	movl	$cpin_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
cpin_rdval:					# check for reading.
	testb	$RO, (%eax)			# readable?
	jz	cpin_bad			# nope -- user can't read.
	addl	$[CLSIZE*4], %eax		# pte += CLSIZE.
	subl	$CLBYTES, %ecx			# count -= CLBYTES.
	jg	cpin_rdval			# while (count > 0)...
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	/*
	 * Address space is ok!  Copy the data.
	 * Code is clone of bcopy().
	 */
cpin_ok:
	movl	%esi, %eax			# save register var
	movl	%edx, %esi			# relocated from address
	movl	%edi, %edx			# save register var
	movl	SPARG1, %edi			# to
	movl	SPARG2, %ecx			# count
	shrl	$2, %ecx			# Figure # words to do.
	rep;	smovl				# do longs.  Leaves ECX=0.
	movb	SPARG2, %cl			# ECX = low-order byte of count.
	andb	$3, %cl				# # remaining bytes to transfer
	rep;	smovb				# move the bytes.
	movl	%eax, %esi			# restore registers
	movl	%edx, %edi
	subl	%eax, %eax			# return zero ==> succeed.
	RETURN
	/*
	 * Process has phys-maps ==> use C-version of useracc() to check
	 * address space.  If all ok, then can copyin().
	 */
cpin_uacc:
	pushl	$B_READ				# check for read
	pushl	4+SPARG2			# count
	pushl	8+SPARG0			# "from"
	CALL	__useracc			# check for legit.
	addl	$12, %esp			# clear stack
	movl	SPARG0, %edx			# cpin_ok assumes...
	addl	$VA_USER, %edx			# edx = relocated "from" address
	orl	%eax, %eax			# is ok to copy?
	jne	cpin_ok				# yup.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
cpin_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movl	$EFAULT, %eax			# return EFAULT ==>
	RETURN					# Fail.

/*
 * copyout(from, to, count)
 *	char *from, *to;
 *	unsigned int count;
 *
 * Copy data from kernel space to user space.
 *
 * Return 0 on success.  Return EFAULT on access failure.
 *
 * Assumes RW consists of bits in low byte of pte (see machine/seg.h).
 */

ENTRY(copyout)
	cmpb	$0, VA_UAREA+U_PMAPCNT		# any phys-mapped stuff?
	jne	cpout_uacc			# yup -- use C useracc.
	/*
	 * Verify user address space is writeable (clone from useracc()).
	 */
	movl	SPARG1, %eax			# to.
	movl	%eax, %ecx			# for count offset.
	addl	$VA_USER, %eax			# + User start vaddr
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	leal	VA_PT(,%eax,4), %edx		# edx -> 1st pte.
	andl	$CLOFSET, %ecx			# ecx = base offset in page.
	addl	SPARG2, %ecx			# ecx = count.
#ifdef	DEBUG
	cmpl	$128*1024, %ecx			# catch large/negative counts
	jna	cpout_count_ok			# unsigned compare
	pushl	$cpout_mesg			# failed so panic
	CALL	_panic
	.data
cpout_mesg:
	.asciz	"copyout: count > 128K"
	.text
cpout_count_ok:
#endif	DEBUG
	movl	$cpout_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
cpout_wrval:					# check for reading.
	movb	(%edx), %al			# low byte of pte.
	andb	$RW, %al			# get only "RW" bits.
	cmpb	$RW, %al			# is writeable?
	jne	cpout_bad			# nope -- user can't write.
	addl	$[CLSIZE*4], %edx		# pte += CLSIZE.
	subl	$CLBYTES, %ecx			# count -= CLBYTES.
	jg	cpout_wrval			# while (count > 0)...
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	/*
	 * Address space is ok!  Copy the data.
	 * Code is clone of bcopy().
	 */
cpout_ok:
	movl	%edi, %eax			# save registers
	movl	%esi, %edx
	movl	SPARG0, %esi			# from
	movl	SPARG1, %edi			# to
	addl	$VA_USER, %edi			# relocate to kernel vaddr.
	movl	SPARG2, %ecx			# count
	shrl	$2, %ecx			# Figure # words to do.
	rep;	smovl				# do longs.  Leaves ECX=0.
	movb	SPARG2, %cl			# ECX = low-order byte of count.
	andb	$3, %cl				# # remaining bytes to transfer
	rep;	smovb				# move the bytes.
	movl	%edx, %esi			# restore registers
	movl	%eax, %edi
	subl	%eax, %eax			# return zero ==> succeed.
	RETURN
	/*
	 * Process has phys-maps ==> use C-version of useracc() to check
	 * address space.  If all ok, then can copyin().
	 */
cpout_uacc:
	pushl	$B_WRITE			# check for write
	pushl	4+SPARG2			# count
	pushl	8+SPARG1			# "to"
	CALL	__useracc			# check for legit.
	addl	$12, %esp			# clear stack
	orl	%eax, %eax			# is ok to copy?
	jne	cpout_ok			# yup.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
cpout_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movl	$EFAULT, %eax			# return EFAULT ==>
	RETURN					# Fail.

/*
 * Fetch a byte from User space.
 *
 * fubyte(address)
 * fuibyte(address)
 *	char *address;
 *
 * Return fetched byte if successful.
 * Return -1 if address invalid.
 */

ENTRY(fuibyte)
ENTRY(fubyte)
	movl	SPARG0, %eax			# from.
	cmpb	$0, VA_UAREA+U_PMAPCNT		# process using pmaped memory?
	jne	fb_uacc				# yes, use C useracc()
	/*
	 * Verify byte is user readable (clone from useracc()).
	 */
	addl	$VA_USER, %eax			# + User start vaddr
	movl	%eax, %edx			# fetch from here (later).
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	movl	$fb_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
	testb	$RO, VA_PT(,%eax,4)		# readable?
	jz	fb_bad				# nope -- user can't read.
	/*
	 * Byte is readable by user.  Get it.
	 */
fb_ok:	subl	%eax, %eax			# most of result is zero.
	movl	%eax, VA_UAREA+U_FLTADDR	# reset fault address.
	movb	(%edx), %al			# get the byte.
	RETURN					# Done.
	/*
	 * Process has physical maps.  Use C-version of useracc() to
	 * check if ok to transfer.
	 */
fb_uacc:
	pushl	$B_READ				# check for reading
	pushl	$1				# count = 1
	pushl	%eax				# from
	CALL	__useracc			# check if ok to transfer
	addl	$12, %esp			# clear stack
	movl	SPARG0, %edx			# fb_ok assumes...
	addl	$VA_USER, %edx			# edx = relocated "from" address
	orl	%eax, %eax			# is ok to copy?
	jne	fb_ok				# yup.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
fb_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movl	$-1, %eax			# return -1 ==>
	RETURN					# Fail.


/*
 * Fetch a doubleword from User space.
 *
 * fuword(address)
 * fuiword(address)
 *	char *address;
 *
 * Return fetched doubleword if successful.
 * Return -1 if address invalid.
 */

ENTRY(fuiword)
ENTRY(fuword)
	movl	SPARG0, %eax			# from.
	cmpb	$0, VA_UAREA+U_PMAPCNT		# process using pmaped memory?
	jne	fw_uacc				# yes, use C useracc()
	/*
	 * Verify word is user readable (clone from useracc()).
	 */
	movl	$fw_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
	addl	$VA_USER, %eax			# + User start vaddr
	movl	%eax, %edx			# fetch from here (later).
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	testb	$RO, VA_PT(,%eax,4)		# 1st-byte readable?
	jz	fw_bad				# nope -- user can't read.
	leal	3(%edx), %eax			# &last-byte
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	testb	$RO, VA_PT(,%eax,4)		# last-byte readable?
	jz	fw_bad				# nope -- user can't read.
	/*
	 * Word is readable by user.  Get it.
	 */
fw_ok:	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address.
	movl	(%edx), %eax			# get the word.
	RETURN					# Done.
	/*
	 * Process has physical maps.  Use C-version of useracc() to
	 * check if ok to transfer.
	 */
fw_uacc:
	pushl	$B_READ				# check for reading
	pushl	$4				# count = 4
	pushl	%eax				# from
	CALL	__useracc			# check if ok to transfer
	addl	$12, %esp			# clear stack
	movl	SPARG0, %edx			# fw_ok assumes...
	addl	$VA_USER, %edx			# edx = relocated "from" address
	orl	%eax, %eax			# is ok to copy?
	jne	fw_ok				# yup.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
fw_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movl	$-1, %eax			# return -1 ==>
	RETURN					# Fail.

/*
 * Store a byte of data into User space.
 *
 * subyte(address, data)
 * suibyte(address, data)
 *	char *address;
 *	char data;
 *
 * Return zero if successful.
 * Return -1 if address invalid.
 */

ENTRY(suibyte)
ENTRY(subyte)
	movl	SPARG0, %eax			# to.
	cmpb	$0, VA_UAREA+U_PMAPCNT		# process using pmaped memory?
	jne	sb_uacc				# yes, use C useracc()
	/*
	 * Verify byte is user writable (clone from useracc()).
	 */
	movl	$sb_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
	addl	$VA_USER, %eax			# + User start vaddr
	movl	%eax, %edx			# fetch from here (later).
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	movb	VA_PT(,%eax,4), %cl		# is page...
	andb	$RW, %cl			#	...writable ??
	cmpb	$RW, %cl			# eh?
	jne	sb_bad				# nope -- user can't write it.
	/*
	 * Byte is writable by user.  Write it.
	 */
sb_ok:	subl	%eax, %eax			# return value = 0 ==> ok.
	movl	%eax, VA_UAREA+U_FLTADDR	# reset fault address.
	movb	SPARG1, %cl			# write...
	movb	%cl, (%edx)			#	...the byte.
	RETURN					# Done.
	/*
	 * Process has physical maps.  Use C-version of useracc() to
	 * check if ok to transfer.
	 */
sb_uacc:
	pushl	$B_WRITE			# check for writing
	pushl	$1				# count = 1
	pushl	%eax				# "to"
	CALL	__useracc			# check if ok to transfer
	addl	$12, %esp			# clear stack
	movl	SPARG0, %edx			# sb_ok assumes...
	addl	$VA_USER, %edx			# edx = relocated "to" address
	orl	%eax, %eax			# is ok to copy?
	jne	sb_ok				# yup.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
sb_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movl	$-1, %eax			# return -1 ==>
	RETURN					# Fail.

/*
 * Store a doubleword of data into User space.
 *
 * suword(address, data)
 * suiword(address, data)
 *	char *address;
 *	int data;
 *
 * Return zero if successful.
 * Return -1 if address invalid.
 */

ENTRY(suiword)
ENTRY(suword)
	movl	SPARG0, %eax			# to.
	cmpb	$0, VA_UAREA+U_PMAPCNT		# process using pmaped memory?
	jne	sw_uacc				# yes, use C useracc()
	/*
	 * Verify word is user writable (clone from useracc()).
	 */
	movl	$sw_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
	addl	$VA_USER, %eax			# + User start vaddr
	movl	%eax, %edx			# fetch from here (later).
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	movb	VA_PT(,%eax,4), %cl		# is page...
	andb	$RW, %cl			#	...writable ??
	cmpb	$RW, %cl			# eh?
	jne	sw_bad				# nope -- user can't write it.
	leal	3(%edx), %eax			# &last-byte
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	movb	VA_PT(,%eax,4), %cl		# is page...
	andb	$RW, %cl			#	...writable ??
	cmpb	$RW, %cl			# eh?
	jne	sw_bad				# nope -- user can't write it.
	/*
	 * Word is writable by user.  Write it.
	 */
sw_ok:	subl	%eax, %eax			# return value = 0 ==> ok.
	movl	%eax, VA_UAREA+U_FLTADDR	# reset fault address.
	movl	SPARG1, %ecx			# write...
	movl	%ecx, (%edx)			#	...the word.
	RETURN					# Done.
	/*
	 * Process has physical maps.  Use C-version of useracc() to
	 * check if ok to transfer.
	 */
sw_uacc:
	pushl	$B_WRITE			# check for writing
	pushl	$4				# count = 4
	pushl	%eax				# "to"
	CALL	__useracc			# check if ok to transfer
	addl	$12, %esp			# clear stack
	movl	SPARG0, %edx			# sw_ok assumes...
	addl	$VA_USER, %edx			# edx = relocated "to" address
	orl	%eax, %eax			# is ok to copy?
	jne	sw_ok				# yup.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
sw_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movl	$-1, %eax			# return -1 ==>
	RETURN					# Fail.

/*
 * useracc(base, count, rw)
 *	char	*base;
 *	int	count;
 *	int	rw;			# B_READ or B_WRITE
 *
 * Validate a range of user addresses for read or write.  Used in physio()
 * and others to validate the IO before DMAing/etc all over something.
 *
 * Return non-zero if successful, zero if bad access.
 *
 * Assumes RO is single bit in low byte of pte, RW consists of bits in
 * low byte of pte (see machine/seg.h).
 */

ENTRY(useracc)
	cmpb	$0, VA_UAREA+U_PMAPCNT		# any phys-mapped stuff?
	jne	cuseracc			# yup -- use C version.
	movl	SPARG0, %eax			# base.
	movl	%eax, %ecx			# for count offset.
	addl	$VA_USER, %eax			# + User start vaddr
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT (== pte idx)
	leal	VA_PT(,%eax,4), %edx		# edx -> 1st pte.
	andl	$CLOFSET, %ecx			# ecx = base offset in page.
	addl	SPARG1, %ecx			# ecx = count.
	cmpl    $0x0, SPARG1			# check for negative count
	jl      uacc_bad
	movl	$uacc_bad, VA_UAREA+U_FLTADDR	# in case really bad address.
	cmpl	$B_WRITE, SPARG2		# looking for write?
	jne	uacc_read			# nope -- check for reading.
uacc_write:					# check for writing.
	movb	(%edx), %al			# low byte of pte.
	andb	$RW, %al			# get only "RW" bits.
	cmpb	$RW, %al			# is writeable?
	jne	uacc_bad			# nope -- user can't write.
	addl	$[CLSIZE*4], %edx		# pte += CLSIZE.
	subl	$CLBYTES, %ecx			# count -= CLBYTES.
	jg	uacc_write			# while (count > 0)...
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movb	$1, %al				# return non-zero ==>
	RETURN					# success.
uacc_read:					# check for reading.
	testb	$RO, (%edx)			# readable?
	jz	uacc_bad			# nope -- user can't read.
	addl	$[CLSIZE*4], %edx		# pte += CLSIZE.
	subl	$CLBYTES, %ecx			# count -= CLBYTES.
	jg	uacc_read			# while (count > 0)...
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	movb	$1, %al				# return non-zero ==>
	RETURN					# success.
	/*
	 * Fail: either pte didn't test, or faulted in non-existent part
	 * of page-table (and trap() landed us here via u.u_fltaddr).
	 */
uacc_bad:				
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address
	subl	%eax, %eax			# return zero ==>
	RETURN					# Fail.
	/*
	 * Use C version of useracc() -- process has phys maps, so need
	 * to look a pte's in more detail.  Note kludge: assembler can't
	 * handle jump to external label.
	 */
cuseracc:
	#jmp	__useracc			# ideally could just jump...
	movl	$__useracc, %eax		# "jmp"...
	jmp	*%eax				#	... __useracc()

/*
 * Copy a null terminated string from user space into kernel space.
 *
 * copyinstr(fromaddr, toaddr, maxlen, lencopied)
 *	char	*fromaddr, *toaddr;
 *	int	maxlen, *lencopied;
 *
 * Returns 0 for success, EFAULT for bad source address, ENOENT for not
 * enough room.
 *
 * Does *not* store length moved if EFAULT.
 */

ENTRY(copyinstr)
	pushl	%ebp				# standard entry
	movl	%esp, %ebp			#
	pushl	%esi				# save...
	pushl	%edi				#	...register
	pushl	%ebx				#		...vars
	movl	BPARG0, %esi			# "fromaddr"
	movl	BPARG1, %edi			# "toaddr"
	movl	BPARG2, %ebx			# ebx = max len, bytes in buffer
	testl	%ebx, %ebx			# reasonable max?
	jle	cios_bad			# nope.
	cmpb	$0, VA_UAREA+U_PMAPCNT		# process using pmaped memory?
	jne	cis_uacc			# yup -- use C useracc to check
cis_ok:	addl	$VA_USER, %esi			# relocated to kernel vaddr
	movl	%esi, %eax			# relocated "from"
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT == pte index
	leal	VA_PT(,%eax,4), %edx		# edx -> pte's
	movl	$cios_bad, VA_UAREA+U_FLTADDR	# in case *bad* fault.
	movl	%esi, %ecx			# count...
	andl	$CLOFSET, %ecx			#	bytes...
	negl	%ecx				#		to end...
	addl	$CLBYTES, %ecx			#			of page.
0:						# New page.
	cmpl	%ecx, %ebx			# want min(pgcnt, total cnt)
	jg	1f				# if total cnt <= pg cnt...
	movl	%ebx, %ecx			# limit by remaining count
1:	testb	$RO, (%edx)			# is page readable?
	jz	cios_bad			# nope.
	subl	%ecx, %ebx			# ebx = count after page
2:
	slodb					# al = fetch byte thru esi
	sstob					# store byte thru edi
	orb	%al, %al			# moved a null byte?
	jz	cios_done			# yup -- finish up.
	loop	2b				# keep going if more room.
	orl	%ebx, %ebx			# any buffer left?
	jz	cios_incomplete			# nope -- ran out of buffer.
	addl	$[CLSIZE*4], %edx		# pte += CLSIZE
	movl	$CLBYTES, %ecx			# page boundary now.
	jmp	0b				# continue.
	/*
	 * Process has phys-maps.  Use C verison of useracc() to check
	 * transfer range.  This is somewhat redundant with checking
	 * when moving data, but those don't check mapped pte's.
	 *
	 * At this point, %esi == unrelocated from-address, %edi == to-address,
	 * %ebx == maxlen (known > 0).
	 */
cis_uacc:
	pushl	$B_READ				# need to read it.
	pushl	%ebx				# max length.
	pushl	%esi				# from-address.
	CALL	__useracc			# useracc(from, maxlen, B_READ)
	addl	$12, %esp			#
	testl	%eax, %eax			# is ok to read it?
	jnz	cis_ok				# Yup -- continue.
	/*
	 * _useracc() ==> not good.  In case error was due to being too
	 * close to a phys-mapped space, try a fubyte() loop.
	 */
0:	pushl	%esi				# unrelocated from-address.
	CALL	_fubyte				# %al = fubyte(%esi)
	popl	%ecx				#
	testl	%eax, %eax			# result < 0 ...
	jl	cios_bad			#	==> bad news.
	sstob					# %al -> (%edi), ++%edi.
	incl	%esi				# bump "from" address.
	testb	%al, %al			# moved a null byte?
	jz	cios_done			# yup - end of string.
	decl	%ebx				# -- maxlen.
	jnz	0b				# if any left, continue.
	jmp	cios_incomplete			# else return ENOENT.

/*
 * The following code through the RETURN after cios_incomplete
 * are shared among copyinstr, copyoutstr, copystr.
 */

/*
 * No dice.  Bad fault on verify pte's, or non-accessible page.
 */
cios_bad:
	movl	BPARG3, %esi			# &length
	orl	%esi, %esi			# should we return length?
	je	9f				# nope.
	movl	$0, (%esi)			# make believe nothing copied.
9:	popl	%ebx				# restore...
	popl	%edi				#	...register
	popl	%esi				#		...vars.
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address.
	movl	$EFAULT, %eax			# EFAULT return value
	leave					#
	RETURN
/*
 * Done -- moved a string including the null byte.
 * Assumes %edi points after the null byte copied.
 */
	.align	2
cos_done:					# copyoutstr() done.
	subl	$VA_USER, %edi			# un-relocate.
cios_done:
	movl	BPARG3, %esi			# &length
	orl	%esi, %esi			# should we return length?
	je	9f				# nope.
	movl	%edi, %eax			# copied= addr after last byte
	subl	BPARG1, %eax			#	- addr of first byte
	movl	%eax, (%esi)			# store length copied.
9:	popl	%ebx				# restore...
	popl	%edi				#	...register
	popl	%esi				#		...vars.
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address.
	subl	%eax, %eax			# return success (done)
	leave					# restore frame
	RETURN					# Done.
/*
 * Ran out of buffer.
 */
	.align	2
cios_incomplete:
	movl	BPARG3, %esi			# &length
	orl	%esi, %esi			# should we return length?
	je	9f				# nope.
	movl	BPARG2, %eax			# copied = maxlen
	movl	%eax, (%esi)			# since ecx -> 0.
9:	popl	%ebx				# restore...
	popl	%edi				#	...register
	popl	%esi				#		...vars.
	movl	$0, VA_UAREA+U_FLTADDR		# reset fault address.
	movl	$ENOENT, %eax			# return ENOENT
	leave					# restore frame
	RETURN					# Done.

/*
 * Copy a null terminated string from kernel space to user space.
 *
 * copyoutstr(fromaddr, toaddr, maxlen, &lencopied)
 *	char	*fromaddr, *toaddr;
 *	int	maxlen, *lencopied;
 *
 * Returns 0 for success, EFAULT for bad target address, ENOENT for not
 * enough room.
 *
 * Does *not* store length moved if EFAULT.
 */

ENTRY(copyoutstr)
	pushl	%ebp				# standard entry
	movl	%esp, %ebp			#
	pushl	%esi				# save...
	pushl	%edi				#	...register
	pushl	%ebx				#		...vars
	movl	BPARG0, %esi			# "fromaddr"
	movl	BPARG1, %edi			# "toaddr"
	movl	BPARG2, %ebx			# ebx = max len, bytes in buffer
	testl	%ebx, %ebx			# reasonable max?
	jle	cios_bad			# nope.
#ifdef	DEBUG
	/*
	 * Currently, only caller is execve(), thus no caller has phys-mapped
	 * memory.  When/if need this, clone code from copyinstr().
	 */
	cmpb	$0, VA_UAREA+U_PMAPCNT		# process using pmaped memory?
	je	9f				# nope -- is ok.
	pushl	$8f				# yup -- bad news...
	CALL	_panic				#	... can't happen.
8:	.asciz	"copyoutstr: phys-mapped memory"
9:
#endif	DEBUG
	addl	$VA_USER, %edi			# relocated to kernel vaddr
	movl	%edi, %eax			# relocated "to"
	shrl	$PGSHIFT, %eax			# >>= PGSHIFT == pte index
	leal	VA_PT(,%eax,4), %edx		# edx -> pte's
	movl	$cios_bad, VA_UAREA+U_FLTADDR	# in case *bad* fault.
	movl	%edi, %ecx			# count...
	andl	$CLOFSET, %ecx			#	bytes...
	negl	%ecx				#		to end...
	addl	$CLBYTES, %ecx			#			of page.
0:						# New page.
	cmpl	%ecx, %ebx			# want min(pgcnt, total cnt)
	jg	1f				# if total cnt <= pg cnt...
	movl	%ebx, %ecx			# limit by remaining count
1:	movb	(%edx), %al			# protection byte
	andb	$RW, %al			# is page...
	cmpb	$RW, %al			#	...writeable?
	jne	cios_bad			# nope.
	subl	%ecx, %ebx			# ebx = count after page
2:
	slodb					# al = fetch byte thru esi
	sstob					# store byte thru edi
	orb	%al, %al			# moved a null byte?
	jz	cos_done			# yup -- finish up.
	loop	2b				# keep going if more room.
	orl	%ebx, %ebx			# any buffer left?
	jz	cios_incomplete			# nope -- ran out of buffer.
	addl	$[CLSIZE*4], %edx		# pte += CLSIZE
	movl	$CLBYTES, %ecx			# page boundary now.
	jmp	0b				# continue.

/*
 * Copy a null terminated string from here to there in kernel space.
 *
 * copystr(fromaddr, toaddr, maxlen, &lencopied)
 *	char	*fromaddr, *toaddr;
 *	int	maxlen, *lencopied;
 */

ENTRY(copystr)
	pushl	%ebp				# standard entry
	movl	%esp, %ebp			#
	pushl	%esi				# save...
	pushl	%edi				#	...register
	pushl	%ebx				#		...vars
	movl	BPARG0, %esi			# "fromaddr"
	movl	BPARG1, %edi			# "toaddr"
	movl	BPARG2, %ecx			# = max len, bytes left in buf
	orl	%ecx, %ecx			# reasonable max?
	jle	cios_bad			# nope.
2:
	slodb					# al = fetch byte thru esi
	sstob					# store byte thru edi
	orb	%al, %al			# moved a null byte?
	jz	cios_done			# yup -- finish up.
	loop	2b				# keep going if more room.
	jmp	cios_incomplete			# ran out of buffer.
