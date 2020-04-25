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

#undef	RAW_ETHER
#define RAW_ETHER

#ifndef	lint
static	char	rcsid[] = "$Header: raw_cb.c 2.3 87/04/30 $";
#endif

/*
 * raw_cb.c
 *	Raw control block handline routines
 */

/* $Log:	raw_cb.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../netinet/in.h"

/*
 * Routines to manage the raw protocol control blocks. 
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

raw_attach(so)
	register struct socket *so;
{
	struct mbuf *m;
	register struct rawcb *rp;

	register struct socket_peer * sop;
	spl_t splevel;

	/* N.B. M_DONTWAIT argument is not used (also not used in 4.2) */

	m = m_getclr(M_DONTWAIT, MT_PCB);

	if (m == (struct mbuf *) NULL)
		return (ENOBUFS);

	if (sbreserve(&so->so_snd, rawsndq) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, rawrcvq) == 0)
		goto bad2;

	rp = mtod(m, struct rawcb *);

	/* N.B. M_DONTWAIT argument is not used (also not used in 4.2) */

	m = m_getclr(M_DONTWAIT, MT_SOPEER);
	if(m == (struct mbuf *) NULL) {
		(void) m_free(dtom(rp));
		return(ENOBUFS);
	}

	sop = mtod(m, struct socket_peer *);
	init_lock(&sop->sop_lock, G_SOCK);

	sop->sop_refcnt = 1;	/* reference count starts with no ref
					from pr_input */

	so->so_sopp = (struct socket_peer *) sop;

	/*
	 * init rcb lock which is rcb_sop.sop_lock -
	 * however - using explicitly allocated socket_peer now to
	 * 	establish mutex.
	 * used in socket_peer lock protocol and in head of rawcb lists
	 * 	initialized in raw_init.
	 */

	rp->rcb_sopp = (struct socket_peer *) sop;

	/* NOTE: points to explicit socket_peer.  */

	init_lock(&(rp->rcb_sopp->sop_lock), G_SOCK);

	rp->rcb_socket = so;
	rp->rcb_pcb = (caddr_t) NULL;

	so->so_pcb = (caddr_t)rp;

	rp->rcb_sopp = sop;

	sop->sop_refcnt++;	/* we now have another reference to it */

	/*
	 * NOTE: since no reference yet, this cb is not locked so list
	 * deadlock is not a consideration.
	 */

	splevel = RCB_LOCK();	/* lock rawcb list */
	insque(rp, &rawcb);
	rp->rcb_head = &rawcb;
	RCB_UNLOCK(splevel);	/* unlock rawcb list */

	return(0);
bad2:
	sbrelease(&so->so_snd);
bad:
	(void) m_free(m);
	return (ENOBUFS);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */

raw_detach(rp)
	register struct rawcb *rp;
{
	struct socket 		*so = rp->rcb_socket;
	struct rawcb		*head;
	struct socket_peer	*saved_sopp;

	head = rp->rcb_head;
	rp->rcb_head = (struct rawcb*) NULL;
	saved_sopp = rp->rcb_sopp;

	if(head) {
		so->so_pcb = (caddr_t) NULL;
		if (rp->rcb_route.ro_rt)
			rtfree(rp->rcb_route.ro_rt);
		if (rp->rcb_options)
			m_freem(dtom(rp->rcb_options));
		sofree(so);
		(void) v_lock(&saved_sopp->sop_lock, SPLNET);
		(void) RCB_LOCK();	/* lock rawcb list */
		(void)p_lock(&saved_sopp->sop_lock, SPLNET);
		remque(rp);
		RCB_UNLOCK(SPLNET);	/* unlock rawcb list */
		m_freem(dtom(rp));
	}
	if(--(saved_sopp->sop_refcnt) == 0)
		(void) m_free(dtom(saved_sopp));
}

/*
 * Disconnect and possibly release resources.
 */

raw_disconnect(rp)
	struct rawcb *rp;
{

	rp->rcb_flags &= ~RAW_FADDR;
	if (rp->rcb_socket->so_state & SS_NOFDREF)
		raw_detach(rp);		/* sofrees rp->rcb_socket */
}

raw_bind(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
	register struct rawcb *rp;

	if (ifnet == (struct ifnet*) NULL)
		return (EADDRNOTAVAIL);

	rp = sotorawcb(so);

/* BEGIN DUBIOUS (sic 4.2bsd) */

	/*
	 * Should we verify address not already in use?
	 * Some say yes, others no.
	 */

	/*
	 * above policy eases the mutex problem since the list does not
	 * have to be looked into for a match and thereby forcing a SOUNLOCK
	 * WINDOW etc.
	 */

	switch (addr->sa_family) {

#ifdef INET
	case AF_IMPLINK:
	case AF_INET: {
		if (((struct sockaddr_in *)addr)->sin_addr.s_addr &&
		    ifa_ifwithaddr(addr) == (struct ifaddr *) NULL) {
			return (EADDRNOTAVAIL);
		}
		break;
	}
#endif

#ifdef RAW_ETHER

	case AF_RAWE:

		/*
		 * unconditionally, copy addr into sockets rcb
		 * AF_RAWE sockaddr looks like:
		 * 	sa_family == AF_RAWE
		 * 	sa_data = hostorder(ether_type) (zero fill to 16)
		 *		  ifp->if_unit;
		 */

		rp->rcb_flags |= RAW_DONTROUTE;
		break;

#endif RAW_ETHER

	default: 
		return (EAFNOSUPPORT);

	}	/* end switch */

/* END DUBIOUS */

	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_laddr, sizeof (*addr));
	rp->rcb_flags |= RAW_LADDR;
	return (0);
}

/*
 * Associate a peer's address with a
 * raw connection block.
 */

raw_connaddr(rp, nam)
	struct rawcb *rp;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);

	bcopy((caddr_t)addr, (caddr_t)&rp->rcb_faddr, sizeof(*addr));
	rp->rcb_flags |= RAW_FADDR;
}
