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

#ident	"$Header: set_if.c 1.1 1991/07/31 00:03:58 $"

/*
 * set_if.c
 *	set interface stuff
 */

/* $Log: set_if.c,v $
 *
 */
#include "defs.h"

extern int errno;

set_ifEntry(ifTablep, mib_ifEntryp)
	struct ifTable *ifTablep;
	struct mib_ifEntry_struct *mib_ifEntryp;
{
	int flags = 0;
	struct ifreq ifr;
	int type;

	strcpy(ifr.ifr_name, ifTablep->ifname);
	if (ioctl(ctl_sock, SIOCGIFFLAGS, &ifr) < 0) {
		syslog(LOG_ERR, "SIOCGIFFLAGS: %m");
		return(-1);
	}
	if (mib_ifEntryp->ifAdminStatus == MIB_IFSTATUS_UP) {
		ifr.ifr_flags |= IFF_UP;
		type = SNMP_TRAP_LINKUP;
	} else {
		ifr.ifr_flags &= ~IFF_UP;
		type = SNMP_TRAP_LINKDOWN;
	}

	if (ioctl(ctl_sock, SIOCSIFFLAGS, &ifr) < 0) {
		syslog(LOG_ERR, "SIOCSIFFLAGS: %m");
		return(-1);
	}
	snmp_send_trap(type, 0);

	return(0);
}
