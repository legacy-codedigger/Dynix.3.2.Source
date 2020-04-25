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
 * $Header: mutex.h 2.32 90/11/08 $
 *
 * mutex.h
 *	Implementation dependent mutual-exclusion interface definitions.
 *
 * i386 version.
 */

/* $Log:	mutex.h,v $
 */

/*
 * Processor interrupt masking (lowest level mutex).
 *
 * Note: cross compiler and lint don't yet understand asm functions.
 */

#if	!defined(GENASSYM) && !defined(lint)

asm DISABLE()
{
	cli
}

asm ENABLE()
{
	sti
}

#else

void DISABLE();
void ENABLE();

#endif  /* !defined(GENASSYM) && !defined(lint) */

/*
 * Gates.  Unlocked state == 0 to allow implicit initialization.
 *
 * P_GATE, VOID_P_GATE, V_GATE define implementation independent
 * direct gate interface.  "spl" isn't used in this implementation.
 */

#define	G_UNLOCKED	0
#define	G_LOCKED	1

#define	CPGATEFAIL	0
#define	CPGATESUCCEED	1

#ifdef	DEBUG
#define	v_gate(g)	( v_gate_asm(g), v_gate_dbg(), ENABLE() )
#else	!DEBUG
#define	v_gate(g)	( v_gate_asm(g), ENABLE() )
#endif	DEBUG

#define	P_GATE(g,spl)	p_gate(&(g))
#define	CP_GATE(g)	cp_gate(&(g))
#define	VOID_P_GATE(g)	p_gate(&(g))
#define	V_GATE(g,spl)	v_gate(&(g))

/*
 * Locks.
 *
 * init_lock() interface provided as a macro to avoid need to represent
 * SLIC gates numbers.
 */

#define	L_UNLOCKED	G_UNLOCKED
#define	L_LOCKED	G_LOCKED

#define	CPLOCKFAIL	-1		/* illegal SPL value */

#define	init_lock(l,g)	*(lock_t *)(l) = L_UNLOCKED

#if	defined(KXX) || defined(SLIC_GATES)
spl_t	_cp_lock();
#define cp_lock(l,p)	((*(lock_t*)(l)==L_UNLOCKED)? _cp_lock(l,p): CPLOCKFAIL)
#endif	KXX

#define v_lock(l,p)	( v_lock_asm(l), splx(p) )

/*
 * Implementation dependent semaphore interfaces.
 *
 * init_sema() interface provided as a macro to avoid need to represent
 * SLIC gates numbers.
 */

#define	init_sema(sema, value, flags, gate) { \
	(sema)->s_gate = G_UNLOCKED; \
	(sema)->s_count = (value); \
	(sema)->s_head = (sema)->s_tail = (struct proc *)(sema); \
}

#define	init_rwsema(rw, policy, gate)	_init_rwsema(rw, policy, G_UNLOCKED)
void	_init_rwsema();

/*
 * Macros to lock the memory lists, unlock, and wait for free memory
 * to exist.
 *
 * Note that can race between UNLOCK_MEM and WAIT_MEM.  This low
 * probability race is resolved in vmmeter().
 *
 * TRY_LOCK_MEM() uses SPLHI since caller may already hold a lock at SPLHI.
 */

#define	LOCK_MEM	l.splmem = p_lock(&mem_alloc, SPLMEM)
#define	UNLOCK_MEM	v_lock(&mem_alloc, l.splmem)
#define	WAIT_MEM	{ (void) cv_sema(&runout); p_sema(&mem_wait, PRSWAIT); }
#define	TRY_LOCK_MEM(spl) \
	(((spl = cp_lock(&mem_alloc, SPLHI)) != CPLOCKFAIL) \
		? (l.splmem = spl, 1) : 0)

/*
 * Implementation dependent semaphore manipulation macros.
 *
 * lockup_sema:		lock access to semaphore structure.
 * unlock_sema:		unlock access to semaphore structure.
 *
 * "spl" argument ignored in this implementation.
 */

#define lockup_sema(s,spl)	P_GATE((s)->s_gate,spl)
#define unlock_sema(s,spl)	V_GATE((s)->s_gate,spl)

/*
 * p_event() can be p_sema(). p_sema will allow multiple waiters however.
 */

#define p_event		p_sema

/* 
 * Set the sema count to zero. spl is not used in this implementation.
 */
#define clear_event(sema,spl)\
{\
	lockup_sema(sema,spl);\
	sema_count(sema) = 0;\
	unlock_sema(sema,spl);\
}
#define clear_sema(sema)\
{\
	lockup_sema(sema);\
	sema_count(sema) = 0;\
	unlock_sema(sema);\
}

#if	!defined(GENASSYM) && !defined(lint)

/*
 * In-line expansion code for portable gate and lock implementations.
 * When/if the assembler can handle jumps between text subsegments,
 * these asm functions can be more efficient by putting the "fail/spin"
 * code out-of-line, leaving the success path completely in-line.
 *
 * For ASM versions, see machine/asm.h
 */

#if	defined(KXX) || defined(SLIC_GATES)

/*
 * v_gate_asm(gaddr)
 *	gate_t	*gaddr;
 *
 * Low level release a gate.  Ok to just write on the variable, since
 * only writes on gate are from holding process or behind SLIC gate.
 */

asm	v_gate_asm(gaddr)
{
%mem gaddr;
	movl	gaddr, %ecx			/* in case pointer on stack */
	movb	$G_UNLOCKED, (%ecx)		/* release the gate */
}

/*
 * v_lock_asm(lock)
 *	lock_t	*l;
 *
 * Low-level release a lock.  Does *not* un-mask interrupts (see v_lock()).
 * Ok to just write on the variable, since only writes on lock are from
 * holding process or behind SLIC gate.
 */

asm	v_lock_asm(lock)
{
%mem lock;
	movl	lock, %ecx			/* in case pointer on stack */
	movb	$L_UNLOCKED, (%ecx)		/* release the lock */
%reg lock;
	movb	$L_UNLOCKED, (lock)		/* release the lock */
}

#else	Real HW

/*
 * void p_gate(gate)
 *	gate_t *gate;
 *
 * Acquire a gate.  Disable processor interrupts.  Spin with interrupts enabled.
 */

asm	p_gate(gaddr)
{
%mem gaddr; lab loop, spin, done;
	movb	$G_LOCKED, %al			/* value to exchange */
	movl	gaddr, %ecx			/* &gate */
loop:	cli					/* interrupts OFF */
	xchgb	%al, (%ecx)			/* try for gate */
	cmpb	$G_UNLOCKED, %al		/* got it?? */
	je	done				/* yup */
	sti					/* nope -- re-enable ints */
spin:	cmpb	$G_UNLOCKED, (%ecx)		/* spin until gate avail */
	je	loop				/* try again when gate free */
	jmp	spin				/* spin... */
done:
}

/*
 * bool_t cp_gate(gate)
 *	gate_t *gate;
 *
 * Acquire a gate.  Block processor interrupts.
 *
 * To work well on SGS hardware, need to poll gate first and see if it's
 * held -- don't want idle() processors to thrash the runQ gate until
 * one really takes an entry off.
 *
 * Return CPGATESUCCEED or CPGATEFAIL.
 */

asm	cp_gate(gaddr)
{
%mem gaddr; lab done, fail;
	movl	$CPGATESUCCEED, %eax		/* assume succeed */
	movb	$G_LOCKED, %dl			/* value to exchange */
	movl	gaddr, %ecx			/* &gate */
	cmpb	%dl, (%ecx)			/* is locked? */
	je	fail				/* yup -- don't even try */
	cli					/* interrupts OFF */
	xchgb	%dl, (%ecx)			/* try for gate */
	cmpb	$G_UNLOCKED, %dl		/* got it? */
	je	done				/* yup */
	sti					/* nope - interrupts on again */
fail:	movl	$CPGATEFAIL, %eax		/* return failure */
done:
}

/*
 * v_gate_asm(gaddr)
 *	gate_t	*gaddr;
 *
 * Low level release a gate.
 *
 * Must use locked operation since compatibility mode cache (or no cache)
 * uses bus lock line; just writing the variable can race with an attempt
 * to acquire the gate.
 */

asm	v_gate_asm(gaddr)
{
%mem gaddr;
	movb	$G_UNLOCKED, %al		/* value to exchange */
	movl	gaddr, %ecx			/* in case pointer on stack */
	xchgb	%al, (%ecx)			/* release the gate */
}

#define I486BUG3
#ifdef I486BUG3
/*
 * On steps of the i486 up to C0 (Beta), we must inhibit interrupts until
 * we know that the SLIC lmask timing window is closed.  Errata #3 for the
 * i486 states that if interrupt is presented to the i486, but is removed
 * before the i486 can generate interrupt acknowledge, the chip will behave
 * in an undefined fashion.  The only way this happens on Symmetry is when
 * the interrupt arrives as the SLIC lmask is written--the interrupt gets
 * droppped when the mask takes effect, potentially before the interrupt
 * is acknowledged.  By hard-masking interrupt on the chip, we cause the
 * i486 to ignore the interrupt line, avoiding the problem entirely.
 *
 * The files containing this workaround are: asm.h, intctl.h, and mc_mutex.h
 */
#endif /* I486BUG3 */

/*
 * spl_t
 * p_lock(lockp, retipl)
 *	lock_t	*lockp;
 *	spl_t	retipl;
 *
 * Lock the lock and return at interrupt priority level "retipl".
 * Return previous interrupt priority level.
 *
 * When the assembler handles inter-sub-segment branches, the "fail/spin"
 * code can be moved out-of-line in (eg) ".text 3".
 *
 * After writing SLIC local mask, must do a read to synchronize the write
 * and then insure 9/12 cycles occur before the xchg (to allow a now masked
 * interrupt to occur before hold the locked resource).  The actual delay
 * is per-processor; a countdown delay loop is used.
 *
 * The basic time to fall straight through the loop is:
 *
 *	Instruction			i386	i486
 *	movb	(%ecx),%ah		4	1
 *	movl	DELAYPTR,%ecx		4	1
 *	movl	(%ecx),%ecx		4	1
 * 0:	subl	$1,%ecx			2	1
 *	jg	0b			3/10	1/3
 *	popl	%ecx			4	4
 *					---	---
 *					21	9
 *
 * Each further iteration of the subl..jg loop takes an addition 12 clocks
 * on an i386, and 4 clocks on an i486.
 */
asm	p_lock(laddr, spl)
{
%mem laddr, spl; lab loop, spin, done;
/PEEPOFF
	movl	laddr, %ecx			/* &lock into KNOWN register */
	movl	spl, %eax			/* spl into KNOWN register */
	movl	_va_slic_lmask, %edx		/* get slic mask address */
loop:	movb	(%edx), %ah			/* get old interrupt mask */
#ifdef I486BUG3
	pushfl
	cli
#endif
	movb	%al, (%edx)			/* write new mask */
	movb	(%edx), %al			/* dummy read/sync write */
	movl	_plocal_slic_delay,%edx		/* point to delay value */
	movl	(%edx),%edx			/* get delay value */
0:	subl	$1,%edx				/* count down to 0 */
	jg	0b
#ifdef I486BUG3
	popfl
#endif
	movb	$L_LOCKED, %dl			/* value to exchange */
	xchgb	%ah, %al			/* move return value */
	xchgb	%dl, (%ecx)			/* try for lock */
	cmpb	$L_UNLOCKED, %dl		/* got it? */
	je	done				/* yup */
	movl	_va_slic_lmask, %edx		/* get slic mask address */
	movb	%al, (%edx)			/* restore previous mask */
	xchgb	%ah, %al			/* restore previous spl */
spin:	cmpb	$L_UNLOCKED, (%ecx)		/* spin until... */
	je	loop				/*	...lock is clear */
	jmp	spin				/* while not clear... */
done:
/PEEPON
}

/*
 * spl_t
 * cp_lock(lockp, retipl)
 *	lock_t	*lockp;
 *	spl_t	retipl;
 *
 * Conditionally acquire a lock.
 *
 * If lock is available, lock the lock and return at interrupt priority
 * level "retipl". Return previous interrupt priority level.
 * If lock is unavailable, return CPLOCKFAIL.
 *
 * See comments in p_lock() about writing SLIC mask.
 */

asm	cp_lock(laddr, spl)
{
%mem laddr, spl; lab done;
/PEEPOFF
	movl	laddr, %ecx			/* &lock into KNOWN register */
	movl	spl, %eax			/* spl into KNOWN register */
	movl	_va_slic_lmask, %edx		/* get slic mask address */
	movb	(%edx), %ah			/* get old interrupt mask */
#ifdef I486BUG3
	pushfl
	cli
#endif
	movb	%al, (%edx)			/* write new mask */
	movb	(%edx), %al			/* dummy read/sync write */
	movl	_plocal_slic_delay,%edx		/* point to delay value */
	movl	(%edx),%edx			/* get delay value */
0:	subl	$1,%edx				/* count down to 0 */
	jg	0b
#ifdef I486BUG3
	popfl
#endif
	movb	$L_LOCKED, %dl			/* value to exchange */
	xchgb	%ah, %al			/* move return value */
	xchgb	%dl, (%ecx)			/* try for lock */
	cmpb	$L_UNLOCKED, %dl		/* got it? */
	je	done				/* yup */
	movl	_va_slic_lmask, %edx		/* get slic mask address */
	movb	%al, (%edx)			/* restore previous mask */
	movl	$CPLOCKFAIL, %eax		/* and return failure */
done:
/PEEPON
}

/*
 * v_lock_asm(lock)
 *	lock_t	*l;
 *
 * Low-level release a lock.  Does *not* un-mask interrupts (see v_lock()).
 *
 * Must use locked operation since compatibility mode cache (or no cache)
 * uses bus lock line; just writing the variable can race with an attempt
 * to acquire the gate.
 */

asm	v_lock_asm(lock)
{
%mem lock;
	movb	$L_UNLOCKED, %al		/* value to exchange */
	movl	lock, %ecx			/* in case pointer on stack */
	xchgb	%al, (%ecx)			/* release the lock */
%reg lock;
	movb	$L_UNLOCKED, %al		/* value to exchange */
	xchgb	%al, (lock)			/* release the lock */
}

#endif	KXX

#endif	!GENASSYM && !lint
