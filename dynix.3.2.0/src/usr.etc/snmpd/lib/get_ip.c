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

/* $Header: get_ip.c 1.2 1991/08/02 16:12:07 $ */

/* $Log: get_ip.c,v $
 *
 *
 */

#include "defs.h"

extern struct ifTable IfTable[];
extern int if_num_entries;
extern int mib_ipAddrEntry_nentries;
extern struct mib_ipAddrEntry_struct *mib_ipAddrEntry;

get_ip_mib_stats(mib_ip)
	struct mib_ip_struct *mib_ip;
{
	init_mmap();		/* never know who's going to be the first */

	if (*ipforwarding)
		mib_ip->ipForwarding = 1;
	else
		mib_ip->ipForwarding = 2;
	mib_ip->ipInReceives = ipstat->ips_total;
	mib_ip->ipInHdrErrors = ipstat->ips_badsum + ipstat->ips_tooshort +
	    ipstat->ips_toosmall + ipstat->ips_badhlen +ipstat->ips_badlen +
	    ipstat->ips_fragdropped;
	mib_ip->ipInAddrErrors = ipstat->ips_cantforward;
	mib_ip->ipForwDatagrams = ipstat->ips_forward;
	mib_ip->ipInDiscards = ipintrq->ifq_drops;
	mib_ip->ipInDelivers =  mib_ip->ipInReceives - (mib_ip->ipInHdrErrors +
	    mib_ip->ipForwDatagrams + mib_ip->ipInAddrErrors);
	mib_ip->ipReasmTimeout = IPFRAGTTL;
	mib_ip->ipReasmReqds = ipstat->ips_fragments;
	mib_ip->ipReasmFails = ipstat->ips_fragdropped+ipstat->ips_fragtimeout;

#ifdef	KERN3_2
	mib_ip->ipDefaultTTL = *ip_ttl;
	mib_ip->ipInUnknownProtos = ipstat->ips_unknownproto;
	mib_ip->ipOutRequests = ipstat->ips_outreq;
	mib_ip->ipOutDiscards = ipstat->ips_discards;
	mib_ip->ipOutNoRoutes = ipstat->ips_noroute;
	mib_ip->ipReasmOKs = ipstat->ips_reassok;
	mib_ip->ipFragFails = ipstat->ips_fragfail;
	mib_ip->ipFragOKs = ipstat->ips_fragok;
	mib_ip->ipFragCreates = ipstat->ips_newfrags;
#else
	mib_ip->ipDefaultTTL = MAXTTL;
	mib_ip->ipInUnknownProtos = 0;
	mib_ip->ipOutRequests = 0;
	mib_ip->ipOutDiscards = 0;
	mib_ip->ipOutNoRoutes = 0;
	mib_ip->ipReasmOKs = 0;
	mib_ip->ipFragFails = 0;
	mib_ip->ipFragOKs = 0;
	mib_ip->ipFragCreates = 0;
#endif
	return(0);
}

int
compare_ipAddrEnt(entry1, entry2)
	struct mib_ipAddrEntry_struct *entry1, *entry2;
{
	if ((u_long)(ntohl(entry1->ipAdEntAddr)) >
	    (u_long)(ntohl(entry2->ipAdEntAddr)))
		return(1);
	else if ((u_long)(ntohl(entry1->ipAdEntAddr)) <
	    (u_long)(ntohl(entry2->ipAdEntAddr)))
		return(-1);

	return(0);
}


get_ipAddr(mib_ipAE)
	struct mib_ipAddrEntry_struct **mib_ipAE;
{
	struct mib_ipAddrEntry_struct *ipAE;
	struct ifTable *ifTablep;
	struct ifreq ifr;
	int i, j, nentries, count;
	struct sockaddr_in *sin;

	ifTablep = IfTable;
	if (if_num_entries == 0)
		if_num_entries = fill_ifTable(ifTablep);
	count = nentries = if_num_entries;

	if (*mib_ipAE)
		free((char *)(*mib_ipAE));

	ipAE = *mib_ipAE = (struct mib_ipAddrEntry_struct *)
		malloc(nentries * sizeof(struct mib_ipAddrEntry_struct));


	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	for (i=0; i < nentries; i++, ifTablep++) {
		strcpy(ifr.ifr_name, ifTablep->ifname);
		if (ioctl(ctl_sock, SIOCGIFADDR, (char *) &ifr) < 0) {
			continue;	/* no address configured */
		}
		ipAE->ipAdEntAddr = sin->sin_addr.s_addr;
		ipAE->ipAdEntIfIndex = i+1;

		if (ioctl(ctl_sock, SIOCGIFNETMASK, (char *) &ifr) < 0) {
			syslog(LOG_ERR, "ioctl: SIOCGIFNETMASK: %m\n");
			return(-1);
		}
		ipAE->ipAdEntNetMask = sin->sin_addr.s_addr;

		if (ioctl(ctl_sock, SIOCGIFFLAGS, (char *) &ifr) < 0) {
			syslog(LOG_ERR, "ioctl: SIOCGIFFLAGS: %m\n");
			return(-1);
		}
		if (ifr.ifr_flags & IFF_BROADCAST) {
			if (ioctl(ctl_sock, SIOCGIFBRDADDR, (char *) &ifr)< 0) {
				syslog(LOG_ERR, "ioctl: SIOCGIFBRDADDR: %m\n");
				return(-1);
			}
			ipAE->ipAdEntBcastAddr =
			    (htonl(sin->sin_addr.s_addr) & 0x1);
		} 
		else
			ipAE->ipAdEntBcastAddr = 0;
		ipAE->ipAdEntReasmMaxSize = IP_MAXPACKET;


		/* build the objid extension for this entry */
		j=0;
		ipAE->objid[j++] = (oid)(ipAE->ipAdEntAddr & 0xff);
		ipAE->objid[j++] = (oid)((ipAE->ipAdEntAddr & 0xff00) >> 8);
		ipAE->objid[j++] = (oid)((ipAE->ipAdEntAddr & 0xff0000) >> 16);
		ipAE->objid[j++] = (oid)((ipAE->ipAdEntAddr & 0xff000000) >> 24);
		ipAE++;
		count--;
	}

	nentries -= count;
	qsort((char *)(*mib_ipAE), nentries, sizeof(struct mib_ipAddrEntry_struct), compare_ipAddrEnt);

	return(nentries);
}

int
compare_ipNetToMediaEnt(entry1, entry2)
	struct mib_ipNetToMediaEntry_struct *entry1, *entry2;
{
	if (entry1->ipNetToMediaIfIndex > entry2->ipNetToMediaIfIndex)
		return(1);
	else if (entry1->ipNetToMediaIfIndex < entry2->ipNetToMediaIfIndex)
		return(-1);

	if ((u_long)(ntohl(entry1->ipNetToMediaNetAddress)) >
	    (u_long)(ntohl(entry2->ipNetToMediaNetAddress)))
		return(1);
	else if ((u_long)(ntohl(entry1->ipNetToMediaNetAddress)) <
	    (u_long)(ntohl(entry2->ipNetToMediaNetAddress)))
		return(-1);

	return(0);
}

get_ipNetToMedia(mib_ipNetToMediaEntry)
	struct mib_ipNetToMediaEntry_struct **mib_ipNetToMediaEntry;
{
	struct mib_ipNetToMediaEntry_struct *ipe;
	struct arptab *at, *atp;
	int nentries, i, j, k, l, needfree;
	int fl = 0, size;
	struct mib_ipAddrEntry_struct *ipAE;
	struct ifTable *ifTablep;
	int count;

	init_mmap();		/* never know who's going to be the first */

	atp = arptab;
	count = nentries = arptab_size;

	if (*mib_ipNetToMediaEntry)
		free((char *)(*mib_ipNetToMediaEntry));

	ipe = (struct mib_ipNetToMediaEntry_struct *)
		malloc(nentries * sizeof(struct mib_ipNetToMediaEntry_struct));
	*mib_ipNetToMediaEntry = ipe;

	if (ipe == NULL)
		return(-1);

	for (i=0; i < nentries; i++, atp++) {
		if (atp->at_flags == 0)
			continue;	/* empty entry */
		count--;
		memcpy(ipe->ipNetToMediaPhysAddress, atp->at_enaddr, 6);
		ipe->ipNetToMediaNetAddress = atp->at_iaddr.s_addr;
		if (atp->at_flags & ATF_PERM)
			ipe->ipNetToMediaType = MIB_IPNETOTMEDIATTYPE_STATIC;
		else
			ipe->ipNetToMediaType = MIB_IPNETOTMEDIATTYPE_DYNAMIC;
		ipe->PhysAddrLen = 6;

		ipe->ipNetToMediaIfIndex = 0;

		/*
		 * In order to find the index into the interface table, we 
		 * must first see if it has been built. Then search thru it 
		 * for an interface name that matches the name returned by 
		 * if_config. note: there does not have to be an entry in 
		 * the interface table, as not all entries in the 
		 * interface table have to be configured with ip.
		 */
		ifTablep = IfTable;
		if (if_num_entries == 0)
			if_num_entries = fill_ifTable(ifTablep);

		/* get ip config info */
		if (mib_ipAddrEntry_nentries == 0)
			mib_ipAddrEntry_nentries = get_ipAddr(&mib_ipAddrEntry);

		/* check the subnet mask for each if entry */
		ipAE = mib_ipAddrEntry;
		for (l=0; l<mib_ipAddrEntry_nentries; l++, ipAE++){
		    if ((ipAE->ipAdEntAddr & ipAE->ipAdEntNetMask)
			== (atp->at_iaddr.s_addr & ipAE->ipAdEntNetMask)) {
			ipe->ipNetToMediaIfIndex = ipAE->ipAdEntIfIndex;
			break;
		    }
		}

		/* build the objid extension for this entry */
		j=0;

		ipe->objid[j++] = (oid)(ipe->ipNetToMediaIfIndex);
		ipe->objid[j++] = (oid)(ipe->ipNetToMediaNetAddress & 0xff);
		ipe->objid[j++] = (oid)((ipe->ipNetToMediaNetAddress & 0xff00) >> 8);
		ipe->objid[j++] = (oid)((ipe->ipNetToMediaNetAddress & 0xff0000) >> 16);
		ipe->objid[j++] = (oid)((ipe->ipNetToMediaNetAddress & 0xff000000) >> 24);
		ipe++;
	}

	nentries -= count;
	qsort((char *)(*mib_ipNetToMediaEntry), nentries, sizeof(struct mib_ipNetToMediaEntry_struct), compare_ipNetToMediaEnt);

	return(nentries);
}

int
compare_ipRouteEnt(entry1, entry2)
	struct mib_ipRouteEntry_struct *entry1, *entry2;
{

	if ((u_long)(ntohl(entry1->ipRouteDest)) >
	    (u_long)(ntohl(entry2->ipRouteDest)))
		return(1);
	else if ((u_long)(ntohl(entry1->ipRouteDest)) <
	    (u_long)(ntohl(entry2->ipRouteDest)))
		return(-1);

	return(0);
}

get_ipRoute(mib_ipRouteEntry)
	struct mib_ipRouteEntry_struct **mib_ipRouteEntry;
{
	struct mbuf **rthashhead;
	struct mbuf *m;
	struct rtentry *rt;
	struct sockaddr_in *sin;
	int doinghost = 1;
	int i, nentries, count;
	struct mib_ipRouteEntry_struct *ipr;

	init_mmap();		/* never know who's going to be the first */

	rthashhead = rthost;

	nentries = count_routes();
	nentries += 10;		/* slop */
	count = nentries;

	if (*mib_ipRouteEntry)
		free((char *)(*mib_ipRouteEntry));

	ipr = *mib_ipRouteEntry = (struct mib_ipRouteEntry_struct *)
		malloc(nentries * sizeof(struct mib_ipRouteEntry_struct));
	if (if_num_entries == 0)
		if_num_entries = fill_ifTable(IfTable);
	if (mib_ipAddrEntry_nentries == 0)
		mib_ipAddrEntry_nentries = get_ipAddr(&mib_ipAddrEntry);


again:
	for (i = 0; i < rthashsize; i++) {
		if (rthashhead[i] == NULL)
			continue;
		m = rthashhead[i];
		while (m) {
			m = mbuf_unmap(m, struct mbuf *);
			rt = mtod(m, struct rtentry *);
			if (rt < (struct rtentry *) mlo || 
			    rt > (struct rtentry *) mhi)
				break;
			assign_route(rt, ipr);
			m = m->m_next;
			ipr++;
			count--;
		}
	}

	if (doinghost) {
		doinghost = 0;
		rthashhead = rtnet;
		goto again;
	}

	nentries -= count;
	qsort((char *)(*mib_ipRouteEntry), nentries, sizeof(struct mib_ipRouteEntry_struct), compare_ipRouteEnt);

	return(nentries);
}

count_routes()
{
	struct mbuf **rthashhead;
	int doinghost = 1;
	int i, count = 0;
	struct mbuf *m;

	rthashhead = rthost;
again:
	for (i = 0; i < rthashsize; i++) {
		if (rthashhead[i] == NULL)
			continue;
		m = rthashhead[i];
		while (m) {
			count++;
			m = mbuf_unmap(m, struct mbuf *);
			m = m->m_next;
		}
	}
	if (doinghost) {
		doinghost = 0;
		rthashhead = rtnet;
		goto again;
	}
	return(count);
}

assign_route(rt, ipr)
	struct rtentry *rt;
	struct mib_ipRouteEntry_struct *ipr;
{
	struct sockaddr_in *sin;
	int j, l;
	struct ifTable *ifTablep;
	struct mib_ipAddrEntry_struct *ipAddrEntry;

	sin = (struct sockaddr_in *)&rt->rt_dst;
	ipr->ipRouteDest =  sin->sin_addr.s_addr;
	ipr->ipRouteMetric1 = 0;
	ipr->ipRouteMetric2 = 0;
	ipr->ipRouteMetric3 = 0;
	ipr->ipRouteMetric4 = 0;
	sin = (struct sockaddr_in *)&rt->rt_gateway;
	ipr->ipRouteNextHop = sin->sin_addr.s_addr;
	if (rt->rt_flags & RTF_GATEWAY)
		ipr->ipRouteType = MIB_IPROUTETYPE_REMOTE;
	else
		ipr->ipRouteType = MIB_IPROUTETYPE_DIRECT;
	if (rt->rt_flags & RTF_DYNAMIC)
		ipr->ipRouteProto = MIB_IPROUTEPROTO_ICMP;
	else
		ipr->ipRouteProto = MIB_IPROUTEPROTO_LOCAL;

	ipr->ipRouteAge = 0;
	ipr->ipRouteMask = 0;
	ipr->ipRouteIfIndex = 0;

	ifTablep = IfTable;
	ipAddrEntry = mib_ipAddrEntry;
	for (l=0; l<mib_ipAddrEntry_nentries; l++, ipAddrEntry++){
		if ((ipAddrEntry->ipAdEntAddr & ipAddrEntry->ipAdEntNetMask) == 
		    (ipr->ipRouteDest & ipAddrEntry->ipAdEntNetMask)){
			ipr->ipRouteMask = ipAddrEntry->ipAdEntNetMask;
			ipr->ipRouteIfIndex = ipAddrEntry->ipAdEntIfIndex;
			break;
		}
	}
	if (ipr->ipRouteDest == 0)
		ipr->ipRouteMask = 0;


	/* build the objid extension for this entry */
	j=0;
	ipr->objid[j++] = (oid)(ipr->ipRouteDest & 0xff);
	ipr->objid[j++] = (oid)((ipr->ipRouteDest & 0xff00) >> 8);
	ipr->objid[j++] = (oid)((ipr->ipRouteDest & 0xff0000) >> 16);
	ipr->objid[j++] = (oid)((ipr->ipRouteDest & 0xff000000) >> 24);
}
