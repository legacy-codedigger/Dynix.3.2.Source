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
static	char	rcsid[] = "$Header: ip_input.c 2.17 1991/04/30 17:17:23 $";
#endif

/* $Log: ip_input.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../net/if.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/tcp.h"

#include "../balance/engine.h"
#include "../h/vm.h"
#include "../net/netisr.h"
#include "../machine/pte.h"
#include "../machine/plocal.h"


u_char	ip_protox[IPPROTO_MAX];
struct	ifqueue	ipintrq;		/* ip packet input queue */
struct	in_ifaddr *in_ifaddr;		/* first inet address */
struct	ipstat	ipstat;			/* stats */
struct	ipq	ipq;			/* ip reass. queue */
u_short	ip_id;				/* ip packet ctr, for ids */

int iptimo = 0;				/* flag to effect a timeout */

/*
 * We need to save the IP options in case a protocol wants to respond
 * to an incoming packet over the same route if the packet got here
 * using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known
 * to us.
 */

int	ip_nhops = 0;
static	struct ip_srcrt {
	char	nop;				/* one NOP to align */
	char	srcopt[IPOPT_OFFSET + 1];	/* OPTVAL, OLEN and OFFSET */
	struct	in_addr route[MAX_IPOPTLEN];
} ip_srcrt;

/*
 * IP initialization: fill in IP protocol switch table.
 * All protocols not implemented in kernel go to raw IP protocol handler.
 */

ip_init()
{
	register struct protosw *pr;
	register int i;

	pr = pffindproto(PF_INET, IPPROTO_RAW, SOCK_RAW);

	ASSERT(pr != (struct protosw *) NULL, "ip_init");

	for (i = 0; i < IPPROTO_MAX; i++)
		ip_protox[i] = pr - inetsw;

	for (pr = inetdomain.dom_protosw;
	       pr < inetdomain.dom_protoswNPROTOSW; pr++)
		if (pr->pr_domain->dom_family == PF_INET &&
		    pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
			ip_protox[pr->pr_protocol] = pr - inetsw;

	ipq.next = ipq.prev = &ipq;

	ip_id = time.tv_sec & 0xffff;
	ipintrq.ifq_maxlen = ipqmaxlen;

	/*
	 * note that even though this is queued into a "snd" queue, it is 
	 * received data.  The transmit and receive paths are separate
	 * and the input queue to the network is output from drivers to 
	 * the ip level through the ip ifnet structure.
	 */

	init_lock(&ipintrq.ifq_lock, G_IFNET);

	/*
	 * Initialize ARP.
	 */

	arp_init();

}

struct	ip *ip_reass();
struct	sockaddr_in ipaddr = { AF_INET };
struct	route ipforward_rt;

extern	int	ipgateway;

/*
 * Ip input routine.  Checksum and byte swap header.  If fragmented
 * try to reassemble.  If complete and fragment queue exists, discard.
 * Process options.  Pass to next level.
 */

ipintr()
{
	register struct ip *ip;
	register struct mbuf *m;
	struct mbuf *m0;
	register int i;
	register struct ipq *fp;
	register struct in_ifaddr *ia;
	struct ifnet *ifp;
	int hlen;
	spl_t splevel;
	int link_broadcast;

	/*
	 * Get next datagram off input queue and get IP header
	 * in first mbuf.
	 */

	splevel = IF_LOCK(&ipintrq);
	if (ipintrq.ifq_busy) {
		IF_UNLOCK(&ipintrq, splevel);
		return;
	}
	ipintrq.ifq_busy = 1;
	goto skiplock;
next:
	splevel = IF_LOCK(&ipintrq);

skiplock:

	/*
	 * check for a time out event - if time out has occurred
	 * ip_timeout() goes through the list decrementing ipq_ttl;
	 * NOTE: IF_ENQUE can occur while processing list.  Also,
	 * theoretically another timeout can occur if it takes longer
	 * than a half second to process the ipq!
	 */

	if(iptimo) {
		iptimo = 0;
		IF_UNLOCK(&ipintrq, splevel);
		ip_timeout();
		goto next;
	}

	/*
	 * if queue is empty, unbusies queue and m == null
	 */

	IF_DEQUEUEIF(&ipintrq, m, ifp);

	/*
	 * unlock so more data can be queued right away
	 */

	IF_UNLOCK(&ipintrq, splevel);
	if (m == (struct mbuf *) NULL)
		return;
	/*
	 * The driver below us will set MF_BROADCAST if this packet
	 * was recieved as a result of a link level broadcast.
	 */
	link_broadcast = m->m_flags & MF_BROADCAST;

	/*
	 * If no IP addresses have been set yet but the interfaces
	 * are receiving, can't do anything with incoming packets yet.
	 */

	if (in_ifaddr == NULL)
		goto bad;
	ipstat.ips_total++;

	/*
	 * note: m_pullup can fail due to failure to allocate mbufs.
	 * Therefore, this ipstat.ips_toosmall may not be accurate.
	 * perhaps need an ipstat.ips_drops...  Nonetheless, the packet
	 * is dropped => best effort fails.
	 *
	 * further note: m_pullup *m_freem's* if it fails so go to
	 * next rather than bad.
	 *
	 * If this is a mcluster (like from the ether) - we need the
	 * headers in an mbuf. 
	 */

	if (((m->m_off > MMAXOFF) || (m->m_len < sizeof(struct ip)))
	    && ((m = m_pullup(m, sizeof(struct ip))) == (struct mbuf*) NULL))
	{
		ipstat.ips_toosmall++;
		goto next;
	}

	ip = mtod(m, struct ip *);
	hlen = ip->ip_hl << 2;
	if(hlen < sizeof(struct ip)) {	/* minimum header length */
		ipstat.ips_badhlen++;
		goto bad;
	}
	if(hlen > m->m_len) {
		if((m = m_pullup(m, hlen)) == (struct mbuf*) NULL){
			ipstat.ips_badhlen++;
			goto next;
		}
		ip = mtod(m, struct ip*);
	}

	if (ipcksumi)
		if (ip->ip_sum = in_cksum(m, hlen)) {
			ipstat.ips_badsum++;
			goto bad;
		}

	/*
	 * Convert fields to host representation.
	 */

	ip->ip_len = ntohs((u_short)ip->ip_len);
	if (ip->ip_len < hlen) {
		ipstat.ips_badlen++;
		goto bad;
	}
	ip->ip_id = ntohs(ip->ip_id);
	ip->ip_off = ntohs((u_short)ip->ip_off);

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */

	i = -(u_short)ip->ip_len;
	m0 = m;
	for (;;) {
		i += m->m_len;
		if (m->m_next == (struct mbuf *) NULL)
			break;
		m = m->m_next;
	}
	if (i != 0) {
		if (i < 0) {
			ipstat.ips_tooshort++;
			m = m0;		/* restore m to mfree entire list */
			goto bad;
		}
		if (i <= m->m_len)
			m->m_len -= i;
		else
			m_adj(m0, -i);
	}
	m = m0;

	/*
	 * Process options and, if not destined for us,
	 * ship it on.  ip_dooptions returns 1 when an
	 * error was detected (causing an icmp message
	 * to be sent and the original packet to be freed).
	 */

	ip_nhops = 0;		/* for source routed packets */
	if (hlen > sizeof (struct ip) && ip_dooptions(ip, ifp))
		goto next;

	/*
	 * Check our list of addresses, to see if the packet is for us.
	 */

#define	satosin(sa)	((struct sockaddr_in *)(sa))

	for (ia = in_ifaddr; ia; ia = ia->ia_next) {

		if (IA_SIN(ia)->sin_addr.s_addr == ip->ip_dst.s_addr)
			goto ours;
		if (
#ifdef	DIRECTED_BROADCAST
		    ia->ia_ifp == ifp &&
#endif
		    (ia->ia_ifp->if_flags & IFF_BROADCAST)) {
			u_long t;

			if (satosin(&ia->ia_broadaddr)->sin_addr.s_addr ==
			    ip->ip_dst.s_addr)
				goto ours;
			if (ip->ip_dst.s_addr == ia->ia_netbroadcast.s_addr)
				goto ours;

			/*
			 * Look for all-0's host part (old broadcast addr),
			 * either for subnet or net.
			 */

			t = ntohl(ip->ip_dst.s_addr);
			if (t == ia->ia_subnet)
				goto ours;
			if (t == ia->ia_net)
				goto ours;
		}
	}
	if (ip->ip_dst.s_addr == INADDR_BROADCAST)
		goto ours;
	if (ip->ip_dst.s_addr == INADDR_ANY)
		goto ours;

	/*
	 * Not for us; drop if its a broadcast, else
	 * forward if possible and desirable.
	 */
	
	if (link_broadcast)
		m_freem(dtom(ip));
	else
		ip_forward(ip, ifp);
	goto next;

ours:
	/*
	 * Look for queue of fragments of this datagram.
	 */

	for (fp = ipq.next; fp != &ipq; fp = fp->next) {
		if (ip->ip_id == fp->ipq_id &&
		    ip->ip_src.s_addr == fp->ipq_src.s_addr &&
		    ip->ip_dst.s_addr == fp->ipq_dst.s_addr &&
		    ip->ip_p == fp->ipq_p)
			goto found;
	}
	fp = (struct ipq *) NULL;
found:

	/*
	 * Adjust ip_len to not reflect header,
	 * set ip_mff if more fragments are expected,
	 * convert offset of this to bytes.
	 */

	ip->ip_len -= hlen;
	((struct ipasfrag *)ip)->ipf_mff = 0;
	if (ip->ip_off & IP_MF)
		((struct ipasfrag *)ip)->ipf_mff = 1;
	ip->ip_off <<= 3;

	/*
	 * If datagram marked as having more fragments
	 * or if this is not the first fragment,
	 * attempt reassembly; if it succeeds, proceed.
	 */

	if (((struct ipasfrag *)ip)->ipf_mff || ip->ip_off) {
		ipstat.ips_fragments++;
		ip = ip_reass((struct ipasfrag *)ip, fp);
		if (ip == (struct ip * ) NULL)	/* still fragmented */
			goto next;
		ipstat.ips_reassok++;
		m = dtom(ip);
	} else
		if (fp)
			ip_freef(fp);
	/*
	 * Switch out to protocol's input routine.
	 */

	(*inetsw[ip_protox[ip->ip_p]].pr_input)(m, ifp);
	goto next;
bad:
	m_freem(m);
	goto next;
}


/*
 * Take incoming datagram fragment and try to
 * reassemble it into whole datagram.  If a chain for
 * reassembly of this datagram already exists, then it
 * is given as fp; otherwise have to make a chain.
 */

struct ip *
ip_reass(ip, fp)
	register struct ipasfrag *ip;
	register struct ipq *fp;
{
	register struct mbuf *m = dtom(ip);
	register struct ipasfrag *q;
	struct mbuf *t;
	int hlen = ip->ip_hl << 2;
	int i, next;

	/*
	 * Presence of header sizes in mbufs
	 * would confuse code below.
	 */

	m->m_off += hlen;
	m->m_len -= hlen;

	/*
	 * If first fragment to arrive, create a reassembly queue.
	 */

	if (fp == (struct ipq *) NULL) {
		if ((t = m_get(M_DONTWAIT, MT_FTABLE)) == (struct mbuf *) NULL)
			goto dropfrag;
		fp = mtod(t, struct ipq *);
		insque(fp, &ipq);
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ip->ip_p;
		fp->ipq_id = ip->ip_id;
		fp->ipq_next = fp->ipq_prev = (struct ipasfrag *)fp;
		fp->ipq_src = ((struct ip *)ip)->ip_src;
		fp->ipq_dst = ((struct ip *)ip)->ip_dst;
		fp->ipq_time = time.tv_sec;           /* timestamp the entry */
		q = (struct ipasfrag *)fp;
		goto insert;
	}

	/*
	 * Find a segment which begins after this one does.
	 */

	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = q->ipf_next)
		if (q->ip_off > ip->ip_off)
			break;
	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */

	if (q->ipf_prev != (struct ipasfrag *)fp) {
		i = q->ipf_prev->ip_off + q->ipf_prev->ip_len - ip->ip_off;
		if (i > 0) {
			if (i >= ip->ip_len)
				goto dropfrag;
			m_adj(dtom(ip), i);
			ip->ip_off += i;
			ip->ip_len -= i;
		}
	}

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */

	while (q != (struct ipasfrag *)fp &&
				ip->ip_off + ip->ip_len > q->ip_off)
		{
		i = (ip->ip_off + ip->ip_len) - q->ip_off;
		if (i < q->ip_len) {
			q->ip_len -= i;
			q->ip_off += i;
			m_adj(dtom(q), i);
			break;
		}
		q = q->ipf_next;
		t = dtom(q->ipf_prev);
		ip_deq(q->ipf_prev);
		m_freem(t);
	}

insert:
	/*
	 * Stick new segment in its place;
	 * check for complete reassembly.
	 */

	ip_enq(ip, q->ipf_prev);
	next = 0;
	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = q->ipf_next) {
		if (q->ip_off != next)
			return ((struct ip *) NULL);
		next += q->ip_len;
	}
	if (q->ipf_prev->ipf_mff)
		return ((struct ip *) NULL);

	/*
	 * Reassembly is complete; concatenate fragments.
	 */

	q = fp->ipq_next;
	m = dtom(q);
	t = m->m_next;
	m->m_next = (struct mbuf *) NULL;
	m_cat(m, t);
	q = q->ipf_next;
	while (q != (struct ipasfrag *)fp) {
		t = dtom(q);
		q = q->ipf_next;
		m_cat(m, t);
	}

	/*
	 * Create header for new ip packet by
	 * modifying header of first packet;
	 * dequeue and discard fragment reassembly header.
	 * Make header visible.
	 */

	ip = fp->ipq_next;
	ip->ip_len = next;
	((struct ip *)ip)->ip_src = fp->ipq_src;
	((struct ip *)ip)->ip_dst = fp->ipq_dst;

        /* Just before this entry is dequeued, we double-check
	 * to make sure that it's ttl has not expired.  If it has
         * pitch the whole thing.
         */
	if (2*(time.tv_sec - fp->ipq_time) > fp->ipq_ttl) {
		/* Its expired */
		remque(fp);
		(void) m_free(dtom(fp));
		m = dtom(ip);
		ipstat.ips_fragtimeout++;
		goto dropfrag;
	}

	remque(fp);
	(void) m_free(dtom(fp));
	m = dtom(ip);
	m->m_len += (ip->ip_hl << 2);
	m->m_off -= (ip->ip_hl << 2);
	return ((struct ip *)ip);

dropfrag:
	ipstat.ips_fragdropped++;
	m_freem(m);
	return ((struct ip *) NULL);
}

/*
 * Free a fragment reassembly header and all
 * associated datagrams.
 */

ip_freef(fp)
	struct ipq *fp;
{
	register struct ipasfrag *q, *p;

	for (q = fp->ipq_next; q != (struct ipasfrag *)fp; q = p) {
		p = q->ipf_next;
		ip_deq(q);
		m_freem(dtom(q));
	}
	remque(fp);
	(void) m_free(dtom(fp));
}

/*
 * Put an ip fragment on a reassembly chain.
 * Like insque, but pointers in middle of structure.
 */

ip_enq(p, prev)
	register struct ipasfrag *p, *prev;
{
	p->ipf_prev = prev;
	p->ipf_next = prev->ipf_next;
	prev->ipf_next->ipf_prev = p;
	prev->ipf_next = p;
}

/*
 * To ip_enq as remque is to insque.
 */

ip_deq(p)
	register struct ipasfrag *p;
{
	p->ipf_prev->ipf_next = p->ipf_next;
	p->ipf_next->ipf_prev = p->ipf_prev;
}

/*
 * IP timer processing;
 * if a timer expires on a reassembly queue, discard it.
 *
 * algorithm: if something is in ipq, schedule a timeout event by
 * signalling IFQ.  This avoids concurrent access to ipq since only
 * processed in ipintr().
 */

ip_slowtimo()
{
	spl_t splevel;

	/*
	 * if there is something in the ipfrag queue (ipq.next) &&
	 * && timeout not currently pending (iptimo == 0)
	 * schedule an ip_timeout() by incrementing iptimo.
	 */

	if(ipq.next != &ipq && !iptimo) {
		splevel = IF_LOCK(&ipintrq);
		iptimo++;
		if (!ipintrq.ifq_busy)
			schednetisr(NETISR_IP);
		IF_UNLOCK(&ipintrq, splevel);
	}
}

/*
 * ip_timeout()
 * {
 * 	register struct ipq *fp;
 * 
 * 	fp = ipq.next;
 * 	while (fp != &ipq) {
 * 		fp = fp->next;
 * 		if (2*(time.tv_sec - fp->prev->ipq_time) > fp->prev->ipq_ttl) {
 * 			ipstat.ips_fragtimeout++;
 * 			ip_freef(fp->prev);
 * 		}
 * 	}
 * }
 * 
 */

/* Go thru the reassembly queue, throwing away entries which have
 * expired their ttl.  Since entries are enqueued in time order,
 * the list is traversed backwards.  As soon as a non-expired entry
 * is encountered the routine returns since all subsequent entries
 * will be more recent (and hence unexpired).  Note that this policy
 * assumes a constant ttl value (= IPFRAGTTL) for all entries.
 */

ip_timeout()
{
	register struct ipq *fp;

	fp = ipq.prev;
	while (fp != &ipq) {
		fp = fp->prev;
		if (2*(time.tv_sec - fp->next->ipq_time) > IPFRAGTTL) {
			/* fragment timed out */
			ipstat.ips_fragtimeout++;
			ip_freef(fp->next);
		} else {
			/* all previous entries even more recent .. */
			return;
		}
	}
}

/*
 * Drain off all datagram fragments.
 */

ip_drain()
{
	while (ipq.next != &ipq) {
		ipstat.ips_fragdropped++;
		ip_freef(ipq.next);
	}
}

struct in_ifaddr *ip_rtaddr();

/*
 * Do option processing on a datagram,
 * possibly discarding it if bad options
 * are encountered.
 */
ip_dooptions(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register u_char *cp;
	int opt, optlen, cnt, off, code, type = ICMP_PARAMPROB;
	register struct ip_timestamp *ipt;
	register struct in_ifaddr *ia;
	struct in_addr *sin;
	n_time ntime;
	struct in_addr dest;

	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

		/*
		 * Source routing with record.
		 * Find interface with current destination address.
		 * If none on this machine then drop if strictly routed,
		 * or do nothing if loosely routed.
		 * Record interface address and bring up next address
		 * component.  If strictly routed make sure next
		 * address on directly accessible net.
		 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			ipaddr.sin_addr = ip->ip_dst;
			ia = (struct in_ifaddr *)
				ifa_ifwithaddr((struct sockaddr *)&ipaddr);
			if (ia == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr)) {
				/*
				 * End of source route.  Should be for us.
				 */
				save_rte(cp, ip->ip_src);
				break;
			}
			/*
			 * locate outgoing interface
			 */
			bcopy((caddr_t)(cp + off), (caddr_t)&ipaddr.sin_addr,
			    sizeof(ipaddr.sin_addr));
			if ((opt == IPOPT_SSRR &&
			    in_iaonnetof(in_netof(ipaddr.sin_addr)) == 0) ||
			    (ia = ip_rtaddr(ipaddr.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
			ip->ip_dst = ipaddr.sin_addr;
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_RR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;			/* 0 origin */
			if (off > optlen - sizeof(struct in_addr))
				break;
			bcopy((caddr_t)(cp + off), (caddr_t)&ipaddr.sin_addr,
			    sizeof(ipaddr.sin_addr));
			/*
			 * locate outgoing interface
			 */
			if ((ia = ip_rtaddr(ipaddr.sin_addr)) == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((caddr_t)&(IA_SIN(ia)->sin_addr),
			    (caddr_t)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *)ip;
			ipt = (struct ip_timestamp *)cp;
			if (ipt->ipt_len < 5)
				goto bad;
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof (long)) {
				if (++ipt->ipt_oflw == 0)
					goto bad;
				break;
			}
			sin = (struct in_addr *)(cp+cp[IPOPT_OFFSET]-1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				if (in_ifaddr == 0)
					goto bad;	/* ??? */
				bcopy((caddr_t)&IA_SIN(in_ifaddr)->sin_addr,
				    (caddr_t)sin, sizeof(struct in_addr));
				sin++;
				break;

			case IPOPT_TS_PRESPEC:
				bcopy((caddr_t)sin, (caddr_t)&ipaddr.sin_addr,
				    sizeof(struct in_addr));
				if (ifa_ifwithaddr((struct sockaddr *)&ipaddr) == 0)
					continue;
				if (ipt->ipt_ptr + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len)
					goto bad;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				goto bad;
			}
			ntime = iptime();
			bcopy((caddr_t)&ntime, (caddr_t)sin, sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	return (0);
bad:
	dest.s_addr = 0;
	icmp_error(ip, type, code, ifp, dest);
	return (1);
}

/*
 * Given address of next destination (final or next hop),
 * return internet address info of interface to be used to get there.
 */
struct in_ifaddr *
ip_rtaddr(dst)
	 struct in_addr dst;
{
	register struct sockaddr_in *sin;
	register struct in_ifaddr *ia;

	sin = (struct sockaddr_in *) &ipforward_rt.ro_dst;

	if (ipforward_rt.ro_rt == 0 || dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_addr = dst;

		rtalloc(&ipforward_rt);
	}
	if (ipforward_rt.ro_rt == 0) {
		ipstat.ips_noroute++;
		return ((struct in_ifaddr *)0);
	}
	/*
	 * Find address associated with outgoing interface.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_ifp == ipforward_rt.ro_rt->rt_ifp)
			break;
	return (ia);
}

/*
 * Save incoming source route for use in replies,
 * to be picked up later by ip_srcroute if the receiver is interested.
 */
save_rte(option, dst)
	u_char *option;
	struct in_addr dst;
{
	unsigned olen;

	olen = option[IPOPT_OLEN];
	if (olen > sizeof(ip_srcrt) - 1) {
		if (ipprintfs)
			printf("save_rte: olen %d\n", olen);
		return;
	}
	bcopy((caddr_t)option, (caddr_t)ip_srcrt.srcopt, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	ip_srcrt.route[ip_nhops++] = dst;
}

/*
 * Retrieve incoming source route for use in replies,
 * in the same form used by setsockopt.
 * The first hop is placed before the options, will be removed later.
 */
struct mbuf *
ip_srcroute()
{
	register struct in_addr *p, *q;
	register struct mbuf *m;

	if (ip_nhops == 0)
		return ((struct mbuf *)0);

	m = m_get(M_DONTWAIT, MT_SOOPTS);
	if (m == (struct mbuf *) NULL)
		return ((struct mbuf *)0);
	m->m_len = ip_nhops * sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1;

	/*
	 * First save first hop for return route
	 */
	p = &ip_srcrt.route[ip_nhops - 1];
	*(mtod(m, struct in_addr *)) = *p--;

	/*
	 * Copy option fields and padding (nop) to mbuf.
	 */
	ip_srcrt.nop = IPOPT_NOP;
	bcopy((caddr_t)&ip_srcrt, mtod(m, caddr_t) + sizeof(struct in_addr),
	    IPOPT_OFFSET + 1 + 1);
	q = (struct in_addr *)(mtod(m, caddr_t) +
	    sizeof(struct in_addr) + IPOPT_OFFSET + 1 + 1);
	/*
	 * Record return path as an IP source route,
	 * reversing the path (pointers are now aligned).
	 */
	while (p >= ip_srcrt.route)
		*q++ = *p--;
	return (m);
}

/*
 * Strip out IP options, at higher
 * level protocol in the kernel.
 * Second argument is buffer to which options
 * will be moved, and return value is their length.
 */

ip_stripoptions(ip, mopt)
	struct ip *ip;
	struct mbuf *mopt;
{
	register int i;
	register struct mbuf *m;
	register caddr_t opts;
	int olen;

	olen = (ip->ip_hl<<2) - sizeof (struct ip);
	m = dtom(ip);
	opts = (caddr_t)(ip + 1);
	if (mopt) {
		mopt->m_len = olen;
		mopt->m_off = MMINOFF;
		bcopy(opts, mtod(mopt, caddr_t), (unsigned)olen);
	}
	i = m->m_len - (sizeof (struct ip) + olen);
	bcopy(opts  + olen, opts, (unsigned)i);
	m->m_len -= olen;
	ip->ip_hl = sizeof(struct ip) >> 2;
}

u_char inetctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		0,		EHOSTDOWN,	EHOSTUNREACH,
	ENETUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

#ifndef	IPFORWARDING
#define	IPFORWARDING	1
#endif
#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding (possibly because we have only a single external
 * network), just drop the packet.  This could be confusing if ipforwarding
 * was zero but some routing protocol was advancing us as a gateway
 * to somewhere.  However, we must let the routing protocol deal with that.
 */

ip_forward(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register int error, type = 0, code;
	register struct sockaddr_in *sin;
	struct mbuf *mcopy;
	struct in_addr dest;

	dest.s_addr = 0;
	if (ipprintfs)
		printf("forward: src %x dst %x ttl %x\n", ip->ip_src,
			ip->ip_dst, ip->ip_ttl);
	ip->ip_id = htons(ip->ip_id);

	if (ipforwarding == 0 || in_interfaces <= 1) {

		ipstat.ips_cantforward++;

		/*
		 * 4.3 uses #ifdef's here - DYNIX prefers binary
		 * configurable values.
		 */

		if (ipgateway) {
			type = ICMP_UNREACH, code = ICMP_UNREACH_NET;
			goto sendicmp;
		} else {
			m_freem(dtom(ip));
			return;
		}
	}
	if (in_canforward(ip->ip_dst) == 0) {
		m_freem(dtom(ip));
		return;
	}
	if (ip->ip_ttl < IPTTLDEC) {
		type = ICMP_TIMXCEED, code = ICMP_TIMXCEED_INTRANS;
		goto sendicmp;
	}
	ip->ip_ttl -= IPTTLDEC;

	/*
	 * Save at most 64 bytes of the packet in case
	 * we need to generate an ICMP message to the src.
	 */
	mcopy = m_copy(dtom(ip), 0, imin((int)ip->ip_len, 64));

	sin = (struct sockaddr_in *)&ipforward_rt.ro_dst;
	if (ipforward_rt.ro_rt == 0 ||
	    ip->ip_dst.s_addr != sin->sin_addr.s_addr) {
		if (ipforward_rt.ro_rt) {
			RTFREE(ipforward_rt.ro_rt);
			ipforward_rt.ro_rt = 0;
		}
		sin->sin_family = AF_INET;
		sin->sin_addr = ip->ip_dst;

		rtalloc(&ipforward_rt);
	}
	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modfied by a redirect.
	 */
#define	satosin(sa)	((struct sockaddr_in *)(sa))
	if (ipforward_rt.ro_rt && ipforward_rt.ro_rt->rt_ifp == ifp &&
	    (ipforward_rt.ro_rt->rt_flags & RTF_DYNAMIC) == 0 &&
	    satosin(&ipforward_rt.ro_rt->rt_dst)->sin_addr.s_addr != 0 &&
	    ipsendredirects && ip->ip_hl == (sizeof(struct ip) >> 2)) {
		struct in_ifaddr *ia;
		extern struct in_ifaddr *ifptoia();
		u_long src = ntohl(ip->ip_src.s_addr);
		u_long dst = ntohl(ip->ip_dst.s_addr);

		if ((ia = ifptoia(ifp)) &&
		   (src & ia->ia_subnetmask) == ia->ia_subnet) {
		    if (ipforward_rt.ro_rt->rt_flags & RTF_GATEWAY)
			dest = satosin(&ipforward_rt.ro_rt->rt_gateway)->sin_addr;
		    else
			dest = ip->ip_dst;
		    /*
		     * If the destination is reached by a route to host,
		     * is on a subnet of a local net, or is directly
		     * on the attached net (!), use host redirect.
		     * (We may be the correct first hop for other subnets.)
		     */
		    type = ICMP_REDIRECT;
		    code = ICMP_REDIRECT_NET;
		    if ((ipforward_rt.ro_rt->rt_flags & RTF_HOST) ||
		       (ipforward_rt.ro_rt->rt_flags & RTF_GATEWAY) == 0)
			code = ICMP_REDIRECT_HOST;
		    else for (ia = in_ifaddr; ia = ia->ia_next; )
			if ((dst & ia->ia_netmask) == ia->ia_net) {
			    if (ia->ia_subnetmask != ia->ia_netmask)
				    code = ICMP_REDIRECT_HOST;
			    break;
			}
		    if (ipprintfs)
		        printf("redirect (%d) to %x\n", code, dest);
		}
	}

	error = ip_output(dtom(ip), (struct mbuf *)0, &ipforward_rt,
		IP_FORWARDING);
	if (error)
		ipstat.ips_cantforward++;
	else if (type)
		ipstat.ips_redirectsent++;
	else {
		if (mcopy)
			m_freem(mcopy);
		ipstat.ips_forward++;
		return;
	}
	if (mcopy == NULL)
		return;
	ip = mtod(mcopy, struct ip *);
	type = ICMP_UNREACH;
	switch (error) {

	case 0:				/* forwarded, but need redirect */
		type = ICMP_REDIRECT;
		/* code set above */
		break;

	case ENETUNREACH:
	case ENETDOWN:
		if (in_localaddr(ip->ip_dst))
			code = ICMP_UNREACH_HOST;
		else
			code = ICMP_UNREACH_NET;
		break;

	case EMSGSIZE:
		code = ICMP_UNREACH_NEEDFRAG;
		break;

	case EPERM:
		code = ICMP_UNREACH_PORT;
		break;

	case ENOBUFS:
		type = ICMP_SOURCEQUENCH;
		break;

	case EHOSTDOWN:
	case EHOSTUNREACH:
		code = ICMP_UNREACH_HOST;
		break;
	}
sendicmp:
	icmp_error(ip, type, code, ifp, dest);
}

ip_unknownprot()
{
	ipstat.ips_unknownproto++;
}