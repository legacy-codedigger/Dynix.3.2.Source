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
.asciz	"$Header: start.s 2.6 90/11/08 $"
.text

/*
 * System startup.
 */

/* $Log:	start.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

	.globl	_u
	.set	_u,VA_UAREA		# U-area virtual address
	.globl	_l
	.set	_l,VA_PLOCAL		# processor local data virtual address


	.globl  fpae_extra_status
	.set    fpae_extra_status,VA_UAREA+FPAE_EXTRA_STATUS

/*
 * Startup code.
 *
 * Variable "upyet" is zero when first booted, set non-zero once the
 * system is initialized.
 *
 * The first processor to come up does a bunch of system-wide initializations,
 * including creating the first KL1PT and allocating system data-structures.
 * This processor runs on proc[0] stack during initialization; other processors
 * run on their processor-local U-area.
 *
 * As all processors start, they have to init their page-tables and local
 * environment.
 *
 * Note that one processor ever does the sysinit's.  Each processor might
 * do the self-init case numerous times, as the processors are dynamically
 * brought on-line and off-line.
 *
 * If (upyet == 0) {
 *	#
 *	# 1st processor to breathe.
 *	#
 *	Get a U-area to use for stack -- this will be proc[0] U-area;
 *	Set up stack in here;
 *	sysinit(free, puser);		# does the self-init, too
 *	SP = &_u+UPAGES*NBPG;		# put stack in correct place
 *	++upyet;			# declare system "up"
 *	main(U-area base);		# no return for caller. proc[1] returns
 *	Set up and "return" to init process.
 * } else {
 *	#
 *	# Just another processor coming on-line.
 *	#
 *	engine = &my processor engine structure;
 *	Setup stack in processor-local U-area;
 *	selfinit(r7, phys U-area base);
 *	SP = &_u+UPAGES*NBPG;		# put stack in correct place
 *	swtch(NULL);			# start running something!
 * }
 *
 * At entry, stand-alone startoff has arranged we're running as a 32-bit
 * machine with 4Gig segments.
 */

	.data
	.globl	_upyet
	.align	2
_upyet:	.long	0			# indication of system initialized

	.text
	.globl	start
	.align	2
start:
	cli				# int's OFF, just to be safe.
	cmpl	$0, _upyet		# Are we first?
	jne	sysup			# no -- system already up.
/*
 * We're the first.  Allocate a U-area above top of kernel on page
 * boundary for UPAGES pages, and get to it!
 */
	leal	_end+CLBYTES-1, %edi	# round up...
	andl	$NOT_CLOFSET, %edi	#	to kernel page boundary.
	leal	UBYTES(%edi), %esi	# Set up stack...
	movl	%esi, %esp		#	to top of U-area.
	movl	%esi, %ebp		# just for completeness.
/*
 * Clear bss.
 * Then do basic system inits.
 */
	leal	_edata, %edx
	leal	_end, %eax
	subl	%edx, %eax
	pushl	%eax			# length of BSS
	pushl	%edx			# base of BSS
	CALL	_bzero			# clear it !!
	#addl	$8, %esp		# clear stack.

	pushl	%edi			# phys addr of U-area base.
	pushl	%esi			# 1st free memory address.
	CALL	_sysinit		# sysinit(1st free memory,puarea)
	#addl	$8, %esp		# clear stack
/*
 * Set up SP to "correct" value.
 */
	leal	_u+UBYTES, %eax		# Real top of U-area.
	movl	%eax, %esp		# Set up stack.
	movl	%eax, %ebp		# for completeness.
/*
 * The system is now "up".  Before calling main, must bump upyet since
 * main/init can bring up other processors.  Main() must be careful to
 * *not* do this until the system is really ready for other processors
 * to start (ie, all initializations complete).
 */
	movb	$1, _upyet		# Yup -- we're up!
/*
 * Now call main(), passing proc[0]'s U-area base, so main() can set
 * up proc[0] properly.  Note that the code that goes into main will never
 * return; it becomes proc[0] -- the swapper.  Proc[1] does return, as
 * the init process.
 */
	pushl	%edi			# proc[0] U-area phys==virt base.
	CALL	_main			# finish initialization.
	addl	$4, %esp		# clear stack.
/*
 * Now, start the init process, which is busily living in user-mode.
 * Fake an interrupt-return stack to return to user @ LOWPAGES*NBPG.
 */
	pushl	$USER_DS		# user SS, padded.
	pushl	$USRSTACK		# user SP.
	pushl	$[KERNEL_IOPL|FLAGS_IF]	# flags = kernel IOPL, int's ON.
	pushl	$USER_CS		# user CS, padded.
	pushl	$[LOWPAGES*NBPG]	# user EIP.
	movl	_boothowto, %edi	# pass boot flags to init process
	movl	_sec0eaddr, %esi	# pass SEC0 ether address to init
	iret				# start user!

/*
 * We aren't the first.  Thus the system is already initialized.  All
 * we need do is get a real stack, then init-self, and swtch()!
 *
 * Stack comes from processor private U-area, located by the engine structure.
 * Locate engine structure based on SLIC address of processor.
 */

sysup:
	movzbw	VA_SLIC+SL_PROCID, %bx	# Processor ID.
	movl	_engine, %edi		# &engine[0].
	subl	%edx, %edx		# counter for logical processor #.
lookagn:
	cmpw	%bx, E_SLICADDR(%edi)	# found me yet?
	je	foundme			# yup.
	addl	$ENGSIZE, %edi		# edi -> next engine structure.
	incl	%edx			# engine index += 1.
	cmpl	_Nengine, %edx		# went past it?
	jl	lookagn			# no -- look again.
/*
 * Didn't find self in engine structures.
 * Really need to panic here, but can't since we can't talk to anything,
 * and we have no stack.  Thus, stop and hang forever.
 */
dead:	cli				# OFF interrupts.
	jmp	dead			# just in case...
/*
 * Found "me".  Set up stack and call selfinit() to complete initialization.
 */
foundme:
	movl	E_LOCAL(%edi), %esi	# 1st of processor local is U-area.
	leal	UBYTES(%esi), %eax	# Set up stack...
	movl	%eax, %esp		#	to top of U-area.
	movl	%eax, %ebp		# just for completeness.
/*
 * Init self.  Sets up page-tables and turns on mapping.  Also, local
 * resources are initialized.
 */
	pushl	%edx			# logical "me".
	CALL	_selfinit		# selfinit(proc#)
	#addl	$8, %esp		# clear stack
/*
 * Set up SP to "correct" value.
 * Then, swtch(NULL) -- ie, enter the switcher and wait for something to do.
 */
	leal	_u+UBYTES, %eax		# Real top of U-area.
	movl	%eax, %esp		# Set up stack.
	movl	%eax, %ebp		# just for completeness.
	pushl	$0			# NULL
	CALL	_swtch			# swtch(NULL)...  Become productive!
	# No Return, Ever!

/*
 * setup_seg_regs()
 *	Setup segment registers, task-state and LDT registers.
 *
 * Called when processor had initialized GDT and IDT, but before
 * paging is enabled.
 *
 * Need to re-load ALL segment registers.
 */

ENTRY(setup_seg_regs)
	movl	$KERNEL_TSS, %eax	# task-state-segment selector.
	ltr	%ax			# set up processor-local TSS.
	subl	%eax, %eax		# a zero for...
	lldt	%ax			#	...non-existant LDT.
	movl	$KERNEL_DS, %eax	# official kernel DS selector.
	movw	%ax, %ds		# all...
	movw	%ax, %ss		#	...these
	movw	%ax, %es		#		...use
	movw	%ax, %fs		#			...same
	movw	%ax, %gs		#				...value
	/*
	 * Arrange long-return, which re-loads code-segment register.
	 */
	popl	%eax			# return EIP.
	pushl	$KERNEL_CS		# set up...
	pushl	%eax			#	...long return.
	lret				# and return, re-loading CS.
