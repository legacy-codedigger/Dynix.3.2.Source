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
static	char	rcsid[] = "$Header: ddp_usrreq.c 1.10 1991/05/10 23:08:58 $";
#endif

/* $Log: ddp_usrreq.c,v $
 *
 */

/*	ddp_usrreq.c	1.0	85/02/25	*/

/* 
 * Idea -- Insert appletalk (ddp) handler between if layer and socket layer
 *	  with minimal effect on other parts of system.  Ie, don't 
 *	  "fix" the rest of the system to deal more generically with
 *	  sockaddr's; rather, use (alleged) hacks.
 * Files affected:
 *	- if_xx.c -- see above; also notice outgoing appletalk dgrams and
 *	  fill in appropriate values for lap and the enet type; also, notice
 *	  incomming appletlak dgrams and (sneakily) pass "ifp" to ddpintr().
 *	  note:  this includes if_loop.c...
 *	- if_ether.c --  if subject "internet" address has no bits except
 *	  in the low byte, it is an appletalk node number.  (hack)
 *	- proto.c (4.2: in_proto.c) ddp protocol switch entry.
 *	- netintr() (4.2: ??) must check for NETISR_DDP
 *	- Several defines are in the makefile.  They should eventually go into
 *	  header files somewhere.
 * Note: 
 *	Go to some pains to get/keep ddp's short-alligned to make access
 * 	of shorts possible without resorting to byte-at-a-time operations.
 *	TODO -- make netstat program know about ddpstat's; is there
 *		a "loopback net"?; For each reboot
 * 		a node number uniquely defines an if.
 */

#define	AT		/* APPLETALK */

#ifdef	AT		/* whole file is conditional */

#define	IS4DOT2		/* IS4DOT2 is vestiage of original code */

#ifdef	IS4DOT2

#include "../h/param.h"

#ifdef	sequent
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../machine/gate.h"
#include "../machine/intctl.h"

extern	int (*netisrvec[])();

#define	G_PCB	G_IFNET		/* PCB mutex gate assignment */

extern	int	ddpintr();

#endif	sequent

#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../netinet/in.h"

#else	!IS4DOT2

#define	MT_HEADER	0	/* cause lint to fail; oh, well */
#define	MT_PCB		0
#define	PRU_BIND	(PRU_NREQ+1)
#define	PRU_LISTEN	(PRU_NREQ+2)
#define	soreserve(x,y,z) 1
#include <sys/param.h>
#include "net/misc.h"
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <netinet/in.h>
#include <errno.h>

#endif	IS4DOT2

#include "../netat/atalk.h"
#include "../netat/katalk.h"

struct	sockaddr_at ddp_at = { AF_APPLETALK };

struct atcb ddpcb;

/*
 * DDP protocol implementation.
 */

ddp_init()
{
	ddpcb.at_next = ddpcb.at_prev = &ddpcb;
	ddpintq.ifq_maxlen = ddpqmaxlen;

#ifdef sequent

	/*
	 * place the ddpintr() address into the appropriate netisrvec
	 * entry as a handle to ddpintr.
	 */

	netisrvec[NETISR_DDP] = ddpintr;

	/*
	 * multiprocessing initialization
	 * NOTE: uses same gate as IP ifnet
	 */

	init_lock(&ddpintq.ifq_lock, G_IFNET);
	init_lock(&ddpcb.at_sop.sop_lock, G_PCB);

	ddpcb.at_sopp = &ddpcb.at_sop;
	ddpcb.at_lport = ATPORT_RESERVED - 1;

#endif sequent

}

/*
 * Make a short ddp into a long ddp. The only consistant way to tell anyone
 * the network from which the dgram came in from is to build a complete
 * applebus internet address -- eg, build a long ddp.
 * Assume -- we receive short ddp short-alligned
 */

sddpintr(m0, lp, dp, ifp)
	register struct mbuf *m0;
	register struct lap *lp;
	register struct ddp *dp;
	register struct ifnet *ifp;
{
	register struct ddpshort *dps;
	extern struct ifnet loif[];
	struct ifnet *lop = &loif[0];
	u_short net;

	ddpstat.ddps_shortddps++;
	dps = mtod(m0, struct ddpshort *);

	/*
	 * do simple rtmp
	 */

	if (dps->D_type == ddpTypeRTMP) {
	 	ddpstat.ddps_rtmppckts++;

		/*
		 * net number might not be short alligned
		 */

		bcopy(((caddr_t)dps)+SIZEOFDDPSHORT, (caddr_t)&net,
		  sizeof(short));

		net = ntohs(net);

		ifpsetnet(ifp, net);
		ifpsetbridge(ifp, lp->src);

		goto release;
	}
	dp->d_cksum = 0;
	dp->d_dsno = dps->D_dsno;
	dp->d_ssno = dps->D_ssno;

	/*
	 * If it came from loopback, say it was actually from the local net.
	 * Makes atforme() simpler.
	 */

	if ((net = ifpgetnet(ifp)) == ifpgetnet(lop))
		net = 0;
	else
		net = htons(net);
	bcopy((caddr_t) &net,(caddr_t) &dp->d_dnet, sizeof(short));
	bcopy((caddr_t) &net, (caddr_t) &dp->d_snet, sizeof(short));

	dp->d_snode = lp->src;
	dp->d_dnode = lp->dst;
	dp->d_type = dps->D_type;
	return 1;
release:
	m_freem(m0);
	return 0;
}

ddpintr()
{
	struct ddp d;
	register struct ddp *di = &d;
	register struct atcb *atp;
	register struct mbuf *m0, *m, *mm;
	register struct ifnet *ifp;
	struct lap lp;
	int len, l = 0, wasshort;
	struct sockaddr_at daddr, saddr;
	extern struct ifnet *ifnet;
#ifdef sequent
	spl_t splevel;
#endif sequent

	ddpstat.ddps_ddpintr++;

#ifdef sequent
	splevel = IF_LOCK(&ddpintq);
	if (ddpintq.ifq_busy) {
		IF_UNLOCK(&ddpintq, splevel);
		return;
	}
	ddpintq.ifq_busy = 1;
	goto skiplock;
again:
	splevel = IF_LOCK(&ddpintq);

skiplock:

	/*
	 * if queue is empty, unbusies queue and m == null
	 */

	IF_DEQUEUEIF(&ddpintq, m0, ifp);

	/*
	 * unlock so more data can be queued right away
	 */

	IF_UNLOCK(&ddpintq, splevel);
	if (m0 == (struct mbuf *) NULL)
		return;

#else sequent

again:
	/*
	 * Get next datagram off input queue 
	 */

	IF_DEQUEUE(&ddpintq, m0);

	if (m0 == 0)
		return;
#endif sequent

	ddpstat.ddps_ddppckts++;
	m = m0;

	if (ifp == NULL) {
		m_freem(m);
	 	ddpstat.ddps_nullifp++;
		goto again;
	}

	/*
	 * check short ddp
	 */

	if (((m->m_off > MMAXOFF) || (m->m_len < SIZEOFLAP+SIZEOFDDPSHORT))
	      && ((m = m_pullup(m, SIZEOFLAP+SIZEOFDDPSHORT)) == 0))
	{
		ddpstat.ddps_hdrops++;
		goto again;
	}

	bcopy(mtod(m, caddr_t), (caddr_t) &lp, SIZEOFLAP);

	m->m_off += SIZEOFLAP;
	m->m_len -= SIZEOFLAP;

	if (lp.type == LT_SHORTDDP) {
		bcopy(mtod(m, caddr_t), (caddr_t)di, SIZEOFDDPSHORT);
		if (sddpintr(m, &lp, di, ifp) == 0)
			goto again;
		/*
		 * compensate for different header size
		 */

		wasshort = SIZEOFDDP - SIZEOFDDPSHORT;
	}else{

		/*
		 * check for SIZEOFDDP
		 */

		if ((m->m_len < SIZEOFDDP)
	  	   && ((m = m_pullup(m, SIZEOFDDP)) == 0)) {
			ddpstat.ddps_hdrops++;
			goto again;
		}
		bcopy(mtod(m, caddr_t), (caddr_t)di, SIZEOFDDP);
		wasshort = 0;
	}

	/*
	 * get host order
	 * d_length is handled by atgetlength
	 */

	di->d_cksum = ntohs(di->d_cksum);	/* mumble */

	/*
	 *  XXX someday do checksum...
	 */

	di->d_dnet = ntohs(di->d_dnet);
	di->d_snet = ntohs(di->d_snet);

	m->m_off += SIZEOFDDP - wasshort;
	m->m_len -= SIZEOFDDP - wasshort;

	/*
	 * now, have long ddp in d.
	 * reject long ddp's not intended for this net
	 */

	if (di->d_dnet)
		if (di->d_dnet != ifpgetnet(ifp)) {
			ddpstat.ddps_wrongnet++;
			goto bad;
		}

	len = atgetlength(di) - (SIZEOFDDP - wasshort);
	if (len > DDPMAXSIZE) {
		ddpstat.ddps_badlen++;
		goto bad;
	}

	/*
	 * Make mbuf data length reflect DDP length.
	 * If not enough data to reflect DDP length, drop.
	 */

	mm = m;
	l = 0;
	while(mm) {
		l += mm->m_len;
		if(mm->m_next == 0) break;
		mm = mm->m_next;
	}
	if (len != l) {
		if (len > l) {
			ddpstat.ddps_badlen++;
			goto bad;
		}
		l = l - len;
		if (l < mm->m_len)
			mm->m_len -= l;
		else
		    m_adj(m, -l);
	}

	if (!atforme(di)) {

		ddpstat.ddps_forward++;

		/*
		return at_forward(di, m);
		*/

		goto bad;
	}

	/*
	 * Locate pcb for datagram.
	 */

	daddr.at_net = di->d_dnet;
	daddr.at_node = di->d_dnode;
	saddr.at_net = di->d_snet;
	saddr.at_node = di->d_snode;


#ifdef sequent

	/*
	 * at_pcblookup looks up an appletalk address in the pcb list.
	 * The list is locked while searching to guard against
	 * concurrent changes.
	 * When/if a pcb is found, it's sop_lock is acquired, it's
	 * sop_refcnt count is incremented and the pcb list is unlocked.
	 * If a pcb is not found that is bound to the appletalk address,
	 * NULL is returned.
	 */

#endif sequent

	if ((atp = at_pcblookup(&ddpcb, daddr.at_addr, saddr.at_addr,
			di->d_dsno&0xff, ATPLOOKUP_WILDCARD)) == NULL) {

		ddpstat.ddps_noddpcb++;
		goto bad;
	}


#ifdef sequent

	/*
	 * decrement sop_refcnt incremented by pcblookup
	 */

	atp->at_sopp->sop_refcnt--;

	/*
	 * note bene the pcb is *locked* when lookup finds it.
	 * this also locks the bound socket_peer which mutexes state changes
	 * in the bound socket.  Also note that if ddp spins on this lock
	 * because the user happens to be looking at this socket, then no
	 * other network processing goes on - i.e. a user has blocked other
	 * user data from being processed.  This data is queued on the network
	 * interface.  Possible enhancements to this approach include
	 * 1) droping the packet (after all it is "best effort") or, 2) queuing
	 * the packet aside somewhere and trying again later (might be delivered
	 * out of sequence then).
	 */

	/*
	 * check for state change here in case there is no longer a
	 * socket reference.
	 */

	if (atp->at_socket) {
		
		/*
		 * Still have socket to talk to
		 *
		 * Construct sockaddr format source address.
		 * Stuff source address and datagram in user buffer.
		 */

		ddp_at.at_sno = di->d_ssno;
		ddp_at.at_net = di->d_snet;
		ddp_at.at_node = di->d_snode;
		ddp_at.at_ptype = di->d_type;

		ddp_at.at_abridge = ifpgetbridge(ifp);

		if (sbappendaddr(&atp->at_socket->so_rcv,
		    (struct sockaddr *)&ddp_at,
		    m, (struct mbuf *)0) == 0) {
			ATP_UNLOCK(atp, SPLNET);
			ddpstat.ddps_sockerr++;
			m_freem(m);
			goto again;
		}
		sorwakeup(atp->at_socket);

	} else {

		/*
		 * socket disappeared apparently at_detach is trying
		 * to get rid of this (e.g. spinning on atpcb list lock
		 * to remque it atpcb is gone but need to unlock it so
		 * detach can finish.
		 */

		m_freem(m);
	}

	ATP_UNLOCK(atp, SPLNET);

#else  sequent

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */

	ddp_at.at_sno = di->d_ssno;
	ddp_at.at_net = di->d_snet;
	ddp_at.at_node = di->d_snode;
	ddp_at.at_ptype = di->d_type;

	/*
	 * !! - ifp may go away when we do away with carrying lap on input
	 */

	ddp_at.at_abridge = ifpgetbridge(ifp);

	if (sbappendaddr(&atp->at_socket->so_rcv, (struct sockaddr *)&ddp_at,
	  m, (struct mbuf *)0) == 0) {
		ddpstat.ddps_sockerr++;
		goto bad;
	}

	sorwakeup(atp->at_socket);

#endif sequent

	goto again;
bad:
	ddpstat.ddps_bad++;
	m_freem(m);
	goto again;
}

#ifdef notyet	/* not called */
at_forward(ddp, m0)
	register struct ddp *ddp;
	struct mbuf *m0;
{
	register struct ifnet *ifp;
	struct sockaddr_at d;
	register struct lap *lp;
	register struct mbuf *m, *n;
	register int error = ENETUNREACH;

	if (atgethops(ddp) > ATMAXHOPS)
		goto drop;
	atbumphops(ddp);	/* bump hop count */

	d.at_family = AF_APPLETALK;
	d.at_net = ddp->d_dnet;
	d.at_node = ddp->d_dnode;
	if ((ifp = atroute(&d)) == 0)
		goto drop;

	/*
	 * back to net order
	 */

	ddp->d_dnet = htons(ddp->d_dnet);
	ddp->d_snet = htons(ddp->d_snet);

	/*
	 * get an mbuf for ddp to avoid short-allignment probs.
	 */

	n = m_get(M_DONTWAIT, MT_HEADER);
	if (n == 0) {
		error = ENOBUFS;
		goto drop;
	}
	n->m_next = m0;

	/*
	 * if layer gets yet another mbuf for it's header
	 */

	n->m_off = MMINOFF;
	n->m_len = SIZEOFDDP;
	bcopy((caddr_t)ddp, mtod(n, caddr_t), SIZEOFDDP);

	/*
	 * get another mbuf for lap to avoid short-allignment probs.
	 */

	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		error = ENOBUFS;
		m0 = n;		/* to avoid eating mbuf *n */
		goto drop;
	}
	m->m_next = n;

	/*
	 * if layer gets yet another mbuf for it's header
	 */

	m->m_off = MMINOFF;
	m->m_len = SIZEOFLAP;

	lp = mtod(m, struct lap *);
	lp->type = LT_DDP;
	return (*ifp->if_output)(ifp, m, &d);

drop:
	m_freem(m0);
	return error;
}
#endif notyet

ddp_output(atp, m0)
	register struct atcb *atp;
	register struct mbuf *m0;
{
	register struct ifnet *ifp;
	register struct mbuf *m;
	register int len = 0;
	int error = 0;
	struct sockaddr_at dst;

	dst = atp->at_faddr;
	if ((ifp = atroute(&dst)) == 0) {
		error = ENETUNREACH;
		goto release;
	}

	/*
	 * Calculate data length and get a mbuf
	 * for DDP header.
	 */

	for (m = m0; m; m = m->m_next)
		len += m->m_len;

	if (len > DDPMAXSIZE) {
		error = EINVAL;
		goto release;
	}

	/*
	 * Even if room for ddp header in 1st mbuf, get another --
	 * want ddp to be short-aligned.  ddp's are odd-byted.
	 */

	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == 0) {
		error = ENOBUFS;
		goto release;
	}
	m->m_next = m0;
	m->m_off = MMINOFF;

	if ((dst.at_net == 0) || (dst.at_node == ifpgetnode(ifp)) ||
	    dst.at_net == ifpgetnet(ifp))
		return sddpoutput(ifp, atp, m, len, &dst);
	else
		return ddpoutput(ifp, atp, m, len, &dst);
release:
	m_freem(m0);
	return error;
}

sddpoutput(ifp, atp, m, len, d)
	register struct ifnet *ifp;
	register struct atcb *atp;
	register struct mbuf *m;
	register int len;
	register struct sockaddr_at *d;
{
	struct ddpshort *dp;
	struct lap *lp;
	struct mbuf *n;
	int error = 0;

	m->m_len = SIZEOFDDPSHORT;		/* real size */
	dp = mtod(m, struct ddpshort *);
	atsetlength(dp, len + SIZEOFDDPSHORT);
	dp->D_ssno = atp->at_lport;
	dp->D_dsno = atp->at_fport;
	dp->D_type = atp->atcb_ptype;

	/*
	 * get ANOTHER mbuf for lap to, again, avoid short-allignment probs.
	 */

	n = m_get(M_DONTWAIT, MT_HEADER);
	if (n == 0) {
		error = ENOBUFS;
		goto release;
	}
	n->m_next = m;
	m = n;

	/*
	 * if layer gets yet another mbuf for it's header
	 */

	m->m_off = MMINOFF;
	m->m_len = SIZEOFLAP;

	lp = mtod(m, struct lap *);
	lp->dst = d->at_node;		/* for short ddp's only */
	lp->type = LT_SHORTDDP;

	return (*ifp->if_output)(ifp, m, d);

release:
	m_freem(m);
	return error;
}

ddpoutput(ifp, atp, m, len, d)
	register struct ifnet *ifp;
	register struct atcb *atp;
	register struct mbuf *m;
	register int len;
	register struct sockaddr_at *d;
{
	struct ddp *dp;
	struct lap *lp;
	struct mbuf *n;
	int error = 0;

	m->m_len = SIZEOFDDP;			/* real size */
	dp = mtod(m, struct ddp *);
	atsetlength(dp, len + SIZEOFDDP); 	/* sets hopcount=0, too */
	dp->d_cksum = 0;			/* XXX  hmmm... */
	dp->d_dnet = htons(atp->at_fnet);
	dp->d_dnode = atp->at_fnode;
	dp->d_snet = htons(ifpgetnet(ifp));
	dp->d_snode = atp->at_lnode;
	dp->d_ssno = atp->at_lport;
	dp->d_dsno = atp->at_fport;
	dp->d_type = atp->atcb_ptype;

	/*
	 * get ANOTHER mbuf for lap, to, again, avoid short-allignment probs.
	 */

	n = m_get(M_DONTWAIT, MT_HEADER);

	if (n == 0) {
		error = ENOBUFS;
		goto release;
	}
	n->m_next = m;
	m = n;

	/*
	 * if layer gets yet another mbuf for it's header
	 */

	m->m_off = MMINOFF;
	m->m_len = SIZEOFLAP;

	lp = mtod(m, struct lap *);
	lp->type = LT_DDP;

	return (*ifp->if_output)(ifp, m, d);

release:
	m_freem(m);
	return error;
}


/*ARGSUSED*/
ddp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct atcb *atp = sotoatcb(so);
	struct sockaddr_at save_sat;
	int error = 0;
#ifdef sequent
	spl_t splevel;
#endif sequent
#ifdef IS4DOT2
	int is4dot2 = 1;
#else
	int is4dot2 = 0;
#endif

	if (is4dot2)
		if (nam)
			nam = mtod(nam, struct mbuf *);

	if (atp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		if (atp != NULL) {
			error = EINVAL;
			break;
		}
#ifdef sequent

		/*
		 * at_pcballoc gets the pcb resources, links the pcb to
		 * the socket, and sets up a socket_peer structure with
		 * a refcnt of 1.  This socket_peer provides mutex with
		 * the pr_input routines performing network services.
		 * NOTE BENE insque moved to attach routine (here) to
		 * avoid racing with timeout.  REVISIT - NBD w/ ddp
		 */

#endif sequent
		error = at_pcballoc(so, &ddpcb);

		if (error)
			break;

		if (!is4dot2) {
			if ((sbreserve(&so->so_snd, 2048) == 0)
			  || ((sbreserve(&so->so_rcv, 2048) == 0))) {
				error = ENOBUFS;
				break;
			}
			error = at_pcbbind(so, (struct sockaddr_at *) nam);
		}

		else
#ifdef sequent
			error = soreserve(so, ddp_sendspace, ddp_recvspace);
#else
			error = soreserve(so, 2048, 2048);
#endif sequent

#ifdef sequent

		/*
		 * the insque is moved to avoid race with clock on attach
		 */

		atp = sotoatcb(so);
		splevel = ATP_LOCK(&ddpcb);	/* lock the ddpcb *list* */

		/* unbound ddpcb into list */

		insque(atp, &ddpcb);
		atp->at_head = &ddpcb;

		/* now legit entry */

		ATP_UNLOCK(&ddpcb, splevel);	/* unlock the inpcb list */
#endif sequent
		break;

	case PRU_DETACH:
		if (atp == NULL) {
			error = ENOTCONN;
			break;
		}
		at_detach(atp);
		break;

	case PRU_BIND:
		if (is4dot2)
			error = at_pcbbind(so, (struct sockaddr_at *) nam);
		break;

	case PRU_LISTEN:
		if (!is4dot2)
			error = EOPNOTSUPP;
		else
			error = EINVAL;
		break;

	case PRU_CONNECT:

		if ((atp->atcb_flags & (AT_FADDR|AT_IGNADDR)) == (AT_FADDR)) 
			return (EISCONN);

		error = at_connaddr(atp, (struct sockaddr_at *) nam);
		if(!error)
			soisconnected(so);
		break;

	case PRU_ACCEPT:

		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if ((atp->atcb_flags & AT_FADDR) == 0)
			return (ENOTCONN);
#ifdef sequent
		soisdisconnected(so);
		at_disconnect(atp);
#else
		at_disconnect(atp);
		soisdisconnected(so);
#endif sequent
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:

#ifdef sequent
		{
		/*
		 * must do this differently than a monoprocessor.
		 * Since at_pcblookup does not mutex the pcb's in the
		 * ddpcb list for comparison (too expensive), it is not
		 * appropriate to use that space temporarily to
		 * store an output address.  Use a static structure to pass the
		 * necessary information for ddp_output to use.
		 */

		struct atcb local_atp;

		local_atp = *atp;	/* struct copy */

		if (nam) {
 			bcopy((caddr_t) &local_atp.at_laddr,
				(caddr_t) &save_sat, sizeof(save_sat));
			if ((atp->atcb_flags & (AT_FADDR|AT_IGNADDR)) ==
				(AT_FADDR)) {
				m_freem(m);
				return (EISCONN);
			}

			error = at_connaddr(&local_atp,
					(struct sockaddr_at *) nam);

		} else if ((atp->atcb_flags & AT_FADDR) == 0) {
			m_freem(m);
			return (ENOTCONN);
		}

		if(!error) {
			error = ddp_output(&local_atp, m);
			m = NULL;
		}

		if (nam) {	/* AUTOBIND */
			at_disconnect(atp);
 			bcopy((caddr_t) &save_sat,
				(caddr_t) &atp->at_laddr, sizeof(save_sat));
		}
		break;
		}
#else sequent
		if (nam) {
 			bcopy(&atp->at_laddr, &save_sat, sizeof(save_sat));
			if ((atp->atcb_flags & (AT_FADDR|AT_IGNADDR)) ==
				(AT_FADDR))
				return (EISCONN);

			at_connaddr(atp, nam);

		} else if ((atp->atcb_flags & AT_FADDR) == 0)
			return (ENOTCONN);
		error = ddp_output(atp, m);
		m = NULL;
		if (nam) {
			at_disconnect(atp);
 			bcopy(&save_sat, &atp->at_laddr, sizeof(save_sat));
		}
		break;
#endif sequent

	case PRU_ABORT:
		at_disconnect(atp);
#ifdef sequent
		soisdisconnected(so);
		sofree(so);
#else
		sofree(so);
		soisdisconnected(so);
#endif sequent
		break;

	case PRU_SOCKADDR:

		*((struct sockaddr_at *)nam) = atp->at_laddr;
		(dtom(nam))->m_len = sizeof(struct sockaddr_at);
		break;

	case PRU_CONTROL:
		if ((int)m == SIOIGNADDR)
			atp->atcb_flags |= AT_IGNADDR;
		else
			error = EINVAL;
		m = NULL;
		break;

	case PRU_SENSE:
		m = NULL;
		/* fall thru... */

	case PRU_RCVD:
	case PRU_RCVOOB:
	case PRU_SENDOOB:
#ifdef	IS4DOT2
	case PRU_CONNECT2:
	case PRU_PEERADDR:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
#endif
		error =  EOPNOTSUPP;
		break;

	default:
		panic("ddp_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}

/*
 * Can't/don't use standard routing routines (rtalloc()) since they are all
 * heavily oriented toward the Internet world.  Instead, use the unused
 * if_host[1] field in the if structure for appletalk much the same way the 
 * internet world uses the if_addr field.  Ideally, there should be a list of
 * protocol addresses attached to each interface.  Anyway, when we want to route
 * something, we look for the route ourselves by chasing down through all
 * the if's, much like rtalloc() itself.
 *
 * Note that this is not an rtmp router yet, just a dumb search of the if's
 * for a matching net.
 */

/*
 * if the dst net is 0, send to the first if on the list which
 * is an AT if.
 */
 
struct ifnet *
atroute(sat)
	struct sockaddr_at *sat;
{
	register struct ifnet *ifp;
	register u_short net = sat->at_net;

	if (net == 0) {
		for (ifp = ifnet; ifp; ifp = ifp->if_next) {
			if (ifpgetnode(ifp))
				return ifp;
		}
		return NULL;
	}
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifpgetnet(ifp) == net)
			return ifp;
	}
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifpgetbridge(ifp) != 0)
			return ifp;
	}
	return NULL;
}

/*
 * Routines to manage the appletalk protocol control blocks.  Should be in
 * "at_pcb.c"?
 */

/*
 * Allocate local host information.
 * Return error.
 */

at_pcbbind(so, sat)
	struct socket *so;
	register struct sockaddr_at *sat;
{
	register struct atcb *atp;
	register struct atcb *head = &ddpcb;
	u_char lport = 0;
	struct sockaddr_at saddr;
#ifdef sequent
	register struct socket_peer * saved_sopp;
#endif sequent

	if (ifnet == 0)
		return (EADDRNOTAVAIL);

	saddr.at_net = saddr.at_node = 0;

#ifdef sequent

	atp = sotoatcb(so);

	/*
	 * open WINDOW to avoid deadlock with timeout routine.
	 * unlock the pcb, then lock the pcb list, then relock
	 * the pcb (using a saved_sopp in case the socket and/or pcb
	 * disappears during the window).
	 */

	saved_sopp = atp->at_sopp;
	ATP_UNLOCK(atp, SPLNET);	/* WINDOW */
	(void) ATP_LOCK(head);
	(void) p_lock(&saved_sopp->sop_lock, SPLNET);

	/*
	 * socket and pcb could have disappeared in this window
	 * so check the refcnt.  If called from user, refcnt should
	 * be 2.  If called from net, should be 3.  Unless the
	 * socket and/or pcb are being deleted.
	 */

	if(saved_sopp->sop_refcnt <= 1) {

		/*
		 * they did disappear! - leave it to caller to
		 * clean up - but unlock the list and return
		 * EADDRNOTAVAIL
		 */

		ATP_UNLOCK(head, SPLNET);
		return (EADDRNOTAVAIL);
	}
#endif sequent

	if (sat) {
		if (sat->at_family != AF_APPLETALK) {
#ifdef sequent
			ATP_UNLOCK(head, SPLNET);
#endif sequent
			return (EAFNOSUPPORT);
		}
		lport = sat->at_sno&0xff;

		/*
		 * Dissallow on this port if:
		 * 	1.  There is already a wildcard client or
		 *	2.  There is already an exact-match client
		 */

		if (lport) {
			if (lat_pcblookup(head, sat->at_addr, saddr.at_addr,
			  lport, ATPLOOKUP_WILDCARD)) {
#ifdef sequent
				    ATP_UNLOCK(head, SPLNET);
#endif sequent
				    return (EADDRINUSE);
			}
		}
	}

#ifndef sequent	/* moved to above to get atp sooner */
	atp = sotoatcb(so);
#endif sequent

	if (sat) {
		atp->at_laddr = *sat;
		atp->atcb_ptype = sat->at_ptype;
	}
	if (lport == 0) {

		/*
		 * system assigns local port.  watchout for too many
		 * associations
		 */

		int ntimes = 0;

		do {
			if (ntimes++ > ATPORT_RESERVED) {
				ATP_UNLOCK(head, SPLNET);
				return (EADDRINUSE);
			}

			/*
			 * use int arithmetic for comparison (portable)
			 */

			if ((head->at_lport+1) > ((ATPORT_RESERVED*2)-1))
				head->at_lport = ATPORT_RESERVED;
			else
				head->at_lport++;

			lport = head->at_lport;
		}
#ifdef sequent
		while (lat_pcblookup(head, atp->at_laddr.at_addr,
			saddr.at_addr, lport, 0));
#else
		while (at_pcblookup(head, atp->at_laddr.at_addr,
			saddr.at_addr, lport, 0));
#endif sequent

	}

	atp->at_lport = lport;

#ifdef sequent
	ATP_UNLOCK(head, SPLNET);
#endif sequent

	return (0);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */

at_detach(atp)
	register struct atcb *atp;
{
	struct socket *so = atp->at_socket;
#ifdef sequent
	register struct atcb*		head;
	register struct socket_peer* 	saved_sopp;

	/*
	 * The socket_peer is locked when at_detach is called
	 * so can safely sever the socket bond to the pcb.
	 */


	/*
	 * In order to remque, the list head must be locked; however, if
	 * pcblookup is currently referencing this pcb, DEADLOCK can occur.
	 * To avoid this open a WINDOW in which pcblookup can acquire the
	 * pcb lock. at_pcblookup() discovers after ATP_LOCK that the atp is
	 * disconnected and gives that information back to the network peer
	 * (i.e. nofind).  In the meantime, at_detach() locks the list
	 * and removes the pcb so it won't be found by pcblookup.  The refcnt
	 * is managed by not decrementing it until there is no longer a kernel
	 * reference.  Nothing can disappear during the window since there is
	 * no socket reference to it and no at_head.
	 * at_head is used to indicate to pcblookup that this pcb isn't
	 * really in list even if pcblookup finds it and wins the race to
	 * lock it back.  at_head == NULL indicates that pcb is being removed.
	 */

	head = atp->at_head;
	atp->at_head = (struct atcb *) NULL;
	saved_sopp = atp->at_sopp;

	if(head) {	/* not currently being detached */
		so->so_pcb = (caddr_t) NULL;
		sofree(so);
		ATP_UNLOCK(atp, SPLNET);	/* open WINDOW */
		(void) ATP_LOCK(head);		/*   WINDOW - lock the list */
		(void) p_lock(&saved_sopp->sop_lock, SPLNET);
		remque(atp);
		ATP_UNLOCK(head, SPLNET);
		(void) m_free(dtom(atp));

	}else{	
		/* In process of being deleted. */
	}

	if(--(saved_sopp->sop_refcnt) == 0)
		(void) m_free(dtom(saved_sopp));

	/* N.B. leaves sop locked if it is not released */
#else
	so->so_pcb = 0;
	sofree(so);
	remque(atp);
	m_free(dtom(atp));
#endif sequent
}

/*
 * Disconnect and possibly release resources.
 */

at_disconnect(atp)
	struct atcb *atp;
{
	if ((atp->atcb_flags & AT_IGNADDR) == 0)
		atp->at_fnet = atp->at_fnode = atp->at_fport = 0;
	atp->atcb_flags &= ~AT_FADDR;

#define	SS_USERGONE	SS_NOFDREF

	if (atp->at_socket->so_state & SS_USERGONE)
		at_detach(atp);
}

/*
 * Associate a peer's address with a
 * ddp connection block.
 */

at_connaddr(atp, addr)
	struct atcb *atp;
	struct sockaddr_at *addr;
{
	atp->at_faddr = *addr;
	atp->atcb_flags |= AT_FADDR;
	if ((atp->at_laddr.at_net == 0) && (atp->at_laddr.at_node == 0)) {
		struct sockaddr_at d;
		struct ifnet *ifp;

		d.at_family = AF_APPLETALK;
		d.at_net = addr->at_net;
		d.at_node = addr->at_node;
		ifp = atroute(&d);
		if (ifp == 0)
			return ENETUNREACH;

		atp->at_laddr.at_net = ifpgetnet(ifp);
		atp->at_laddr.at_node = ifpgetnode(ifp);
		atp->at_laddr.at_abridge = ifpgetbridge(ifp);
	}

	return(0);
}

/*
 * called with a wildcard -- says match this if you can.  Always return the
 * first matched atcb.
 *
 * TODO -- don't allow reception by a ddp client of a ddp with a protocol not
 * matching the protocol specified -- that's how the Mac works...  Hmmmm.
 */ 

struct atcb *
at_pcblookup(head, laddr, faddr, lport, flags)
	register struct atcb *head;
	struct a_addr laddr, faddr;
	register u_char lport;
	int flags;
{
	register struct atcb *atp, *match = 0;
	register int matchwild = 2, wildcard;
#ifdef sequent
	spl_t splevel;
	splevel = ATP_LOCK(head);	/* lock the pcb list */
#endif sequent

	for (atp = head->at_next; atp != head; atp = atp->at_next) {

		if (atp->at_lport != lport)
			continue;

		wildcard = 0;

		/*
		 * a wildcard address is a (net, node) pair both 0.  Below,
		 * first, if the local address is not zero client wants only
		 * dgrams from a specific address.  Else, he will accept
		 * from anyone.
		 */

		if ((atp->at_laddr.at_net != 0)||(atp->at_laddr.at_node != 0)) {

			/*
			 * client wants specific address. check for match
			 */

#ifdef sequent		/* Is this a Kinetics error for loopback?? */

			if ((laddr.at_Net &&
				(atp->at_laddr.at_net != laddr.at_Net))

#else sequent

			if ((atp->at_laddr.at_net != laddr.at_Net)

#endif sequent
			  || (atp->at_laddr.at_node != laddr.at_Node))
				continue;

			if ((atp->at_faddr.at_net != 0) ||
				(atp->at_faddr.at_node != 0)) {

				/*
				 * check for specific foreign address
				 */

				if ((atp->at_faddr.at_net != faddr.at_Net) ||
				    (atp->at_faddr.at_node != faddr.at_Node)){
					continue;
				}else{
#ifdef sequent
					(void) ATP_LOCK(atp);
					if(atp->at_head) {
						atp->at_sopp->sop_refcnt++;
					}else{
						ATP_UNLOCK(atp, SPLNET);
						atp = (struct atcb *) NULL;
					}

					ATP_UNLOCK(head, splevel);
#endif sequent
					return(atp);
				}
			} else {
				wildcard++;
			}
		} else {
			wildcard++;
		}

		if (wildcard && (flags & ATPLOOKUP_WILDCARD) == 0)
			continue;		/* want exact match */

		if (wildcard < matchwild) {

			/*
			 * Found at least one match.   (Use it unless find
			 * exact match.)
			 */

			match = atp;	
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}

	if(match) {
		(void) ATP_LOCK(match);
		if(match->at_head) {
			match->at_sopp->sop_refcnt++;
		}else{
			ATP_UNLOCK(match, SPLNET);
			match = (struct atcb *) NULL;
		}
	}
	ATP_UNLOCK(head, splevel);

	return (match);
}

/*
 * lat_pcblookup - version of pcblookup that assumes list is already
 * locked and does not lock or refcnt++ the found entry.
 */

struct atcb *
lat_pcblookup(head, laddr, faddr, lport, flags)
	register struct atcb *head;
	struct a_addr laddr, faddr;
	register u_char lport;
	int flags;
{
	register struct atcb *atp, *match = 0;
	register int matchwild = 2, wildcard;

	for (atp = head->at_next; atp != head; atp = atp->at_next) {

		if (atp->at_lport != lport)
			continue;

		wildcard = 0;

		/*
		 * a wildcard address is a (net, node) pair both 0.  Below,
		 * first, if the local address is not zero client wants only
		 * dgrams from a specific address.  Else, he will accept
		 * from anyone.
		 */

		if ((atp->at_laddr.at_net != 0)||(atp->at_laddr.at_node != 0)) {

			/*
			 * client wants specific address. check for match
			 */

			if ((atp->at_laddr.at_net != laddr.at_Net)
			  || (atp->at_laddr.at_node != laddr.at_Node)) {
				continue;
			}
			if ((atp->at_faddr.at_net != 0) ||
				(atp->at_faddr.at_node != 0)) {

				/*
				 * check for specific foreign address
				 */

				if ((atp->at_faddr.at_net != faddr.at_Net) ||
				    (atp->at_faddr.at_node != faddr.at_Node)){
					continue;
				}else{
					return(atp);
				}
			} else {
				wildcard++;
			}
		} else {
			wildcard++;
		}

		if (wildcard && (flags & ATPLOOKUP_WILDCARD) == 0)
			continue;		/* want exact match */

		if (wildcard < matchwild) {

			/*
			 * Found at least one match.   (Use it unless find
			 * exact match.)
			 */

			match = atp;	
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}

	return (match);
}

#ifdef notused
/*
 * if_atnet - find matching net number for an if
 */

if_atnet(net)
	register int net;
{
	register struct ifnet *ifp = ifnet;

	while (ifp) {
		if (ifpgetnet(ifp) == net)
			return net;
		ifp = ifp->if_next;
	}
	return 0;
}
#endif notused

atforme(dp)
	register struct ddp *dp;
{
	extern struct ifnet *ifnet;
	register struct ifnet *ifp;

	if (dp->d_dnet) {
		for (ifp = ifnet; ifp; ifp = ifp->if_next) {
			if (ifpgetnet(ifp) == dp->d_dnet) {
				if (ifpgetnode(ifp) == dp->d_dnode || 
				  dp->d_dnode == 0xff)
					return 1;
			}
		}
	}
	else
		return 1;
	return 0;
}


/*ARGSUSED*/
at_pcballoc(so, head)
	struct socket *so;
	struct atcb *head;
{
	register struct atcb *atp;
	struct mbuf *m;
#ifdef sequent
	register struct socket_peer*	sop;
#endif sequent

#ifdef IS4DOT2
	m = m_getclr(M_DONTWAIT, MT_PCB);
#else
	MSGET((struct atcb *)m, struct atcb, 1);
#endif

	if (m == NULL)
		return (ENOBUFS);

	atp = mtod(m, struct atcb *);
	atp->at_socket = so;

#ifdef sequent

	/*
	 * N.B. M_DONTWAIT argument is not used (also not used in 4.2)
	 */

	m = m_getclr(M_DONTWAIT, MT_SOPEER);
	if(m == (struct mbuf *) NULL) {
		(void) m_free(dtom(atp));
		return(ENOBUFS);
	}
	sop = mtod(m, struct socket_peer *);
	init_lock(&sop->sop_lock, G_SOCK);

	/*
	 * There are initially two references to the pcb, one from
	 * socket management and one from at_pcb management.
	 *
	 * at_sopp points to explicit socket_peer not the at_sop.
 	 * also so->so_sopp -> same socket_peer
 	 */

	sop->sop_refcnt = 2;
	so->so_sopp = (struct socket_peer *) sop;
	atp->at_sopp = (struct socket_peer *) sop;

	/*
	 * sequent moves the insque to the attach routine - this may
	 * actually be a non-concern with ddp since there are no timers.
	 */

#else

	insque(atp, head);

#endif sequent

	so->so_pcb = (caddr_t)atp;
	return (0);
}
#endif AT
