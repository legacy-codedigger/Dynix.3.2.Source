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
.asciz	"$Header: cswitch.s 2.22 1992/02/12 23:19:23 $"
.text

/*
 * cswitch.s
 *	Context Switching asm support routines.
 *
 * Save() and resume() deal with a very minimal stack context:
 *
 *	ret PC			from whence it came
 *	caller EBP		saved frame pointer
 *
 * A pointer to the saved EBP is stored in u.u_sp for each process.
 *
 * Conditionals:
 *	-DWATCHPT:	turn on watchpoint code.
 *
 * i386 version.
 */

/* $Log: cswitch.s,v $
 *
 *
 */

#include "assym.h"
#include "../machine/asm.h"

/*
 * save()
 *	Save process context for later resumption.
 *
 * Save process context.  No registers saved (caller insured this).
 * Stack pointer is saved in Uarea.  Floating point context is saved in
 * uarea if necessary.  Also, the FPU is disabled.
 *
 * Note that _resume has an in-line version of _save. If _save changes, this
 * in-line code may also.
 *
 * Return zero to denote save was done.
 */

ENTRY(save)
	pushl	%ebp				# save callers frame pointer.
	movl	%esp, VA_UAREA+U_SP		# save SP for later resume.
	/*
	 * Save FPA context if process was using FPA.
	 */
	cmpl	$0, VA_FPA_PTE			# is FPA mapped??
	jne	save_fpa			# yup.
	/*
	 * Save FPU context if process was using FPU.
	 */
	cmpb	$0, VA_PLOCAL+L_USINGFPU 	# process was using FPU?
	jne	save_fpu			# yup -- save FPU also.
	subl	%eax, %eax			# Return 0 - this is save
	jmp	*4(%esp)			# "return", keep stack context.
save_fpu:					# save float context.
#ifdef COBUG
	/*
	 * The FPU is probably not on when working around the i486
	 * chip bug.  Turn it on before trying to save it.
	 */
	testw	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
1:
#endif /* COBUG */
	fnsave	VA_UAREA+U_FPUSAVE		# save FPU state, no pre-"wait".
	wait					# wait for save to complete.
	movl	VA_PLOCAL+L_FPUOFF,%eax		# want to turn it off.
	movl	%eax, %cr0			# it's off now!
	movb	$0, VA_PLOCAL+L_USINGFPU	# Process no longer using FPU.
	subl	%eax, %eax			# Return 0 - this is save.
	jmp	*4(%esp)			# "return", keep stack context.
	/*
	 * Save FPA context.  This is an in-line expansion of save_fpa().
	 */
save_fpa:
	movl	VA_FPA+FPA_STCTX, %eax		# FPA process context register.
	movl	%eax, VA_UAREA+U_FPA_PCR	# save in u.u_fpasave.
	movl	%esi, %eax			# save register variable.
	movl	%edi, %edx			# save register variable.
	leal	VA_FPA+FPA_STOR_R1, %esi	# source == FPA R1.
	leal	VA_UAREA+U_FPA_REGS, %edi	# dest = u.u_fpasave.fpa_regs.
	movl	$FPA_NREGS, %ecx		# count
	rep;	smovl				# save the state.
	movl	$0, VA_FPA_PTE			# OFF mapping.
	movl	%eax, %esi			# restore register variable.
	movl	%edx, %edi			# restore register variable.
	cmpb	$0, VA_PLOCAL+L_USINGFPU	# process was using FPU?
	jne	save_fpu			# yup -- save FPU also.
	subl	%eax, %eax			# Return 0 - this is save.
	jmp	*4(%esp)			# "return", keep stack context.

/*
 * resume(curproc, newproc, locked)
 *	struct proc *curproc, *newproc, *locked;
 *
 * Resume does the actual context switch to the new process. The current process
 * context is saved, if necessary. The current process state lock is also
 * released, if necessary.
 *
 * Resume is called at splhi with the G_RUNQ locked. 
 *
 * Resume returns non-zero. Since the return is to the instruction following
 * a call to save(), the non-zero return distinguishes a resume() from a save().
 *
 * If watchpoints are enabled, they are turned off at entry.  They are turned
 * on in the new process context as appropriate.
 *
 * Returns at SPL0.
 *
 * ASSUMES:
 *	If locked != NULL, locked == curproc.
 *	If curproc == NULL, running on private stack.
 *	Caller holds runQ gate.
 *
 * Note the in-line version of save.  If save needs to be changed, this may too.
 */

ENTRY(resume)
#ifdef	WATCHPT
	cmpb	$0, VA_PLOCAL+L_WATCHPTON	# watchpoints on?
	jne	res_turn_wp_off			# yup -- turn them off.
res_wp_off:					# watchpoints now disabled.
#endif	WATCHPT
	movl	SPARG1, %ecx			# process to resume.
	cmpl	$0, SPARG0			# Saved running process yet?
	je	res_on_priv			# Yes -- already on private stk.
	/*
	 * Save context of current process (ala save()).
	 * No branch when system loaded (ie, other processes to run).
	 */
	pushl	%ebp				# save callers frame pointer.
	movl	%esp, VA_UAREA+U_SP		# save SP for later resume.
	/*
	 * Save FPA context if process was using FPA.
	 * NOTE: Must not alter %ecx.
	 */
	cmpl	$0, VA_FPA_PTE			# is FPA mapped??
	jne	res_fpa				# nope.
res_nofpa:
	/*
	 * Save FPU context if process was using it.
	 * NOTE: Must not alter %ecx.
	 */
	cmpb	$0, VA_PLOCAL+L_USINGFPU	# process was using FPU?
	jne	res_float
res_nofloat:
	/*
	 * Resume new context.  Assumes ECX -> process to resume.
	 * Must preserve processor-local data mapping.
	 *
	 * Must check if need to release process state lock
	 * of out-going process; must do after on new page-tables to avoid
	 * races in (eg) vfork() and better handle parallel-runQ.
	 *
	 * Careful of stack -- pushed EBP above.  If gets too wierd,
	 * could "movl %esp, %ebp" after save EBP above, and use BPARGx.
	 *
	 * Uses processor specific private stack value to avoid problems
	 * if (eg) NMI before load new SP after switching page-tables:
	 * Can't use outgoing %esp value on new process stack.
	 *
	 * Assumes SPARG2==SPARG0 if SPARG2 != NULL.
	 */
	movl	P_PTB1(%ecx), %eax		# phys addr of process PT.
	movl	VA_PLOCAL+L_PLOCAL_PTE, %edx	# L1 pte mapping plocal stuff.
	movl	%edx, PLOCAL_PTE_OFF(%eax)	# preserve plocal mapping.
	movl	4+SPARG2, %edx			# process to unlock (if !NULL).
	movl	VA_PLOCAL+L_PRIVSTK, %esp	# use private stack for a bit.
	movl	%eax, %cr3			# switch'em! (& flush TLB).
	orl	%edx, %edx			# unlock process context?
	jz	1f				# nope.
	V_LOCK_ASM(P_STATE(%edx))		# v_lock(&p->p_state, SPLHI).
1:	movl	VA_UAREA+U_SP, %esp		# start using his real stack.
res_on_new:
	movl	VA_PLOCAL+L_ENG, %eax		# &engine[me]
	movb	P_PRI(%ecx), %dl		# engine priority now ...
	movb	%dl, E_PRI(%eax)		#		new process'
	shlb	$1, %dl				# (pri/4) << 3, for SLIC sl_ipl
	movb	%dl, VA_SLIC+SL_IPL		# new interrupt arbitration prio
	movb	$PIDLE, E_NPRI(%eax)		# no nudge idle
	movw	VA_PLOCAL+L_ME, %ax		# process on...
	movw	%ax, P_ENGNO(%ecx)		#	...engine L_ME.
	movb	$SONPROC, P_STAT(%ecx)		# process now on processor
	movb	$0, VA_PLOCAL+L_NOPROC		# ditto
	/*
	 * Release runQ gate, re-enable interrupts and (almost) done!
	 */
	V_GATE_ASM(_g_runq)			# unlock runQ.
#ifdef	DEBUG
	movl	$0, VA_PLOCAL+L_HOLDGATE	# no longer holding gate.
#endif	DEBUG
	SPLX_ASM($SPL0)				# interrupts in to SLIC again.
	sti					# allow interrupts again.
#ifdef	WATCHPT
	/*
	 * If process has watchpoints, turn them on.
	 */
	cmpl	$DCR_OFF, VA_UAREA+U_WATCHPT+WP_CONTROL	# watchpoints?
	jne	res_watchpt			# yup -- restore them.
res_wp_ret:
#endif	WATCHPT
	/*
	 * When return, process is on its way again.
	 */
	popl	%ebp				# restore process's frame.
	movb	$1, %al				# return non-zero ==> resume.
	RETURN
/*
 * Process is already on private stack -- no need to check for releasing
 * process state lock.  This is a clone of resume-context code above for this
 * case, allows above ("hotter") code to not have extra if's about process
 * state lock (and not have to push/pop register).
 *
 * Already on private stack at this point, with "uarea" virtual address
 * in sp.  For safety, get processor specific SP, in case take (eg) NMI
 * after loading cr3 but before re-set final SP value.
 */
	.align	2
res_on_priv:
	movl	VA_PLOCAL+L_PLOCAL_PTE, %edx	# L1 pte mapping plocal stuff.
	movl	P_PTB1(%ecx), %eax		# phys addr of process PT.
	movl	%edx, PLOCAL_PTE_OFF(%eax)	# preserve plocal mapping.
	movl	VA_PLOCAL+L_PRIVSTK, %esp	# use private stack for a bit.
	movl	%eax, %cr3			# switch'em! (& flush TLB).
	movl	VA_UAREA+U_SP, %esp		# start using his real stack.
	jmp	res_on_new			# finish resuming.
/* 
 * Process has floating-point accelerator (FPA) context -- save it.
 * This code is an in-line expansion of save_fpa().
 * Is ok to modify the register variables here, since we'll be resuming
 * a process.
 */
	.align	2
res_fpa:
	movl	VA_FPA+FPA_STCTX, %eax		# FPA process context register.
	movl	%eax, VA_UAREA+U_FPA_PCR	# save in u.u_fpasave.
	movl	%ecx, %eax			# can't destroy.
	leal	VA_FPA+FPA_STOR_R1, %esi	# source == FPA R1.
	leal	VA_UAREA+U_FPA_REGS, %edi	# dest = u.u_fpasave.fpa_regs.
	movl	$FPA_NREGS, %ecx		# count
	rep;	smovl				# save the state.
	movl	%eax, %ecx			# get this back.
	movl	$0, VA_FPA_PTE			# OFF mapping.
	jmp	res_nofpa			# continue saving state.
/* 
 * Process has floating-point-unit context -- save it.
 */
	.align	2
res_float:					# save float context.
#ifdef COBUG
	/*
	 * The FPU is probably not on when working around the i486
	 * chip bug.  Turn it on before trying to save it.
	 */
	testw	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
1:
#endif /* COBUG */
	fnsave	VA_UAREA+U_FPUSAVE		# save FPU state, no pre-"wait".
	wait					# wait for save to complete.
	movl	VA_PLOCAL+L_FPUOFF, %eax	# want to turn it off.
	movl	%eax, %cr0			# it's off now!
	movb	$0, VA_PLOCAL+L_USINGFPU	# Process no longer using FPU.
	jmp	res_nofloat			# continue saving state.
#ifdef	WATCHPT
/*
 * Turn watchpoints OFF, since they were on at entry to resume().
 */
	.align	2
res_turn_wp_off:
	subl	%eax, %eax			# turn watchpoints...
	movl	%eax, %db7			#	...OFF!
	movb	%al, VA_PLOCAL+L_WATCHPTON	# note in processor-local state.
	jmp	res_wp_off			# continue...
/*
 * Process has watchpoints.  Restore them before returning.
 */
	.align	2
res_watchpt:
	movl	$VA_UAREA+U_WATCHPT, %edx	# EDX -> watchpoint state.
	movl	WP_VADDR+0(%edx), %eax		# watchpoint...
	movl	%eax, %db0			#	...virtual address 0.
	movl	WP_VADDR+4(%edx), %eax		# watchpoint...
	movl	%eax, %db1			#	...virtual address 1.
	movl	WP_VADDR+8(%edx), %eax		# watchpoint...
	movl	%eax, %db2			#	...virtual address 2.
	movl	WP_VADDR+12(%edx), %eax		# watchpoint...
	movl	%eax, %db3			#	...virtual address 3.
	movl	$DSR_CLEAR, %eax		# clear...
	movl	%eax, %db6			#	...status.
	movl	WP_CONTROL(%edx), %eax		# control register...
	movl	%eax, %db7			#	...turns ON watchpoints!
	movb	$1, VA_PLOCAL+L_WATCHPTON 	# remember in local state.
	jmp	res_wp_ret			# continue return in new proc.
#endif	WATCHPT

/*
 * use_private()
 *	Switch to run off of the per processor private stack and uarea.
 *
 * On i386, this implies switch to running on per-processor page-table.
 * This page-table has already mapped the per-processor local data
 * and Uarea, so no need to save/restore VA_PLOCAL_PTE.
 *
 * Does *not* disable watchpoints if enabled -- lets resume() handle that.
 */

ENTRY(use_private)
	movl	(%esp), %ecx			# return address.
	movl	VA_PLOCAL+L_PRIVSTK, %esp	# private stack.
	movl	VA_PLOCAL+L_PRIV_PT, %eax	# phys addr of per-processor PT.
	movl	%eax, %cr3			# Now on private PT.
	movl	$VA_UAREA+UBYTES, %ebp		# get real EBP...
	leal	-0x40(%ebp), %esp		# and real ESP, allow locals.
	jmp	*%ecx				# "return".

/*
 * Assembly assist to deal with the FPU.
 */

/*
 * init_fpu()
 *	Called at online time to init FPU (387 needs an FNINIT to
 *	turn off ERROR input -- this is used to differentiate from a 287).
 *
 * Called after l.fpuon, l.fpuoff are set up.
 */

ENTRY(init_fpu)
	movl	VA_PLOCAL+L_FPUON, %eax		# want to turn it ON.
	movl	%eax, %cr0			# it's on now!
	fninit					# init FPU state.
	movl	VA_PLOCAL+L_FPUOFF, %eax	# No longer using FPU.
	movl	%eax, %cr0			# it's off now!
	movb	$0, VA_PLOCAL+L_USINGFPU	# init software state.
	RETURN

/*
 * disable_fpu()
 *	Turn off use of the FPU on the processor.
 */

ENTRY(disable_fpu)
	cmpb	$0, VA_PLOCAL+L_USINGFPU	# Using fpu??
	jne	9f				# yup -- disable.
	RETURN					# no -- avoid slow instructions.
9:
#ifdef COBUG
	/*
	 * The FPU is probably not on when working around the i486
	 * chip bug.  Turn it on before trying to save it.
	 */
	testw	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
1:
#endif /* COBUG */
	fninit					# zap FPU state (in case junk).
	movl	VA_PLOCAL+L_FPUOFF, %eax	# No longer using FPU.
	movl	%eax, %cr0			# it's off now!
	movb	$0, VA_PLOCAL+L_USINGFPU	# No longer using FPU.
	RETURN

/*
 * restore_fpu()
 *	Restore FPU context in calling process.
 *
 * Restores FPU registers from the Uarea and enables the FPU.
 * FPU context was saved in save() or resume() if the process previously
 * used the FPU, else have standard initial context.
 *
 * restore_fpu() CANNOT trap - if saved FPU state has exception, will be
 * delivered on next floating-point operation.
 */

ENTRY(restore_fpu)
#ifdef	FPU_SIGNAL_BUG
	orw	$UF_USED_FPU, VA_UAREA+U_FLAGS
#endif
	movl	VA_PLOCAL+L_FPUON, %eax		# want to turn it ON.
	movl	%eax, %cr0			# it's on now!
	movb	$1, VA_PLOCAL+L_USINGFPU	# Process(or) now using FPU.
	frstor	VA_UAREA+U_FPUSAVE		# restore context.
	RETURN

#ifdef	FPU_SIGNAL_BUG
/*
 * save_fpu(addr)
 *	Save FPU context to the given address of a struct fpusave area.
 *
 * As a side effect, leaves FPU reinitialized.
 *
 * If you change this routine, be sure to examine save_fpu_fork() just
 * below.
 *
 * Typically called via:
 *	if (l.usingfpu) save_fpu((struct fpusave *)addr);
 *
 * Can't trap.
 */

ENTRY(save_fpu)
#ifdef COBUG
	/*
	 * The FPU is probably not on when working around the i486
	 * chip bug.  Turn it on before trying to save it.
	 */
	testw	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
1:
#endif /* COBUG */
	movl	SPARG0, %eax
	fnsave	(%eax)				# save FPU state,no pre-"wait".
	wait					# wait for save to complete.
	RETURN

#else	/* FPU_SIGNAL_BUG */
/*
 * save_fpu()
 *	Save FPU context. As a side effect, leaves FPU reinitialized.
 * If you change this routine, be sure to examine save_fpu_fork() just
 * below.
 *
 * Saves the fpu registers into the uarea.
 * Typically called via:
 *	if (l.usingfpu) save_fpu();
 *
 * Can't trap.
 */

ENTRY(save_fpu)
#ifdef COBUG
	/*
	 * The FPU is probably not on when working around the i486
	 * chip bug.  Turn it on before trying to save it.
	 */
	testw	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
1:
#endif /* COBUG */
	fnsave	VA_UAREA+U_FPUSAVE		# save FPU state, no pre-"wait".
	wait					# wait for save to complete.
	RETURN

#endif	/* FPU_SIGNAL_BUG */

/*
 * save_fpu_fork()
 *      Identical to save_fpu(), except that FPU context is unchanged
 *      in current process.  Used only by fork().
 */

ENTRY(save_fpu_fork)
#ifdef COBUG
	/*
	 * The FPU is probably not on when working around the i486
	 * chip bug.  Turn it on before trying to save it.
	 */
	testw	$PL_C0BUG, VA_PLOCAL+L_FLAGS
	jz	1f
	movl	VA_PLOCAL+L_FPUON, %eax
	movl	%eax, %cr0
1:
#endif /* COBUG */
	fnsave	VA_UAREA+U_FPUSAVE		# save FPU state, no pre-"wait".
	wait					# wait for save to complete.
	frstor  VA_UAREA+U_FPUSAVE              # bring back context
	RETURN
