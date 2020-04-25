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
 * $Header: mutex.h 2.6 90/01/26 $
 *
 * mutex.h
 *	Implementation independent mutual-exclusion interface definitions.
 *
 * Typedef's for gate_t, lock_t, rwlock_t, sema_t are in types.h
 *
 * This file contains definitions common to all implementations.
 * Implementation specific definitions are in machine/mutex.h.
 */

/* $Log:	mutex.h,v $
 */

#ifdef	KERNEL
#include "../machine/mutex.h"
#else
#include <machine/mutex.h>
#endif	KERNEL

/*
 * Locks.
 *
 * v_lock() and cp_lock() are #define's in all implementations so far.
 */

spl_t	p_lock();

/* void	v_lock(); */
/* spl_t cp_lock(); */

/*
 * Semaphores.
 */

/*
 * The s_head,s_tail pointers allow the sema-structure to be the
 * actual head of a doubly-linked list.  The pointers are the first
 * parts of the structure to allow the same list-manipulation to be
 * used as for processes (see proc[] structure).
 *
 * An alternative list structure keeps one pointer in the semaphore
 * structure, pointing at the tail of the list.  This allows the head
 * and tail to be easily located, but requires more overhead for
 * force_v_sema().  If the number of semaphores becomes large, the
 * space savings of this method may be useful.
 */

void	p_sema();
void	v_sema();
void	force_v_sema();
bool_t	cp_sema();
bool_t	cv_sema();
void	vall_sema();
void	p_sema_v_lock();
void	v_event();

/*
 * Various semaphore manipulation macros.
 *
 * sema_count:		count value of the semaphore; can be an lvalue.
 * sema_avail:		true if sema is "available" (eg, count > 0).
 * blocked_sema:	true if a process is waiting on the semaphore.
 */

#define	sema_count(s)		(s)->s_count
#define	sema_avail(s)		(sema_count(s) > 0)
#define	blocked_sema(s)		(sema_count(s) < 0)

/*
 * Macros to manipulate multiple "reader" / Single "writer" semaphores.
 * Can also be viewed as shared/exclusive.  See h/types.h for structure
 * definition.
 *
 * Selectable policy: Strong Writer Preference (SWP) blocks new readers if a
 * waiting writer exists.  Weak Writer Preference (WWP) always allows new
 * readers if no writers queued, but v_writer() wakes writers ahead of readers.
 */

#define	RWL_INIT(rl)		SL_INIT(rl)
#define	RWL_EMPTY(rl)		SL_EMPTY(rl)
#define	RWL_ENQUEUE(rl, ent)	SL_APPEND(rl, ent)
#define	RWL_DEQUEUE(rl, ent, t)	SL_DEQUEUE(rl, ent, t)

#define	RWSEMA_IDLE(rw)		((rw)->rw_count == 0)
#define	RWSEMA_RDOK(rw)		((rw)->rw_count >= 0)
#define	RWSEMA_RDBUSY(rw)	((rw)->rw_count > 0)
#define	RWSEMA_WRBUSY(rw)	((rw)->rw_count < 0)
#define	RWSEMA_WRBLOCKED(rw)	(!RWL_EMPTY(&(rw)->rw_wrwait))
#define	RWSEMA_RDBLOCKED(rw)	(!RWL_EMPTY(&(rw)->rw_rdwait))

#define	RWSEMA_SETIDLE(rw)	((rw)->rw_count = 0)
#define	RWSEMA_SETWRBUSY(rw)	((rw)->rw_count = -1)
#define	RWSEMA_NEWREADER(rw)	(++(rw)->rw_count)
#define	RWSEMA_DROPREADER(rw)	(--(rw)->rw_count)

#define	RWSEMA_SWP	's'	/* strong writer preference */
#define	RWSEMA_WWP	'w'	/* weak writer preference */

void	p_reader();
void	p_reader_v_lock();
bool_t	cp_reader();
void	v_reader();

void	p_writer();
void	p_writer_v_lock();
bool_t	cp_writer();
void	v_writer();
