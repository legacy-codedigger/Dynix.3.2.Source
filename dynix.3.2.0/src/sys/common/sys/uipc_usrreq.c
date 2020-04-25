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

#ifndef	lint
static	char	rcsid[] = "$Header: uipc_usrreq.c 2.17 1991/04/15 19:15:13 $";
#endif

/*
 * upic_usrreq.c
 *	Protocol engines for Unix domain protocol
 *	and Unix domain protocol control blocks
 */

/* $Log: uipc_usrreq.c,v $
 *
 */

#include "../h/param.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/unpcb.h"
#include "../h/un.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/stat.h"

#include "../h/vmmeter.h"
#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"

/*
 * Unix communications domain.
 *
 * TODO:
 *	SEQPACKET, RDM
 *	rethink name space problems
 *	need a proper out-of-band
 */

struct	sockaddr sun_noname = { AF_UNIX };

sema_t uipc_sema;

uipc_init()
{
	init_sema(&uipc_sema,1,0,G_SOCK);
	return (0);
}

/*
 * Note on syncronization:  The syncronization model for unix domain sockets
 * has been drastically changed.  In the past a single global semaphore
 * protected most operations (such as connect and detach) from interfering
 * with each other.  Although simple, this methodology had the nasty
 * tendancy to interfere with processes which didn't need it (e.g. the
 * closing of pipes).  As such the model has been revised as follows:
 * A socket and unpcb share a single socket peer.  Locking this is sufficient
 * to lock both.  This lock is also sufficient for bind and listen (as no
 * other sockets are involved in that action).  However, once we attempt
 * to connect we have a delima -- we must inspect and potentially modify
 * another socket and/or unpcb.  To avoid deadlock we imploy the global
 * semaphore again -- but in a refined sense.  Basically we use the semaphore
 * to get connected and to become disconnected or to detach; i.e. any socket 
 * undergoing these operations must grab the global sema first as a
 * way to syncronize among the various processes.
 * 
 * Whenever possible two connected sockets share a common socket peer; e.g.
 * pipes are created so that both sockets and both unpcb contain pointers
 * to the same socket peer structure.  In this way locking one socket has
 * the effect of locking both.  The same is true for connected stream
 * sockets -- pipes come to share this sop via piconnect, which never
 * touches any of this code.  For stream sockets (but not pipes) the
 * PRU_CONNECT path is followed through uipc_usrreq.  Here we use the
 * semaphore to prevent other sockets from interfering with our connect
 * process.  Once connected, however, we can begin sharing a common socket
 * peer, and any operation after that point no longer requires the use
 * of the global semaphore since every subsequent operation will only ever
 * involve one or both of these sockets.
 * 
 * The only other use for the global sema is with datagram sockets.  The
 * notion of "connected" datagram sockets causes much grief and convolutions
 * in the code that follows.  The unpcb fields unp_refs and unp_nextref are
 * used to thread the unpcbs which are "connected" to a given socket.  When
 * one socket in the chain goes away, others in the chain must have the
 * relevant fields updated accordingly.  In sendto'ing a socket, the code
 * temporarily "connects" to the destination (threading this chain), transfers 
 * the data, then disconnects (again updating the chain).  In other words 
 * nearly every operation
 * on a datagram socket potentially mucks with other, arbitary sockets 
 * and/or unpcbs.  Rather than try to be spiffy and lock just those sockets 
 * we're about to modify, the model is to grab the global semaphore and hold
 * it until all the unpcbs and/or sockets we're going to modify are modified.
 *
 * We employ the p_sema_v_lock routine to grab the semaphore for us, after
 * which we lock the socket again -- consistency with socket layers above
 * us dictate that the socket is locked when we enter uipc_usrreq and is
 * still locked when we leave.  Operations can go on around us when we sleep
 * for the global semaphore and our socket is unlocked, but the socket
 * itself won't go away during that time (at least our process still has
 * a reference to it).  Therefore after every acquisition of the semaphore 
 * we must check (again) to see whether to proceed.
 */

/*ARGSUSED*/
uipc_usrreq(so, req, m, nam, rights)	/* entered with so_lock *held* */
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct unpcb *unp = sotounpcb(so);
	register struct socket *so2 = NULL;
	int error = 0;
	int sema_held = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (req != PRU_SEND && rights && rights->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (unp == (struct unpcb *)NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:

		if (unp) {
			error = EISCONN;
			break;
		}
		error = unp_attach(so);
		break;

	case PRU_DETACH:	/* so_lock held */

		unp_detach(unp);
		break;

	case PRU_BIND:
		error = unp_bind(unp, nam);
		break;

	case PRU_LISTEN:
		if (unp->unp_vnode == 0)
			error = EINVAL;
		break;

	case PRU_CONNECT:
		error = unp_connect(so, nam, &sema_held);

		/*
		 * undo unp_connect lockee - the connectee - only if it's 
		 * a separate lock.  Check for self reference.
		 */

		if (error == 0) {
			if (unp->unp_csop == 0 && 
			    so != unp->unp_conn->unp_socket) {
				(void) SOUNLOCK(unp->unp_conn->unp_socket,
				    SPLNET);
			}
		}

		break;

	case PRU_CONNECT2:	/* used by socketpair */
		error = unp_connect2(so, (struct socket *)nam);
		break;

	case PRU_DISCONNECT:
		if (unp->unp_conn == (struct unpcb *) NULL)
			break;
		if (unp->unp_csop == 0) { 	/* no common sop */
			p_sema_v_lock(&uipc_sema, PZERO,
				&so->so_sopp->sop_lock, SPLNET);
			SOLOCK(so);
			sema_held = 1;
			if (unp->unp_conn == NULL) {
				/* 
				 * must have went away when we grabbed 
				 * the semaphore 
				 */
				break;
			}
		}
		unp_disconnect(unp);
		break;

	case PRU_ACCEPT:
		/*
		 * Pass back name of connected socket, if it
		 * was bound and we are still connected (our peer
		 * may have closed already)
		 * Syncronization: only stream sockets get this far,
		 * hence we already share a socket peer.
		 */
		if (unp->unp_conn && unp->unp_conn->unp_addr) {
			nam->m_len = unp->unp_conn->unp_addr->m_len;
			bcopy(mtod(unp->unp_conn->unp_addr, caddr_t),
			    mtod(nam, caddr_t), (unsigned)nam->m_len);
		} else {
			nam->m_len = sizeof(sun_noname);
			*(mtod(nam, struct sockaddr *)) = sun_noname;
		}
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		unp_usrclosed(unp);	/* N.B. NO-OP routine */
		break;

	case PRU_RCVD:
		switch (so->so_type) {

		case SOCK_DGRAM:
			panic("uipc 1");
			/*NOTREACHED*/

		case SOCK_STREAM:

			/*
			 * socket_peer lock is held, so both sockets are
			 * locked.
			 */

#define	rcv (&so->so_rcv)
#define snd (&so2->so_snd)

			if (unp->unp_conn == 0)
				break;
			so2 = unp->unp_conn->unp_socket;

			/*
			 * Transfer resources back to send port
			 * and wakeup any waiting to write.
			 */
                        snd->sb_mbmax += unp->unp_mbcnt - rcv->sb_mbcnt;
                        unp->unp_mbcnt = rcv->sb_mbcnt;
                        snd->sb_hiwat += unp->unp_cc - rcv->sb_cc;
                        unp->unp_cc = rcv->sb_cc;
#undef snd
#undef rcv
			/*
			 * N.B. sowwakeup can take affect immediately
			 * on multi-processors
			 */
			sowwakeup(so2);
			break;

		default:
			panic("uipc 2");
		}
		break;

	case PRU_SEND:

		/*
		 * socket buffer is locked when called from sosend routine
		 * for send, sendto, and sendmsg - also by soo_rw when
		 * WRITE done.
		 */

		switch (so->so_type) {

		case SOCK_DGRAM: {

			struct sockaddr *from;

			if (rights) {
				error = EOPNOTSUPP;
				break;
			}
			if (nam) {
				if (unp->unp_conn) {
					error = EISCONN;
					break;
				}
				/*
				 * N.B.: unp_connect will grab global semaphore
				 * and leave it held if successful
				 */
				error = unp_connect(so, nam, &sema_held);
				if (error)
					break;
				so2 = unp->unp_conn->unp_socket;
			} else {
				if (unp->unp_conn == 0
				    || unp->unp_conn->unp_socket == 0) {
					error = ENOTCONN;
					break;
				}
				/* 
				 * Since we're about to go muck with the
				 * other unpcb and socket, grab global
				 * sema first
				 */
				p_sema_v_lock(&uipc_sema, PZERO,
					&so->so_sopp->sop_lock, SPLNET);
				SOLOCK(so);
				sema_held = 1;
				if (unp->unp_conn == 0 || 
				    (so2 = unp->unp_conn->unp_socket) == NULL) {
					/*
					 * must have went away when we grabbed 
					 * the semaphore
					 */
					error = ENOTCONN;
					break;
				} else {
					if (so != so2)
						SOLOCK(so2);
				}
			}
			/*
			 * in either case, so2 now points to the second socket,
			 * the locks on so and so2 are held, and the global
			 * sema is held -- we may call unp_disconnect below,
			 * for which we may need the sema.
			 */

			if (unp->unp_addr)
				from = mtod(unp->unp_addr, struct sockaddr *);
			else
				from = &sun_noname;

			if (sbspace(&so2->so_rcv) > 0) {
				/*
				 * N.B. sbappendaddr returns (0) if the append
				 * fails.
				 */
				if (sbappendaddr(&so2->so_rcv, from, m,
				    rights)) {
					m = (struct mbuf *)NULL;
					sorwakeup(so2);
				} else
					error = ENOBUFS;
			} else
				error = ENOBUFS;

			ASSERT_DEBUG(sema_held != 0, 
			    "disconnect without sema_held");
			if (nam) {
				unp_disconnect(unp);	/* sema still held */
			} 
			
			if (so != so2)
				SOUNLOCK(so2, SPLNET);

			break;
		}

		case SOCK_STREAM:
#define	rcv (&so2->so_rcv)
#define	snd (&so->so_snd)

			if (rights && rights->m_len) {
				error = EOPNOTSUPP;
				break;
			}

			ASSERT(unp->unp_conn != 0, "uipc 3");

			so2 = unp->unp_conn->unp_socket;

			/*
			 * Send to paired receive port, adjust the
			 * socket buffer sizes to reflect new data, then
			 * wake up readers.
			 */

			/*
			 * We share a common socket peer with the other
			 * side of this connection, so it is OK
			 * to fiddle with peer's socket struct.
			 */

			sbappend(rcv, m);

			m = (struct mbuf *)NULL;
                        snd->sb_mbmax -=
                            rcv->sb_mbcnt - unp->unp_conn->unp_mbcnt;
                        unp->unp_conn->unp_mbcnt = rcv->sb_mbcnt;
                        snd->sb_hiwat -= rcv->sb_cc - unp->unp_conn->unp_cc;
                        unp->unp_conn->unp_cc = rcv->sb_cc;
#undef snd
#undef rcv
			/*
			 * note that with multiple processors,
			 * sorwakeup can happen immediately.
			 */

			sorwakeup(so2);
			break;

		default:
			panic("uipc 4");
		}
		break;

	case PRU_ABORT:		 /* so_lock *held* */

		if (unp->unp_csop == 0) { 	/* no common sop */
			p_sema_v_lock(&uipc_sema, PZERO,
				&so->so_sopp->sop_lock, SPLNET);
			SOLOCK(so);
			sema_held = 1;
		}
		unp_drop_sock(unp, ECONNABORTED);
		break;

	case PRU_SENSE:

		/*
		 * Syncronization: we only touch the other socket if
		 * this is a stream socket -- if we're connected, we
		 * already share the socket peer
		 */
		((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
		if (so->so_type == SOCK_STREAM && unp->unp_conn != 0) {
			so2 = unp->unp_conn->unp_socket;
			((struct stat *) m)->st_blksize += so2->so_rcv.sb_cc;
		}
		/*
		 * since m isn't really an mbuf (its a stat structure
		 * sitting on the stack somewhere), nuke the value 
		 * so we don't try to free it later.
		 */
		m = (struct mbuf *)NULL;
		break;

	case PRU_RCVOOB:

		error = EOPNOTSUPP;

		/*
		 * soreceive() releases the mbuf it passes through to us
		 * so we nullify the pointer here to avoid freeing it twice.
		 */

		m = (struct mbuf *)NULL;

		break;

	case PRU_SENDOOB:

		/*
		 * from 4.3: note sosend() assumes pr_usrreq() eats mbuf
		 */

		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		if (unp->unp_addr) {
			nam->m_len = unp->unp_addr->m_len;
			bcopy(mtod(unp->unp_addr, caddr_t),
			    mtod(nam, caddr_t), (unsigned)nam->m_len);
		} else
			nam->m_len = 0;
		break;

	case PRU_PEERADDR:
		if (unp->unp_conn) {
			if (unp->unp_csop == 0 &&	/* no common sop */
			    so != unp->unp_conn->unp_socket) {	/* not conn'd to self */
				/* 
				 * Since we're about to go muck with the
				 * other unpcb and socket, grab global
				 * sema first
				 */
				p_sema_v_lock(&uipc_sema, PZERO,
					&so->so_sopp->sop_lock, SPLNET);
				SOLOCK(so);
				sema_held = 1;
				if (unp->unp_conn != NULL &&
				    (so2 = unp->unp_conn->unp_socket) != NULL) {
					SOLOCK(so2);
				} else {
					/*
					 * must have went away when we grabbed
					 * the semaphore
					 */
					nam->m_len = 0;
					break;
				}
			}
				
			if (unp->unp_conn->unp_addr) {
				nam->m_len = unp->unp_conn->unp_addr->m_len;
				bcopy(mtod(unp->unp_conn->unp_addr, caddr_t),
			    	    mtod(nam, caddr_t), (unsigned)nam->m_len);
			} else
				nam->m_len = 0;
			if (so2 != NULL)
				SOUNLOCK(so2, SPLNET);
		} else
			nam->m_len = 0;
		break;

	case PRU_SLOWTIMO:
		break;

	default:
		panic("piusrreq");
	}
release:
	if (m) {
		m_freem(m);
	}
	if (sema_held) {
		v_sema(&uipc_sema);
	}

	return (error);
}

unp_attach(so)
	struct socket *so;
{
	register struct mbuf *m;
	register struct unpcb *unp;
	register struct socket_peer * sop;
	int error;
	
	error = soreserve(so, unp_sendspace, unp_recvspace);

	if (error)
		return (error);

	/*
	 * N.B. M_DONTWAIT argument is not used (also not used in 4.2)
	 *
	 * however, this routine is called when a socket is first being
	 * created, therefore if we run out of buffers, return an error
	 * and socreate returns ENOBUFS to the user.
	 */

	m = m_getclr(M_DONTWAIT, MT_PCB);

	if (m == (struct mbuf *)NULL)
		return (ENOBUFS);

	unp = mtod(m, struct unpcb *);

	m = m_getclr(M_DONTWAIT, MT_SOPEER);
	if (m == (struct mbuf *)NULL) {
		(void) m_free(dtom(unp));
		return (ENOBUFS);
	}

	sop = mtod(m, struct socket_peer *);

	init_lock(&sop->sop_lock, G_SOCK);
	
	so->so_sopp = sop;
	
	sop->sop_refcnt = 2;

	/* point socket to pcb and pcb to socket */

	so->so_pcb = (caddr_t)unp;
	unp->unp_socket = so;

	return (0);
}

unp_detach(unp)
	register struct unpcb *unp;
{
	/* N.B.: so_lock is *held* */

	struct socket *so = unp->unp_socket;
	struct vnode *vp = unp->unp_vnode;
	int sema_held = 0;

	if (vp) {
		/* 
		 * syncronization: can only muck with vnode if its
		 * locked tight -- we're trying to prevent unp_connect
		 * from grabbing socket pointer and proceeding to attempt
		 * to connect to us -- it couldn't have grabbed the
		 * global sema yet, and we may get it first, releasing the
		 * socket and invalidating its local cache of this pointer.
		 * So we prevent the inspection by locking the vnode.
		 */
		VN_LOCK(vp);
		vp->v_socket = (struct socket *)NULL;
		VN_UNLOCK(vp);
		VN_RELE(vp);
		unp->unp_vnode = (struct vnode *)NULL;
	}
	/*
	 * Grab global sema if not sharing a socket peer -- 
	 * datagram sockets use the unp_refs and unp_nextref fields
	 * to thread a chain of * "connected" sockets -- since the graph 
	 * of those unpcb's is unbounded, be better have the global sema here,
	 * as unp_drop and unp_disconnect are going to be mucking with
	 * other's unpcbs!
	 */

	if (unp->unp_csop == 0) { 	/* no common sop */
		p_sema_v_lock(&uipc_sema, PZERO,
			&so->so_sopp->sop_lock, SPLNET);
		SOLOCK(so);
		sema_held = 1;
	}

	if (unp->unp_conn) {

		/*
		 * subtle, if this is still a connected socket pair
		 * (e.g. pipe), then if one is disconnected and the other
		 * is not, make the second one disconnected here in order
		 * to close out waiters etc.
		 */
		unp_disconnect(unp);	/* mark disconnected */
	}

	while (unp->unp_refs) {
		unp_drop(unp->unp_refs, ECONNRESET);
	}

	/*N.B. so_lock *held* */

	soisdisconnected(unp->unp_socket);

	unp->unp_socket->so_pcb = (caddr_t)NULL;

	if (--unp->unp_socket->so_sopp->sop_refcnt == 0) {
		(void) m_free(dtom(unp->unp_socket->so_sopp));
	}

	m_freem(unp->unp_addr);
	(void) m_free(dtom(unp));

	if (sema_held) {
		v_sema(&uipc_sema);
	}

	return;
}

unp_bind(unp, nam)
	struct unpcb *unp;
	struct mbuf *nam;
{
	struct sockaddr_un *soun = mtod(nam, struct sockaddr_un *);
	struct vnode *vp;
	struct vattr vattr;
	int error;

 	if (unp->unp_vnode != NULL || nam->m_len == MLEN)
 		return (EINVAL);
	*(mtod(nam, caddr_t) + nam->m_len) = 0;

	/* SHOULD BE ABLE TO ADOPT EXISTING AND wakeup() ALA FIFO's */

	vattr_null(&vattr);
	vattr.va_type = VSOCK;
	vattr.va_mode = 0777;
	error = vn_create(soun->sun_path, UIOSEG_KERNEL, &vattr, EXCL, 0, &vp);
	if (error) {
		if (error == EEXIST)
			return (EADDRINUSE);
		return (error);
	}
	/*
	 * Syncronization: can't inspect or modify vnode without tight
	 * lock on it.
	 */
	VN_LOCK(vp);
	vp->v_socket = unp->unp_socket;		/* socket is bound */
	VN_UNLOCK(vp);
	unp->unp_vnode = vp;
	unp->unp_addr = m_copy(nam, 0, (int)M_COPYALL);

	/* We return with vnode unlocked but referenced */

	VN_UNLOCKNODE(vp);

	return (0);
}

unp_connect(so, nam, sema_held)
	struct socket *so;
	struct mbuf *nam;
	int *sema_held;
{
	register struct sockaddr_un *soun = mtod(nam, struct sockaddr_un *);
	struct vnode *vp;
	int error;
	register struct socket *so2;
	struct unpcb *unp2, *aunp;

	register struct socket *asock;	/* accept socket */
	struct socket_peer * asock_sopp;

	*sema_held = 0;			/* haven't grabbed anything yet */

	if (nam->m_len + (nam->m_off - MMINOFF) == MLEN)
		return (EMSGSIZE);

	*(mtod(nam, caddr_t) + nam->m_len) = 0;

	/*
	 * N.B. lookupname can sleep.  We unlock the socket here so
	 * concurrent operations (e.g. other sendto's) can continue
	 * without spinning -- they'll end up here anyway sooner or later.
	 */
	SOUNLOCK(so, SPLNET);
	error =
	    lookupname(soun->sun_path, UIOSEG_KERNEL, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	(void) SOLOCK(so);

	if (error)
		return (error);

	if (vp->v_type != VSOCK) {
		VN_PUT(vp);
		return (ENOTSOCK);
	}

	/*
	 * Syncronization: cant do inspect or modify vnode without tight
	 * lock on it.  We're trying to protect against concurrent detach
	 * of the socket we're trying to connect to.
	 */
	VN_LOCK(vp);
	so2 = vp->v_socket;

	if (so2 == (struct socket *)NULL) {
		VN_UNLOCK(vp);
		VN_PUT(vp);
		return (ECONNREFUSED);
	}

	/*
	 * At this point there is no protection against concurrent
	 * modification of so2, with the exception that the
	 * vnode we now have a handle on can't go away
	 * while we've got a reference to it.  The other side
	 * can detach when we let the lock go though.
	 * But now's the time to grab the global
	 * semaphore and hope for the best.  We'll recover
	 * later if the other side does go away while we sleep.
	 */
	VN_UNLOCK(vp);
	p_sema_v_lock(&uipc_sema, PZERO,
		&so->so_sopp->sop_lock, SPLNET);
	SOLOCK(so);
	VN_LOCK(vp);
	if (sotounpcb(so)->unp_conn != NULL) {
		/*
		 * some other process connected while we grabbed
		 * the semaphore.
		 */
		v_sema(&uipc_sema);
		VN_UNLOCK(vp);
		VN_PUT(vp);
		return (EISCONN);
	} else if ((so2 = vp->v_socket) == NULL) {
		/*
		 * other side went away while we grabbed the sema
		 */
		v_sema(&uipc_sema);
		VN_UNLOCK(vp);
		VN_PUT(vp);
		return (ECONNREFUSED);
	} else {
		VN_UNLOCK(vp);
		if (so != so2) 
			SOLOCK(so2);	/* can't go away without global sema */
	}
	*sema_held = 1;

	/*
	 * We now have both so and so2 locked, and global sema held.
	 * We hold the global sema if so == so2 because the dgram code
	 * arbitrarily interconnects sockets.  Its conceivable that 
	 * many users are connecting to the same socket (including the
	 * owner of the socket itself), so to be save we hold the sema
	 * until after the disconnect.
	 *
	 * If we have return an error, so2 is unlocked, and sema is released.
	 */

	if (so->so_type != so2->so_type) {
		if (so != so2) 
			SOUNLOCK(so2, SPLNET);
		v_sema(&uipc_sema);
		*sema_held = 0;
		VN_PUT(vp);
		return (EPROTOTYPE);
	}

	switch(so->so_type) {

	case SOCK_DGRAM:
		if ((error = unp_connect2(so, so2))) {
			if (so != so2)
				SOUNLOCK(so2, SPLNET);
			v_sema(&uipc_sema);
			*sema_held = 0;
		}

		/*
		 * Now that we are about to return successfully connected, 
		 * so and so2 are still locked, and the sema is still held.
		 * This is an optimization for the silly DGRAM sendto code, 
		 * which will promptly disconnect a few cycles from
		 * now, after transfering the data.
		 */

		VN_PUT(vp);
		return (error);

	case SOCK_STREAM:

		if ((so2->so_options&SO_ACCEPTCONN) == 0) {
			if (so2 != so)
				SOUNLOCK(so2, SPLNET);
			v_sema(&uipc_sema);
			*sema_held = 0;
			VN_PUT(vp);
			return (ECONNREFUSED);
		}

		/*
		 * Save a reference to listen socket -- sonewconn will
		 * unlock it and open an WINDOW which we'll have to
		 * recover from later
		 */
		asock = so2;
		asock_sopp = so2->so_sopp;
		asock_sopp->sop_refcnt++;

		/*
		 * Drop global semaphore so if so2 does go away, the disconnect
		 * processing sonewconn may perform doesn't deadlock behind
		 * us.
		 */
		v_sema(&uipc_sema);
		*sema_held = 0;

		/*
		 * note, check for ACCEPTCONN must be done after
		 * sonewconn since it is checking for concurrent
		 * deletion of so_head.
		 */

		if ((so2 = sonewconn(so2)) == (struct socket *)NULL
		   || !(asock->so_options & SO_ACCEPTCONN)) {

			/*
			 * there is no new so, or there is and the
			 * head is disappearing. check for
			 * self reference and allow so_head to finish
			 */

			if (so2)
				(void) SOUNLOCK(so2, SPLNET);

			/*
			 * refcnt-- before check since self-reference
			 * also refcnt++  - can this be more efficient?
			 */

			asock_sopp->sop_refcnt--;
			if (so != asock)
				if (asock_sopp->sop_refcnt)
					(void) v_lock(&asock_sopp->sop_lock,
							SPLNET);
				else
					(void) m_free(dtom(asock_sopp));
			VN_PUT(vp);
			return (ECONNREFUSED);
		}

		unp2 = sotounpcb(so2);
		aunp = sotounpcb(asock);
		if (aunp->unp_addr)
			unp2->unp_addr = m_copy(aunp->unp_addr, 0, 
						(int) M_COPYALL);

		error = unp_connect2(so, so2);
		ASSERT(error == 0, "unp_connect");

		/*
		 * so and so2 connected ok
		 */

		so2->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
		so2->so_state |= SS_ISCONNECTED;

		/*
		 * wake up accept socket waiter 
		 *
		 * note that if this is a connect request to an
		 * accept socket, so2 is new socket on accept side,
		 * not the accept socket. There are not always
		 * connect waiters.
		 */

		(void) vall_sema(asock->so_conn_semap);

		/*
		 * can now unlock the so_head (asock)
		 * successful new socket is unlocked by caller (_CONNECT)
		 */

		asock_sopp->sop_refcnt--;

		if (asock != so)
			(void) SOUNLOCK(asock, SPLNET);

		VN_PUT(vp);

		return (error);

	default:
		if (so != so2)
			SOUNLOCK(so2, SPLNET);
		v_sema(&uipc_sema);
		*sema_held = 0;
		VN_PUT(vp);
		return (EPROTOTYPE);	/* should not happen */
	}
}

/*
 * unp_connect2 - connect two unix domain sockets - both so and so2 are
 * locked upon entry. If SOCK_STREAM, the acceptee socket is also
 * locked (for soisconnected).  Also, if a SOCK_STREAM connect
 * request, so2 is a new socket that is locked, but has no reference to
 * it yet (will get reference when return to accept call); therefore it
 * is OK to fiddle with the so_sopp and point it to so's socket_peer.
 */

unp_connect2(so, so2)
	register struct socket *so;
	register struct socket *so2;
{
	register struct unpcb *unp = sotounpcb(so);
	register struct unpcb *unp2;

	if (so2->so_type != so->so_type)
		return (EPROTOTYPE);

	unp2 = sotounpcb(so2);
	unp->unp_conn = unp2;

	switch (so->so_type) {

	case SOCK_DGRAM:

		unp->unp_nextref = unp2->unp_refs;
		unp2->unp_refs = unp;

		/* FIX moved from unp_connect */

		so2->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
		so2->so_state |= SS_ISCONNECTED;

		so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
		so->so_state |= SS_ISCONNECTED;

		(void) vall_sema(so2->so_conn_semap);

		break;

	case SOCK_STREAM:

		unp2->unp_conn = unp;

		/*
		 * point so2 to so's socket_peer.  This is OK since so2 is a
		 * new socket.  (for the case of socketpair *both* so and so2
		 * are new sockets; however, for an accept/connect sequence,
		 * the connect socket is so - and it does have a reference.
		 * point so2->so_sopp to so's socket_peer before signalling
		 * soisconnected.  Note: this leaves the unreferenced lock
		 * in the released sopp locked, but that is OK.
		 * Since we now share a sop, we set relevant state bits
		 * so we can avoid global semaphore semantics used elsewhere
		 */

		unp->unp_csop = 1;
		unp2->unp_csop = 1;

		(void) m_free(dtom(so2->so_sopp));

		so2->so_sopp = so->so_sopp;

		/* need ref for each detach and soclose */
	
		so->so_sopp->sop_refcnt = 4;

		soisconnected(so2);

		soisconnected(so);

		break;

	default:
		panic("unp_connect2");
	}
	return (0);
}

unp_disconnect(unp)
	struct unpcb *unp;
{		
	/*
	 * N.B. enter with so_lock held.
	 * Datagram special note:  Better have global sema held!  The
	 * for loop below walks across and modifies arbitary unpcb's!!
	 */
	register struct unpcb *unp2 = unp->unp_conn;

	unp->unp_conn = (struct unpcb *)NULL;

	switch (unp->unp_socket->so_type) {

	case SOCK_DGRAM:	/* N.B.: global sema is held */
		if (unp2->unp_refs == unp)
			unp2->unp_refs = unp->unp_nextref;
		else {
			unp2 = unp2->unp_refs;
			for (;;) {
				ASSERT(unp2 != (struct unpcb *)NULL,
						"unp_disconnect");
				if (unp2->unp_nextref == unp)
					break;
				unp2 = unp2->unp_nextref;
			}
			unp2->unp_nextref = unp->unp_nextref;
		}
		unp->unp_nextref = (struct unpcb *)NULL;
		unp->unp_socket->so_state &= ~SS_ISCONNECTED;
		break;

	case SOCK_STREAM:
		soisdisconnected(unp->unp_socket);
		unp2->unp_conn = (struct unpcb *)NULL;
		soisdisconnected(unp2->unp_socket);
		break;
	}
	return;
}

/*ARGSUSED*/
unp_usrclosed(unp)
	struct unpcb *unp;
{
	return;
}

unp_drop(unp, errno)
	struct unpcb *unp;
	int errno;
{
	/*
	 * This is gross, but this is dynix.  Basically this routine
	 * is now used as a convient way for unp_detach to notify any
	 * connected datagram sockets that the socket they were attached
	 * to has now gone away.  These sockets will never be on any
	 * socket queue, so there's no need to check -- we're just marking
	 * an error and clearing some pointers, not blowing them away.
	 *
	 * N.B. enter with so_lock held and global sema held, as unp_disconnect
	 * walks across and modifies arbitrary unpcb's!
	 */

	struct socket *so = unp->unp_socket;

        so->so_error = errno;

	if (unp->unp_conn) {
		unp_disconnect(unp);
	}

	return;
}

unp_drop_sock(unp, errno)
	struct unpcb *unp;
	int errno;
{
	/*
	 * This is gross, but this is dynix.  Basically we get here
	 * by calling soabort, which calls through the protosw to PRU_ABORT.
	 * Soabort is called in two cases: to get rid of embrionic connections
	 * when a listen socket is going away, and to recover from an
	 * accept error.  The later never occurs, and since soclose calls
	 * soqremque before calling us, we've no longer the knowledge
	 * that we ever were on a socket queue.  But we are, nevertheless
	 * the last to see the socket, so its our responsibility to get rid 
	 * of it.  This is not the case for unp_drop, which gets called
	 * from unp_detach as described above.
	 * 
	 * N.B. enter with so_lock held and global sema held, as unp_disconnect
	 * walks across and modifies arbitrary unpcb's!
	 */
	struct socket *so = unp->unp_socket;
	struct socket_peer *ssop;

        so->so_error = errno;
	if (unp->unp_conn) {
		unp_disconnect(unp);
	}

	/*
	 * release *most* resources -- we've already been soqremque'd
	 * but soabort still has a reference to our socket peer
	 */

	so->so_pcb = (caddr_t) 0;
	m_freem(unp->unp_addr);
	(void) m_free(dtom(unp));
	ssop = so->so_sopp;
	sofree(so);
	ssop->sop_refcnt--;

	return;
}
