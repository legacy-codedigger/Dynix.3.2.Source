/* $Copyright:	$
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
.asciz	"$Header: locore.s 2.31 91/02/26 $"
.text

/*
 * locore.s
 *	Machine dependent low-level kernel stuff.
 *
 * Mostly interrupt and trap handling.
 *
 * Very machine dependent.  Intel 80386 version.
 */

/* $Log:	locore.s,v $
 */

#include "assym.h"
#include "../machine/asm.h"

/*
 * Hardware interrupt handlers.
 *
 * These are pointed at thru a combination of the per-processor
 * Interrupt Descriptor Table (IDT) and Global Descriptor Table (GDT).
 *
 * There is one handler per SLIC bin.  Bins 1-7 are handled in a common
 * manner (the HW interrupts).  Bin 0 is special cased for SW interrupts.
 *
 * Bins 1-7 handle as follows:
 *	Save scratch registers.
 *	eax = bin#.
 *	Goto "dev_common".
 *
 * Dev_common:
 *	count device interrupt (except bin 7, used for clocks)
 *	Save entry IPL.
 *	Set up new IPL.
 *	Read vector from SLIC.
 *	Tell SLIC ok to accept another interrupt.
 *	Verify vector as valid.
 *	Call interrupt handler thru int_bin_table[].
 *	If returning to user mode, check for and handle redispatches
 *		(via falling into trap handler (T_SWTCH)).
 *	Else, disable interrupts, restore previous IPL, and return.
 *
 * Handlers are called with vector number as argument.  The bin #
 * information is *NOT* passed to the handler.
 *
 * All interrupts enter via interrupt-gates, thus SW must re-enable
 * interrupts at processor.  The main reason for interrupt-gates instead of
 * trap-gates is that the SLIC still yanks on the interrupt line until SW
 * tells SLIC it has the interrupt; thus if enter with trap-gate, it will
 * re-enter constantly and overflow the stack.  Also, other interrupts can
 * occur (eg, FPA) and can mess up %es,%ds assignments (since kernel
 * doesn't save/restore these explicitly, rather by "knowing" context).
 *
 * Interrupts save scratch registers in a "push-all" consistent order;
 * if need to handle re-dispatch, can push remaining registers and
 * behave like a trap.
 *
 * Thus after dev_common saves entry SPL, the stack looks like:
 *
 *	old SS, padded		only if inter-segment (user->kernel)
 *	old SP			only if inter-segment (user->kernel)
 *	flags
 *	CS, padded		sense user-mode entry from RPL field
 *	IP			return context
 *	EAX			scratch registers
 *	ECX			ditto
 *	EDX			more such
 *	old SPL			entry SLIC local mask
 *
 * Bin 0 (SW) interrupts are handled via reading the bin0 message register
 * and looping until we clear it out, calling an appropriate SW interrupt
 * handler for each bit:
 *
 *	Save scratch registers.
 *	spl1().
 *	ldata = SLIC Bin0 message data.
 *	ON processor ints.
 *	loop {
 *		BIT = FFS(ldata).
 *		If no bits set {
 *			OFF processor ints.
 *			spl0().
 *			restore registers.
 *			rett.
 *		}
 *		clear BIT in ldata.
 *		call SW handler(BIT).
 *		ldata |= SLIC Bin0 message data.
 *	}
 *
 * All entries clear "direction" flag, since C environment assumes this.
 *
 * Bin 0 interrupt handler uses an interrupt gate, to turn OFF processor
 * interrupts until ready to accept another bin0 (or higher) interrupt.
 */

#define	INTR_ENTER(bin) \
	pushl	%eax; \
	pushl	%ecx; \
	pushl	%edx; \
	movw	$KERNEL_DS, %ax; \
	movw	%ax, %ds; \
	movw	%ax, %es; \
	movl	$bin, %eax

	.text

ENTRY(bin1int)
	INTR_ENTER(1)			# bin 1 interrupt.
	incl	VA_PLOCAL+L_CNT+V_INTR	# count the device interrupt.
	jmp	dev_common		# common handling.

ENTRY(bin2int)
	INTR_ENTER(2)			# bin 2 interrupt.
	incl	VA_PLOCAL+L_CNT+V_INTR	# count the device interrupt.
	jmp	dev_common		# common handling.

ENTRY(bin3int)
	INTR_ENTER(3)			# bin 3 interrupt.
	incl	VA_PLOCAL+L_CNT+V_INTR	# count the device interrupt.
	jmp	dev_common		# common handling.

ENTRY(bin4int)
	INTR_ENTER(4)			# bin 4 interrupt.
	incl	VA_PLOCAL+L_CNT+V_INTR	# count the device interrupt.
	jmp	dev_common		# common handling.

ENTRY(bin5int)
	INTR_ENTER(5)			# bin 5 interrupt.
	incl	VA_PLOCAL+L_CNT+V_INTR	# count the device interrupt.
	jmp	dev_common		# common handling.

ENTRY(bin6int)
	INTR_ENTER(6)			# bin 6 interrupt.
	incl	VA_PLOCAL+L_CNT+V_INTR	# count the device interrupt.
	jmp	dev_common		# common handling.

ENTRY(bin7int)
	INTR_ENTER(7)			# bin 7 interrupt.
	# Don't count clock ticks.
	# Optimize: clocks fall thru.

/*
 * Common handling for bin 1-7 interrupts.  EAX == bin # on entry.
 * Indexing on int_bin_table assumes the bin_header structure is
 * 8-bytes (quad-word).
 */
dev_common:
	movl	_va_slic_lmask, %edx	# EDX = address of slic mask register
	movb	(%edx), %cl		# ECX = old SPL.  Save...
	movb	spltab(%eax), %ch	# new SPL from table...
	movb	%ch, (%edx)		#	...masks bin and lower.
	pushl	%ecx			#	...below scratch regs on stack.
	# the low byte is the spl the high is our new spl
	movl	_va_slic, %edx		# EDX = address of slic base
	movzbl	SL_BININT(%edx), %ecx	# ECX = vector # from message data.
	/*
	 * The cpu will block until the following write completes.
	 * This will insure that the spl mask has been set.
	 */
	movb	$0, SL_BININT(%edx)	# tell SLIC ok for more interrupts.
	sti				# ok for more interrupts now.
	cld				# in case intr'd code had it set.
	leal	_int_bin_table(,%eax,8), %edx # EDX -> intr table for this bin.
	pushl	%ecx			# argument = vector #.
	cmpl	%ecx, BH_SIZE(%edx)	# valid vector?
	jle	bogusint		# nope.
	movl	BH_HDLRTAB(%edx), %eax	# EAX == base of vectors for this bin.
	CALL	*(%eax,%ecx,4)		# call handler.
	addl	$4, %esp		# clear stack.
intdone:
	testb	$RPL_MASK, INTR_SP_CS(%esp)	# returning to user mode?
	je	intret			# no -- don't worry about reschedule.
	cmpl	$0, VA_PLOCAL+L_RUNRUN	# yup -- "my" runrun flag set?
	jne	doresched		# yup -- involuntary switch.
	/*
	 * Really returning to user.  Must be careful to restore DS,ES
	 * after update SLIC (else can't address it!).
	 */
	cli				# OFF processor interrupts.
	popl	%eax			# entry SLIC local mask (SPL).
	movl	_va_slic_lmask, %edx	# EDX = address of slic mask register
	movb	%al, (%edx)		# restore entry SPL.
	movb	(%edx), %al		# sync+2
	movw	$USER_DS, %ax		# restore...
	movw	%ax, %ds		#	...user DS
	movw	%ax, %es		#		...user ES.
	popl	%edx			# restore...
	popl	%ecx			#	...scratch
	popl	%eax			#		...registers.
	iret				# return to user from interrupt.
/*
 * Interrupt return to kernel.  Don't fuss with DS, ES.
 * since the SLIC mask is now gaurenteed to be stronger than the saved
 * spl we no longer need to worry when it takes affect with respect to the
 * the iret.
 */
intret:
	cli				# OFF processor interrupts.
	popl	%eax			# entry SLIC local mask (SPL).
	/*
	 * %al is the old mask, %ah is the current mask.
	 * It is possible that the saved mask was set to a higher value than
	 * we are currently running at.  This would happen for example if
	 * we had just written to the slic mask and within 500 ns the interrupt
	 * that we are returning from occured.
	 * In this case we must be careful that we don't hit the iret
	 * without insuring that the mask has been set.
	 */
	cmpb	%al, %ah		# going back to higer SPL?
	movl	_va_slic_lmask, %edx	# EDX = address of slic mask register
	jb	1f			# going to higher spl
	movb	%al, (%edx)		# restore entry SPL.
	popl	%edx			# restore...
	popl	%ecx			#	...scratch
	popl	%eax			#		...registers.
	iret				# return from interrupt.
1:
	movb	%al, (%edx)		# restore entry SPL.
	movb	(%edx), %ah		# synch the write (enuf pad follows).
					# (ok to 45Mhz on a i486)
	popl	%edx			# restore...
	popl	%ecx			#	...scratch
	popl	%eax			#		...registers.
	iret				# return from interrupt.

/*
 * spltab[]
 *	Maps bin # to IPL value to put in SLIC local-mask register.
 *
 * spltab[i] masks interrupts `i' and lower priority.
 */
	.align	2
spltab:
	.byte	SPL1			# [0]
	.byte	SPL2			# [1]
	.byte	SPL3			# [2]
	.byte	SPL_HOLE		# [3]
	.byte	SPL4			# [4]
	.byte	SPL5			# [5]
	.byte	SPL6			# [6]
	.byte	SPL7			# [7]

/*
 * Got bogus interrupt...  Vector # larger than allocated handler table
 * for the bin.  Dev_common already pushed vector #.
 */

	.text
bogusint:
	pushl	%eax			# bin #
	CALL	_bogusint		# complain about this!
	addl	$8, %esp		# clear junk off stack.
	jmp	intdone			# return from interrupt.

/*
 * Undefined SW trap handler.
 */
ENTRY(swt_undef)
	pushl	$swtundef		# panic message
	CALL	_panic			# no deposit, no return
	#addl	$4, %esp		# not really
	#RETURN				# not really

	.data
swtundef:
	.asciz	"Undefined software trap"
	.text

/*
 * Arrange to redispatch (call trap() with simulated T_SWTCH type code).
 * At entry here, stack is "standard" interrupt stack, including saved SPL.
 * Must complete "push-all" and otherwise behave like a trap.
 *
 * Already executing using KERNEL_DS.
 */
	.align	2
doresched:
	addl	$4, %esp		# pop interrupt SPL (should be SPL0).
	pushl	%ebx			# save...
	subl	$4, %esp		#		(dummy "SP")
	pushl	%ebp			#	...other
	pushl	%esi			#		...registers
	pushl	%edi			#			...ala "pushall"
	pushl	$T_SWTCH		# "switch" trap-type.
	movl	_va_slic_lmask, %edx
	movb	$SPL0, (%edx)		# new value == SPL0.
	jmp	trap_common

/*
 * Bin0 (SW) interrupt handler.  Entered thru interrupt gate, thus
 * interrupts masked at processor.
 *
 * Called routines must *not* redispatch; they must behave as interrupts.
 */
ENTRY(bin0int)
	pushl	%eax			# save...
	pushl	%ecx			#	...scratch
	pushl	%edx			#		...registers.
	movw	$KERNEL_DS, %ax		# establish...
	movw	%ax, %ds		#	...kernel DS
	movw	%ax, %es		#		... kernel ES.
	# was SPL_ASM($SPL1,%al) but to set SPL to mask bin 0.
	# But slic mask may now be greater than spl0 due to sychronisation
	# slippage, so add spl1 to what it currently is.
	movl	_va_slic_lmask, %edx		# get slic mask address
	movb	(%edx), %al
	movb	$SPL1, %ah		# store the new mask for int_ret
	movb	%ah, (%edx) 
        movb	(%edx), %ah		# Dummy read-sync write
	movl	_plocal_slic_delay, %edx	# Wait for slic_lmask to be written
	movl	(%edx), %edx
0:	subl	$1, %edx
	jg	0b
	pushl	%eax			# save entry SPL (should be SPL0).
	movl	_va_slic, %edx           # get slic address
	movzbl	SL_B0INT(%edx), %ecx    # ECX = Bin0 message data (mask).
	sti				# ON processor interrupts.
	cld				# in case intr'd code had it set.
0:	bsfl	%eax, %ecx		# Find software trap bit
	je	intdone			# no bit ==> done.
	btrl	%eax, %ecx		# clear soft interrupt bit.
	pushl	%ecx			# save remaining interrupt data.
	CALL	*_softvec(,%eax,4)	# call soft interrupt routine.
	popl	%ecx			# restore interrupt data
        movl	_va_slic, %edx		# get slic address
	orb	SL_B0INT(%edx), %cl	# CL |= Bin0 message data (mask)
	jmp	0b			# repeat until no bits set.

/*
 * Unconditionally configured SW interrupt handlers.
 */

/*
 * reched()
 *	Set self `runrun' to cause re-dispatch.
 */
ENTRY(resched)
	movl	$1, VA_PLOCAL+L_RUNRUN	# Set runrun flag
	RETURN

/*
 * undef()
 *	No such.  Somebody goofed.
 */
ENTRY(undef)
	pushl	$undefmsg		# panic message
	CALL	_panic			# no deposit, no return
	#addl	$4, %esp		# not really
	#RETURN				# not really

	.data
undefmsg:
	.asciz	"Undefined software interrupt"
	.text

/*
 * splx(newmask)
 *	Lower interrupt priority mask back to previous value.
 *
 * DEBUG version checks that mask is actually "lowering" -- ie, staying the
 * same or enabling more interrupts.  Non-debug version is in-line expanded
 * (see machine/intctl.h).
 *
 * Since 1's in the mask enable interrupts, (oldmask & newmask) == oldmask is ok.
 */

#ifdef	DEBUG
ENTRY(splx)
	movl	_va_slic_lmask, %ecx		# get slic mask address
	movb	(%ecx), %ah			# old mask value from SLIC.
	movb	SPARG0, %al			# new mask.
	movb	%al, (%ecx)		 	# install new mask.
	andb	%ah, %al			# %al = old & new.
	cmpb	%ah, %al			# ok if (old & new) == old.
	jne	9f				# not ok -- down in flames.
	RETURN					# ok -- done.
9:	pushl	$8f				# bad news.
	CALL	_panic				# really bad news.
8:	.asciz	"splx: bad nesting"
#endif	DEBUG

/*
 * Processor Trap Handlers.
 *
 * There are two forms of trap entry -- those that push an error code
 * and those that don't.
 *
 * TRAP_ENTER_NOERR assumes no trap error code, no need to pop it.
 * TRAP_ENTER_ERR assumes HW pushed the error code, thus pops it into
 *	l.trap_err_code.
 *
 * All entries are thru trap gates, thus interrupts are on at entry.
 *
 * No save/restore of SPL -- code is supposed to nest this properly.
 */

#define	TRAP_ENTER_NOERR(type) \
	pushal; \
	movw	$KERNEL_DS, %ax; \
	movw	%ax, %ds; \
	movw	%ax, %es; \
	pushl	$type

#define	TRAP_ENTER_ERR(type) \
	cs; movw	kernel_ds, %ds; \
	cs; movw	kernel_ds, %es; \
	popl	VA_PLOCAL+L_TRAP_ERR_CODE; \
	pushal; \
	pushl	$type

/*
 * TRAP_ENTER_ERR must load a value for kernel DS, but need to use
 * it (to pop error code) before can load thru a register.
 */

kernel_ds:.word KERNEL_DS
user_ds:.word USER_DS

/*
 * T_DIVERR -- integer divide error.  No error code.
 */
ENTRY(t_diverr)
	TRAP_ENTER_NOERR(T_DIVERR)
	jmp	trap_common

/*
 * T_DBG -- debug trap/fault, single step, etc.  No error code.
 */
ENTRY(t_dbg)
#ifdef COBUG
/*
 * If working around the C0 bug for this engine, debug traps can mean
 * something different: that our FPU instruction has completed.  If this
 * is the case, we would like to just turn off the i387, and let the
 * user continue.
 */
	/* Point to kernel data space */
	cs; movw        kernel_ds, %ds;
	/*
	 * If we're not involved, let upper levels handle it
	 */
	testw	$UF_FPSTEP, VA_UAREA+U_FLAGS
	jz	1f

	pushl	%eax
	/*
	 * Disable further access until we handle it specially
	 */
	fnstsw  %eax
	movl	VA_PLOCAL+L_FPUOFF, %eax
	movl	%eax, %cr0
	popl	%eax

	/*
	 * We behave differently depending on previous state of T bit
	 */
	testw	$UF_OTBIT, VA_UAREA+U_FLAGS
	jnz	2f

	/*
	 * Trace bit wasn't on; clear T bit, restart instruction
	 */
	andw	$-1^UF_FPSTEP, VA_UAREA+U_FLAGS
	andl	$-1^FLAGS_TF, 8(%esp)

	/* Restore data, extra segment */
	cli
	cs; movw        user_ds, %ds;
	cs; movw        user_ds, %es;
	iret

2:
	/*
	 * It was on before; clean ourselves up, then let upper levels
	 * piggy-back on this trap
	 */
	andw	$-1^[UF_FPSTEP+UF_OTBIT], VA_UAREA+U_FLAGS
	/* VVV fall into regular case */

1:
#endif /* COBUG */
	TRAP_ENTER_NOERR(T_DBG)
	jmp	trap_common

/*
 * T_NMI -- Non-Maskable Interrupt.  No error code.
 * Entered thru interrupt gate (interrupts disabled).
 *
 * If probe_nmi == NULL, handle as trap (which will panic the system).
 * Else, jump to probe_nmi.  The "jump" is via an iret, to allow NMI's
 * again in the processor (80386 disables NMI's until an iret is executed).
 * The "iret" also removes the NMI stack frame.
 */
#ifdef	KERNEL_PROFILING
	.globl	_kp_nmi
#endif	KERNEL_PROFILING

ENTRY(t_nmi)
	TRAP_ENTER_NOERR(T_NMI)
	movl	_probe_nmi, %eax	# probe_nmi procedure, or NULL.
	cmpl	$0, %eax		# probing?
#ifndef	KERNEL_PROFILING
	je	trap_common		# no -- a real NMI.
#else	KERNEL_PROFILING
	jne	1f			# yes
	cld				# in case trap'd code had it set.
	CALL	_kp_nmi			# assume profiler NMI
	testl	%eax,%eax		# was it really?
	je	kp_trapret		# yes -- return from trap
	jmp	trap_common		# no -- a real NMI.
1:
#endif	KERNEL_PROFILING
	addl	$4, %esp		# "pop" trap type.
	movl	%eax, SP_EIP(%esp)	# alter return IP to probe_nmi function.
	popal				# restore all regs.
	iret				# "jump" to probe-NMI handler.

	.data
	.globl	_probe_nmi
_probe_nmi:
	.long	0
	.text

/*
 * T_INT3 -- single-byte interrupt (breakpoint).  No error code.
 */
ENTRY(t_int3)
	TRAP_ENTER_NOERR(T_INT3)
	jmp	trap_common

/*
 * T_INTO -- interrupt on overflow.  No error code.
 */
ENTRY(t_into)
	TRAP_ENTER_NOERR(T_INTO)
	jmp	trap_common

/*
 * T_CHECK -- array bounds check.  No error code.
 */
ENTRY(t_check)
	TRAP_ENTER_NOERR(T_CHECK)
	jmp	trap_common

/*
 * T_UND -- undefined/illegal op-code.  No error code.
 */
ENTRY(t_und)
	TRAP_ENTER_NOERR(T_UND)
	jmp	trap_common

/*
 * T_DNA -- device not available (FPU).  No error code.
 */
ENTRY(t_dna)

#ifdef COBUG
/*
 * If working around the C0 bug for this engine, give non-presence special
 * treatment.  Note that we go to special trouble to avoid using registers;
 * we are doing all of this before the "pusha" in the name of efficiency.
 */
	/* Point to kernel data space */
	cs; movw        kernel_ds, %ds;
	/*
	 * Test engine flag, skip all the fuss if there's no problem
	 */
	testl	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f

	/*
	 * Need to worry.  If this is our first access (i.e., FPU has no
	 * context yet), let the upper level handle it
	 */
	cmpb	$0, VA_PLOCAL+L_USINGFPU
	jz	1f

	/*
	 * Record old state of trace-bit, so we can restore it
	 * correctly when we're done.
	 */
	testl	$FLAGS_TF, 8(%esp)
	jz	2f
	orw	$UF_OTBIT, VA_UAREA+U_FLAGS
2:
	/*
	 * This is a successive access.  This means that although the i387
	 * is not accessible, it has been set up and is ready for use.  We
	 * enable access, but simultaneously set single-step mode.  See the
	 * t_dbg entry for how we handle this.
	 */
	pushl	%eax
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
	popl	%eax
	orl	$FLAGS_TF, 8(%esp)
	orw	$UF_FPSTEP, VA_UAREA+U_FLAGS

	/* Restore data, extra segment */
	cli
	cs; movw        user_ds, %ds;
	cs; movw        user_ds, %es;
	/*
	 * Restart the instruction
	 */
	iret
1:
#endif /* COBUG */
	TRAP_ENTER_NOERR(T_DNA)
	jmp	trap_common

/*
 * T_SYSERR -- System error (double fault).  Zero value error code.
 */
ENTRY(t_syserr)
	TRAP_ENTER_ERR(T_SYSERR)
	jmp	trap_common

/*
 * T_RES -- Reserved trap entry.  Serious Problem.
 *
 * This is entered via interrupt-gate (interrupts masked at processor).
 * Use "splhi()" to insure interrupts can't be turned on; panic printf's
 * will re-enable processor interrupts due to "v_gate()".
 *
 * This is used in all otherwise unused slots in the IDT.  Thus it catches
 * bogus interrupt vectors from the hardware.
 */
ENTRY(t_res)
	TRAP_ENTER_NOERR(T_RES)
	SPL_ASM($SPLHI,%bl)			# %bl = splhi()
	sti					# processor now allows int's.
	jmp	trap_common

/*
 * T_BADTSS -- Invalid Task-State Segment.  Selector error code.
 */
ENTRY(t_badtss)
	TRAP_ENTER_ERR(T_BADTSS)
	jmp	trap_common

/*
 * T_NOTPRES -- Segment/gate not present.  Selector error code.
 */
ENTRY(t_notpres)
	TRAP_ENTER_ERR(T_NOTPRES)
	jmp	trap_common

/*
 * T_STKFLT -- Stack fault.  Selector or zero error code.
 */
ENTRY(t_stkflt)
	TRAP_ENTER_ERR(T_STKFLT)
	jmp	trap_common

/*
 * T_GPFLT -- General protection fault.  Selector or zero error code.
 */
ENTRY(t_gpflt)
	TRAP_ENTER_ERR(T_GPFLT)
	jmp	trap_common

/*
 * T_COPERR -- Math co-processor error (FPU).  No error code.
 */
ENTRY(t_coperr)
	TRAP_ENTER_NOERR(T_COPERR)
	jmp	trap_common

/*
 * t_fpa -- FPA exception.  This is actually an interrupt.
 *
 * Enter via interrupt gate so interrupts are disabled.
 * Must read FPA process context register (PCR), then mask all exceptions
 * and insure this is sync'd, then finally enable interrupts and call
 * fpa_trap() to do the dirty work.
 */
ENTRY(t_fpa)
	/*
	 * Enter much like bin0int.
	 */
	pushl	%eax			# save...
	pushl	%ecx			#	...scratch
	pushl	%edx			#		...registers.
	movw	$KERNEL_DS, %ax		# establish...
	movw	%ax, %ds		#	...kernel DS
	movw	%ax, %es		#		... kernel ES.
	movl    _va_slic_lmask, %ecx     # get slic mask address
	movb    (%ecx), %al             # %al = entry SPL.
	movb	$SPL0, %ah		# for intdone
	pushl	%eax			# save entry SPL.
	/*
	 * Read FPA PCR, then mask all exceptions.
	 */
	movl	VA_FPA+FPA_STCTX, %ecx			# %ecx = FPA PCR
	movl	$FPA_PCR_EM_ALL, VA_FPA+FPA_LDCTX	# mask all exceptions.
	movl	VA_FPA+FPA_STCTX, %edx			# synch the above write.
	/*
	 * Now can re-enable interrupts and call real FPA trap handler.
	 * Once re-enable processor interrupts, can take SLIC interrupt.
	 * Note that SLIC interrupt goes first if FPA and SLIC arrive
	 * at processor simultaneously.
	 */
	sti				# interrupts ON again.
	pushl	%ecx			# call it with nasty PCR.
	CALL	_fpa_trap		# poke at process.
	popl	%ecx			# clear stack.
	jmp	intdone			# all done.

/*
 * T_PGFLT -- Page fault.  Page-fault error code.
 *
 * Chip bug (thru at least B1's) makes page-fault error code unreliable.
 * Thus, don't use and optimize by clearing stack and use faster trap entry.
 *
 * If MFG or DEBUG kernel, always use the C trap() code.
 * Otherwise, do it faster if possible.
 */
ENTRY(t_pgflt)
	addl	$4, %esp			# clear bogus PFEC off stack.
#if	defined(MFG) || defined(DEBUG)
	TRAP_ENTER_NOERR(T_PGFLT)
	movl	%cr2, %eax			# faulted address.
	movl	%eax, VA_PLOCAL+L_TRAP_ERR_CODE	# save for trap().
	#jmp	trap_common			# optimize: fall thru.
#else
	/*
	 * Try to "fast-path".  Save only scratch registers.
	 * If process is being profiled or should swap itself, or the
	 * faulted address isn't in the user-address-space, use trap().
	 * Else call pagein() directly.
	 */
	pushl	%eax			# save...
	pushl	%ecx			#	...scratch
	pushl	%edx			#		...registers.
	movw	$KERNEL_DS, %ax		# establish...
	movw	%ax, %ds		#	...kernel DS
	movw	%ax, %es		#		... kernel ES.
	movl	%cr2, %eax		# faulted address.
	cld				# in case trap'd code had it set.
	cmpl	$0, VA_UAREA+U_PROF+PR_SCALE	# process being profiled?
	jne	pgflt_use_trap		# yup -- use trap().
	movl	VA_UAREA+U_PROCP, %edx	# is process...
	testl	$SFSWAP, P_FLAG(%edx)	#	...supposed to swap?
	jnz	pgflt_use_trap		# yup - use trap().
	subl	$VA_USER, %eax		# adjust address to user relative.
	cmpl	$[USER_SPACE-UBYTES], %eax	# <= top of user space?
	jnb	pgflt_use_trap2		# nope -- use trap() (note: unsigned).
	pushl	%eax			# and pass to pagein().
	CALL	_pagein			# returns true if succeed, else false.
	testl	%eax, %eax		# pagein() was ok?
	jz	pgflt_use_trap1		# nope -- use trap().
	addl	$4, %esp		# pop fault address.
	/*
	 * pagein() was successful.  Restore process priority in case
	 * process blocked inside pagein().  This does:
	 *	p->p_pri = p->p_usrpri;
	 *	l.eng->e_pri = p->p_pri;
	 *	SLICPRI(l.eng->e_pri>>2);
	 * from trap().
	 */
	movl	VA_UAREA+U_PROCP, %edx	# u.u_procp
	movb	P_USRPRI(%edx), %al	# u.u_procp->p_usrpri
	movb	%al, P_PRI(%edx)	# u.u_procp->p_pri = u.u_procp->p_usrpri
	movl	VA_PLOCAL+L_ENG, %edx	# &engine[me]->e_pri = ...
	movb	%al, E_PRI(%edx)	#		new process' prio.
	shlb	$1, %al			# (pri / 4) << 3, for SLIC sl_ipl.
	movl    _va_slic, %edx           # get slic base address
	movb    %al, SL_IPL(%edx)       # new interrupt arbitration prio.
	/*
	 * Bump stats and exit via intdone by pushing "false" SPL0 on stack.
	 * Could optimize a bit more and expand "intdone" here (without the
	 * SPL0 fuss), but prefer to keep the complicated code in one place.
	 */
	incl	VA_PLOCAL+L_CNT+V_FAULTS # count the fault.
	incl	VA_PLOCAL+L_CNT+V_TRAP	# and the trap.
	movb	$SPL0, %al
	movb	%al, %ah
	pushl	%eax			# now frame looks like interrupt.
	jmp	intdone			# and return.
	/*
	 * For some reason, can't use fast path.  Finish a "trap" stack frame
	 * and fall into normal trap code.  Faulting address is in %eax.
	 * Land at pgflt_use_trap1 if pagein() failed.  Recovers user fault
	 * address from stack (pagein() modified to beginning of page).
	 */
pgflt_use_trap1:
	popl	%eax			# pagein() failed -- recover user addr.
pgflt_use_trap2:
	addl	$VA_USER, %eax		# un-user-relocate.
pgflt_use_trap:
	movl	%eax, VA_PLOCAL+L_TRAP_ERR_CODE	# save for trap().
	pushl	%ebx			# save...
	subl	$4, %esp		#		(dummy "SP")
	pushl	%ebp			#	...other
	pushl	%esi			#		...registers
	pushl	%edi			#			...ala "pushall"
	pushl	$T_PGFLT		# "page-fault" trap-type.
	#jmp	trap_common		# optimize: fall thru.
#endif	MFG|DEBUG

/*
 * Common code for processor traps.  Call trap(type).
 * Entry code already pushed trap type.
 */
trap_common:
	cld				# in case trap'd code had it set.
	CALL	_trap			# trap(type)
#ifdef	KERNEL_PROFILING
kp_trapret:
#endif	KERNEL_PROFILING
	addl	$4, %esp		# clear off traptype
	testb	$RPL_MASK, SP_CS(%esp)	# going back to user mode?
	je	9f			# Nope -- avoid seg-reg fuss.
	cli				# restoring ds, es can't reenter!
	movw	$USER_DS, %ax		# restore...
	movw	%ax, %ds		#	...user-mode DS
	movw	%ax, %es		#		...user-mode ES.
9:	popal				# restore interrupted registers.
	iret				# back from whence we came.

/*
 * System-call entry points.  Handled much like traps, except call
 * syscall() instead of trap() (to avoid indirection).  Also, various
 * optimizations on exit based on knowledge we 'must' have come from
 * user-mode.
 *
 * Different entry for each number of arguments.
 *
 * SPL is properly nested at syscall exit, thus can avoid re-program of
 * SL_LMASK (eg, "must" have entered at SPL0, and it "must" be that now
 * since about to return to user mode ==> no need to reprogram).
 *
 * Don't check "runrun" for redispatch here, since syscall() already
 * did that before returning.
 */

/*
 * Macros to handle syscall entry, exit.
 *
 * SYS_ENTER saves user general registers, copies arguments as appropriate,
 * invokes syscall handler.  Also clear "direction" flag since user code
 * have set it, and C-environment assumes it's clear.
 *
 * SYS_CALL checks validity of syscall handler encoded in syscall number,
 * and calls the syscall handler.  Assumes %eax is unchanged from entry value
 * (ie, SYS_ENTER and arg-copying doesn't modify it).
 *
 * All entries are thru trap gates, thus interrupts are on at entry.
 *
 * On K20, SYS_EXIT should check SLIC write-buffer to insure NMI taken
 * in user mode isn't really a system panic.  This is ignored for
 * K20 since it's not production.
 *
 * All registers are pushed before calling syscall/trap, thus syscall()
 * and trap() frames may differ.
 */

#define	SYS_ENTER(nargs) \
	pushal; \
	cld; \
	movw	$KERNEL_DS, %bx; \
	movw	%bx, %es; \
	movw	%bx, %ds; \
	SYS_COPY/**/nargs

#define	SYS_CALL \
	shrl	$16, %eax; \
	cmpl	_syscall_nhandler, %eax; \
	jae	9f; \
	CALL	*_syscall_handler(,%eax,4)

#define	SYS_EXIT \
	movw	$USER_DS, %ax; \
	cli; \
	movw	%ax, %ds; \
	movw	%ax, %es; \
	popal; \
	iret

#define	SYS_BAD \
9:	CALL	_nosyscall; \
	int	3

#define	SYSCALL(nargs)	SYS_ENTER(nargs); SYS_CALL; SYS_EXIT; SYS_BAD

/*
 * Macros to copy appropriate number of parameters.
 * Assume ES and DS set up for kernel data.
 *
 * Single-arg syscall passes value in ECX.  Two arg syscall passes in
 * ECX, EDX.  All others pass pointer to args in ECX (so libc procedure
 * doesn't need to save/restore ESI).
 *
 * SYS_COPY_ARGS() assumes USER_ADDR_MASK is 2**n-1, thus can "and".  If
 * this changes (see machine/vmparam.h, machine/genassym.c), must change
 * SYS_COPY_ARGS().
 */

#define	SYS_COPY_ARGS(n)	\
	movl	$VA_UAREA+U_ARG, %edi; \
	andl	$USER_ADDR_MASK, %ecx; \
	leal	VA_USER(%ecx), %esi; \
	movl	$n, %ecx; \
	rep;	smovl

#define	SYS_COPY0			/* NOP */

#define	SYS_COPY1 \
	movl %ecx, VA_UAREA+U_ARG

#define	SYS_COPY2 \
	movl %ecx, VA_UAREA+U_ARG; \
	movl %edx, VA_UAREA+U_ARG+4

#define	SYS_COPY3 \
	movl	$sys_copy_err, VA_UAREA+U_FLTADDR; \
	SYS_COPY_ARGS(3); \
	movl	$0, VA_UAREA+U_FLTADDR

#define	SYS_COPY4 \
	movl	$sys_copy_err, VA_UAREA+U_FLTADDR; \
	SYS_COPY_ARGS(4); \
	movl	$0, VA_UAREA+U_FLTADDR

#define	SYS_COPY5 \
	movl	$sys_copy_err, VA_UAREA+U_FLTADDR; \
	SYS_COPY_ARGS(5); \
	movl	$0, VA_UAREA+U_FLTADDR

#define	SYS_COPY6 \
	movl	$sys_copy_err, VA_UAREA+U_FLTADDR; \
	SYS_COPY_ARGS(6); \
	movl	$0, VA_UAREA+U_FLTADDR

/*
 * The syscall handlers.
 */

ENTRY(t_svc0)
	SYSCALL(0)			# 0-argument system call.

ENTRY(t_svc1)
	SYSCALL(1)			# 1-argument system call.

ENTRY(t_svc2)
	SYSCALL(2)			# 2-argument system call.

ENTRY(t_svc3)
	SYSCALL(3)			# 3-argument system call.

ENTRY(t_svc4)
	SYSCALL(4)			# 4-argument system call.

ENTRY(t_svc5)
	SYSCALL(5)			# 5-argument system call.

ENTRY(t_svc6)
	SYSCALL(6)			# 6-argument system call.

/*
 * On copy error (bad fault), come here.
 * Assumes stack is ready to "SYS_EXIT".
 *
 * Could post SIGSEGV to process instead of just fail syscall.
 */
sys_copy_err:
	movl	$EFAULT, SP_EAX(%esp)		# return EAX = EFAULT.
	orl	$FLAGS_CF, SP_FLAGS(%esp)	# set carry-flag (error).
	movl	$0, VA_UAREA+U_FLTADDR		# clear fault recovery address.
	SYS_EXIT				# return error to user.
