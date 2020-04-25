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
#ifndef _SYS_SOCKETVAR_INCLUDED
#define _SYS_SOCKETVAR_INCLUDED

/*
 * $Header: socketvar.h 2.10 91/02/25 $
 */

/* $Log:	socketvar.h,v $
 */

/*
 * socket_peer contains a lock and refcnt to be bound to either a
 * local sharing of a socket pair or the sharing of protocol control
 * blocks with a network.
 */

struct socket_peer {
	lock_t	sop_lock;	/* lock for peer sockets */
	short	sop_refcnt;	/* reference count for peer lock */
	caddr_t sop_locker;	/* address of locking routine */
};

/*
 * Kernel structure per socket.
 * Contains send and receive semaphores.
 * Linked to socket structure via m_next pointer to facilitate release.
 * Formerly contained in socketext.h.
 */

struct socket_extension {
	sema_t 	sx_connect_sema;	/* semaphore for connection queue */
	struct	sx_bufext {
		sema_t	sbx_buf_sema;	/* semaphore for sblock/sbunlock */
		sema_t	sbx_buf_wait;	/* semaphore for sbwait/sbwakeup */
		struct mbuf* sx_tail;
		u_long	Sb_lowat;	/* low water mark (not used yet) */
		u_long	Sb_timeo;	/* timeout (not used yet) */
		struct	proc *Sb_sel;	/* process selecting read/write */
		short	Sb_flags;	/* flags, see below */
	} sx_rcv, sx_snd;
};

#define sb_lowat	sb_sbx->Sb_lowat
#define sb_timeo	sb_sbx->Sb_timeo
#define sb_sel		sb_sbx->Sb_sel
#define sb_flags	sb_sbx->Sb_flags

#define	soextensionof(so) \
		mtod(((dtom((so)))->m_next), struct socket_extension *)

/*
 * Kernel structure per socket.
 * Contains send and receive buffer queues,
 * handle on protocol and pointer to protocol
 * private data and error information.
 * also contains so_lock and a pointer to a so_lock
 * used to mutex structure
 */

struct socket {	/* N.B. must fit into 112 byte mbuf!! */
	lock_t	so_lock;		/* lock structure for mutex on so */
	struct socket_peer * so_sopp;	/* pointer to socket peer */
	sema_t* so_conn_semap;		/* pointer to connection sema */
	short	so_type;		/* generic type, see socket.h */
	short	so_options;		/* from socket call, see socket.h */
	short	so_linger;		/* time to linger while closing */
	short	so_state;		/* internal state flags SS_*, below */
	caddr_t	so_pcb;			/* protocol control block */
	struct	protosw *so_proto;	/* protocol handle */

/*
 * Variables for connection queuing.
 * Socket where accepts occur is so_head in all subsidiary sockets.
 * If so_head is 0, socket is not related to an accept.
 * For head socket so_q0 queues partially completed connections,
 * while so_q is a queue of connections ready to be accepted.
 * If a connection is aborted and it has so_head set, then
 * it has to be pulled out of either so_q0 or so_q.
 * We allow connections to queue up based on current queue lengths
 * and limit on number of queued connections for this socket.
 */
	struct	socket *so_head;	/* back pointer to accept socket */
	struct	socket *so_q0;		/* queue of partial connections */
	short	so_q0len;		/* partials on so_q0 */
	struct	socket *so_q;		/* queue of incoming connections */
	short	so_qlen;		/* number of connections on so_q */
	short	so_qlimit;		/* max number queued connections */
/*
 * Variables for socket buffering.
 */
	struct	sockbuf {
		u_long	sb_cc;		/* actual chars in buffer */
		u_long	sb_hiwat;	/* max actual char count */
		u_long	sb_mbcnt;	/* chars of mbufs used */
		u_long	sb_mbmax;	/* max chars of mbufs to use */
		struct	mbuf *sb_mb;	/* the mbuf chain */
		struct	sx_bufext *sb_sbx;	/* pointer to sb's semas */
	} so_rcv, so_snd;
	short	so_timeo;		/* connection timeout */
	u_short	so_error;		/* error affecting connection */
	u_short	so_oobmark;		/* chars to oob mark */
	short	so_pgrp;		/* pgrp for signals */
};

#define SB_MAX          (64*1024)       /* max chars in sockbuf */
#define	SB_SEL		0x01		/* buffer is selected */
#define	SB_COLL		0x02		/* collision selecting */

/*
 * Socket state bits.
 */

#define	SS_NOFDREF		0x001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTING	0x008	/* in process of disconnecting */
#define	SS_CANTSENDMORE		0x010	/* can't send more data to peer */
#define	SS_CANTRCVMORE		0x020	/* can't receive more data from peer */
#define	SS_RCVATMARK		0x040	/* at mark on input */

#define	SS_PRIV			0x080	/* privileged for broadcast, raw... */
#define	SS_NBIO			0x100	/* non-blocking ops */
#define	SS_ASYNC		0x200	/* async i/o notify */

/*
 * Macros for sockets and socket buffering.
 */

/*
 * how much space is there in a socket buffer (so->so_snd or so->so_rcv)
 */

#define	sbspace(sb) \
    (MIN((int)((sb)->sb_hiwat - (sb)->sb_cc),\
	 (int)((sb)->sb_mbmax - (sb)->sb_mbcnt)))

/*
 * do we have to send all at once on a socket?
 */

#define	sosendallatonce(so) \
    ((so)->so_proto->pr_flags & PR_ATOMIC)

/*
 * can we read something from so?
 */

#define	soreadable(so) \
    ((so)->so_rcv.sb_cc || ((so)->so_state & SS_CANTRCVMORE) || \
        (so)->so_qlen || (so)->so_error)


/*
 * can we write something to so?
 */

#define	sowriteable(so) \
    (sbspace(&(so)->so_snd) > 0 && \
        (((so)->so_state&SS_ISCONNECTED) || \
          ((so)->so_proto->pr_flags&PR_CONNREQUIRED)==0) || \
     ((so)->so_state & SS_CANTSENDMORE) || \
     (so)->so_error)


/*
 * adjust counters in sb reflecting allocation of m
 */

#define	sballoc(sb, m) { \
	(sb)->sb_cc += (m)->m_len; \
	(sb)->sb_mbcnt += MSIZE; \
	if ((m)->m_off > MMAXOFF) \
		(sb)->sb_mbcnt += MCLBYTES; \
}

/*
 * adjust counters in sb reflecting freeing of m
 */

#define	sbfree(sb, m) { \
	(sb)->sb_cc -= (m)->m_len; \
	(sb)->sb_mbcnt -= MSIZE; \
	if ((m)->m_off > MMAXOFF) \
		(sb)->sb_mbcnt -= MCLBYTES; \
}

/*
 * Set lock (actually get the semaphore) on socket sb.
 * Assumes that the socket is locked.
 * Invoked from kernel system calls to lock an so_snd or so_rcv sb.
 * Also invoked by sorflush when socket is being shutdown and sb being flushed.
 *
 * sbunlock is the countermacro to sblock.
 * Assumes that the socket is locked.
 * Release lock (semaphore) on snd or rcv sockbuf sb.
 */

#define sblock(sb) { \
		if (sema_avail(&(sb)->sb_sbx->sbx_buf_sema)) \
			sema_count(&(sb)->sb_sbx->sbx_buf_sema) = 0; \
		else { \
			struct socket *sbso; \
			sbso = mtod(dtom((sb)), struct socket *); \
			p_sema_v_lock(&(sb)->sb_sbx->sbx_buf_sema, PZERO+1, \
				&(sbso->so_sopp->sop_lock), SPLNET); \
			(void) p_lock(&(sbso->so_sopp->sop_lock), SPLNET); \
		} \
	}

#define	sbunlock(sb) { \
		if (blocked_sema(&(sb)->sb_sbx->sbx_buf_sema)) \
			v_sema(&(sb)->sb_sbx->sbx_buf_sema); \
		else \
			sema_count(&(sb)->sb_sbx->sbx_buf_sema) = 1; \
	}

/*
 * sowakeup now has additional argument. This is a bool_t flag to indicate
 * whether to wakeup all waiters.
 */

#define	sorwakeup(so)		sowakeup(so, &(so)->so_rcv, 0)
#define	sowwakeup(so)		sowakeup(so, &(so)->so_snd, 0)

#define	sorwakeupall(so)	sowakeup(so, &(so)->so_rcv, 1)
#define	sowwakeupall(so)	sowakeup(so, &(so)->so_snd, 1)

/*
 * The spl level at which a socket lock is held is a performance issue.
 * Note that the spl only prevents lower priority interrupts
 * from gaining the current processor - it does not keep them
 * from executing on a *different* processor.  Thus it is
 * possible for a lower priority process to spin while the lock
 * is held.  This should not occur often, and in fact usually the
 * situation is expected to be favorable such that a different
 * process is on a different processor accessing a different
 * socket for a different user.
 *
 * Note that the appropriate lock for this socket can be other
 * than the so_lock in this structure - thus the so_sopp pointer.
 *
 * so_sopp points to this socket's socket_peer which contains the
 * necessary lock and reference count to mutex the socket in the
 * domain/protocol engines.
 */

#define	SOLOCK(so)		p_lock(&((so)->so_sopp->sop_lock), SPLNET)
#define	SOUNLOCK(so, splevel)	v_lock(&((so)->so_sopp->sop_lock), (splevel))

#ifdef KERNEL
struct socket *sonewconn();
struct socket *soalloc();
extern somaxcon;
#endif
#endif	/* _SYS_SOCKETVAR_INCLUDED */
