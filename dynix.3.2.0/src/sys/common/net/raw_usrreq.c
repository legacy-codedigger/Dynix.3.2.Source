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
static	char	rcsid[] = "$Header: raw_usrreq.c 2.11 1991/04/30 23:52:34 $";
#endif

#undef	RAW_ETHER
#define RAW_ETHER

/*
 * raw_usrreq.c
 *	Raw interfaces
 */

/* $Log: raw_usrreq.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/netisr.h"
#include "../net/raw_cb.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/vmparam.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

struct rawcb rawcb;			/* head of list */

			/*
			 * NOTE rawif is used as an ifqueue only,
			 * i.e. NO OUTPUT through if_snd!
			 * anticipate use of stats.
			 */
struct ifnet rawif;	

/*
 * Initialize raw connection block q.
 */

raw_init()
{

	rawcb.rcb_next = rawcb.rcb_prev = &rawcb;
	rawif.if_snd.ifq_maxlen = raw_ifq_maxlen;

	rawcb.rcb_sopp = (struct socket_peer *) &rawcb.rcb_sop;
	rawcb.rcb_sop.sop_refcnt = 1;

	init_lock(&rawcb.rcb_sop.sop_lock, G_SOCK);
	init_lock(&rawif.if_snd.ifq_lock, G_IFNET);
}

/*
 * Raw protocol interface.
 */

raw_input(m0, proto, src, dst)
	struct mbuf *m0;
	struct sockproto *proto;
	struct sockaddr *src, *dst;
{
	register struct mbuf *m;
	struct raw_header *rh;
	spl_t splevel;

	/*
	 * Rip off an mbuf for a generic header.
	 *
	 * N.B. M_DONTWAIT argument is not used (also not used in 4.2)
	 */

	m = m_get(M_DONTWAIT, MT_HEADER);

	if (m == (struct mbuf *)NULL) {
		m_freem(m0);
		return;
	}

	m->m_next = m0;

	/*
	 * ifp passing - accommodate so we can use rawintr for
	 * rawE packets.
	 */

	m->m_len = sizeof(struct raw_header) + sizeof(struct ifnet*);
	*(mtod(m, struct ifnet **)) = (struct ifnet*) 0;/* don't care */
	m->m_off += sizeof(struct ifnet*);
	rh = mtod(m, struct raw_header *);
	m->m_off -= sizeof(struct ifnet*);

	rh->raw_dst = *dst;
	rh->raw_src = *src;
	rh->raw_proto = *proto;

	/*
	 * Header now contains enough info to decide
	 * which socket to place packet in (if any).
	 * Queue it up for the raw protocol process
	 * running at software interrupt level.
	 */

	splevel = IF_LOCK(&rawif.if_snd);	/* lock raw input queue */

	if (IF_QFULL(&rawif.if_snd)) {		/* queue full */
		IF_DROP(&rawif.if_snd);
		IF_UNLOCK(&rawif.if_snd, splevel);
		m_freem(m);
		return;
	}

	IF_ENQUEUE(&rawif.if_snd, m);
	if (!(rawif.if_snd.ifq_busy)) {
		IF_UNLOCK(&rawif.if_snd, splevel);
		schednetisr(NETISR_RAW);
	} else {
		IF_UNLOCK(&rawif.if_snd, splevel);
	}
	return;
}

/*
 * Raw protocol input routine.  Process packets entered
 * into the queue at interrupt time.  Find the socket
 * associated with the packet(s) and move them over.  If
 * nothing exists for this packet, drop it.
 */

rawintr()
{
	struct mbuf *m;
	register struct rawcb *rp;
	register struct protosw *lproto;
	register struct raw_header *rh;
	struct socket *last, *prev;

	spl_t splevel;

	/*
	 * Get next datagram off input queue and get raw header
	 * in first mbuf.
	 */

	splevel = IF_LOCK(&rawif.if_snd);
	if (rawif.if_snd.ifq_busy) {
		IF_UNLOCK(&rawif.if_snd, splevel);
		return;
	}
	rawif.if_snd.ifq_busy = 1;
	goto skiplock;
next:
	splevel = IF_LOCK(&rawif.if_snd);
skiplock:

	/*
	 * if queue is empty, unbusies queue and m == null
	 */

	IF_DEQUEUE(&rawif.if_snd, m);

	/*
	 * unlock so more data can be queued right away
	 */

	IF_UNLOCK(&rawif.if_snd, splevel);

	if (m == (struct mbuf *)NULL)
		return;

	IF_ADJ(m);

	if (m == (struct mbuf *)NULL)
		return;

	rawif.if_ipackets++;

	/*
	 * pullup at least a raw_header
	 */

	if (((m->m_off > MMAXOFF) || (m->m_len < sizeof(struct raw_header)))
	      && ((m = m_pullup(m, sizeof(struct raw_header))) == 0))
		goto next;	/* m is already freed */

	rh = mtod(m, struct raw_header *);

	last = (struct socket *)NULL;

	/*
	 * raw_usrreq does its raw_cb lookup here, first locking list and
	 * then locking each individual raw_cb to look at it.
	 * Note that to avoid deadlock other processes that access both
	 * the list and elements in the list must lock in the same order.
	 * Usually this means that a WINDOW is opened by the higher layers
	 * when binding and detaching.
	 */

	splevel = RCB_LOCK();	/* lock rcb list */

	/*
	 * This algorithm allows for multiple matches in the list.
	 * sbappendaddr's are done for m_copies of the mbuf chain until
	 * the last one to which m is sbappendaddr
	 */

	for (rp = rawcb.rcb_next; rp != &rawcb; rp = rp->rcb_next) {

		(void) SOLOCK(rp->rcb_socket);

		if (rp->rcb_head == (struct rawcb *)NULL) {
			/*
			 * rcb in process of being removed
			 */
			SOUNLOCK(rp->rcb_socket, SPLNET);
			continue;
		}

		lproto = rp->rcb_socket->so_proto;

#ifdef notyet
		if (lproto->pr_family != rh->raw_proto.sp_family){
			SOUNLOCK(rp->rcb_socket, SPLNET);
			continue;
		}
#endif notyet
		if (lproto->pr_protocol &&
		    lproto->pr_protocol != rh->raw_proto.sp_protocol) {
			SOUNLOCK(rp->rcb_socket, SPLNET);
			continue;
		}

		/*
		 * We assume the lower level routines have
		 * placed the address in a canonical format
		 * suitable for a structure comparison.
		 */

#define equal(a1, a2) \
	(bcmp((caddr_t)&(a1), (caddr_t)&(a2), sizeof (struct sockaddr)) == 0)

		if ((rp->rcb_flags & RAW_LADDR) &&
		    !equal(rp->rcb_laddr, rh->raw_dst)) {
			SOUNLOCK(rp->rcb_socket, SPLNET);
			continue;
		}
		if ((rp->rcb_flags & RAW_FADDR) &&
		    !equal(rp->rcb_faddr, rh->raw_src)) {
			SOUNLOCK(rp->rcb_socket, SPLNET);
			continue;
		}

		prev = last;
		last = rp->rcb_socket;

		SOUNLOCK(rp->rcb_socket, SPLNET);

		if (prev) {
			struct mbuf *n;

			(void) SOLOCK(prev);

			/*
			 * N.B. raw_cb and peer socket are LOCKED
			 */

			if ((n = m_copy(m->m_next, 0, (int)M_COPYALL))) {
				if (sbappendaddr(&prev->so_rcv, &rh->raw_src,
				    n, (struct mbuf *)NULL) == 0)
					m_freem(n);
				else
					sorwakeup(prev);
			} 
			SOUNLOCK(prev, SPLNET);
		}
	}
	if (last) {
		(void) SOLOCK(last);
		if (sbappendaddr(&last->so_rcv, &rh->raw_src,
		    m->m_next, (struct mbuf *)0) == 0)
			m_freem(m->m_next);
		else
			sorwakeup(last);
		(void) m_free(m);		/* header */
		SOUNLOCK(last, SPLNET);
	} else {
		rawif.if_ierrors++;
		if (rh->raw_proto.sp_protocol == AF_INET)
			ip_unknownprot();
		m_freem(m);
	}

	RCB_UNLOCK(splevel);
	goto next;
}

/*ARGSUSED*/
raw_ctlinput(cmd, arg)
	int cmd;
	caddr_t arg;
{

	if (cmd < 0 || cmd > PRC_NCMDS)
		return;
	/* INCOMPLETE */
}

/*ARGSUSED*/
raw_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	register struct rawcb *rp = sotorawcb(so);
	register int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (rights && rights->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (rp == (struct rawcb *)NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	/*
	 * Allocate a raw control block and fill in the
	 * necessary info to allow packets to be routed to
	 * the appropriate raw interface routine.
	 */

	case PRU_ATTACH:
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		if (rp) {
			error = EINVAL;
			break;
		}
		error = raw_attach(so);
		break;

	/*
	 * Destroy state just before socket deallocation.
	 * Flush data or not depending on the options.
	 */

	case PRU_DETACH:
		if (rp == (struct rawcb *)NULL) {
			error = ENOTCONN;
			break;
		}
		raw_detach(rp);
		break;

	/*
	 * If a socket isn't bound to a single address,
	 * the raw input routine will hand it anything
	 * within that protocol family (assuming there's
	 * nothing else around it should go to). 
	 */

	case PRU_CONNECT:
		if (rp->rcb_flags & RAW_FADDR) {
			error = EISCONN;
			break;
		}
		raw_connaddr(rp, nam);
		soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		goto release;

	case PRU_BIND:
		if (rp->rcb_flags & RAW_LADDR) {
			error = EINVAL;			/* XXX */
			break;
		}
		error = raw_bind(so, nam);
		break;

	case PRU_DISCONNECT:

		if ((rp->rcb_flags & RAW_FADDR) == 0) {
			error = ENOTCONN;
			break;
		}

		soisdisconnected(so);

		/* note: sofrees(rp->rp_socket) */

		raw_disconnect(rp);
		break;

	case PRU_SHUTDOWN:

		/*
		 * Mark the connection as being incapable of further input.
		 */

		socantsendmore(so);
		break;

	case PRU_SEND:

	{
		struct rawcb lrawcb;

		/*
		 * Ship a packet out.  The appropriate raw output
		 * routine handles any massaging necessary.
		 *
		 * NOTE: lrawcb used to avoid collision of xmit/rcv
		 */

		lrawcb = *rp;		/* struct copy */

		if (nam) {
			if (rp->rcb_flags & RAW_FADDR) {
				error = EISCONN;
				break;
			}
			raw_connaddr(&lrawcb, nam);
		} else if ((rp->rcb_flags & RAW_FADDR) == 0) {
			error = ENOTCONN;
			break;
		}

		/*
		 * note use of lrawcb
		 */

		error = (*so->so_proto->pr_output)(&lrawcb, m);

		if(nam && lrawcb.rcb_route.ro_rt)
			RTFREE(lrawcb.rcb_route.ro_rt);
		m = (struct mbuf *)NULL;
		break;
	}

	case PRU_ABORT:

		soisdisconnected(so);
		raw_disconnect(rp);	/* sofrees rp->rp_socket */
		sofree(so);
		break;

	case PRU_SENSE:

		/*
		 * stat: don't bother with blocksize.
		 */

		return(0);

	/*
	 * Not supported.
	 */

	case PRU_RCVOOB:
	case PRU_RCVD:
		return(EOPNOTSUPP);	/* do not m_free(m) */

	case PRU_ACCEPT:
	case PRU_LISTEN:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		bcopy((caddr_t)&rp->rcb_laddr, mtod(nam, caddr_t),
		    sizeof (struct sockaddr));
		nam->m_len = sizeof (struct sockaddr);
		break;

	case PRU_PEERADDR:
		bcopy((caddr_t)&rp->rcb_faddr, mtod(nam, caddr_t),
		    sizeof (struct sockaddr));
		nam->m_len = sizeof (struct sockaddr);
		break;

	default:
		panic("raw_usrreq");
	}
release:
	if (m != (struct mbuf *)NULL)
		m_freem(m);
	return (error);
}

#ifdef RAW_ETHER

/*
 * rawE additions to accommodate raw packets coming from Ethernet
 */

#include "../h/domain.h"

extern int raw_input(), rawE_output(), raw_usrreq();

extern struct domain rawEdomain;

struct protosw rawEsw[] = {
{ SOCK_RAW,	&rawEdomain,	0,		PR_ATOMIC|PR_ADDR,
  raw_input,	rawE_output,	0,		0,
  raw_usrreq,
  0,		0,		0,	0,
}
};

struct domain rawEdomain =
    { AF_RAWE, "rawE", 0, 0, 0, rawEsw,
		&rawEsw[sizeof(rawEsw)/sizeof(rawEsw[0])] };

#include "../sec/sec.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/if_ether.h"
#include "../netif/if_se.h"

extern struct se_state *se_state;	/* pointer to array of sec structs */
extern int se_max_unit;

/*ARGSUSED*/
rawE_output(rp, m)
	struct rawcb *rp;
	struct mbuf * m;
{
	struct ifnet *ifp;
	struct se_state * softp;
	struct sockaddr dest;
	struct sockaddr_rawE *rawEp;
	int unit;
	int error = 0;
	struct ether_header* hp;

	rawEp = (struct sockaddr_rawE *)&rp->rcb_faddr;
	
	unit = rawEp->rawE_unit;

	if (unit < 0 || unit > se_max_unit) 
		return (EINVAL);

	ifp = (struct ifnet *) &se_state[unit];

	/*
	 * set up for se_output to copy the ether header
	 * since it only knows about AF_INET and AF_UNSPEC.
	 */

	dest.sa_family = AF_UNSPEC;
	bcopy(mtod(m, caddr_t), (caddr_t) dest.sa_data, 14);

	/*
	 * check for self address
	 * NOTE: does not check for other controllers.
	 */

	softp = (struct se_state *) ifp;
	if (bcmp((char *)softp->ss_arp.ac_enaddr,
			    (char *)dest.sa_data, 6) == 0) {

		struct ether_header * eh;

		/*
		 * insert source address
		 */

		eh = mtod(m, struct ether_header *);
		bcopy((caddr_t)softp->ss_arp.ac_enaddr,
				(caddr_t) eh->ether_shost, 6);
		error = rawE_loop(m, ifp);
		return (error);
	}

	/*
	 * must fool se_output into transmitting this non-INET
	 * packet by byte swapping ether_type, removing ether_header
	 * from mbuf chain and specifying AF_UNSPEC.  rawE_loop
	 * takes as is.
 	 */

	hp = (struct ether_header *) dest.sa_data;
	hp->ether_type = ntohs(hp->ether_type);

	m->m_off += sizeof(struct ether_header);
	m->m_len -= sizeof(struct ether_header);

	error = (*ifp->if_output)(ifp, m, &dest);
	return (error);
}

rawE_loop(m, ifp)
	struct mbuf * m;
	struct ifnet *ifp;
{
	struct mbuf * mrh;
	struct raw_header *rh;
	struct ether_header * hp;
	spl_t	splevel;

	/*
	 * this can probably be done better by setting up dest and
	 * calling raw_input()
	 */

	mrh = m_getclrm(M_DONTWAIT, MT_DATA, 1);
	if (mrh == (struct mbuf *)NULL) {
		m_freem(m);
		return (ENOBUFS);
	}
	
	/*
	 * link the raw_header into the ether packet for 4.2
	 * compatibility (?)
	 */

	mrh->m_next = m;
	m = mrh;

	/*
	 * ifp passing
	 * Place interface pointer before the data
	 * for the receiving protocol.
	 */

	*(mtod(m, struct ifnet **)) = &rawif;
	m->m_off += sizeof(struct ifnet *);
	m->m_len = sizeof(struct raw_header) + sizeof(struct ifnet *);

	rh = mtod(mrh, struct raw_header*);

	/*
	 * set up raw header, using type as sa_data for bind.
	 * raw_input() could do this if static struct set up.
	 */

	rh->raw_proto.sp_family = AF_RAWE;

	/*
	 * for now assign NULL for protocol
	 */

	rh->raw_proto.sp_protocol = AF_UNSPEC;

	/*
	 * copy AF_RAWE and ether_type in for dst addr
	 */

	rh->raw_dst.sa_family = AF_RAWE;

	hp = mtod(m->m_next, struct ether_header *);

	bcopy((caddr_t)&hp->ether_type,
		(caddr_t)rh->raw_dst.sa_data, 2);

	bcopy((caddr_t)&hp->ether_type,
		(caddr_t)rh->raw_src.sa_data, 2);

	/*
	 * copy AF_RAWE and if_unit # in for src addr
 	 */

	rh->raw_src.sa_family = AF_RAWE;

	bcopy((caddr_t)&ifp->if_unit,
		(caddr_t)&rh->raw_src.sa_data[2], sizeof(short));

	m->m_off -= sizeof(struct ifnet *);

	/*
	 * lock the raw input queue
	 */

	splevel = IF_LOCK(&rawif.if_snd);

	if (IF_QFULL(&rawif.if_snd)) { /* queue is full, droppit */
		IF_DROP(&rawif.if_snd);
		IF_UNLOCK(&rawif.if_snd, splevel);
		m_freem(m);
		return (ENETDOWN);
	}

	IF_ENQUEUE(&rawif.if_snd, m);
	if (!(rawif.if_snd.ifq_busy)) {
		IF_UNLOCK(&rawif.if_snd, splevel);
		schednetisr(NETISR_RAW);
	} else 
		IF_UNLOCK(&rawif.if_snd, splevel);

	return (0);
}
#endif RAW_ETHER
