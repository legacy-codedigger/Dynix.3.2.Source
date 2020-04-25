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
static	char	rcsid[] = "$Header: if_loop.c 2.11 1991/05/10 23:09:33 $";
#endif

/*
 * if_loop.c
 * 	Loopback interface driver for protocol testing and timing.
 */

/* $Log: if_loop.c,v $
 *
 *
 */

#define	PROMISCUOUS	/* allow promiscuous kernel */
#define	AT		/* APPLETALK */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/vmmeter.h"
#include "../h/vmsystm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#ifdef AT
#include "../h/socketvar.h"
#endif AT
#include "../h/errno.h"
#include "../h/ioctl.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/vmparam.h"
#include "../machine/plocal.h"

#define	LOMTU	(1024+512)

#ifdef PROMISCUOUS
#include "../net/promisc.h"
#include "../netinet/if_ether.h"
#endif PROMISCUOUS

#ifdef AT

#include "../netat/atalk.h"
#include "../netat/katalk.h"

#endif AT

extern	struct	ifnet loif[];
extern	int num_loif;
int	looutput(), loioctl();

loattach()
{
	register struct ifnet *ifp;
	int i;

	for (i = 0; i < num_loif; i++) {
		ifp = &loif[i];

		ifp->if_name = "lo";
		ifp->if_unit = i;
		ifp->if_mtu = LOMTU;
		ifp->if_flags = IFF_LOOPBACK;
		ifp->if_ioctl = loioctl;
		ifp->if_output = looutput;
		init_lock(&ifp->if_snd.ifq_lock, G_IFNET);
		if_attach(ifp);

#ifdef AT
		ifpsetnet(ifp, 0);
		ifpsetnode(ifp, 1);
#endif AT
	}
}

looutput(ifp, m0, dst)
	struct ifnet *ifp;
	register struct mbuf *m0;	/* note: released if dropped */
	struct sockaddr *dst;
{
	spl_t splevel;
	register struct ifqueue *ifq;
	struct mbuf *m;
#ifdef PROMISCUOUS
	struct mbuf *mpromisc;
	struct promiscif * mp;
	struct ether_header *hp;
#endif PROMISCUOUS

	/*
	 * two locks are held here because the loopback effectively both
	 * transmits and receives.  Therefore, it increments both the number of
	 * if_opackets and if_ipackets.  It only increments if_ipackets if the
	 * packet can be ENQUEUED on the ipintrq.  The ipintrq is locked as if
	 * this were a network interface driver which it is.
	 *
	 * Note, this does not deadlock since transmission is never in
	 * the other direction and therefore the locks are not ever
	 * both acquired by anything else.
	 */

	if (m0 == (struct mbuf *) NULL)
		return(ENOBUFS);

	if ((ifp->if_flags & IFF_UP) == 0) {
		m_freem(m0);
		ifp->if_odiscards++;
		return(ENETDOWN);
	}

	/*
	 * lock ifp (loif)
	 */

	splevel = IF_LOCK(&ifp->if_snd); /* spl -> SPLIMP */
	ifp->if_opackets++;

#ifdef PROMISCUOUS
	if (promiscon) { /* promiscon => copy packet to promiscintrq. */

		ifq = &promiscq;

		/*
		 * get an mbuf for passing loif to promiscintr too
		 * and COPY mbuf chain for promiscq.
		 * ALSO get an mbuf for the "ether_header".
		 */

		mpromisc = m_getm(M_DONTWAIT, MT_HEADER, 2);
		if(mpromisc) {
			IF_UNLOCK(&ifp->if_snd, splevel);
			mpromisc->m_next->m_next = m0;
			mpromisc->m_next->m_off = MMINOFF;
			mpromisc->m_next->m_len = sizeof(struct ether_header);
			mpromisc->m_off = MMINOFF;
			mpromisc->m_len = sizeof(struct promiscif);
			mp = mtod(mpromisc, struct promiscif *);
			mp->promiscif_ifnet = (caddr_t) &loif[0];
			mp->promiscif_flag = PROMISC_LOOP;
			hp = mtod(mpromisc->m_next, struct ether_header *);
			bzero((caddr_t) hp, sizeof(struct ether_header));

			switch (dst->sa_family) {
			case AF_INET:
				hp->ether_type = htons(ETHERPUP_IPTYPE);
				break;
			case AF_APPLETALK:
				hp->ether_type = htons(ETHERPUP_ATALKTYPE);
				break;
			default:
				break;
			}

			hp->ether_shost[5] = 0x7f;

			/*
			 * use a fake destination address to distinquish
			 * loopback packets from packets from net.
			 */

			bcopy((caddr_t)loopbackfakeaddr,
				(caddr_t)hp->ether_dhost, 6);

			splevel = IF_LOCK(ifq);
			if (IF_QFULL(ifq)) {
				IF_DROP(ifq);
				IF_UNLOCK(ifq, splevel);
				m_freem(mpromisc);
				return(ENOBUFS);
			}
			IF_ENQUEUE(ifq, mpromisc);
			if (!ifq->ifq_busy) {
				schednetisr(NETISR_PROMISC);
			}
			IF_UNLOCK(ifq, splevel);
			return(0);
		}
	}

	/*
	 * note, if no mbufs for promisc data - continue and
	 * try to send it standard loopback => monitor misses it.
	 */

#endif PROMISCUOUS

	/*
	 * ifp passing
	 * Place interface pointer before the data
	 * for the receiving protocol.
	 */

	if (m0->m_off <= MMAXOFF &&
	    m0->m_off >= MMINOFF + sizeof(struct ifnet *)) {
		m0->m_off -= sizeof(struct ifnet *);
		m0->m_len += sizeof(struct ifnet *);
	} else {
		MGET(m, M_DONTWAIT, MT_HEADER);
		if (m == (struct mbuf *)0) {
			ifp->if_idiscards++;
			IF_UNLOCK(&ifp->if_snd, splevel);
			m_freem(m0);
			return (ENOBUFS);
		}
		m->m_off = MMINOFF;
		m->m_len = sizeof(struct ifnet *);
		m->m_next = m0;
		m0 = m;
	}
	*(mtod(m0, struct ifnet **)) = ifp;

	switch (dst->sa_family) {
	case AF_INET:
		ifq = &ipintrq;
		(void) IF_LOCK(ifq);	/* lock the ip input queue */
		if (IF_QFULL(ifq)) {	/* full queue? */
			IF_DROP(ifq);
			IF_UNLOCK(ifq, SPLIMP);
			IF_UNLOCK(&ifp->if_snd, splevel);
			m_freem(m0);
			return (ENOBUFS);
		}

		/*
		 * everything OK so far - queue it up
		 */

		IF_ENQUEUE(ifq, m0);

		/*
		 * if the queue is busy, schednetisr unecessary since
		 * ipintr currently working on it.
		 */

		if(!(ifq->ifq_busy)) {
			IF_UNLOCK(ifq, SPLIMP);
			schednetisr(NETISR_IP);
		}else
			IF_UNLOCK(ifq, SPLIMP);
		break;

#ifdef AT
	case AF_APPLETALK:
		ifq = &ddpintq;

		/*
		 * Dangerous hack -- pass ifp # up to higher levels
		 * TODO - convert AT to 4.3 ifp passing
		 */

		(void) IF_LOCK(ifq);	/* lock the ddp input queue */
		if (IF_QFULL(ifq)) {
			IF_DROP(ifq);
			IF_UNLOCK(ifq, SPLIMP);
			IF_UNLOCK(&ifp->if_snd, splevel);
			m_freem(m0);
			return (0);	/* ENOBUFS can cause infinite send! */
		}

		IF_ENQUEUE(ifq, m0);

		/*
		 * if the queue is busy, schednetisr unecessary since
		 * ddtintr currently working on it.
		 */

		if(!(ifq->ifq_busy)) {
			IF_UNLOCK(ifq, SPLIMP);
			schednetisr(NETISR_DDP);
		}else
			IF_UNLOCK(ifq, SPLIMP);
		break;
#endif AT

	default:
		ifp->if_oerrors++;
		IF_UNLOCK(&ifp->if_snd, splevel);
		m_freem(m0);
		return (EAFNOSUPPORT);
	}
	ifp->if_ipackets++;
	IF_UNLOCK(&ifp->if_snd, splevel);
	return (0);
}

/*
 * Process an ioctl request.
 */

/* ARGSUSED */
loioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	int error = 0;
	spl_t splevel;

	/*
	 * this lock only guards against concurrent ioctl's - generally, the
	 * access to the if_flags and if_addr is not mutex'd since it
	 * does not change very often (typically NEVER).  This means that
	 * during a SIOCSIFADDR, race conditions can arise such that a
	 * program gets an invalid address compare.  This is considered
	 * safe enough.
	 */

	splevel = IF_LOCK(&ifp->if_snd);	/* SPLIMP */

	switch (cmd) {

	case SIOCSIFADDR:

		ifp->if_flags |= IFF_UP;

		/*
		 * Everything else is done at a higher level.
		 */

		break;

	default:
		error = EINVAL;
	}
	IF_UNLOCK(&ifp->if_snd, splevel);
	return (error);
}
