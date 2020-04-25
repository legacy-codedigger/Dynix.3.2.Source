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

#ident	"$Header: get_if.c 1.1 1991/07/31 00:03:17 $"

/*
 * get_if.c
 *	get interface statistics
 */

/* $Log: get_if.c,v $
 *
 */

#include "defs.h"

extern int if_num_entries;

get_if(ifTablep)
	struct ifTable *ifTablep;
{
	struct ifreq ifr;
	int i;
	struct mib_ifEntry_struct *mp;

	mp = ifTablep->mib_ifEntry;
	if (strncmp(ifTablep->ifname, "lo", 2) == 0) {
		strcpy(mp->ifDescr, "Software Loopback Driver");
		mp->ifType = MIB_IFTYPE_SOFTWARELOOPBACK;
		mp->ifSpeed = 30000000;		/* arbitrary */
		bzero(mp->ifPhysAddress, sizeof(mp->ifPhysAddress));
		mp->PhysAddrLen = 0;
		bzero(mp->ifSpecific, sizeof(mp->ifSpecific));
		mp->SpecLen = 0;
	} 
	else if (strncmp(ifTablep->ifname, "se", 2) == 0) {
		strcpy(mp->ifDescr, "Sequent SCED Ethernet Card");
		mp->ifType = MIB_IFTYPE_ISO88023CSMACD;
		mp->ifSpeed = 10000000;		/* 10 MB */
		bzero(mp->ifSpecific, sizeof(mp->ifSpecific));
		mp->SpecLen = 0;
		get_ifnet_addr(ifTablep->ifname, mp->ifPhysAddress);
		mp->PhysAddrLen = 6;
	}

	strcpy(ifr.ifr_name, ifTablep->ifname);
	if (ioctl(ctl_sock, SIOCGIFMTU, &ifr) < 0) {
		syslog(LOG_ERR, "SIOCGIFMTU: %m");
		return(-1);
	}
	mp->ifMtu = ifr.ifr_mtu;
	if (ioctl(ctl_sock, SIOCGIFFLAGS, &ifr) < 0) {
		syslog(LOG_ERR, "SIOCGIFFLAGS: %m");
		return(-1);
	}
	if (ifr.ifr_flags & IFF_UP)
		mp->ifOperStatus = MIB_IFSTATUS_UP;
	else
		mp->ifOperStatus = MIB_IFSTATUS_DOWN;
	mp->ifAdminStatus = MIB_IFSTATUS_UP;

	get_if_stats(ifTablep->ifname, mp);

	return(0);
}

fill_ifTable(ifTablep)
	struct ifTable *ifTablep;
{
	int count = 0;
	struct ifconf ifc;
	struct ifreq *ifr;
	char buf[BUFSIZ], *cp, *cplim;

	init_mmap();		/* never know who's going to be the first */

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(ctl_sock, SIOCGIFCONF, (char *)&ifc) < 0) {
		syslog(LOG_ERR, "SIOCGIFCONF: %m");
		return(-1);
	}

	cplim = buf + ifc.ifc_len;
	for (cp = buf; cp < cplim; cp += sizeof(struct ifreq)) {
		ifr = (struct ifreq *)cp;

		ifTablep->fname[0] = 0;	/* unused */
		strcpy(ifTablep->ifname, ifr->ifr_name);

		if ((ifTablep->mib_ifEntry = (struct mib_ifEntry_struct *)
		    malloc(sizeof(struct mib_ifEntry_struct))) ==  NULL) {
			return (0);	/* XXX what about other memory */
		}
		count++;
		ifTablep->mib_ifEntry->ifLastChange = 0;
		ifTablep->mib_ifEntry->ifIndex = count + 1;

		ifTablep++;
	}
	return(count);
}

ifunit(name)
	char *name;
{
	register char *cp;

	for (cp = name; cp < name + IFNAMSIZ && *cp; cp++)
		if (*cp >= '0' && *cp <= '9')
			break;
	if (*cp == '\0' || cp == name + IFNAMSIZ)
		return (-1);
	return(*cp - '0');
}

struct ifnet_addrs {
	char	ea_name[IFNAMSIZ];
	int	ea_unit;
	char	ea_addr[6];
	struct	ifnet *ea_caddr;
} ifnet_addr[MAX_IF];

init_ifnet_addr()
{
	struct arpcom arpc;
	struct ifnet *ifp;
	int i = 0;
	static int initialized = 0;

	if (initialized)
		return;

	initialized = 1;

	init_mmap();		/* never know who's going to be the first */

	get_from_core(ifnet, &ifp, sizeof(ifp));	/* dereference ifnet */

	/*
	 * Walk the ifnet chain
	 */
	do {
		if (get_from_core(ifp, &arpc, sizeof(arpc)) < 0) {
			return(-1);
		}
		if (get_from_core(arpc.ac_if.if_name, ifnet_addr[i].ea_name,
		    IFNAMSIZ) < 0) {
			return(-1);
		}
		if (strcmp(ifnet_addr[i].ea_name, "se") == 0) {
			bcopy(arpc.ac_enaddr, ifnet_addr[i].ea_addr,
			sizeof(arpc.ac_enaddr));
		}
		sprintf(ifnet_addr[i].ea_name, "%s%d",
		ifnet_addr[i].ea_name, arpc.ac_if.if_unit);
		ifnet_addr[i].ea_caddr = ifp;
		i++;
		ifp = arpc.ac_if.if_next;
	} 
	while (ifp);

	for (; i < MAX_IF; i++)
		bzero(&ifnet_addr[i], sizeof(ifnet_addr[i]));
}

get_ifnet_addr(name, addr)
	char *name, *addr;
{
	int unit, i;

	unit = ifunit(name);
	if (unit == -1) {
		syslog(LOG_ERR, "can't parse ifunit: %s", name);
		return(-1);
	}

	init_ifnet_addr(); /* never know who's going to be the first */

	for (i = 0; i < MAX_IF; i++) {
		if (strcmp(ifnet_addr[i].ea_name, name) == 0) {
			bcopy(ifnet_addr[i].ea_addr, addr,
			sizeof(ifnet_addr[i].ea_addr));
			return;
		}
	}
	bzero(name, sizeof(ifnet_addr[0].ea_addr));	/* XXX */
}

get_if_stats(name, mp)
	char *name;
	struct mib_ifEntry_struct *mp;
{
	struct ifnet ifn;
	struct ifnet *ifp = 0;
	int i;

	init_ifnet_addr();	/* never know who's going to be the first */


	for (i = 0; i < MAX_IF; i++) {
		if (strcmp(ifnet_addr[i].ea_name, name) == 0) {
			ifp = ifnet_addr[i].ea_caddr;
			break;
		}
	}

	if (ifp == NULL) {
		syslog(LOG_WARNING, "get_if_stats: name not found: %s", name);
		return;
	}

	if (get_from_core(ifp, &ifn, sizeof(ifn)) < 0) 
		return(-1);

	/* XXX */ 

	mp->ifInUnknownProtos = 0;	/* everything received, via raw ether*/
	mp->ifOutDiscards = ifn.if_snd.ifq_drops;
	mp->ifOutErrors = ifn.if_oerrors;
	mp->ifOutQLen = ifn.if_snd.ifq_len;
	mp->ifInErrors = ifn.if_ierrors;

#ifdef	KERN3_2
	mp->ifInOctets = ifn.if_ibytes;
	mp->ifInUcastPkts = ifn.if_ipackets;	
	mp->ifInNUcastPkts = ifn.if_inunicast;
	mp->ifInDiscards = ifn.if_idiscards;
	mp->ifOutOctets = ifn.if_obytes;;
	mp->ifOutUcastPkts = ifn.if_opackets;
	mp->ifOutNUcastPkts = ifn.if_onunicast;
#else
	mp->ifInOctets = 0;
	mp->ifInUcastPkts = ifn.if_ipackets;	
	mp->ifInNUcastPkts = 0;
	mp->ifInDiscards = 0;
	mp->ifOutOctets = 0;
	mp->ifOutUcastPkts = ifn.if_opackets;
	mp->ifOutNUcastPkts = 0;
#endif
}

get_from_core(off, buf, size)
	off_t off;
	char *buf;
	int size;
{
	int n;

	lseek(km, off, 0);
	if ((n = read(km, buf, size)) != size) {
		syslog(LOG_ERR, "read %d bytes: %m", n);
		return(-1);
	}
	return(0);
}

