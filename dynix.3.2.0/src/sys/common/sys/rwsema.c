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

#ifndef	lint
static	char	rcsid[] = "$Header: rwsema.c 1.4 90/05/08 $";
#endif

/*
 * rwsema.c
 *	Reader/Writer (Shared/Exclusive) semaphores.
 *
 * Implementation should be machine independent.
 *
 * NOTES:
 *	Could be built from sema_t's and lock_t.  Done as separate object
 *	instead of built from lock_t and sema_t.  Major reason is efficiency
 *	of space and lower overhead.  In particular, removes loops that
 *	test state in P() operations (avoid extra mutex round-trip during
 *	contention -- when process wakes, it "has" the semaphore).
 *
 *	Reader/writer interraction policy selectable when semaphore
 *	initialized.  Support for weak writer preference or strong writer
 *	preference.  Vnodes use SWP as this favors "normal UNIX" operations.
 *
 *	Could consider "adaptable" WWP policy which senses queued writer and
 *	temporarily becomes SWP if too many readers have entered -- ie, avoid
 *	starving the writer(s).  This requires at least a counter, which
 *	increases size of rwsema_t.  This still needs to be selectable for
 *	those cases where a single thread asserts multiple read-locks (eg,
 *	Multi-Threading prototype).
 *
 *	Assume all waiting priorities <= PZERO (ie, non-signalable).  Else
 *	need additional support in signal handling to know which "force_v"
 *	to use.  Ideally, force_v() is virtual function.  Misc implementation
 *	simplifications assume no "force_v" is necessary.
 *
 *	No SIGWOKE semantics.
 *
 *	Really should do all this in C++, but enough use of this object in C
 *	based structures (eg, vnodes) that do in C short term.  Sigh ;-)
 *
 * TODO:
 *	Inline rwsema_block() and rwsema_wake() to see what (if any)
 *	performance difference they make.
 *
 *	For applications using both reader and writer locks, see how fair
 *	the policy is.  Ie, does it allow readers or writers to dominate too
 *	much?  Likely application/use dependent.
 */

/* $Log:	rwsema.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

#define	enter(rw, s)	P_GATE((rw)->rw_mutex, s)
#define	exit(rw, s)	V_GATE((rw)->rw_mutex, s)

#define	RWSEMA_STRONG(rw)	((rw)->rw_policy == RWSEMA_SWP)
#define	RWSEMA_WEAK(rw)		((rw)->rw_policy == RWSEMA_WWP)

static	void	rwsema_block();
static	void	rwsema_wake();

/*
 * _init_rwsema()
 *	Initialize a rwsema.  Assumes no need for gate to initiailize lock.
 *
 * Non-SLIC-gates version.  Eg, Symmetry.
 */

void
_init_rwsema(rw, policy, gate)
	register rwsema_t *rw;
	char	policy;
	gate_t	gate;
{
	RWSEMA_SETIDLE(rw);
	rw->rw_mutex = gate;
	rw->rw_policy = policy;
	RWL_INIT(&rw->rw_wrwait);
	RWL_INIT(&rw->rw_rdwait);
}

/*
 * p_reader()
 *	Obtain read (shared) access to semaphore.
 *
 * Return at SPL0.
 */

void
p_reader(rw, pri)
	register rwsema_t *rw;
{
	GATESPL(s);

	ASSERT_DEBUG(pri <= PZERO, "p_reader: bad pri");

	enter(rw, s);
	if (RWSEMA_RDOK(rw) && (!RWSEMA_STRONG(rw) || !RWSEMA_WRBLOCKED(rw))) {
		RWSEMA_NEWREADER(rw);
		exit(rw, s);
	} else {
		rwsema_block(rw, &rw->rw_rdwait, pri, (lock_t*) NULL);
		ASSERT_DEBUG(RWSEMA_RDBUSY(rw), "p_reader: skew");
	}
}

/*
 * p_reader_v_lock()
 *	Obtain read (shared) access to semaphore, atomicly releasing a lock.
 *
 * Return at SPL0.
 */

void
p_reader_v_lock(rw, pri, lockp)
	register rwsema_t *rw;
	lock_t	*lockp;
{
	GATESPL(s);

	ASSERT_DEBUG(pri <= PZERO, "p_reader_v_lock: bad pri");

	enter(rw, s);
	if (RWSEMA_RDOK(rw) && (!RWSEMA_STRONG(rw) || !RWSEMA_WRBLOCKED(rw))) {
		RWSEMA_NEWREADER(rw);
		exit(rw, s);
		v_lock(lockp, SPL0);
	} else {
		rwsema_block(rw, &rw->rw_rdwait, pri, lockp);
		ASSERT_DEBUG(RWSEMA_RDBUSY(rw), "p_reader_v_lock: skew");
	}
}

/*
 * cp_reader()
 *	Contitionally obtain read (shared) access to semaphore.
 *
 * Note: could test first before enter region.  If frequent use and failure,
 * should consider this.
 *
 * Return true if got it, else false.
 */

bool_t
cp_reader(rw)
	register rwsema_t *rw;
{
	bool_t	value = 1;
	GATESPL(s);

	enter(rw, s);
	if (RWSEMA_RDOK(rw) && (!RWSEMA_STRONG(rw) || !RWSEMA_WRBLOCKED(rw)))
		RWSEMA_NEWREADER(rw);
	else
		value = 0;
	exit(rw, s);
	return value;
}

/*
 * v_reader()
 *	Release read (shared) access to semaphore.
 *
 * If # readers reaches zero, release writers if any.
 */

void
v_reader(rw)
	register rwsema_t *rw;
{
	GATESPL(s);

	enter(rw, s);
	ASSERT_DEBUG(RWSEMA_RDBUSY(rw), "v_reader: skew");
	RWSEMA_DROPREADER(rw);
	if (RWSEMA_IDLE(rw) && RWSEMA_WRBLOCKED(rw)) {
		struct proc *p;
		RWSEMA_SETWRBUSY(rw);
		RWL_DEQUEUE(&rw->rw_wrwait, p, struct proc *);
		exit(rw, s);
		rwsema_wake(p);
	} else
		exit(rw, s);
}

/*
 * p_writer()
 *	Obtain write (exclusive) access to semaphore.
 *
 * Return at SPL0.
 */

void
p_writer(rw, pri)
	register rwsema_t *rw;
{
	GATESPL(s);

	ASSERT_DEBUG(pri <= PZERO, "p_writer: bad pri");

	enter(rw, s);
	if (RWSEMA_IDLE(rw)) {
		RWSEMA_SETWRBUSY(rw);
		exit(rw, s);
	} else {
		rwsema_block(rw, &rw->rw_wrwait, pri, (lock_t*) NULL);
		ASSERT_DEBUG(RWSEMA_WRBUSY(rw), "p_writer: skew");
	}
}

/*
 * p_writer_v_lock()
 *	Obtain write (exclusive) access to semaphore, atomicly releasing a lock.
 *
 * Return at SPL0.
 */

void
p_writer_v_lock(rw, pri, lockp)
	register rwsema_t *rw;
	lock_t	*lockp;
{
	GATESPL(s);

	ASSERT_DEBUG(pri <= PZERO, "p_writer_v_lock: bad pri");

	enter(rw, s);
	if (RWSEMA_IDLE(rw)) {
		RWSEMA_SETWRBUSY(rw);
		exit(rw, s);
		v_lock(lockp, SPL0);
	} else {
		rwsema_block(rw, &rw->rw_wrwait, pri, lockp);
		ASSERT_DEBUG(RWSEMA_WRBUSY(rw), "p_writer_v_lock: skew");
	}
}

/*
 * cp_writer()
 *	Contitionally obtain write (exclusive) access to semaphore.
 *
 * Note: could test first before enter region.  If frequent use and failure,
 * should consider this.
 *
 * Return true if got it, else false.
 */

bool_t
cp_writer(rw)
	register rwsema_t *rw;
{
	bool_t	value = 1;
	GATESPL(s);

	enter(rw, s);
	if (RWSEMA_IDLE(rw))
		RWSEMA_SETWRBUSY(rw);
	else
		value = 0;;
	exit(rw, s);
	return value;
}

/*
 * v_writer()
 *	Release write (exclusive) access to semaphore.
 *
 * Release single queued writer first (weak writer preference), then readers.
 *
 * No livelock while waking readers, since while this is busy additional reads
 * enter the semaphore instead of queue.  With low probability, could have
 * multiple processes in here on same semaphore (race with writer getting in
 * and releasing, and more readers queueing).  Algorithm handles this.
 *
 * Ideally remove "SWP" test in while()-loop, but I think this can livelock
 * in a SWP policy.
 *
 * Implementation does round-trip on semaphore mutex per waking reader to
 * avoid long latency on semaphore.  This behaves like vall_sema().
 */

void
v_writer(rw)
	register rwsema_t *rw;
{
	GATESPL(s);

	enter(rw, s);
	ASSERT_DEBUG(RWSEMA_WRBUSY(rw), "v_writer: skew");
	if (RWSEMA_WRBLOCKED(rw)) {		/* wake writer(s) first */
		struct proc *p;			/* leave "write-busy" set */
		RWL_DEQUEUE(&rw->rw_wrwait, p, struct proc *);
		exit(rw, s);
		rwsema_wake(p);
	} else {				/* wake any/all readers */
		RWSEMA_SETIDLE(rw);
		while (RWSEMA_RDBLOCKED(rw) && RWSEMA_RDOK(rw) &&
		       (!RWSEMA_STRONG(rw) || !RWSEMA_WRBLOCKED(rw))) {
			struct proc *p;
			RWL_DEQUEUE(&rw->rw_rdwait, p, struct proc *);
			RWSEMA_NEWREADER(rw);
			exit(rw, s);
			rwsema_wake(p);
			if (!RWSEMA_RDBLOCKED(rw))	/* heuristic to avoid */
				return;			/* final lock RT */
			enter(rw, s);
		}
		exit(rw, s);
	}
}

/*
 * rwsema_wake()
 *	Wake the process passed as argument.
 *
 * Caller dequeues process, unlocks rwsema, then calls rwsema_wake() to
 * actually wake up the process.
 *
 * Ok to drop rwsema mutex once process is dequeued.  The process can't be
 * dispatched since it's not on run-Q.  This is somewhat simplified by not
 * having a "force_v" interraction.  Thus, only fuss with the process is
 * swapping daemon who also locks process-state.
 *
 * Note that once setrq(), the process *can* dispatch.
 *
 * Clone of code in sema.c:v_sema().  Ideally, all should use same "wake"
 * procedure.
 */

static void
rwsema_wake(p)
	register struct proc *p;
{
	spl_t	s;

	s = p_lock(&p->p_state, SPLHI);

	p->p_stat = SRUN;
	p->p_wchan = NULL;
	if (p->p_flag & SLOAD) {
		VOID_P_GATE(G_RUNQ);
		setrq(p);
		V_GATE(G_RUNQ, SPLHI);
		v_lock(&p->p_state, s);
	} else {
		v_lock(&p->p_state, s);
		v_sema(&runout);
	}
}

/*
 * rwsema_block()
 *	Block calling process on a rwsema queue.
 *
 * Lock process, place on relevant queue, unlock semaphore, switch away (unlock
 * process in switch to avoid race with a V()).  If lock argument, release as
 * soon as process on semaphore queue.
 *
 * Note that this is simpler than what's in p_sema(), since
 * sleep priority is assumed <= PZERO and signals aren't handled.
 *
 * Clone of code in sema.c:p_sema_v_lock().
 *
 * Caller holds rwsema locked.
 * Process doesn't return until it is awoken, and returns at SPL0.
 */

static void
rwsema_block(rw, rl, pri, lockp)
	rwsema_t *rw;
	slink_t	*rl;
	int	pri;
	lock_t	*lockp;
{
	register struct	proc *p = u.u_procp;

	(void) p_lock(&p->p_state, SPLHI);	/* NOTE: nested lock! */
	RWL_ENQUEUE(rl, p);			/* queue at tail */

	exit(rw, SPLHI);			/* proc-state still locked */

	if (lockp) {				/* if releasing a lock ... */
		v_lock(lockp, SPLHI);		/* now is safe: process on Q */
	}

	p->p_stat = SSLEEP;			/* you are getting sleepy...  */
	p->p_wchan = (sema_t *) rw;		/* and you want to sleep here */
	p->p_slptime = 0;			/* but haven't slept yet */
	p->p_pri = pri;				/* wake at this priority */
	u.u_ru.ru_nvcsw++;			/* voluntary context switch */
	swtch(p);				/* returns at SPL0 */
}
