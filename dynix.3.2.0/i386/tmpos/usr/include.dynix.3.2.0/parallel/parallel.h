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

/* $Header: parallel.h 1.17 90/02/20 $
 *
 * parallel.h
 *	Definitions for use in parallel, shared-memory programs.
 */

/* $Log:	parallel.h,v $
 */

/*
 * A "lock" is a byte of memory, initialized to zero (unlocked).
 */

typedef unsigned char	slock_t;		/* 's' for "spin"-lock */

#define	L_UNLOCKED	0
#define	L_LOCKED	1

/*
 * Was a conditional lock request granted (L_SUCCESS) or denied (L_FAILED)
 */
#define L_FAILED	0
#define L_SUCCESS	1

/*
 * A "barrier" allows multiple processes to synchronize by having
 * all of them exit the barrier construct simultaneously.
 *
 * This version assumes <= 255 processes, fits in one 4-byte integer,
 * and is based on spin-locks.
 */

typedef struct	{
	slock_t		b_mutex;	/* mutual exclusion */
	unsigned char	b_limit;	/* number participants */
	unsigned char	b_count;	/* state counter */
	unsigned char	b_useno;	/* current use # (state flag) */
} sbarrier_t;				/* 's' for "spin"-barrier */

/*
 * Other useful declarations.
 */

extern	char	*sbrk(), *shsbrk();
extern	char	*shmalloc();


/*
 * S_LOCK() S_UNLOCK(), S_CLOCK(), S_INIT_LOCK, and S_WAIT_BARRIER functions.
 * These are implemented as "asm functions", and will produce very good code.
 *
 * Need to use the -i flag with the optimizer when these inline functions are
 * used. (cc -O -i).   This is to prevent the optimizer from removing
 * "redundant" moves (which aren't really redundant in this case)
 * in the spin loops.
 *
 * Note for Model A you must use interlocked instructions due
 * cache incoherrency.
 */

#ifndef lint
asm void S_LOCK(laddr)
{
%reg laddr; lab loop, spin, done;
/PEEPOFF
loop:	movb	$L_LOCKED, %al
	xchgb	%al, (laddr)		/* an atomic "test and set" */
	cmpb	$L_UNLOCKED, %al
	je	done			/* if equal, we got the lock */
spin:	cmpb	$L_UNLOCKED, (laddr)	/* spin in cache until unlocked */
	je	loop			/* then, try get lock again */
	jmp	spin
done:
/PEEPON
%mem laddr; lab loop, spin, done;
/PEEPOFF
loop:	movb	$L_LOCKED, %al
	movl	laddr, %ecx
	xchgb	%al, (%ecx)		/* an atomic "test and set" */
	cmpb	$L_UNLOCKED, %al
	je	done			/* if equal, we got the lock */
spin:	cmpb	$L_UNLOCKED, (%ecx)	/* spin in cache until unlocked */
	je	loop			/* then, try get lock again */
	jmp	spin
done:
/PEEPON
}

asm void S_UNLOCK(laddr)
{
%reg laddr;
	movb	$L_UNLOCKED, %al
	xchgb	%al, (laddr)		/* clear lock, "atomically" */
%mem laddr;
	movb	$L_UNLOCKED, %al
	movl	laddr, %ecx
	xchgb	%al, (%ecx)		/* clear lock, "atomically" */
}

asm S_CLOCK(laddr)
{
%reg laddr; lab done;
	movb	$L_LOCKED, %dl
	movl	$L_SUCCESS, %eax
	xchgb	%dl, (laddr)		/* test and set lock */
	cmpb	$L_UNLOCKED, %dl	/* if we got it, return success */
	je	done
	movl	$L_FAILED, %eax		/* else return failure */
done:
%mem laddr; lab done;
	movb	$L_LOCKED, %dl
	movl	laddr, %ecx
	movl	$L_SUCCESS, %eax
	xchgb	%dl, (%ecx)		/* test and set lock */
	cmpb	$L_UNLOCKED, %dl	/* if we got it, return success */
	je	done
	movl	$L_FAILED, %eax		/* else return failure */
done:
}
#endif /* lint */

#define S_INIT_LOCK(lp)	S_UNLOCK(lp)

/*
 * In line barrier initialization macro
 *
 * Note, there's no need to initialize either b_useno (due to its manner
 * of useage, or b_mutex (due to its non-usage).
 */

#define S_INIT_BARRIER(b, l)	((b)->b_limit = (b)->b_count = (l))

/*
 * In line barrier wait procedure.
 *
 * Define offsets of fields in barrier, for use in assembly language.
 */

#define MUTEX	0
#define COUNT	1
#define LIMIT	2
#define USENO	3

#ifndef lint
asm void S_WAIT_BARRIER(baddr)
{
%reg baddr; lab spin;
/PEEPOFF
	movb	USENO(baddr), %al	/* save old barrier useno value */
	lock				/* assert lock on next instruction */
	decb	COUNT(baddr)		/* atomic decrement of barrier count */
	jne	spin			/* if not all checked in, go spin */
	movb	LIMIT(baddr), %dl
	movb	%dl, COUNT(baddr) 	/* all checked in! re-init barrier */
	incb	USENO(baddr)		/* and free all the spinners */
spin:	cmpb	USENO(baddr), %al	/* spin here until all are checked in */
	je	spin
/PEEPON
%mem baddr; lab spin;
/PEEPOFF
	movl	baddr, %ecx		/* load barrier pointer into register */
	movb	USENO(%ecx), %al	/* save old barrier useno value */
	lock				/* assert lock on next instruction */
	decb	COUNT(%ecx)		/* atomic decrement of barrier count */
	jne 	spin			/* if not all checked in, go spin */
	movb	LIMIT(%ecx), %dl
	movb	%dl, COUNT(%ecx)	/* all checked in! re-init barrier */
	incb	USENO(%ecx)		/* and free all the spinners */
spin:	cmpb	USENO(%ecx), %al	/* spin here until all are checked in */
	je	spin
/PEEPON
}

/*
 * This is for internal use by parallel library functions only
 */
asm void _M_JOIN(baddr, clearl)
{
%mem baddr; mem  clearl; lab spin;
/PEEPOFF
	movl	baddr, %ecx		/* load barrier pointer into register */
	movb	USENO(%ecx), %al	/* save old barrier useno value */
	lock				/* assert lock on next instruction */
	decb	COUNT(%ecx)		/* atomic decrement of barrier count */
	jne 	spin			/* if not all checked in, go spin */
	movb	LIMIT(%ecx), %dl
	movb	%dl, COUNT(%ecx)	/* all checked in! re-init barrier */
	movl	clearl, %edx
	movl	$0, (%edx)
	incb	USENO(%ecx)		/* and free all the spinners */
spin:	cmpb	USENO(%ecx), %al	/* spin here until all are checked in */
	je	spin
/PEEPON
}
#endif /* lint */





/*
 * Convenience definitions.
 */

#ifndef	NULL
#define	NULL		0
#endif

/*
 * Various globally used data.
 */

extern	int	errno;

extern	int	_shm_fd;		/* fd for shared data mapped file */
extern	int	_pgoff;			/* getpagesize() - 1 */

/*
 * PGRND() rounds up a value to next page boundary.
 */

#define	PGRND(x)	(char *) (((int)(x) + _pgoff) & ~_pgoff)
