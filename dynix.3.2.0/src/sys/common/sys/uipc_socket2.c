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

#ifndef	lint
static	char	rcsid[] = "$Header: uipc_socket2.c 2.15 91/02/25 $";
#endif

/*
 * upic_socket2.c
 *	Low level socket state and sockbuf management routines
 */

/* $Log:	uipc_socket2.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"

/*
 * Primitive routines for operating on sockets and socket buffers
 */

/*
 * Procedures to manipulate state flags of socket
 * and do appropriate wakeups.  Normal sequence from the
 * active (originating) side is that soisconnecting() is
 * called during processing of connect() call,
 * resulting in an eventual call to soisconnected() if/when the
 * connection is established.  When the connection is torn down
 * soisdisconnecting() is called during processing of disconnect() call,
 * and soisdisconnected() is called when the connection to the peer
 * is totally severed.  The semantics of these routines are such that
 * connectionless protocols can call soisconnected() and soisdisconnected()
 * only, bypassing the in-progress calls when setting up a ``connection''
 * takes no time.
 *
 * From the passive side, a socket is created with
 * two queues of sockets: so_q0 for connections in progress
 * and so_q for connections already made and awaiting user acceptance.
 * As a protocol is preparing incoming connections, it creates a socket
 * structure queued on so_q0 by calling sonewconn().  When the connection
 * is established, soisconnected() is called, and transfers the
 * socket structure to so_q, making it available to accept().
 * 
 * If a socket is closed with sockets on either
 * so_q0 or so_q, these sockets are dropped.
 *
 * If higher level protocols are implemented in
 * the kernel, the wakeups done here will sometimes
 * cause software-interrupt process scheduling.
 *
 * soisconnecting changes the state of the socket - mutex is via
 * so_lock.  The state change is signalled by v_all(connection_sema)
 *
 *	4.2 uses wakeup((caddr_t)&so->so_timeo);
 */

soisconnecting(so)
	register struct socket *so;
{
	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
	(void) vall_sema(so->so_conn_semap);
	return;
}

/*
 * soisconnected changes the state of the socket -  mutex is via
 * a socket_peer lock pointed at by so_sopp.  lock is assumed
 * to be held upon entry.
 *
 *  Full connection is signalled by v_all(connection_sema)
 *
 * 4.2 uses wakeup((caddr_t)&head->so_timeo);
 */

soisconnected(so)
	register struct socket *so;
{
	register struct socket *head;

	head = so->so_head;

	/*
	 * so is a *new* socket that is connected into the connect
	 * queue of its head.  The head socket is also locked.
	 */

	if (head) {

		/*
		 * the following is a panic rather than a sanity check
		 * because soqremque must be executed.
		 */

		if (soqremque(so, 0) == 0) {	/* so_lock *held* */
			panic("soisconnected");
		}

		soqinsque(head, so, 1);	/* so->so_lock held */
		sorwakeupall(head);
		(void) vall_sema(head->so_conn_semap);
	}

	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTED;

	(void) vall_sema(so->so_conn_semap);

	sorwakeupall(so);
	sowwakeupall(so);
	return;
}

/*
 * soisdisconnecting changes state, and
 * signals any process that was waiting on the connect_sema
 *
 * vall_sema replaces wakeup((caddr_t)&so->so_timeo);
 *
 * called with so_lock *held*
 */

soisdisconnecting(so)
	register struct socket *so;
{
	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);

	/*
	 * soisdisconnecting is only called from SPLNET
	 * (e.g. tcp_usrreq.tcp_disconnect) with the socket_peer locked
	 * through SOLOCK
	 */

	sowwakeupall(so);
		
	sorwakeupall(so);

	/*
	 * wakeup anyone waiting on the connection state of this
	 * socket to change. (e.g. receiver).
	 */

	vall_sema(so->so_conn_semap);

	return;
}

/*
 * soisdisconnected called with so_lock held. It changes state, and
 * signals any process that was waiting on the connect_sema
 * vall_sema replaces wakeup((caddr_t)&so->so_timeo);
 *
 * typically called by pr_input routines
 */

soisdisconnected(so)
	register struct socket *so;
{
	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);

	sowwakeupall(so);	/* wakeup everyone waiting on xmit queue */
	sorwakeupall(so);	/* wakeup everyone waiting on rcv queue */

	/*
	 * signal anyone waiting on connect sema - this for instance
	 * completes closing a "half-disconnected" socket.
	 */

	(void) vall_sema(so->so_conn_semap);
	return;
}

/*
 * When an attempt at a new connection is noted on a socket
 * which accepts connections, sonewconn is called.  If the
 * connection is possible (subject to space constraints, etc.)
 * then we allocate a new structure, properly linked into the
 * data structure of the original socket, and return this.
 *
 * accept semantics - called with head so_lock held
 */

struct socket *
sonewconn(head)
	register struct socket *head;
{
	register struct socket *so;
	register struct socket_peer * saved_sopp;

	so = (struct socket *)NULL;
	if (head->so_qlen + head->so_q0len > 3 * head->so_qlimit / 2) {
		return (so);	/* so == NULL => failure */
	}
	if ((so = soalloc()) == (struct socket *)NULL)
		return (so);	/* so == NULL => failure */
	/*
	 * N.B. that head->so_lock is *held* to avoid the rug being pulled
	 * out from under us; however so is not yet attached
	 */

	so->so_type = head->so_type;
	so->so_options = head->so_options &~ SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;

	/*
	 * ATTACH attaches the socket to whatever network address is
	 * appropriate.  For unix domain, this is a user socket.
	 *
	 * pr_attach routines generally require access to a protocol
	 * control block list (e.g. tcb).  If the head socket is locked,
	 * there is conflict with various timer routines which also lock
	 * the list then potentially the head socket.  Therefore unlock
	 * the head.  Note that this creates a WINDOW in which the user
	 * can abort the head socket.  The protocol engine is responsible
	 * for any dangling references created by this WINDOW.  Note,
	 * if the pr_input routine incremented the socket_peer's refcnt,
	 * the socket_peer is not deleted, so it can be
	 * used for the mutex.  Save it, unlock the head and let the pr_attach
	 * routines do their thing.
	 */

	saved_sopp = head->so_sopp;
	v_lock(&saved_sopp->sop_lock, SPLNET);   /* open WINDOW */
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
				    (struct mbuf *)NULL,
				    (struct mbuf *)NULL,
				    (struct mbuf *)NULL)) {
		/*
		 * if ATTACH fails, no pcb is attached and the socket
		 * can be released (NOFDREF).  No socket_peer either.
		 */

		sofree(so);
		(void) p_lock(&saved_sopp->sop_lock, SPLNET);
		return ((struct socket *)NULL);
	}

	/*
	 * protocol has finished its attach - check out head socket again.
	 * N.B. - could have been removed during the WINDOW so see if it still
	 * exists by checking the sop_refcnt.  It should be > 1.
	 * (If removed, it was decremented by the socket management
	 * and the protocol management routines while locked.)
	 *
	 * also can be in the process of being removed so need to
	 * check to see if it is still ACCEPTCONN
	 */

	(void) SOLOCK(so);	/* lock new socket for insque */
	(void) p_lock(&saved_sopp->sop_lock, SPLNET);

	/*
	 * head is disappearing if sohead refcnt-- during window 
	 * refcnt went from 3->1 (still ref'd by whoever called us)
	 */

	if ((saved_sopp->sop_refcnt <= 1) ||
		!(head->so_options & SO_ACCEPTCONN)) {

		/*
		 * head is disappearing forget so reference
		 * pr_detach releases tcp resources,
		 * completion of soclose(head) releases so and sop.
		 */

		so->so_sopp->sop_refcnt--;
		v_lock(&saved_sopp->sop_lock, SPLNET); /* avoid deadlock */

		/*
		 * remove pr_usrreq resources by DETACHing
		 */

		(void) (*so->so_proto->pr_usrreq)
			(so, PRU_DETACH, (struct mbuf *)NULL,
			    (struct mbuf *)NULL, (struct mbuf *)NULL);

		so = (struct socket *)NULL;	/* indicates failure */
		(void) p_lock(&saved_sopp->sop_lock, SPLNET);

	} else 	/* new socket created and head is ok */

		soqinsque(head, so, 0);

	/*
	 * returns with head locked and if everything went OK the new so is
	 * also locked.  If new so was not completely created, so = NULL and 
	 * resources are returned.  head->so_sopp->sop_lock is locked
	 * regardless.
	 */

	return (so);
}

/* 
 * soqinsque requires that the head->so_lock is *held* 
 * and also so->so_lock which is the inqueuee 
 *
 * called with both so_lock's held
 */

soqinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{
	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_q0 = head->so_q0;
		head->so_q0 = so;
	} else {
		head->so_qlen++;
		so->so_q = head->so_q;
		head->so_q = so;
	}
	return;
}

/*
 * soqremque assumes that both so_head and so_lock are *held*
 */

soqremque(so, q)
	register struct socket *so;
	int q;
{
	register struct socket *head, *prev, *next;

	head = so->so_head;
	prev = head;
	for (;;) {
		next = q ? prev->so_q : prev->so_q0;
		if (next == so)
			break;
		if (next == head)
			return (0);
		prev = next;
	}
	if (q == 0) {
		prev->so_q0 = next->so_q0;
		head->so_q0len--;
	} else {
		prev->so_q = next->so_q;
		head->so_qlen--;
	}
	next->so_q0 = next->so_q = (struct socket *)NULL;
	next->so_head = (struct socket *)NULL;
	return (1);
}

/*
 * Socantsendmore indicates that no more data will be sent on the
 * socket; it would normally be applied to a socket when the user
 * informs the system that no more data is to be sent, by the protocol
 * code (in case PRU_SHUTDOWN).  Socantrcvmore indicates that no more data
 * will be received, and will normally be applied to the socket by a
 * protocol when it detects that the peer will send no more data.
 * Data queued for reading in the socket may yet be read.
 *	N.B. so_lock is held to bit fiddle with so_state.
 */

socantsendmore(so)
	struct socket *so;
{
	so->so_state |= SS_CANTSENDMORE;
	sowwakeupall(so);
	return;
}

/*
 * socantrcvmore() called when receive side is closed down
 *
 * called with so_lock held
 */

socantrcvmore(so)
	struct socket *so;
{
	so->so_state |= SS_CANTRCVMORE;
	sorwakeupall(so);
	return;
}

/*
 * Socket select/wakeup routines.
 */

/*
 * sbselqueue() queue a process for a select on a socket buffer.
 *
 * called with so_lock held
 */

sbselqueue(sb)
	struct sockbuf *sb;
{
	spl_t splevel;

	if (sb->sb_sel) {

		/*
		 * If the current process is already known to be selecting,
		 * then nothing more to do. This can happen if sb_sel
		 * contains stale data from a previous call to select from
		 * this process. This avoids false collisions.
		 */

		if (sb->sb_sel == u.u_procp)
			return;

		/*
		 * If 1st selector is waiting on selwait or
		 * has not yet blocked on selwait, then we
		 * have a collision. It is sufficient to lock
		 * only the select_lck since selwakeup cannot
		 * happen without also locking select_lck.
		 */

		splevel = p_lock(&select_lck, SPL6);
		if (sb->sb_sel->p_wchan == &selwait ||
		   (sb->sb_sel->p_flag & SSEL))
			sb->sb_flags |= SB_COLL;
		else
			sb->sb_sel = u.u_procp;
		v_lock(&select_lck, splevel);

	} else
		sb->sb_sel = u.u_procp;
}

/*
 * Wait for data to arrive at/drain from a socket buffer.
 *
 * called with so_lock and SPLNET
 * When awakened sbwait() reacquires so_lock.
 */

sbwait(sb)		
	struct sockbuf *sb;
{
	struct socket *so;

	so = (mtod(dtom(sb), struct socket *));

	/* always called from SPLNET */

	p_sema_v_lock(&sb->sb_sbx->sbx_buf_wait, PZERO+1, /* KILLABLE */
			&(so->so_sopp->sop_lock), SPLNET);

	(void) p_lock(&(so->so_sopp->sop_lock), SPLNET);
}

/*
 * Wakeup processes waiting on a socket buffer.
 *
 * called with so_lock held
 */

sbwakeup(sb, all)
	register struct sockbuf *sb;
	bool_t	all;
{

	if (sb->sb_sel) {
		selwakeup(sb->sb_sel, sb->sb_flags & SB_COLL);
		sb->sb_flags &= ~SB_COLL;
		sb->sb_sel = (struct proc *)NULL;
	}

	if (all)
		(void) vall_sema(&sb->sb_sbx->sbx_buf_wait);
	else
		(void) cv_sema(&sb->sb_sbx->sbx_buf_wait);
}

/*
 * Wakeup processes waiting on a socket buffer, and send signal if required.
 *
 * called with so_lock held
 */

sowakeup(so, sb, all)
	register struct socket *so;
	struct sockbuf *sb;
	bool_t all;
{
	sbwakeup(sb, all);

	if (so->so_pgrp == 0)
		return;
	if (so->so_pgrp > 0)
		gsignal(so->so_pgrp, SIGIO);
	else {
		/*
		 * pfind returns a pointer to a *locked* proc struct
		 */
		struct proc *p = pfind(-so->so_pgrp);

		if (p) {
			lpsignal(p, SIGIO);
			v_lock(&p->p_state, SPLNET);
		}
	}
}

/*
 * Socket buffer (struct sockbuf) utility routines.
 *
 * Each socket contains two socket buffers: one for sending data and
 * one for receiving data.  Each buffer contains a queue of mbufs,
 * information about the number of mbufs and amount of data in the
 * queue, and other fields allowing select() statements and notification
 * on data availability to be implemented.
 *
 * Before using a new socket structure it is first necessary to reserve
 * buffer space to the socket, by calling sbreserve.  This commits
 * some of the available buffer space in the system buffer pool for the
 * socket.  The space should be released by calling sbrelease when the
 * socket is destroyed.
 *
 * The routine sbappend() is normally called to append new mbufs
 * to a socket buffer, after checking that adequate space is available
 * comparing the function sbspace() with the amount of data to be added.
 * Data is normally removed from a socket buffer in a protocol by
 * first calling m_copy on the socket buffer mbuf chain and sending this
 * to a peer, and then removing the data from the socket buffer with
 * sbdrop when the data is acknowledged by the peer (or immediately
 * in the case of unreliable protocols.)
 *
 * Protocols which do not require connections place both source address
 * and data information in socket buffer queues.  The source addresses
 * are stored in single mbufs after each data item, and are easily found
 * as the data items are all marked with end of record markers.  The
 * sbappendaddr() routine stores a datum and associated address in
 * a socket buffer.  Note that, unlike sbappend(), this routine checks
 * for the caller that there will be enough space to store the data.
 * It fails if there is not enough space, or if it cannot find
 * a mbuf to store the address in.
 *
 * The higher-level routines sosend and soreceive (in socket.c)
 * also add data to, and remove data from socket buffers respectively.
 *
 * the socket extension structure contains a pointer to the tail (last
 * mbuf in the list to facilitate append.  The m_act field is used to
 * link separate "records".
 *
 * called with so_lock held
 */

soreserve(so, sndcc, rcvcc)
	register struct socket *so;
	int sndcc, rcvcc;
{
	if (sbreserve(&so->so_snd, sndcc) == 0)
		return (ENOBUFS);
	if (sbreserve(&so->so_rcv, rcvcc) == 0) {
		sbrelease(&so->so_snd);
		return (ENOBUFS);
	}
	return (0);
}

/*
 * Allot mbufs to a sockbuf.
 * Attempt to scale cc so that mbcnt doesn't become limiting
 * if buffering efficiency is near the normal case.
 */

sbreserve(sb, cc)
	struct sockbuf *sb;
{
        if (cc > (u_long)SB_MAX * CLBYTES / (2 * MSIZE + CLBYTES))
		return (0);
	sb->sb_hiwat = cc;
	sb->sb_mbmax = MIN((cc * 2), SB_MAX);
	return (1);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */

sbrelease(sb)	/* called with so_lock held */
	struct sockbuf *sb;
{
	sbflush(sb);
	sb->sb_hiwat = sb->sb_mbmax = 0;
	return;
}

/*
 * Routines to add (at the end) and remove (from the beginning)
 * data from a mbuf queue.
 */
struct mbuf *
sbcompress(sb, m, n)
	register struct sockbuf *sb;
	register struct mbuf *m, *n;
{
	while (m) {		/* process appendee mbuf chain */

		if (m->m_len == 0) {	/* release empty buffers */
			m = m_free(m);
			continue;
		}
		if (n && n->m_off <= MMAXOFF && /* m->m_off <= MMAXOFF && */
		   (n->m_off + n->m_len + m->m_len) <= MMAXOFF &&
		   n->m_type == m->m_type) {

			/*
			 * can condense whole m's data into tail of n mbuf.
			 * note: can't copy into clusters since they might be 
			 * referenced; but copy out of clusters since that
			 * might free up the critical resource.
			 */

			bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
			    (unsigned)m->m_len);

			n->m_len += m->m_len;
			sb->sb_cc += m->m_len;
			m = m_free(m);
			continue;
		}

		/*
		 * append the non-empty mbuf m
		 */

		sballoc(sb, m);

		if (n == (struct mbuf *)NULL)
			sb->sb_mb = m;		/* first mbuf */
		else
			n->m_next = m;		/* append to tail */
		n = m;				/* new tail */
		m = m->m_next;
	}

	if (n)
		n->m_next = (struct mbuf *)NULL;	/* end of chain */
	return (n);
}


/*
 * Append mbuf queue m to sockbuf sb.
 *
 * called with so_lock held
 */

sbappend(sb, m)
	register struct mbuf *m;
	register struct sockbuf *sb;
{
	register struct mbuf *n;

	/*
	 * if there is a chain, there is a tail to the chain.
	 */

	if(n = sb->sb_mb)
		n = sb->sb_sbx->sx_tail;

	sb->sb_sbx->sx_tail = sbcompress(sb, m, n);
	return;
}

/*
 * Append data and address.
 * Return 0 if no space in sockbuf or if
 * can't get mbuf to stuff address in.
 *
 * called with so_lock held
 */

sbappendaddr(sb, asa, m0, rights0)
	struct sockbuf	*sb;
	struct sockaddr	*asa;
	struct mbuf *m0, *rights0;
{
	register struct mbuf *m;
	register int len = sizeof(struct sockaddr);
	register struct mbuf *rights;

	/*
	 * traverse new chain once - compute len for sbspace()
	 * assign m to tail of this chain and prealloc using tempbuf
	 * note: this algorithm does not condense the chain and mfree
	 * empty mbufs, etc. like sbappend() - in order to do so, would
	 * have to keep track of m0 and end of chain.
	 */

        for (m = m0; m; m = m->m_next)	/* note m can == NULL */
		len += m->m_len;
	if (rights0)
		len += rights0->m_len;
	if (len > sbspace(sb))
		return (0);

	/*
	 * N.B. M_DONTWAIT argument is not used (also not used in 4.2)
	 */

	m = m_get(M_DONTWAIT, MT_SONAME);
	if (m == (struct mbuf *)NULL)
		return (0);

	m->m_len = sizeof(struct sockaddr);
	m->m_act = (struct mbuf*) NULL;

	*mtod(m, struct sockaddr *) = *asa;	/* note structure copy */

	if (rights0 && rights0->m_len) {
		rights = m_copy(rights0, 0, rights0->m_len);
		if (rights == (struct mbuf *) NULL) {
			(void) m_freem(m);
			return (0);
		}
		m->m_next = rights;
		sballoc(sb, rights);	/* account for rights */
	}
	sballoc(sb, m);			/* account for asa */

	/*
	 * if there is a tail - link its m_act to new chain
	 */

	if (sb->sb_mb)
		sb->sb_sbx->sx_tail->m_act = m; /* link new chain to tail */
	else
		sb->sb_mb = m;

	sb->sb_sbx->sx_tail = m;	 /* remember new tail for next time */
	m->m_act = (struct mbuf*) NULL;
	if (m0) {
		if (m->m_next)		/* rights */
			m = m->m_next;
		(void) sbcompress(sb, m0, m);
	}
	
	return (1);
}

/*
 * Free all mbufs on a sockbuf mbuf chain.
 * Check that resource allocations return to 0.
 * this is for both snd and rcv socket buffers
 *
 * called with so_lock held
 */

sbflush(sb)
	register struct sockbuf *sb;
{

	if (sb->sb_cc) 
		sbdrop(sb, (int) sb->sb_cc);	/* sb_lock *held* */

	/*
	 * a little ugly, but check for residual zero length data
	 * buffer.  This can occur if a zero-length data gram is left
	 * in the queue since sbdrop() does not drop the trailing
	 * zero length data buffer.
	 */

	while (sb->sb_mb && sb->sb_mb->m_len == 0) {
		sbfree(sb, sb->sb_mb);
		sb->sb_mb = m_free(sb->sb_mb);	/* should be at most *two* */
	}
	ASSERT((sb->sb_cc == 0 && sb->sb_mbcnt == 0 && sb->sb_mb == 0),
		"sbflush 2");
}

/*
 * Drop data from (the front of) a sockbuf chain.
 * requires so_lock be held
 *
 * called with so_lock held
 */

sbdrop(sb, len)
	register struct sockbuf *sb;
	register int len;
{
	register struct mbuf *m, *mn;
	struct mbuf *next;

	next = (m = sb->sb_mb) ? m->m_act : (struct mbuf*) NULL;
	while (len > 0) {
		if (m == (struct mbuf*) NULL) {
			ASSERT(next != (struct mbuf *)NULL, "sbdrop");
			m = next;
			next = m->m_act;
			continue;
		}
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_off += len;
			sb->sb_cc -= len;
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	while (m && m->m_len == 0) {
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	if (m) {
		sb->sb_mb = m;
		m->m_act = next;
	} else
		sb->sb_mb = next;
}

#ifdef notyet
/*
 * Drop a record off the front of a sockbuf
 * and move the next record to the front.
 */

sbdroprecord(sb)
	register struct sockbuf *sb;
{
	register struct mbuf *m, *mn;

	/*
	 * this could be done more efficiently by traversing
	 * the list once to sbfree - then m_freem() the whole
	 * record.  E.g.
	 *
	 * mn = sb->sb_mb;
	 * for(m = mn; m; m = m->m_next)
	 *	sbfree(sb, m);
	 * sb->sb_mb = mn->m_act;
	 * m_freem(mn);
	 */

	m = sb->sb_mb;
	if (m) {
		sb->sb_mb = m->m_act;
		do {
			sbfree(sb, m);
			MFREE(m, mn);
		} while (m = mn);
	}
}
#endif notyet

#ifdef IPCTRACE

/*
 * debug subroutine to check for agreement of sb_cc and sum of
 * mbuf.m_len - generally not compiled in for production code.
 */

checksb_cc(sb)
	register struct sockbuf * sb;
{
	register struct mbuf * m;
	register int len = 0;

	m = sb->sb_mb;
	while (m) {
		len += m->m_len;
		m = m->m_next;
	}
	if (len == sb->sb_cc)
		return (0);	/* OK */
	else
		return (1);	/* sb_cc != sum of m_len's */
}
#endif IPCTRACE
