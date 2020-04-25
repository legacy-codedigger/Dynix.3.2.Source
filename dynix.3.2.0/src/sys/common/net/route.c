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
static	char	rcsid[] = "$Header: route.c 2.8 1991/05/10 23:08:48 $";
#endif

/*
 * route.c
 *	Routing routines
 */

/* $Log: route.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../net/route.h"

lock_t rt_lock;			/* lock for *both* routing tables mutex */

extern	struct	mbuf *rthost[];
extern	struct	mbuf *rtnet[];

struct	rtstat	rtstat;

int	rttrash;		/* routes not in table but not freed */
struct	sockaddr wildcard;	/* zero value for wildcard searches */
extern	int	rthashsize;	/* for netstat, etc. */

/*
 * Packet routing routines.
 */

/*
 *	Initialize routing tables - i.e. rtlock
 */

rttabinit()
{
	init_lock(&rt_lock, G_RT);
}

/*
 * bind a struct route to a struct rtentry
 */

rtalloc(ro)
	register struct route *ro;
{
	register struct rtentry *rt;
	register struct mbuf *m;
	register u_long hash;
	struct sockaddr *dst = &ro->ro_dst;
	int (*match)(), doinghost;
	struct afhash h;
	u_int af = dst->sa_family;
	struct mbuf **table;
	spl_t splevel;

	if (af >= AF_MAX)
		return;

	(*afswitch[af].af_hash)(dst, &h);
	match = afswitch[af].af_netmatch;
	hash = h.afh_hosthash, table = rthost, doinghost = 1;

	/*
	 * this locking is conservative.  It is possible that rt_flags is
	 * modified outside of a critical section; however since there
	 * is a refcnt, it is not possible for the rtentry to disappear.
	 * A potential performance mod is to shorten this critical
	 * section and allow a race for the rt_flags.
	 */

	splevel = p_lock(&rt_lock, SPLNET);	/* mutex rt tables */

	if (ro->ro_rt && ro->ro_rt->rt_ifp &&
	    ro->ro_rt->rt_flags & RTF_UP) {		/* XXX */
		(void) v_lock(&rt_lock, splevel);	/* unlock */
		return;
	}

again:
	for (m = table[RTHASHMOD(hash)]; m; m = m->m_next) {
		rt = mtod(m, struct rtentry *);
		if (rt->rt_hash != hash)
			continue;
		if ((rt->rt_flags & RTF_UP) == 0 ||
		    (rt->rt_ifp->if_flags & IFF_UP) == 0)
			continue;
		if (doinghost) {
			if (bcmp((caddr_t)&rt->rt_dst, (caddr_t)dst,
			    sizeof (*dst)))
				continue;
		} else {
			if (rt->rt_dst.sa_family != af ||
			    !(*match)(&rt->rt_dst, dst))
				continue;
		}

		/*
		 * note all of the continues above, if search gets through this
		 * negative logic, the appropriate rtentry is found (i.e. rt)
		 * at this point.  bind it to route, refcnt++ and rtstat.
		 */

		ro->ro_rt = rt;
		rt->rt_refcnt++;
		if (dst == &wildcard)
			rtstat.rts_wildcard++;
		(void) v_lock(&rt_lock, splevel);	/* unlock */
		return;
	}

	/*
	 * traversal through hash table failed to find match, if this was
	 * first pass looking for route to host, then reset to search rtnet for
	 * route to net instead.
	 */

	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash, table = rtnet;
		goto again;
	}

	/*
	 * Check for wildcard gateway, by convention network 0.
	 * cannot find route in tables - give the wildcard rtentry a shot 
	 */

	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}

	rtstat.rts_unreach++;	 /* cannot find route */

	(void) v_lock(&rt_lock, splevel); /* allow changes after refcnt++ */
}

rtfree(rt)
	register struct rtentry *rt;
{
	spl_t splevel;

	ASSERT(rt, "rtfree");
	splevel = p_lock(&rt_lock, SPLNET);	/* mutex rt tables */
	rt->rt_refcnt--;
	if (rt->rt_refcnt == 0 && (rt->rt_flags&RTF_UP) == 0) {
		rttrash--;
		(void) m_free(dtom(rt));
	}
	(void) v_lock(&rt_lock, splevel);
}

/*
 * Force a routing table entry to the specified
 * destination to go through the given gateway.
 * Normally called as a result of a routing redirect
 * message from the network layer.
 */

rtredirect(dst, gateway, flags, src)
	struct sockaddr *dst, *gateway, *src;
	int flags;
{
	struct route ro;
	register struct rtentry *rt;

	/*
	 * verify the gateway is directly reachable
	 */

	if (ifa_ifwithnet(gateway) == 0) {
		rtstat.rts_badredirect++;
		return;
	}
	ro.ro_dst = *dst;
	ro.ro_rt = 0;
	rtalloc(&ro);
	rt = ro.ro_rt;

#define	equal(a1, a2) \
	(bcmp((caddr_t)(a1), (caddr_t)(a2), sizeof(struct sockaddr)) == 0)

	/*
	 * If the redirect isn't from our current router for this dst,
	 * it's either old or wrong.  If it redirects us to ourselves,
	 * we have a routing loop, perhaps as a result of an interface
	 * going down recently.
	 */
	if ((rt && !equal(src, &rt->rt_gateway)) || ifa_ifwithaddr(gateway)) {
		rtstat.rts_badredirect++;
		if (rt)
			rtfree(rt);
		return;
	}
	/*
	 * Create a new entry if we just got back a wildcard entry
	 * or the the lookup failed.  This is necessary for hosts
	 * which use routing redirects generated by smart gateways
	 * to dynamically build the routing tables.
	 */
	if (rt &&
	    (*afswitch[dst->sa_family].af_netmatch)(&wildcard, &rt->rt_dst)) {
		rtfree(rt);
		rt = 0;
	}
	if (rt == 0) {
		rtinit(dst, gateway, (int)SIOCADDRT,
		    (flags & RTF_HOST) | RTF_GATEWAY | RTF_DYNAMIC);
		rtstat.rts_dynamic++;
		return;
	}
	/*
	 * Don't listen to the redirect if it's
	 * for a route to an interface. 
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to net.
			 */
			rtinit(dst, gateway, (int)SIOCADDRT,
			    flags | RTF_DYNAMIC);
			rtstat.rts_dynamic++;
		} else {
			/*
			 * Smash the current notion of the gateway to
			 * this destination.
			 */
			rt->rt_gateway = *gateway;
		}
		rtstat.rts_newgateway++;
	} else
		rtstat.rts_badredirect++;
	rtfree(rt);
}

/*
 * Routing table ioctl interface.
 */

rtioctl(cmd, data)
	int cmd;
	caddr_t data;
{

	if (cmd != SIOCADDRT && cmd != SIOCDELRT)
		return (EINVAL);
	if (!suser())
		return (u.u_error);
	return (rtrequest(cmd, (struct rtentry *)data));
}

/*
 * Carry out a request to change the routing table.  Called by
 * interfaces at boot time to make their ``local routes'' known,
 * for ioctl's, and as the result of routing redirects.
 */

#include	"../netinet/in.h"

rtrequest(req, entry)
	int req;
	register struct rtentry *entry;
{
	register struct mbuf *m, **mprev;
	struct mbuf **mfirst;
	register struct rtentry *rt;
	struct afhash h;
	spl_t splevel;
	int error = 0, (*match)();
	u_int af;
	u_long hash;
	struct ifaddr *ifa;

	af = entry->rt_dst.sa_family;
	if (af >= AF_MAX)
		return (EAFNOSUPPORT);
	(*afswitch[af].af_hash)(&entry->rt_dst, &h);
	if (entry->rt_flags & RTF_HOST) {
		hash = h.afh_hosthash;
		mprev = &rthost[RTHASHMOD(hash)];
	} else {
		hash = h.afh_nethash;
		mprev = &rtnet[RTHASHMOD(hash)];
	}
	match = afswitch[af].af_netmatch;

	/*
	 * Mutex the routing tables.
	 * Use SPLIMP since some callers are at higher spl.
	 */

	splevel = p_lock(&rt_lock, SPLIMP);

	for (mfirst = mprev; m = *mprev; mprev = &m->m_next) {
		rt = mtod(m, struct rtentry *);
		if (rt->rt_hash != hash)
			continue;
		if (entry->rt_flags & RTF_HOST) {
			if (!equal(&rt->rt_dst, &entry->rt_dst))
				continue;
		} else {
			if (rt->rt_dst.sa_family != entry->rt_dst.sa_family ||
			    (*match)(&rt->rt_dst, &entry->rt_dst) == 0)
				continue;
		}
		if (equal(&rt->rt_gateway, &entry->rt_gateway))
			break;
	}

	switch (req) {

	case SIOCDELRT:
		if (m == 0) {
			error = ESRCH;
			goto bad;
		}
		*mprev = m->m_next;
		if (rt->rt_refcnt > 0) {
			rt->rt_flags &= ~RTF_UP;
			rttrash++;	/* (sic 4.2) */
			m->m_next = (struct mbuf *) NULL;
		} else
			(void) m_free(m);
		break;

	case SIOCADDRT:
		if (m) {
			error = EEXIST;
			goto bad;
		}
		if ((entry->rt_flags & RTF_GATEWAY) == 0) {

			/*
			 * If we are adding a route to an interface,
			 * and the interface is a pt to pt link
			 * we should search for the destination
			 * as our clue to the interface.  Otherwise
			 * we can use the local address.
			 */

			ifa = 0;
			if (entry->rt_flags & RTF_HOST) 
				ifa = ifa_ifwithdstaddr(&entry->rt_dst);
			if (ifa == 0)
				ifa = ifa_ifwithaddr(&entry->rt_gateway);
		} else {

			/*
			 * If we are adding a route to a remote net
			 * or host, the gateway may still be on the
			 * other end of a pt to pt link.
			 */

			ifa = ifa_ifwithdstaddr(&entry->rt_gateway);
		}
		if (ifa == 0) {
			ifa = ifa_ifwithnet(&entry->rt_gateway);
			if (ifa == 0) {
				error = ENETUNREACH;
				goto bad;
			}
		}
		m = m_get(M_DONTWAIT, MT_RTABLE);
		if (m == 0) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_next = *mfirst;
		*mfirst = m;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct rtentry);
		rt = mtod(m, struct rtentry *);
		rt->rt_hash = hash;
		rt->rt_dst = entry->rt_dst;
		rt->rt_gateway = entry->rt_gateway;
		rt->rt_flags = RTF_UP |
		    (entry->rt_flags & (RTF_HOST|RTF_GATEWAY|RTF_DYNAMIC));
		rt->rt_refcnt = 0;
		rt->rt_use = 0;
		rt->rt_ifp = ifa->ifa_ifp;
		break;
	}
bad:
	(void) v_lock(&rt_lock, splevel);

	return (error);
}

/*
 * Set up a routing table entry, normally
 * for an interface.
 */

rtinit(dst, gateway, cmd, flags)
	struct sockaddr *dst, *gateway;
	int cmd, flags;
{
	struct rtentry route;

	bzero((caddr_t)&route, sizeof (route));
	route.rt_dst = *dst;
	route.rt_gateway = *gateway;
	route.rt_flags = flags;
	(void) rtrequest(cmd, &route);
}

/*
 * blow away all routes associated with a particular interface
 */
rtflush(ifp)
	register struct ifnet *ifp;
{
	register struct rtentry *rt;
	register struct mbuf *m;
	register struct mbuf **mprev;
	struct mbuf **table;
	spl_t splevel;
	int doinghost, i;

	table = rthost;
	doinghost = 1;

	splevel = p_lock(&rt_lock, SPLNET);	/* mutex rt tables */

again:
	for (i = 0; i < rthashsize; i++) {
		for (mprev = &table[i]; m = *mprev; mprev = &m->m_next) {
			rt = mtod(m, struct rtentry *);
			if (rt->rt_ifp == ifp) {	/* blow it away */
				*mprev = m->m_next;
				if (rt->rt_refcnt > 0) {
					rt->rt_flags &= ~RTF_UP;
					rttrash++;	/* (sic 4.2) */
					m->m_next = (struct mbuf *) NULL;
				} else
					(void) m_free(m);
			}
		}
	}

	/*
	 * first pass through host table done -- on to net table!
	 */

	if (doinghost) {
		doinghost = 0;
		table = rtnet;
		goto again;
	}
	(void) v_lock(&rt_lock, splevel);
}
