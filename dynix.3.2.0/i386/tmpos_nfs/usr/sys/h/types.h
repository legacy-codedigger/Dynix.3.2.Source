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

/*
 * $Header: types.h 2.23 1991/05/22 18:09:56 $
 *
 * types.h
 *	Basic system types and major/minor device constructing/busting macros.
 */

/* $Log: types.h,v $
 *
 *
 */
#ifndef	_TYPES_
#define _TYPES_

#ifdef KERNEL
#include "../machine/machtypes.h"
#else
#include <machine/machtypes.h>
#endif

/* major part of a device */
#define	major(x)	((int)(((unsigned)(x)>>16)&0xFFFF))

/* minor part of a device */
#define	minor(x)	((int)((x)&0xFFFF))

/* make a device number */
#define	makedev(x,y)	((dev_t)(((x)<<16) | (y)))

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* sys III compat */
typedef	unsigned long	ulong;		/* sys V compat */

typedef	struct	_quad { long val[2]; } quad;
typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	u_long	ino_t;
typedef	long	swblk_t;
#ifdef  _SIZE_T_
typedef	_SIZE_T_	size_t;
#undef  _SIZE_T_
#endif
#ifdef  _TIME_T_
typedef	_TIME_T_	time_t;
#undef	_TIME_T_
#endif
typedef	long	dev_t;
typedef	int	off_t;
typedef	int	bool_t;
typedef	int	spl_t;
typedef long	key_t;		/* used for system V ipc */
typedef u_short	uid_t;
typedef u_short gid_t;

/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user.
 */

#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

typedef	long	fd_mask;
#define	NFDBITS	(sizeof(fd_mask) * 8)		/* bits per mask */
#ifndef	howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#define	FD_SET_SZ(n)	(howmany((n), NFDBITS) * sizeof(fd_mask))

/*
 * For mutex data-structures.
 *
 * For ease of use for user-level code, we declare lock and sema struct's
 * here.  More detail is in h/mutex.h and machine/mutex.h.
 */

typedef	unsigned char	gate_t;

#ifdef	ns32000
typedef	struct	{
	gate_t		l_gate;
	char		l_state;
} lock_t;
#endif	/* ns32000 */

#ifdef	i386
typedef	unsigned char	lock_t;
#endif	/* i386 */

#ifdef	KERNEL
typedef	struct	{
	struct	proc	*s_head;
	struct	proc	*s_tail;
	short		s_count;
	gate_t		s_gate;
	char		s_flags;
} sema_t;
#else
/*
 * Version for lint and other user needs.
 */
typedef	struct	{
	int		*s_head;
	int		*s_tail;
	short		s_count;
	gate_t		s_gate;
	char		s_flags;
} sema_t;
#endif	/* KERNEL */

/*
 * Multiple "reader" / Single "writer" semaphores.  Can also be viewed as
 * shared/exclusive.
 *
 * Selectable policy: Strong Writer Preference (SWP) blocks new readers if a
 * waiting writer exists.  Weak Writer Preference (WWP) always allows new
 * readers if no writers queued, but v_writer() wakes writers ahead of readers.
 */

#ifdef	KERNEL
#include "../h/llist.h"
#else	/* !KERNEL */
typedef	caddr_t	slink_t;
#endif	/* KERNEL */

typedef	struct	rwsema	{	/* reader/writer semaphore */
	gate_t	rw_mutex;	/* overall mutex */
	char	rw_policy;	/* strong/weak writer preference */
	short	rw_count;	/* -1 = busy writer, N>0 = # readers */
	slink_t	rw_wrwait;	/* queued writers */
	slink_t	rw_rdwait;	/* queued readers */
} rwsema_t;

#endif	/* _TYPES_ */
