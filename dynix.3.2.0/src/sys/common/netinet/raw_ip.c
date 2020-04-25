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
static	char	rcsid[] = "$Header: raw_ip.c 2.6 1991/08/14 20:46:49 $";
#endif

/*
 * raw_ip.c
 *
 */

/* $Log: raw_ip.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"


/*
 * Raw interface to IP protocol.
 */

struct	sockaddr_in ripdst = { AF_INET };
struct	sockaddr_in ripsrc = { AF_INET };
struct	sockproto ripproto = { PF_INET };

/*
 * Setup generic address and protocol structures
 * for raw_input routine, then pass them along with
 * mbuf chain.
 */

rip_input(m)
	struct mbuf *m;
{
	register struct ip *ip = mtod(m, struct ip *);

	ripproto.sp_protocol = ip->ip_p;
	ripdst.sin_addr = ip->ip_dst;
	ripsrc.sin_addr = ip->ip_src;
	raw_input(m, &ripproto, (struct sockaddr *)&ripsrc,
	  (struct sockaddr *)&ripdst);
}

/*
 * Generate IP header and pass packet to ip_output.
 * Tack on options user may have setup with control call.
 */

rip_output(rp, m0)
	struct rawcb *rp;
	struct mbuf *m0;
{
	register struct mbuf	*m;
	register struct ip	*ip;
	int len = 0, error;
	struct sockaddr_in	*sin;
	struct socket		*so = rp->rcb_socket;

	/*
	 * Calculate data length and get an mbuf
	 * for IP header.
	 */

	for (m = m0; m; m = m->m_next)
		len += m->m_len;

	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == (struct mbuf *) NULL) {
		error = ENOBUFS;
		goto bad;
	}
	
	/*
	 * Fill in IP header as needed.
	 */

	m->m_off = MMAXOFF - sizeof(struct ip);
	m->m_len = sizeof(struct ip);
	m->m_next = m0;
	ip = mtod(m, struct ip *);
	ip->ip_tos = 0;
	ip->ip_off = 0;
	ip->ip_p = rp->rcb_socket->so_proto->pr_protocol;
	ip->ip_len = sizeof(struct ip) + len;
	if (rp->rcb_flags & RAW_LADDR) {
		sin = (struct sockaddr_in *)&rp->rcb_laddr;
		if (sin->sin_family != AF_INET) {
			error = EAFNOSUPPORT;
			goto bad;
		}
		ip->ip_src.s_addr = sin->sin_addr.s_addr;
	} else
		ip->ip_src.s_addr = 0;
	ip->ip_dst = ((struct sockaddr_in *)&rp->rcb_faddr)->sin_addr;
	ip->ip_ttl = ip_ttl;
	return (ip_output(m, rp->rcb_options, &rp->rcb_route, 
	   (so->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST));
bad:
	m_freem(m);
	return (error);
}

/*
 * Raw IP socket option processing.
 */
rip_ctloutput(op, so, level, optname, m)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **m;
{
	int error = 0;
	register struct rawcb *rp = sotorawcb(so);

	if (level != IPPROTO_IP)
		error = EINVAL;
	else switch (op) {

	case PRCO_SETOPT:
		switch (optname) {
		case IP_OPTIONS:
			return (ip_pcbopts(&rp->rcb_options, *m));

		default:
			error = EINVAL;
			break;
		}
		break;

	case PRCO_GETOPT:
		switch (optname) {
		case IP_OPTIONS:
			if (rp->rcb_options) {
				(*m)->m_off = rp->rcb_options->m_off;
				(*m)->m_len = rp->rcb_options->m_len;
				bcopy(mtod(rp->rcb_options, caddr_t),
				    mtod(*m, caddr_t), (unsigned)(*m)->m_len);
			} else
				(*m)->m_len = 0;
			break;
		default:
			error = EINVAL;
			break;
		}
		break;
	}
	return (error);
}
