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
static	char	rcsid[] = "$Header: uipc_socket.c 2.32 91/03/11 $";
#endif

#define DODROP			/* for analysis of dropped packets */
#undef DODROP			/* for NO analysis of dropped packets */

#ifdef DODROP			/* defined in ../netinet/tcp_input.c */
extern short tcpdrops[64];	/* keep stats on drop codes */
#define DROPIT(x) tcpdrops[(x)]++;
#else
#define DROPIT(x) {;}
#endif DODROP

/*
 * uipc_socket.c - Socket management routines
 */

/* $Log:	uipc_socket.c,v $
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
#include "../h/un.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../net/route.h"
#include "../net/if.h"
#include "../netinet/in.h"

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO: (sic 4.2)
 *	sostat
 *	PR_RIGHTS
 *	clean up select, async
 *	out-of-band is a kludge
 */

/*
 * socreate() is called by the socket(domain, socktype, protocol)
 * system call.  Its function is to acquire the resources required
 * for a socket data structure.  The socket data structure is
 * associated with a u.area the same way that file descriptors
 * are associated.  socreate calls the Unix file system functions
 * to acquire file system items.  Kernel functions called are:
 * init_lock(), and init_sema()
 *
 * socreate() returns the value of a pointer (aso) to the address of the
 * socket structure that socreate sets up.  This is the
 * f_data pointer in a struct file setup by the socket system call.
 */

/*ARGSUSED*/
socreate(dom, aso, type, proto)
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register int error;

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);

	if (prp == (struct protosw *)NULL)
		return (EPROTONOSUPPORT);

	if (prp->pr_type != type)
		return (EPROTOTYPE);

	if ((so = soalloc()) == (struct socket *)NULL)
		return (ENOBUFS);

	/*
	 * Note that the socket structure has not yet been linked into the
	 * struct file.  This means that if the struct file goes away
	 * we're in trouble.  The struct socket should have no concurrent
	 * access on it at this time since it is not really active.
	 */

	so->so_options = 0;
	so->so_state = 0;	/* note that state is literally 0 */
	so->so_type = type;
	so->so_proto = prp;
	if (u.u_uid == 0)
		so->so_state = SS_PRIV;

	/*
	 * pr_usrreq is the major protocol engine handle.  Here the
	 * "ATTACH" handle is called which generally attaches a protocol
	 * control block and a socket_peer (refcnt >= 2) to the socket.
	 * pr_usrreq is always called with three mbuf arguments,
	 * but ATTACH has no need for them
	 */

	if (prp->pr_usrreq) {
		error = (*prp->pr_usrreq)(so, PRU_ATTACH,
					(struct mbuf *)NULL,
					(struct mbuf *)NULL,
					(struct mbuf *)NULL);
	} else 
		error = ENOPROTOOPT;

	if (error) { 	/* something wrong with attach */

		/* 
		 * so_state shouldn't be concurrently accessed here since
		 * something has gone wrong in attaching the protocol control
		 * block. sofree() essentially frees the resources assigned
		 * to the socket.
		 * The NOFDREF indication is for the benefit of sofree so it
		 * knows that the socket has no file system reference and so
		 * resources can be released.
		 */

		so->so_state |= SS_NOFDREF;
		sofree(so);		/* socket is GONE */
		return (error);

	} else 	/* no error */

		/*
		 * aso references a location in the callers
		 * stack which is placed into the
		 * struct file by the socket syscall
		 */

		*aso = so;

	return (error);
}

sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int error;
	spl_t splevel;

	splevel = SOLOCK(so);

	error = (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);

	SOUNLOCK(so, splevel);
	return (error);
}

solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	spl_t splevel;
	int error = 0;

	splevel = SOLOCK(so);
	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);

	if (!error) {
		if (so->so_q == 0) {
			so->so_q = so;
			so->so_q0 = so;
			so->so_options |= SO_ACCEPTCONN;
		}
		if (backlog < 0)
			backlog = 0;
		so->so_qlimit = MIN(backlog, somaxcon);
	}
	SOUNLOCK(so, splevel);
	return (error);
}

/*
 * sofree(so) - free a socket structure and its resources
 *
 * called with so_lock *held*
 */

sofree(so)
	register struct socket *so;
{
	if (so->so_head) {
		DROPIT(42);
		if (!soqremque(so, 0)) {

			/*
			 * indicates that so is not in so_q0 of
			 * so_head, so must be in so_q.  This is a
			 * partially accepted socket waiting for
			 * acceptance.  Let accept or soclose of
			 * parent get rid of this socket.
			 */

			DROPIT(41);
			return;
		}
		so->so_head = (struct socket *)NULL;
	}

	/*
	 * if there is still a file system reference or a pcb, then do not
	 * release the resources - simply return.
	 * If there is a pcb attached - there is still a binding, so
	 * wait till termination of binding to release resources.
	 *
	 * These resources are released when the socket structure is
	 * freed (e.g. but not always soclose).
	 * The socket and socket_extension are freed as an mbuf chain.
	 *
	 * nothing else should happen to this socket
	 */

	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;

	sbrelease(&so->so_snd);
	sorflush(so);

	/*
	 *  m_freem frees mbuf chain which includes socket_extension
	 */

	(void) m_freem(dtom(so));
	return;
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */

soclose(so)
	register struct socket *so;
{
	int error;
	register struct socket_peer * sop;
	spl_t splevel;
	struct socket * partial_so;
	splevel = SOLOCK(so);

	/*
	 * A socket_peer is currently indicated by being a pointer
	 * to something other than the current struct socket so_lock.
	 */

	sop = (struct socket_peer *)so->so_sopp;

	/*
	 * if this is a listen socket, then abort any partial
	 * connections.  This is a little tricky since these
	 * partials represent references to this socket.  Also
	 * they need to lock this socket in order to signal completion
	 * of the connection.  They need to be locked in order to
	 * be aborted thus there is deadlock potential.  To avoid
	 * deadlock - mark the socket as no longer accepting
	 * connections so attempts to do so fail. 
	 * Abort the list of queued connections.  Note, if no
	 * longer accepting connections, then no more connections
	 * will be queued or dequeued.
	 */

	if (so->so_options & SO_ACCEPTCONN) {

		/*
		 * no longer accept connections (or aborts of pending
		 * connections).  This means no further so_q* action.
		 * 
		 * unlock socket to avoid deadlock during DETACH
		 * then relock socket for rest of soclose
		 * 
		 * soabort manages refcnt and releases the so and sop
		 */

		so->so_options &= ~SO_ACCEPTCONN;
		SOUNLOCK(so, SPLNET);
		while (so->so_q0 != so) {
			(void) SOLOCK(so->so_q0);
			partial_so = so->so_q0;
			(void) soqremque(partial_so, 0);
			(void) soabort(partial_so);
		}
		while (so->so_q != so) {
			(void) SOLOCK(so->so_q);
			partial_so = so->so_q;
			(void) soqremque(partial_so, 1);
			(void) soabort(partial_so);
		}
		(void) SOLOCK(so);
	}
	if (so->so_pcb == (caddr_t)NULL) {
		goto discard;		/* lock held */
	}
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so, (struct mbuf *)NULL);
			if (error)
				goto drop; /* lock held */
		}

		/* lock held */

		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) &&
			    (so->so_state & SS_NBIO)) {
				goto drop;	/*lock held */
			}

			/*
			 * so_lock is held checking for so_state change
			 * that indicates socket is no longer connected.
			 * Continue after waking up to
			 * check for the socket not being ISCONNECTED.
			 */

			while (so->so_state & SS_ISCONNECTED) {

				/*
				 * p_sema waiting for the peer to complete
				 * disconnection - note that this cannot be
				 * signalled out until the disconnection is
				 * completed.  For distributed applications
				 * and large networks this can be a
				 * considerable time - e.g. TCP timeout is
				 * about 2 minutes.  This means that if an
				 * attempt is made to signal a process
				 * (e.g. KILL) with a connected socket to a
				 * distant node, it can take a long time to
				 * clean up (e.g. if the peer died before
				 * closing.
				 *
				 * v_lock to allow user in to complete
				 * disconnect 
				 */

				p_sema_v_lock(so->so_conn_semap, PZERO,
						&(sop->sop_lock), SPLNET);

				(void) SOLOCK(so); /* always SPLNET */
			}

		/*
		 * lock held when !ISCONNECTED discovered
		 *
		 * if the socket is such that we should wait around for all
		 * references to be fully disconnected (LINGER) we have done
		 * so - in the case of pipes -> NOLINGER.
		 */

		}
	}
drop:
	/*
	 * so_lock is *held*
	 * original splevel is in splevel
	 * socket is completely disconnected and marked so
	 */

	if (so->so_pcb) {
		int error2;
		error2 = (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
			(struct mbuf *)NULL,
			(struct mbuf *)NULL,
			(struct mbuf *)NULL);
		if (error == 0)
			error = error2;
	}

discard:	/* so_lock is *held* */

	ASSERT(!(so->so_state & SS_NOFDREF), "soclose: NOFDREF");
	/*
	 *+ An attempt has been made to last close a socket that is already
	 *+ last closed.
	 */

	so->so_state |= SS_NOFDREF;	/* tells sofree no file ref exists */

	sofree(so);

	/*
	 * if this socket has a socket_peer, manage refcnt 
	 */

	if (sop != (struct socket_peer *)&so->so_lock) {

		/*
		 * socket has a socket_peer
		 *
		 * decrement socket management's refcnt and
		 * check for other references.
		 */

		sop->sop_refcnt--;
		if (sop->sop_refcnt > 0) {

			/*
			 * reference still exists
			 * unlock sop to let peer in to clean up
			 */

			(void) v_lock(&sop->sop_lock, SPLNET);
		} else {

			/*
			 * no other reference to this socket_peer
			 * so free the socket_peer struct
			 */

			(void) m_free(dtom(sop));
		}
	}

	/*
	 * After sofree() socket is GONE, ergo the lock is GONE, in fact
	 * another processor might likely be fiddling with the mbuf.
	 * DO NOT SOUNLOCK
	 *
	 * Simply get the spl level back to the caller's.
	 */

	splx(splevel);
	return (error);
}

/*
 * Called by soclose() to get rid of pending connections and
 * by pr_input routines to abort a partial connection.
 * To avoid deadlock with the PRU_DETACH routines, soclose()
 * unlocks the head socket after marking it ~SO_ACCEPTCONN
 * in order to avoid enqueue/dequeue activity.
 *
 * NOTE: pr_usrreq.PRU_ABORT can sofree if NOFDREF != 0
 * such is the case sometimes when called from pr_input routines.
 */

soabort(so)
	struct socket *so;
{
	register struct socket_peer * sopp;
	int error= 0;

	/*
	 * socket is locked.
	 *
	 * manage socket_peer refcnt like soclose()
	 */

	sopp = so->so_sopp;
	if (sopp != (struct socket_peer *)&so->so_lock) {

		/*
		 * socket has a socket_peer - this is the usual case
		 * Viz. AF_UNIX and AF_INET domains.
		 *
		 * let protocol abort connection - this can include an
		 * sofree().
		 * decrement socket management's refcnt
		 * and check for other references.
		 */

		error = (*so->so_proto->pr_usrreq)(so, PRU_ABORT,
			(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);

		sopp->sop_refcnt--;
		if (sopp->sop_refcnt > 0) {

			/*
			 * reference still exists
			 * unlock sop to let peer in to clean up
			 */

			(void) v_lock(&sopp->sop_lock, SPLNET);
			DROPIT(49);
		} else {

			/*
			 * no other reference to this socket_peer
			 * free the socket_peer struct
			 */

			(void) m_free(dtom(sopp));
			DROPIT(50);
		}

	} else {		/* no sop - shouldn't happen but don't panic */
		error = EINVAL;
		DROPIT(44);
	}

	if (error == EINVAL)
		sofree(so);

	return (error);
}

/*
 * so is a new socket created by the protocol engine to fulfill the
 * accept/connect request.  The socket is locked and dequeued from
 * the "parent" which is now unlocked.  All file system resources
 * are acquired.
 */

soaccept(so, nam)		
	register struct socket *so;
	struct mbuf *nam;
{
	int error;

	so->so_state &= ~SS_NOFDREF;
	error = so->so_error ? so->so_error :
		(*so->so_proto->pr_usrreq)(so, PRU_ACCEPT,
		 (struct mbuf *)NULL, nam, (struct mbuf *)NULL);
	return (error);
}

soconnect(so, nam)	
	register struct socket *so;
	struct mbuf *nam;
{
	int error;
	spl_t splevel;

	/*
	 * if the socket is bound then there is a socket_peer that needs
	 * locking to mutex state change, otherwise the state won't change and
	 * the "socket_peer" is the socket itself.  If the state check fails
	 * then let protocol connect.  Otherwise, error.  mutex user.
	 */
        if (so->so_options & SO_ACCEPTCONN)
		return (EOPNOTSUPP);

	splevel = SOLOCK(so);
        if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING) &&
            ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
            (error = sodisconnect(so, (struct mbuf *)NULL)))) {
		error = EISCONN;
	} else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
		    (struct mbuf *)NULL, nam, (struct mbuf *)NULL);
	SOUNLOCK(so, splevel);
	return (error);
}

soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int error;
	spl_t splevel;

	splevel = SOLOCK(so1);
	(void) SOLOCK(so2);
	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
			(struct mbuf *)NULL, (struct mbuf *)so2,
			(struct mbuf *)NULL);
	SOUNLOCK(so2, SPLNET);
	SOUNLOCK(so1, splevel);
	return (error);
}

/*
 * sodisconnect - disconnect the socket - assumes so_lock is held
 * Note that pr_usrreq PRU_DISCONNECT called with so_lock held.
 */

sodisconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0)
		return (ENOTCONN);
	if (so->so_state & SS_ISDISCONNECTING)
		return (EALREADY);
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
			(struct mbuf *)NULL, nam, (struct mbuf *)NULL);
	return (error);
}

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 *
 * This routine is a merge of 4.3 with the dynix mods.
 *
 * note: rights not supported
 */

sosend(so, nam, uio, flags, rights)
	register struct socket *so;
	struct mbuf *nam;
	register struct uio *uio;
	int flags;
	struct mbuf *rights;
{
	struct mbuf *top = (struct mbuf *)NULL;
	register struct mbuf *m, **mp;
	register int space;
	int len, error = 0, dontroute, first = 1;
	spl_t splevel;

	splevel = SOLOCK(so);

	if (sosendallatonce(so) && uio->uio_resid > so->so_snd.sb_hiwat) {
		SOUNLOCK(so, splevel);
		return (EMSGSIZE);
	}
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);
	u.u_ru.ru_msgsnd++;

#define	snderr(errno)	{ error = errno; goto release; }

 restart:
	sblock(&so->so_snd);	/* sblock is p_sema operation */
	do {
		if (so->so_state & SS_CANTSENDMORE) 
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;	/* N.B. so_lock is *held* */
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED)
				snderr(ENOTCONN);	/* so_lock *held* */
			if (nam == (struct mbuf *)NULL)
				snderr(EDESTADDRREQ);	/* so_lock *held* */
		}
		if (flags & MSG_OOB)
			space = 1024;
		else {
			space = sbspace(&so->so_snd);
			/*
			 * dont send now 
			 *   if this is a atomic protocol and request > space
			 *   or request >= MCLEN
			 *      and space < MCLEN
			 *      and num on queue >= big mbuf
			 *      and socket is blocking  
			 * Second case is a performance tweek to cause
			 * maximum use of big mbufs!
			 */
			if ( space <= 0 
			    || (sosendallatonce(so) && space < uio->uio_resid)
			    || (uio->uio_resid >= MCLEN/2
				&& space < MCLEN
				&& so->so_snd.sb_cc >= MCLEN
				&& (so->so_state & SS_NBIO) == 0)) {
				if (so->so_state & SS_NBIO) {
					if (first)
						error = EWOULDBLOCK;
					goto release;
				}
				sbunlock(&so->so_snd);
				sbwait(&so->so_snd);
				goto restart;
			}
		}

		mp = &top;
		/*
		 * unlock for uiomove.. reacquire below.
		 */
		SOUNLOCK(so, splevel);
		while (space > 0) {
			/*
			 * if possible use a big mbuf
			 */
			if (uio->uio_resid >= MCLEN / 2 && space >= MCLEN &&
			    (m = m_getcl(M_WAIT, MT_DATA)) != 0) {
				len = MIN(MCLEN, uio->uio_resid);
				space -= MCLEN;
			} else {
				MGET(m, M_WAIT, MT_DATA);
				while ((m == (struct mbuf *)NULL)) {
					/* try to recover from out of mbufs */
					if (!m_expandorwait()) {
						error = ENOBUFS;
						splevel = SOLOCK(so);
						goto release;	/* so_lock held */
					}
					MGET(m, M_WAIT, MT_DATA);
				} /* end ENOBUFS recovery */
				len = MIN(MIN(MLEN, uio->uio_resid), space);
				space -= len;
			}
			error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, uio);
			m->m_len = len;
			*mp = m;
			if (error) {
				splevel = SOLOCK(so);
				goto release;
			}
			mp = &m->m_next;
			if (uio->uio_resid <= 0)
				break;
		}

		(void) SOLOCK(so);
		/*
		 * Recheck error conditions after copy and reacquiring lock
		 * (connection may have closed during copy).
		 */
		if (so->so_state & SS_CANTSENDMORE) 
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;	/* N.B. so_lock is *held* */
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED)
				snderr(ENOTCONN);	/* so_lock *held* */
			if (nam == (struct mbuf *)NULL)
				snderr(EDESTADDRREQ);	/* so_lock *held* */
		}

		if (dontroute)
			so->so_options |= SO_DONTROUTE;

		error = (*so->so_proto->pr_usrreq)(so,
				(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
				    top, (caddr_t)nam, rights);
		if (dontroute)
			so->so_options &= ~SO_DONTROUTE;
		rights = 0;
		top = 0;
		first = 0;
		if (error)
			break;
	} while (uio->uio_resid);

release: 	/* note so_lock  held */

	sbunlock(&so->so_snd);

	/*
	 * if there is still room to send stuff and another process
	 * wants to - let it try.
	 */

	if(sbspace(&so->so_snd) > 0)
		sowwakeup(so);

	SOUNLOCK(so, splevel);	

	if (top)
		m_freem(top);
	if (error == EPIPE)
		psignal(u.u_procp, SIGPIPE);
	return (error);

} /* end of sosend */

/*
 * soreceive - get received data from socket.
 * called by rcvmsg, rcvfrom, and recv syscalls, and soo_rw on a read
 */

soreceive(so, aname, uio, flags, rightsp)
	register struct socket *so;
	struct mbuf **aname;
	register struct uio *uio;
	int flags;
	struct mbuf **rightsp;
{
	register struct mbuf *m; 
	register int len, error = 0, tomark, savemark;
	struct protosw *pr = so->so_proto;
	struct mbuf *n, *eor, *prevm;
	struct mbuf *mcopy = (struct mbuf *) NULL;
	spl_t	splevel;
	int 	 totlen;
	struct	sockbuf	tmpsb;
	int	gotrights = 0;

	/*
	 * passing access rights is not supported in Dynix 
	 *
	 * rightsp is an mbuf * that is used to return a rights mbuf
	 * (even if it is empty - 4.2bsd)
	 * The routine recvit in ../sys/uipc_syscalls.c
	 * does not initialize this pointer, so soreceive does it here
	 * Not doing so breaks recvit since it m_free's the returned mbuf.
	 */

	if (rightsp)
		*rightsp = (struct mbuf*)NULL;
	if (aname)
		*aname = (struct mbuf *)NULL;

	if (flags & MSG_OOB) {
		m = m_get(M_WAIT, MT_DATA);
		if (m == (struct mbuf *)NULL)
			return (ENOBUFS);
		splevel = SOLOCK(so);	/* mutex so_state,... */
		error = (*pr->pr_usrreq)(so, PRU_RCVOOB,
		    m, (struct mbuf *)(flags & MSG_PEEK), (struct mbuf *)NULL);
		if (error)
			goto bad;
		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;

			/*
			 * uiomove can not be called within lock
			 * since process can be swapped
			 */

			SOUNLOCK(so, splevel);
			error =
			    uiomove(mtod(m, caddr_t), (int)len, UIO_READ, uio);
			m = m_free(m);
			(void) SOLOCK(so);	/* mutex so_state,... */
		} while (uio->uio_resid && error == 0 && m);
bad:
		SOUNLOCK(so, splevel);
		if (m) m_freem(m);
		return (error);

	}	/* end Out-of-band data processing */

	/* not OOB data */

	splevel = SOLOCK(so);
	sblock(&so->so_rcv);		/* sblock is semaphore */

#define	rcverr(errno)	{ error = errno; goto release; }

	/*
	 * so_lock mutex's changes in so_state and sb_cc (data count).
	 * critical section defined by this while block looking
	 * for either the state to change (e.g. DISCONNECT) or data to
	 * arrive (sb_cc != 0).  Note that if cc == 0 and no state change
	 * loop sbwaits (i.e. p_sema) and tries again.
	 */

	while (so->so_rcv.sb_cc == 0) { /* no data to receive */
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;	/* so_lock held */
		}
		if (so->so_state & SS_CANTRCVMORE)
			goto release;	/* so_lock held */
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED))
			rcverr(ENOTCONN); /* so_lock held */

		/*
		 * special case read of 0 bytes with 0 bytes avail.
		 */

		if(uio->uio_resid == 0)
			goto release;

		if (so->so_state & SS_NBIO)
			rcverr(EWOULDBLOCK); /* so_lock held */
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED))
			rcverr(ENOTCONN); /* so_lock held */

		/*
		 * sbunlock is a simple v_sema operation.
		 * Therefore, servicing is FIFO.
		 */

		sbunlock(&so->so_rcv);  /* allow another user to get sb */

		/*
		 * sbwait assumes so_lock is held and does a p_sema_v_lock()
		 * which leaves so_lock *not* held. Therefore critical section
		 * is compromised and thus requires that state be rechecked.
		 *
		 * sbwait reacquires the lock at SPLNET.
		 *
		 * N.B. can be signalled
		 */

		sbwait(&so->so_rcv);	

		sblock(&so->so_rcv);	/* sblock is p_sema operation */
	}

	/*
	 * cc != 0 => data there to receive
	 *
	 * so_lock is held
	 */

	m = so->so_rcv.sb_mb;
	ASSERT((m != (struct mbuf *)NULL), "soreceived no mbufs");
	/*
	 *+ Socket state indicates data is present, but no mbufs are
	 *+ being held on the socket buffer
	 */

	/*
	 * use a tmpsb to update sockbuf until committed
	 * to completing the receive.
	 */

	tmpsb.sb_mb= so->so_rcv.sb_mb;
	tmpsb.sb_cc = so->so_rcv.sb_cc;
	tmpsb.sb_mbcnt = so->so_rcv.sb_mbcnt;

	if (pr->pr_flags & PR_ADDR) {	/* e.g. datagram */

		ASSERT(m->m_type == MT_SONAME, "receive 1a");
		/*
		 *+ The socket has an mbuf chain without a MT_SONAME
		 *+ mbuf as its first element when the underlying
		 *+ protocol requires it
		 */

		eor = m->m_act;		/* remember next record */

		/*
		 * handle SONAME mbuf (first one in chain)
		 */

		if ((flags & MSG_PEEK) == 0) {
			/*
			 * not just peeking - really get the name
			 */
			if (aname)
				*aname = m;
			tmpsb.sb_cc -= m->m_len;
			tmpsb.sb_mbcnt -= MSIZE;
		}
		else	/* just peeking, copy the name */
			if(aname)
				if (((*aname = m_copy(m, 0, m->m_len)) 
				     == (struct mbuf *) NULL)
				     && m->m_len)
					rcverr(ENOBUFS);

		m = m->m_next;	/* past name buffer */

		/*
		 * DYNIX does not support passing rights, but rights
		 * buffer is managed anyway - E.g. AF_UNIX, SOCK_DGRAM
		 */

		if(m && m->m_type == MT_RIGHTS) {

			/*
			 * there is a "rights" buffer - legit?
			 */

			ASSERT(pr->pr_flags & PR_RIGHTS, "receive 2");
			/*
			 *+ The received socket datagram has a rights buffer
			 *+ but the protocol doesn't support passing of rights
			 */
			gotrights = 1;

			if ((flags & MSG_PEEK) == 0) {
				/*
				 * not just peeking - really get the rights
				 */
				if (rightsp)
					*rightsp = m;
				tmpsb.sb_cc -= m->m_len;
				tmpsb.sb_mbcnt -= MSIZE;
			}
			else
				if(rightsp)
				   if (((*rightsp = m_copy(m, 0, m->m_len)) 
					     == (struct mbuf *) NULL)
					     && m->m_len) {
						(void) m_free(*aname);
						*aname = (struct mbuf*) NULL;
						rcverr(ENOBUFS);
					}

			m = m->m_next;	/* past rights buffer */

		}

		if ((flags & MSG_PEEK) == 0)
			tmpsb.sb_mb = m;
	}

	tomark = so->so_oobmark;
	savemark = tomark;
	totlen = uio->uio_resid;
	if (totlen > 0) {
		n = m;
	} else {
		/*
		 * Set n to NULL as nothing will be read.
		 */
		n = (struct mbuf *)NULL;
	}
	prevm = (struct mbuf *)NULL;

	while (m) {
		if (totlen <= 0)
			break;
		len = totlen;
		so->so_state &= ~SS_RCVATMARK;
		if (tomark && len > tomark)
			len = tomark;
		if (len > m->m_len)
			len = m->m_len;

		totlen -= len;

		if (len == m->m_len) {

			if((flags & MSG_PEEK) == 0) {
				sbfree(&tmpsb, m);
				prevm = m;
				tmpsb.sb_mb = m->m_next;
			}

			m = m->m_next;

		} else {

			if ((flags & MSG_PEEK) == 0) {
				if((mcopy = m_copy(m, 0, len))
				    == (struct mbuf*) NULL) {

					so->so_oobmark = savemark;	
					rcverr(ENOBUFS);
				}
				m->m_off += len;
				m->m_len -= len;
				tmpsb.sb_cc -= len;

				if(prevm)

					prevm->m_next = mcopy;

				else	/* 1 mbuf, partially copied */

					n = mcopy;
			}
		}
		if ((flags & MSG_PEEK) == 0 && so->so_oobmark) {
			so->so_oobmark -= len;
			if (so->so_oobmark == 0) {
				so->so_state |= SS_RCVATMARK;
				break;
			}
		}
		if (tomark) {
			tomark -= len;
			if (tomark == 0)
				break;
		}
	}

	if (flags & MSG_PEEK)
		goto do_uiomove;	/* so_lock and sblock held */

	/*
	 * not just peeking - do the sockbuf update for real
	 */

	so->so_rcv.sb_cc = tmpsb.sb_cc;
	so->so_rcv.sb_mbcnt = tmpsb.sb_mbcnt;

	if(pr->pr_flags & PR_ADDR) {

		/*
		 * handle the SONAME and RIGHTS mbufs by either giving
		 * them to the caller or m_freeing them.
		 */

		if(pr->pr_flags & PR_RIGHTS)
			if(rightsp && *rightsp)
				(*rightsp)->m_next = (struct mbuf *)NULL;
			else if (gotrights)
				(void) m_free(so->so_rcv.sb_mb->m_next);

		if (aname)
			(*aname)->m_next = (struct mbuf *)NULL;
		else	
			(void) m_free(so->so_rcv.sb_mb);
	}

	so->so_rcv.sb_mb = tmpsb.sb_mb;

	u.u_ru.ru_msgrcv++;

	if ((so->so_proto->pr_flags & PR_ATOMIC))
	{
		/*
		 * release the residual data (after the read)
		 * handle the case where *nothing* was read
		 * by checking for n == m
		 */

		if(n == m)
			n = (struct mbuf*) NULL;

		while(m) {
			sbfree(&so->so_rcv, m);
			m = m_free(m);
		} 
		so->so_rcv.sb_mb = eor;		/* next record */
	}

	/*
	 * note that sblock and so_lock are held for pr_usrreq
	 */

	if ((so->so_proto->pr_flags & PR_WANTRCVD) && so->so_pcb)
		(*so->so_proto->pr_usrreq)(so, PRU_RCVD,
		    (struct mbuf *)NULL, (struct mbuf *)NULL,
			 (struct mbuf *)NULL);

	sbunlock(&so->so_rcv);

	/*
	 * if there's still more to read and somebody wants it -
	 * wake them up.
	 */

	if(so->so_rcv.sb_cc)
		sorwakeup(so);

do_uiomove:

	/*
	 * Actually copy the data out
	 *
	 * uiomove can not be called within lock since can be swapped
	 */

	SOUNLOCK(so, splevel);

	/*
	 * If totlen is nonzero, then the transfer would have terminated for
	 * some other reason, so length to xfer is uio_resid - totlen
	 */

	totlen = uio->uio_resid - totlen;
	m = n;

	while(m && totlen){

		error = uiomove(mtod(m, caddr_t), (int)m->m_len, UIO_READ, uio);

		if (error)
			break;

		totlen -= m->m_len;
		m = m->m_next;
	}

	if ((flags & MSG_PEEK) == 0)  {

		/*
		 * terminate end of released chain by setting
		 * last mbuf m_next == null;
		 * if chain was only a partial mbuf
		 * m_copy already set n->m_next to NULL.
		 */

		if(prevm && !mcopy)
			prevm->m_next = (struct mbuf*) NULL;
		if(n)
			m_freem(n);

		return(error);
	}

	/*
	 * MSG_PEEK needs to relock and sbunlock
	 */

	SOLOCK(so);
release:

	sbunlock(&so->so_rcv);

	/*
	 * if there's still more to read and somebody wants it -
	 * wake them up.
	 */

	if(so->so_rcv.sb_cc)
		sorwakeup(so);

	SOUNLOCK(so, splevel);
	return (error);

} /* end of soreceive */

soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr;
	spl_t splevel;
	int error = 0;

	splevel = SOLOCK(so);
	pr = so->so_proto; /* Note so_proto is static data */
	how++;
	if (how & FREAD)
		sorflush(so);
	if (how & FWRITE) {
		error = (*pr->pr_usrreq)(so, PRU_SHUTDOWN,
				(struct mbuf *)NULL,
				(struct mbuf *)NULL,
				(struct mbuf *)NULL);
	}
	SOUNLOCK(so, splevel);
	return (error);
}

/*
 * sorflush() - flush the receive queue
 * called with so_lock held and SPLNET
 */

sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb = &so->so_rcv;
	struct sockbuf asb;

	/*
	 * If SS_CANTRCVMORE not set then must set and wakeup
	 * any waiters in soreceive(). SS_CANTRCVMORE should always
	 * be set if sorflush() is called from interrupt level. So,
	 * the p_sema() should not be attempted at interrupt level.
	 */

	if ((so->so_state & SS_CANTRCVMORE) == 0) {

		/*
		 * The so_lock may be dropped when acquiring the sblock
		 * semaphore. If so, it is reacquired at SPLNET.
		 *
		 * sblock p_sema's and may need to wait.
		 */

		sblock(sb);
		socantrcvmore(so);
		sbunlock(sb);		/* v_sema */
	}

	asb = *sb;		/* n.b. structure copy */

	/*
	 * zero everything BUT sb_sbx.
	 */

	sb->sb_cc = 0;
	sb->sb_hiwat = 0;
	sb->sb_mbcnt = 0;
	sb->sb_mbmax = 0;
	sb->sb_lowat = 0;
	sb->sb_timeo = 0;
	sb->sb_mb = (struct mbuf *)NULL;
	sb->sb_sel = (struct proc *)NULL;
	sb->sb_flags = 0;

	sbrelease(&asb);
}

sosetopt(so, level, optname, m)
	register struct socket *so;
	int level, optname;
	register struct mbuf *m;
{
	spl_t splevel;
	int error = 0;
	struct mbuf **m0, *m1;

	/*
	 * the compiler won't allow taking the address
	 * of a passed in parameter so this warped
	 * code is needed
	 */
	m1 = m;
	m0 = &m1;

	splevel = SOLOCK(so);

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			error = (*so->so_proto->pr_ctloutput)
				  (PRCO_SETOPT, so, level, optname, m0);
		else
			error = ENOPROTOOPT;
	} else {
		
		
		switch (optname) {
			
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			/*
			 * 4.3 to 4.2 compat
			 */
			if(m == NULL || m->m_len < sizeof(int)) {
				/* assume its a 4.2 style option */
				so->so_options |= optname;
			} else {
				if(*mtod(m, int *))
					so->so_options |= optname;
				else
					so->so_options &= ~optname;
			}
			
			break;
			
		case SO_LINGER:
			
			/*
			 * 4.2 <-> 4.3 compatibility code
			 */
			if (m == (struct mbuf *)NULL ) {
				SOUNLOCK(so, splevel);
				return (EINVAL);
			}
			
			
			if( m->m_len == sizeof(struct linger)) { /* 4.3 style */
				so->so_linger = mtod(m, struct linger *)->l_linger;
				
			} else if (m->m_len == sizeof(int)) { /* 4.2 style */
				so->so_linger = *mtod(m, int *);
			} else {
				SOUNLOCK(so, splevel);
				return(EINVAL);
			}
			
			if (so->so_proto && so->so_proto->pr_ctloutput)
				error = (*so->so_proto->pr_ctloutput)
				  	(PRCO_SETOPT, so, 
					so->so_proto->pr_protocol, optname, m0);

			if (error == 0)
				so->so_options |= SO_LINGER;
			break;
			
		case SO_SNDBUF:
		case SO_RCVBUF:
			if (sbreserve(optname == SO_SNDBUF ?
				     &so->so_snd : & so->so_rcv,
				     (int) *mtod(m, int *)) == 0) {
				SOUNLOCK(so, splevel);
				return(ENOBUFS);
			}
			break;
			
		case SO_SNDLOWAT:
			so->so_snd.sb_lowat = *mtod(m, int *);
			break;
			
		case SO_RCVLOWAT:
			so->so_rcv.sb_lowat = *mtod(m, int *);
			break;
			
		case SO_SNDTIMEO:
			so->so_snd.sb_timeo = *mtod(m, int *);
			break;
			
		case SO_RCVTIMEO:
			so->so_rcv.sb_timeo = *mtod(m, int *);
			break;
			
		case SO_DONTLINGER:
			so->so_options |= SO_LINGER;
			so->so_linger = 0;
			break;
			
		default:
			error = EINVAL;
		}
	}

	SOUNLOCK(so, splevel);
	return (error);
}

sogetopt(so, level, optname, m)
	register struct socket *so;
	int level, optname;
	register struct mbuf *m;
{
	spl_t splevel;
	int error = 0;
	struct mbuf **m0, *m1;


	/*
	 * the compiler won't allow taking the address
	 * of a passed in parameter so this warped
	 * code is needed
	 */
	m1 = m;
	m0 = &m1;

	splevel = SOLOCK(so);

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			error = (*so->so_proto->pr_ctloutput)
				  (PRCO_GETOPT, so, level, optname, m0);
		else
			error = ENOPROTOOPT;
	} else {
		

		
		switch (optname) {
			
		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_LINGER:
			if (m != (struct mbuf *)NULL) {
				if (m->m_len == sizeof(struct linger)) {
		                        mtod(m, struct linger *)->l_onoff =
               		                    so->so_options & SO_LINGER;
                        		mtod(m, struct linger *)->l_linger = 
					    so->so_linger;
				} else {
					*mtod(m, int *) = so->so_linger;
					m->m_len = sizeof (so->so_linger);
				}
			}
			break;
			
		case SO_TYPE:
			*mtod(m, int *) = so->so_type;
			break;
			
		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;
			
		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;
			
		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;
			
		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;
			
		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;
			
		case SO_SNDTIMEO:
			*mtod(m, int *) = so->so_snd.sb_timeo;
			break;
			
		case SO_RCVTIMEO:
			*mtod(m, int *) = so->so_rcv.sb_timeo;
			break;
			
		default:
			error = EINVAL;

		}

	}

	SOUNLOCK(so, splevel);
	return (error);
}

sohasoutofband(so)
	register struct socket *so;
{
	if (so->so_pgrp == 0)
		return;
	if (so->so_pgrp > 0)
		gsignal(so->so_pgrp, SIGURG);

	else {
		/*
		 * pfind returns a pointer to a *locked* proc struct
		 */
		struct proc *p = pfind(-so->so_pgrp);

		if (p) {
			lpsignal(p, SIGURG);
			v_lock(&p->p_state, SPLNET);
		}
	}
	return;
}

/*
 * soalloc allocates a socket and socket extension and initializes
 * the locks and semaphores
 */

struct socket *
soalloc()
{
	register struct mbuf *m;
	register struct socket *so;
	register struct socket_extension *sx;

	m = m_getclrm(M_DONTWAIT, MT_SOCKET, 2);

	/*
	 * NOTE: two MT_SOCKET type mbuf's are gotten with this
	 * and no MT_SOCKEX type mbuf. Ergo, SOCKEX types are UNUSED.
	 */

	if (m == (struct mbuf *)NULL)		/* get both or none */
		return ((struct socket *)NULL);

	/*
	 * G_SOCK should be within a range - one candidate is to base the
	 * candidates - uid procid random (say so address % MAX-MIN+1)
	 */

	so = mtod(m, struct socket *);
	sx = mtod(m->m_next, struct socket_extension *);

	/*
	 * The following is a temporary lock on the struct socket
	 * lock initialized *before* it is attached by a protocol
	 * engine.  the protocol engine typically binds the socket to a socket
	 * peer structure and points sopp there.  The so_lock is no longer
	 * referenced.
	 */

	so->so_sopp = (struct socket_peer *)&so->so_lock;
	init_lock(&so->so_lock, G_SOCK);
	so->so_snd.sb_sbx = &sx->sx_snd;
	init_sema(&so->so_snd.sb_sbx->sbx_buf_sema, 1, 0, G_SOCK);
	init_sema(&so->so_snd.sb_sbx->sbx_buf_wait, 0, 0, G_SOCK);
	so->so_rcv.sb_sbx = &sx->sx_rcv;
	init_sema(&so->so_rcv.sb_sbx->sbx_buf_sema, 1, 0, G_SOCK);
	init_sema(&so->so_rcv.sb_sbx->sbx_buf_wait, 0, 0, G_SOCK);
	so->so_conn_semap = (sema_t *)&sx->sx_connect_sema;
	init_sema(so->so_conn_semap, 0, 0, G_SOCK);
	return (so);
}
