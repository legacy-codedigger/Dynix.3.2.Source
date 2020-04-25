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

#ident	"$Header: set_at.c 1.1 1991/07/31 00:03:50 $"

/*
 * get_ip.c
 *	get ip stats
 */

/* $Log: set_at.c,v $
 *
 */

#include "defs.h"

set_atEntry(mib_atEntry)
	struct mib_atEntry_struct *mib_atEntry;
{
	static char zeroPhysAddr[6] = { 0,0,0,0,0,0 };

	if (bcmp(mib_atEntry->atPhysAddress, zeroPhysAddr, 6) == 0)
		/* do a delete */
		return(set_at_del(mib_atEntry));
	else
		/* do an add */
		return(set_at_add(mib_atEntry));
}


set_at_add(mib_atEntry)
	struct mib_atEntry_struct *mib_atEntry;
{
	struct arpreq ar;
	struct sockaddr_in *sin;

	bzero(&ar, sizeof(ar));
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_atEntry->atNetAddress;
    	bcopy(mib_atEntry->atPhysAddress, ar.arp_ha.sa_data, 6); 

    	ar.arp_flags = ATF_PERM;	/* XXX */
    
	if (ioctl(ctl_sock, SIOCSARP, (caddr_t)&ar) < 0) {
		syslog(LOG_ERR, "SIOCSARP: %m\n");
		return(-1);
	}
	return (0);
}

set_at_del(mib_atEntry)
	struct mib_atEntry_struct *mib_atEntry;
{
	struct arpreq ar;
	struct sockaddr_in *sin;

	bzero(&ar, sizeof(ar));
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = mib_atEntry->atNetAddress;

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
