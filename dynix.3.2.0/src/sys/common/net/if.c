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

#undef	RAW_ETHER	/* experimental raw-ether kernel */
#define	RAW_ETHER	/* experimental raw-ether kernel */

#undef	PROMISCUOUS	/* UNDO experimental promiscuous kernel */
#define	PROMISCUOUS	/* experimental promiscuous kernel */

#ifndef	lint
static	char	rcsid[] = "$Header: if.c 2.10 1991/05/10 23:08:33 $";
#endif

/*
 * if.c
 *	Net interface management routines
 */

/* $Log: if.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/if.h"
#include "../net/af.h"
#include "../net/netisr.h"

extern short se_mtu;

int		netisr;		/* scheduling bits for network */
extern int	ifqmaxlen;

struct ifnet *ifnet;		/* network interface head */

/*
 * allow for 4 custom pseudo-device driver ether clients
 */

struct custom_client custom_clients[4];


/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */

ifinit()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
		if (ifp->if_init) {
			(*ifp->if_init)(ifp->if_unit);
			if (ifp->if_snd.ifq_maxlen == 0)
				ifp->if_snd.ifq_maxlen = ifqmaxlen;
		}

	if_slowtimo();
}

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */

if_attach(ifp)
	struct ifnet *ifp;
{
	register struct ifnet **p = &ifnet;

	/*
	 * TODO - mutex since late attaches might hose the list!
	 */

	while (*p)
		p = &((*p)->if_next);
	*p = ifp;
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

#define	equal(a1, a2) \
	(bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if (equal(&ifa->ifa_addr, addr))
			return (ifa);
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    equal(&ifa->ifa_broadaddr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}
/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) 
	    if (ifp->if_flags & IFF_POINTOPOINT)
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr.sa_family != addr->sa_family)
				continue;
			if (equal(&ifa->ifa_dstaddr, addr))
				return (ifa);
	}
	return ((struct ifaddr *)0);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is first found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register u_int af = addr->sa_family;
	register int (*netmatch)();

	if (af >= AF_MAX)
		return (0);
	netmatch = afswitch[af].af_netmatch;
	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr.sa_family != addr->sa_family)
			continue;
		if ((*netmatch)(&ifa->ifa_addr, addr))
			return (ifa);
	}
	return ((struct ifaddr *)0);
}

#ifdef notdef
/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = ifnet; ifp; ifp = ifp->if_next)
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr.sa_family == af)
			return (ifa);
	return ((struct ifaddr *)0);
}
#endif

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splnet or eqivalent.
 */

if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		pfctlinput(PRC_IFDOWN, &ifa->ifa_addr);
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */

if_slowtimo()
{
	register struct ifnet *ifp;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp->if_unit);
	}
	timeout(if_slowtimo, (caddr_t)0, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */

struct ifnet *
ifunit(name)
	register char *name;
{
	register char *cp;
	register struct ifnet *ifp;
	int unit;

	for (cp = name; cp < name + IFNAMSIZ && *cp; cp++)
		if (*cp >= '0' && *cp <= '9')
			break;
	if (*cp == '\0' || cp == name + IFNAMSIZ)
		return ((struct ifnet *)NULL);
	unit = *cp - '0';
	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (bcmp(ifp->if_name, name, (unsigned)(cp - name)))
			continue;
		if (unit == ifp->if_unit)
			break;
	}
	return (ifp);
}

/*
 * Interface ioctls.
 */
ifioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	caddr_t data;
{
	register struct ifnet *ifp;
	register struct ifreq *ifr;

	switch (cmd) {

	case SIOCGIFCONF:
		return (ifconf(cmd, data));

	case SIOCSARP:
	case SIOCDARP:
	case SIOCFARP:
		if (!suser())
			return (u.u_error);
		/* FALL THROUGH */
	case SIOCGARP:
		return (arpioctl(cmd, data));
	}
	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name);
	if (ifp == 0)
		return (ENXIO);
	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCSIFFLAGS:
		if (!suser())
			return (u.u_error);
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			int splevel;
			splevel = IF_LOCK(&ifp->if_snd); /* spl -> SPLIMP */
			if_down(ifp);
			IF_UNLOCK(&ifp->if_snd, splevel);
		}
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCSIFMETRIC:
		if (!suser())
			return (u.u_error);
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;

	case SIOCSIFMTU:

		/*
		 * for now, do not allow the if_mtu to be larger than
		 * that initially set up for ether (se_mtu), since a
		 * transmit buffer is initially calloc() for this and
		 * we don't want to do this dynamically.
		 */

		if (ifr->ifr_mtu > se_mtu)
			return (EINVAL);
		ifp->if_mtu = ifr->ifr_mtu;
		break;

	default:
		if (so->so_proto == 0)
			return (EOPNOTSUPP);
		return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
			cmd, data, ifp));
	}
	return (0);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
ifconf(cmd, data)
	int cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp = ifnet;
	register struct ifaddr *ifa;
	register char *cp, *ep;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;

	ifrp = ifc->ifc_req;
	ep = ifr.ifr_name + sizeof (ifr.ifr_name) - 2;
	for (; space > sizeof (ifr) && ifp; ifp = ifp->if_next) {
		bcopy(ifp->if_name, ifr.ifr_name, sizeof (ifr.ifr_name) - 2);
		for (cp = ifr.ifr_name; cp < ep && *cp; cp++)
			;
		*cp++ = '0' + ifp->if_unit; *cp = '\0';
		if ((ifa = ifp->if_addrlist) == 0) {
			bzero((caddr_t)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp, sizeof (ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		} else 
		    for ( ; space > sizeof (ifr) && ifa; ifa = ifa->ifa_next) {
			ifr.ifr_addr = ifa->ifa_addr;
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp, sizeof (ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		}
	}
	ifc->ifc_len -= space;
	return (error);
}

/*
 * the network software interrupt handling routine - i.e. demultiplex
 * netisr's to net input queue handlers - e.g. ipintr
 */

int ipintr(), ffs(), netundef();

#ifdef RAW_ETHER
int rawintr();
#endif RAW_ETHER

#ifdef PROMISCUOUS
int promiscintr();
#endif PROMISCUOUS

/*
 * network software interrupt vectors
 */

/*
 * NEEDS to be moved to conf_net for binary configuration
 */

int (*netisrvec[32])() = {

#ifdef RAW_ETHER
	rawintr,	/* [0] => raw packet handler */
#else
	netundef,	/* [0] => unassigned */
#endif RAW_ETHER

#ifdef PROMISCUOUS	/* [1] => promiscuous handler */
	promiscintr,
#else
	netundef,	/* [1] => unassigned */
#endif PROMISCUOUS

#ifdef INET
	ipintr,		/* [2] => ip protocol engine */
#else
	netundef,	/* [2] => unassigned */
#endif
	netundef,	/* [3] - [31] => unassigned */
	netundef,	netundef,	netundef,		netundef,
	netundef,	netundef,	netundef,		netundef,
	netundef,	netundef,	netundef,		netundef,
	netundef,	netundef,	netundef,		netundef,
	netundef,	netundef,	netundef,		netundef,
	netundef,	netundef,	netundef,		netundef,
	netundef,	netundef,	netundef,		netundef
};

netintr()
{
	register int i;
	GATESPL(splevel); 

	P_GATE(G_NETISR, splevel);	/* should always be SPLNET */
	while (i = ffs(netisr)) {
		i--;
		netisr &= ~(1<<(i));
		V_GATE(G_NETISR, splevel);
		(*netisrvec[i])();
		P_GATE(G_NETISR, splevel);
	}
	V_GATE(G_NETISR, splevel);
}

netundef()
{
	printf("undefined netintr??");
}

