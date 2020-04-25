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

#ident	"$Header: set_ip.c 1.1 1991/07/31 00:04:05 $"

/*
 * get_ip.c
 *	get ip stats
 */

/* $Log: set_ip.c,v $
 *
 */

#include "defs.h"

extern char ipdev[];
extern int errno;

set_ipNetToMedia(mib_ipNetToMediaEntry)
	struct mib_ipNetToMediaEntry_struct *mib_ipNetToMediaEntry;
{
	if (mib_ipNetToMediaEntry->ipNetToMediaType == MIB_IPAT_INVALID)
		/* do a delete */
		return(set_ipNetToMedia_del(mib_ipNetToMediaEntry));
	else
		/* do an add */
		return(set_ipNetToMedia_add(mib_ipNetToMediaEntry));
}


set_ipNetToMedia_add(mib_ipNetToMediaEntry)
	struct mib_ipNetToMediaEntry_struct *mib_ipNetToMediaEntry;
{
	struct arpreq ar;
	struct sockaddr_in *sin;

	bzero(&ar, sizeof(ar));
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_ipNetToMediaEntry->ipNetToMediaNetAddress;
	bcopy(mib_ipNetToMediaEntry->ipNetToMediaPhysAddress,
	ar.arp_ha.sa_data, 6); 

	ar.arp_flags = 0;
	if (mib_ipNetToMediaEntry->ipNetToMediaType == MIB_IPAT_STATIC)
		ar.arp_flags = ATF_PERM;

	if (ioctl(ctl_sock, SIOCSARP, (caddr_t)&ar) < 0) {
		syslog(LOG_ERR, "SIOCSARP: %m\n");
		return(-1);
	}
	return (0);
}

set_ipNetToMedia_del(mib_ipNetToMediaEntry)
	struct mib_ipNetToMediaEntry_struct *mib_ipNetToMediaEntry;
{
	struct arpreq ar;
	struct sockaddr_in *sin;

	bzero(&ar, sizeof(ar));
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_ipNetToMediaEntry->ipNetToMediaNetAddress;

	ar.arp_flags = 0;

	if (ioctl(ctl_sock, SIOCDARP, (caddr_t)&ar) < 0) {
		if (errno == ENXIO)
			syslog(LOG_ERR, 
			"attempt to delete non-existant arp entry: %s\n",
			inet_ntoa(sin->sin_addr.s_addr));
		else
			syslog(LOG_ERR, "SIOCDARP: %m\n");
		return(-1);
	}
	return (0);
}

set_ipRoute(mib_ipRouteEntry)
	struct mib_ipRouteEntry_struct *mib_ipRouteEntry;
{
	if (mib_ipRouteEntry->ipRouteType == MIB_IPROUTETYPE_INVALID)
		/* do a delete */
		return(set_ipRoute_del(mib_ipRouteEntry));
	else
		/* do an add */
		return(set_ipRoute_add(mib_ipRouteEntry));
}

set_ipRoute_add(mib_ipRouteEntry)
	struct mib_ipRouteEntry_struct *mib_ipRouteEntry;
{
	struct rtentry rt;
	struct sockaddr_in *sin;

	bzero(&rt, sizeof(rt));

	sin = (struct sockaddr_in *)&rt.rt_dst;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_ipRouteEntry->ipRouteDest;
	sin = (struct sockaddr_in *)&rt.rt_gateway;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_ipRouteEntry->ipRouteNextHop;
	/* 
	 * Set flags 
	 */
	rt.rt_flags = RTF_UP /* XXX | RTF_NETMAN */;
	/*
		 * if the route is a remote one the set the GATEWAY flag
		 */
	if (mib_ipRouteEntry->ipRouteType == MIB_IPROUTETYPE_REMOTE)
		rt.rt_flags |= RTF_GATEWAY;

	/* set the host flag if neccessary */
	if (snmp_set_hostflg(&rt) < 0)
		return (-1);

	if (ioctl(ctl_sock, SIOCADDRT, (caddr_t)&rt) < 0) {
		syslog(LOG_ERR, "SIOCADDRT: %m");
		return(-1);
	}

	return (0);
}

set_ipRoute_del(mib_ipRouteEntry)
	struct mib_ipRouteEntry_struct *mib_ipRouteEntry;
{
	struct rtentry rt;
	struct sockaddr_in *sin;

	bzero(&rt, sizeof(rt));

	sin = (struct sockaddr_in *)&rt.rt_dst;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_ipRouteEntry->ipRouteDest;
	sin = (struct sockaddr_in *)&rt.rt_gateway;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_ipRouteEntry->ipRouteNextHop;

	/* set the host flag if neccessary */
	if (snmp_set_hostflg(&rt) < 0)
		return (-1);

	if (ioctl(ctl_sock, SIOCDELRT, (caddr_t)&rt) < 0) {
		syslog(LOG_ERR, "SIOCDELRT: %m");
		return(-1);
	}

	return (0);
}

set_ip(mib_ip)
	struct mib_ip_struct *mib_ip;
{
#ifdef	KERN3_2

	if ((u_int)mib_ip->ipDefaultTTL > MAXTTL)
		return(-1);

	*ip_ttl = mib_ip->ipDefaultTTL;
#endif

	if (mib_ip->ipForwarding == 1)	/* do forward */
		*ipforwarding = 1;
	else				/* don't forward */
		*ipforwarding = 0;

	return(0);
}

snmp_set_hostflg(rt)
	struct rtentry *rt;
{
	extern struct ifTable IfTable[];
	extern int if_num_entries;
	struct mib_ipAddrEntry_struct *ipAddrEntry;
	struct ifTable *ifTablep;
	extern struct mib_ipAddrEntry_struct *mib_ipAddrEntry;
	extern int mib_ipAddrEntry_nentries;
	int l;
	struct sockaddr_in *sin;

	init_mmap();		/* never know who's going to be the first */

	/*
	 * In order to determine if it is a host or net route we must first 
	 * see build a list of ip interfaces. Then check the gateway address
	 * against each interface to determine which interface this route will
	 * go over. Then take the subnet mask for that interface and apply it
	 * to the route address. This is actually an educated guess if it 
	 * is a network address or host address.
	 */
	ifTablep = IfTable;
	if (if_num_entries == 0)
		if_num_entries = fill_ifTable(ifTablep);

	/* get ip config info */
	if (mib_ipAddrEntry_nentries == 0)
		mib_ipAddrEntry_nentries = get_ipAddr(&mib_ipAddrEntry);

	/* 
	 * Find the interface this route will go out over by checking that 
	 * the net address for the interface == the gateway net address
	 */
	ipAddrEntry = mib_ipAddrEntry;
	sin = (struct sockaddr_in *)&rt->rt_gateway;
	for (l=0; l<mib_ipAddrEntry_nentries; l++, ipAddrEntry++){
		if ((ipAddrEntry->ipAdEntAddr & ipAddrEntry->ipAdEntNetMask) 
		    == (sin->sin_addr.s_addr & ipAddrEntry->ipAdEntNetMask))
			break;
	}

	/* now find out if the address is a network or host */
	if (ipAddrEntry) {
		sin = (struct sockaddr_in *)&rt->rt_dst;
		if (sin->sin_addr.s_addr & ~ipAddrEntry->ipAdEntNetMask)
			rt->rt_flags |= RTF_HOST;
		return (0);
	}

	return (-1);
}
