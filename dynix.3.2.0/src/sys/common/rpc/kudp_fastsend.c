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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: kudp_fastsend.c 1.13 91/03/11 $";
#endif	lint

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* $Log:	kudp_fastsend.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/file.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/if_ether.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/udp.h"
#include "../netinet/udp_var.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"
#ifdef	i386
#include "../machine/inline.h"
#endif	i386

/*
 * If fragmentation is necessary, an mbuf is allocated to hold this
 * reference count data and lock.
 */
struct refdata {
	int	rd_refcnt;
	lock_t	rd_lock;
};

/*
 * ku_fragfree()
 *
 * Called when each type MCLT_KHEAP mbuf created when ku_fastsend()
 * does fragmentation is freed. When the reference count therein is
 * zero then the refmbp mbuf is freed.
 *
 * If called from the interface after the data has been transmitted,
 * the ku_fastsend() argument mbuf chain which is chained to the refmbp
 * mbuf is freed.
 *
 * If called because make_frag_chain() could not allocate enough mbufs,
 * then only the refmbp mbuf is freed. The ku_fastsend argument mbuf chain
 * has not yet been linked to the refmbp mbuf.
 */
static
ku_fragfree(rdp)
	register struct refdata *rdp;
{
	spl_t	s_ipl;

	s_ipl = p_lock(&rdp->rd_lock, SPLHI);
	if (--rdp->rd_refcnt == 0) {
		v_lock(&rdp->rd_lock, s_ipl);
		m_freem(dtom(rdp));
	} else {
		v_lock(&rdp->rd_lock, s_ipl);
	}
}

/*
 * make_frag_chain
 *	make new mbuf chain consisting entirely of MCLT_KHEAP mbufs.
 *	This is the chain that will be fragmented.
 *
 * For each mbuf in the am mbuf chain, get a MCLT_KHEAP mbuf
 * to point to the mbuf's data. The number of MCLT_KHEAP
 * mbufs allocated will be the initial reference count. If
 * more MCLT_KHEAP mbufs are allocated below, the reference
 * count will be incremented there.
 *
 * Called from ku_fastsend() if fragmentation is to be done.
 * If an error occurs, return NULL after cleaning up.
 * Otherwise, return new chain.
 */

static struct mbuf *
make_frag_chain(am, rdp)
	register struct mbuf *am;	/* original chain */
	struct refdata *rdp;		/* ref count data */
{
	register struct mbuf *m;
	register struct mbuf *lam;
	struct mbuf *head;

	head = lam = (struct mbuf *)NULL;
	for (; am != (struct mbuf *)NULL; am = am->m_next) {
		MGET(m, M_DONTWAIT, MT_DATA);
		while (m == (struct mbuf *)NULL) {
			if (!m_expandorwait()) {
				if (head != (struct mbuf *)NULL) {
					(void) m_freem(head);
				}
				return((struct mbuf *)NULL);
			}
			MGET(m, M_DONTWAIT, MT_DATA);
		}
		/*
		 * Got one. Bump reference count and fill out MCLT_KHEAP mbuf.
		 * No need to mutex rd_refcnt here since no fragments have
		 * been queued to the ifp->if_output() interface.
		 */
		rdp->rd_refcnt++;
		m->m_off = mtod(am, int) - (int)m;
		m->m_len = am->m_len;
		m->m_cltype = MCLT_KHEAP;
		m->m_clfun = ku_fragfree;
		m->m_clarg = (int)rdp;
		m->m_clswp = NULL;
		if (head == (struct mbuf *)NULL) {
			head = m;
			lam = m;
		} else {
			lam->m_next = m;
			lam = m;
		}
	}
	return (head);
}

/*
 *  Take the mbuf chain at am, add ip/udp headers, and fragment it
 *  for the interface output routine.  Once a chain is sent to the
 *  interface, it is freed upon transmission. It is possible for one
 *  fragment to succeed, and another to fail.  If this happens
 *  the the remaining mbuf chain (at am) must be freed. Any resources
 *  freed via MCLT_KHEAP mbufs in the argument mbuf chain must not be
 *  freed until after all fragments have been freed. The caller
 *  must assume that the entire send failed if one fragment failed.
 *  If we get an error before a fragment is sent, then the original
 *  chain is intact and the caller may take other action.
 *
 *  Return codes:
 *	 0:  send ok
 *	-1:  send failed; mchain freed
 *	-2:  send failed; mchain intact
 */
ku_fastsend(so, am, to)
	struct socket *so;		/* socket data is sent from */
	register struct mbuf *am;	/* data to be sent */
	struct sockaddr_in *to;		/* destination data is sent to */
{
	register int datalen;		/* length of all data in packet */
	register int maxlen;		/* max length of fragment */
	register int curlen;		/* data fragment length */
	register int fragoff;		/* data fragment offset */
	register int grablen;		/* number of mbuf bytes to grab */
	register struct ip *ip;		/* ip header */
	register struct udpiphdr *ui;	/* udpip header */
	register struct mbuf *m;	/* ip header mbuf */
	struct in_ifaddr *ia;		/* interface address struct */
	struct ifnet	*ifp;		/* interface */
	struct mbuf	*lam;		/* last mbuf in chain to be sent */
	struct mbuf	*refmbp;	/* holds ref count Q, arg mbuf chain */
	struct refdata	*rdp;		/* ref count and lock */
	struct sockaddr	*dst;		/* packet destination */
	struct inpcb	*inp;		/* inpcb for binding */
	struct ip	*nextip;	/* ip header for next fragment */
	struct route	route;		/* route to send packet */
	static struct route zero_route;	/* to initialize route */
	struct route	*ro;
	spl_t	s_ipl;
	extern	int ip_hdr_cksum();	/* fast ip header checksumming */

	/*
	 * Determine length of data.
	 * This should be passed in as a parameter.
	 */
	datalen = 0;
	for (m = am; m; m = m->m_next) {
		datalen += m->m_len;
	}

	/*
	 * Routing.
	 * We worry about routing early so we get the right ifp.
	 */
	route = zero_route;
	ro = &route;
	ro->ro_dst.sa_family = AF_INET;
	((struct sockaddr_in *)&ro->ro_dst)->sin_addr = to->sin_addr;
	rtalloc(ro);
	if (ro->ro_rt == 0)
		return (-2);
	if ((ifp = ro->ro_rt->rt_ifp) == 0) {
		RTFREE(ro->ro_rt);
		return(-2);
	}
	ro->ro_rt->rt_use++;
	if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
		dst = &ro->ro_rt->rt_gateway;
	} else {
		dst = &ro->ro_dst;
	}

	/*
	 * Get mbuf for ip, udp headers.
	 */
	MGET(m, M_DONTWAIT, MT_HEADER);
	while (m == (struct mbuf *)NULL) {
		if (!m_expandorwait()) {
			RTFREE(ro->ro_rt);
			return (-2);
		}
		MGET(m, M_DONTWAIT, MT_HEADER);
	}
	m->m_off = MMINOFF + sizeof(struct ether_header);
	m->m_len = sizeof(struct ip) + sizeof(struct udphdr);

	/*
	 * Create udpip header.
	 */
	ip = mtod(m, struct ip *);
	ui = (struct udpiphdr *)ip;
	ui->ui_next = 0;
	ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)(sizeof(struct udphdr) + datalen));
	ui->ui_ulen = ui->ui_len;	/* N.B. net order */

	/*
	 * find the appropriate in_addr on the interface
	 * there should be a better way of doing this than scanning
	 * the list of internet interface addresses for an ifp match 
	 * but typically this is a short list (like one)
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
		if (ia->ia_ifp == ifp) {
			ui->ui_src = IA_SIN(ia)->sin_addr;
			break;
		}
	}
	ui->ui_dst = to->sin_addr;

	/*
	 * Bind port, if necessary.
	 */
	inp = sotoinpcb(so);
	if (inp->inp_laddr.s_addr == INADDR_ANY && inp->inp_lport == 0) {
		/*
		 * No one has done the bind. So lock the pcb and bind.
		 * Note: not really necessary in the NFS case since the
		 * server daemon does bind before the nfs_svc call. But..
		 */
		s_ipl = INP_LOCK(inp);
		(void) in_pcbbind(inp, (struct mbuf *)0);
		INP_UNLOCK(inp, s_ipl);
	}

	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = to->sin_port;
	ui->ui_sum = 0;
	if (udpcksumo) {
		/*
		 * Calculate and stuff checksum.
		 */
		m->m_next = am;
		ui->ui_sum = in_cksum(m, sizeof(struct udpiphdr) + datalen);
		if (ui->ui_sum == 0)
			ui->ui_sum = -1;
	}

	/*
	 * Fill in ip specific fields
	 */
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_v = IPVERSION;
	ip->ip_tos = 0;
	GET_IP_ID(ip->ip_id);
	ip->ip_off = 0;
	ip->ip_ttl = MAXTTL;
	ip->ip_sum = 0;		/* this is necessary for ip_hdr_cksum() */

	/*
	 * Fragment the data (if necessary) into packets big enough for the
	 * interface, prepend the header, and send them off.
	 */
	maxlen = (ifp->if_mtu - sizeof(struct ip)) & ~7;
	curlen = sizeof(struct udphdr);
#ifdef	DEBUG
	refmbp = (struct mbuf *)NULL;
#endif	DEBUG

	/*
	 * Check to see if fragmentation will need to be done.
	 */
	if (datalen + curlen > maxlen) {
		/*
		 * Must fragment. So need to allocate mbuf to keep
		 * reference count for number of MCLT_KHEAP mbufs to
		 * be passed to the ifp->if_output() routine. This
		 * mbuf will also contain the lock to mutex the ref count.
		 */ 

		MGET(refmbp, M_DONTWAIT, MT_DATA);
		while (refmbp == (struct mbuf *)NULL) {
			if (!m_expandorwait()) {
				/*
				 * Free udpip header mbuf and route.
				 */
				(void) m_free(m);
				RTFREE(ro->ro_rt);
				return (-2);
			}
			MGET(refmbp, M_DONTWAIT, MT_DATA);
		}
		rdp = mtod(refmbp, struct refdata *);
		init_lock(&rdp->rd_lock, G_NFS);
		rdp->rd_refcnt = 0;

		/*
		 * Replace argument mbuf chain with a chain of MCLT_KHEAP
		 * mbufs pointing to the same data as the arg mbuf chain.
		 */
		lam = am;
		am = make_frag_chain(am, rdp);
		if (am == (struct mbuf *)NULL) {
			/*
			 * Error! Could not allocate enough mbufs.
			 * refmbp mbuf has already been freed via
			 * ku_fragfree(). Free udpip header mbuf, route
			 * and return -2.
			 */
			(void) m_free(m);
			RTFREE(ro->ro_rt);
			return (-2);
		}
		/*
		 * The argument mbuf chain will be freed after all of the
		 * type MCLT_KHEAP mbufs in the fragments are freed.
		 */
		refmbp->m_next = lam;
	}

	fragoff = 0;
	for (;;) {
		register struct mbuf *mm;

		m->m_next = am;
		lam = m;
		while (am->m_len + curlen <= maxlen) {
			curlen += am->m_len;
			lam = am;
			am = am->m_next;
			if (am == 0) {
				ip->ip_off = htons((u_short)(fragoff >> 3));
				goto send;
			}
		}
		if (curlen == maxlen) {
			/*
			 * Incredible luck: last mbuf exactly
			 * filled out the packet.
			 */
			lam->m_next = 0;
		} else {
			/*
			 * We can squeeze part of the next
			 * mbuf into this packet, so we
			 * get a type MCLT_KHEAP mbuf and point it at
			 * this data fragment.
			 */
			ASSERT_DEBUG(refmbp != (struct mbuf *)NULL,
				     "ku_fastsend: unexpected fragmentation");
			MGET(mm, M_DONTWAIT, MT_DATA);
			while (mm == (struct mbuf *)NULL) {
				if (!m_expandorwait()) {
					/*
					 * free entire chain.
					 */
					m_freem(m);	/* includes am */
					RTFREE(ro->ro_rt);
					return (-1);
				}
				MGET(mm, M_DONTWAIT, MT_DATA);
			}

			/*
			 * New MCLT_KHEAP mbuf. Must increment reference count.
			 */
			s_ipl = p_lock(&rdp->rd_lock, SPLHI);
			rdp->rd_refcnt++;
			v_lock(&rdp->rd_lock, s_ipl);

			grablen = maxlen - curlen;
			mm->m_off = mtod(am, int) - (int)mm;
			mm->m_len = grablen;
			mm->m_cltype = MCLT_KHEAP;
			mm->m_clfun = ku_fragfree;
			mm->m_clarg = (int)rdp;
			mm->m_clswp = NULL;
			lam->m_next = mm;
			am->m_len -= grablen;
			am->m_off += grablen;
			curlen = maxlen;
		}
		/*
		 * m now points to the head of an mbuf chain which
		 * contains the max amount that can be sent in a packet.
		 */
		ip->ip_off = htons((u_short)((fragoff >> 3) | IP_MF));
		/*
		 * There are more frags, so we save
		 * a copy of the ip hdr for the next
		 * frag.
		 */
		MGET(mm, M_DONTWAIT, MT_HEADER);
		while (mm == (struct mbuf *)NULL) {
			if (!m_expandorwait()) {
				m_freem(m);	/* includes ip hdr */
				m_freem(am);	/* rest of chain */
				RTFREE(ro->ro_rt);
				return (-1);
			}
			MGET(mm, M_DONTWAIT, MT_HEADER);
		}
		mm->m_off = MMINOFF + sizeof(struct ether_header);
		mm->m_len = sizeof(struct ip);
		nextip = mtod(mm, struct ip *);
		*nextip = *ip;
send:
		/*
		 * Set ip_len and calculate the ip header checksum.
		 */
		ip->ip_len = htons((u_short)(sizeof(struct ip) + curlen));
		ip->ip_sum = ip_hdr_cksum(ip);

		/*
		 * At last, we send it off to the ethernet.
		 */
		if ((*ifp->if_output)(ifp, m, dst)) {
			/*
			 * mbuf chain m has been freed at this point.
			 * am and nextip (if nonnull) must be freed here
			 */
			if (am) {
				(void) m_free(mm);	/* next ip header */
				m_freem(am);		/* rest of chain */
			}
			RTFREE(ro->ro_rt);
			return (-1);
		}
		if (am == 0) {
			RTFREE(ro->ro_rt);
			return (0);
		}
		ip = nextip;
		m = mm;
		fragoff += curlen;
		curlen = 0;
	}
}

#ifdef	MBUFDEBUG
pr_mbuf(p, m)
	char *p;
	struct mbuf *m;
{
	register char *cp, *cp2;
	register struct ip *ip;
	register int len;

	len = 28;
	printf("%s: ", p);
	if (m && m->m_len >= 20) {
		ip = mtod(m, struct ip *);
		printf("hl %d v %d tos %d len %d id %d mf %d off %d ttl %d p %d sum %d src %x dst %x\n",
			ip->ip_hl, ip->ip_v, ip->ip_tos, ip->ip_len,
			ip->ip_id, ip->ip_off >> 13, ip->ip_off & 0x1fff,
			ip->ip_ttl, ip->ip_p, ip->ip_sum, ip->ip_src.s_addr,
			ip->ip_dst.s_addr);
		len = 0;
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		m = m->m_next;
	} else if (m) {
		printf("pr_mbuf: m_len %d\n", m->m_len);
	} else {
		printf("pr_mbuf: zero m\n");
	}
	while (m) {
		printf("m %x addr %x len %d\n", m, mtod(m, caddr_t), m->m_len);
		cp = mtod(m, caddr_t);
		cp2 = cp + m->m_len;
		while (cp < cp2) {
			if (len-- < 0) {
				break;
			}
			printf("%x ", *cp & 0xFF);
			cp++;
		}
		m = m->m_next;
		printf("\n");
	}
}
#endif	MBUFDEBUG
#endif	NFS
