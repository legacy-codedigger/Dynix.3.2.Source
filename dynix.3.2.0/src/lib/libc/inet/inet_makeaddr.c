/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: inet_makeaddr.c 2.1 89/07/25 $ */

#include <sys/types.h>
#include <netinet/in.h>

/*
 * Formulate an Internet address from network + host.  Used in
 * building addresses stored in the ifnet structure.
 */
struct in_addr
inet_makeaddr(net, host)
	int net, host;
{
	u_long addr;

	if (net < IN_CLASSA_MAX)
		addr = (net << IN_CLASSA_NSHIFT) | (host & IN_CLASSA_HOST);
	else if (net < IN_CLASSB_MAX)
		addr = (net << IN_CLASSB_NSHIFT) | (host & IN_CLASSB_HOST);
	else
		addr = (net << IN_CLASSC_NSHIFT) | (host & IN_CLASSC_HOST);
	addr = htonl(addr);
	return (*(struct in_addr *)&addr);
}
