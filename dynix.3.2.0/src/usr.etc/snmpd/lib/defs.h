/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred. */

/* $Header: defs.h 1.1 1991/07/31 00:05:53 $*/

#ifdef	sequent
/*
 * Dynix 3 Include files
 */
#include <stdio.h>
#include <netdb.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include "asn1.h"
#include "parse.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "mib.h"
#include "snmp_vars.h"
#include "var.h"
#include "snmp_api.h"
#include "debug.h"
#include "mmap.h"

#ifndef	IP_MAXPACKET
#define IP_MAXPACKET      65535
#endif

#else 

/*
 * Dynix/ptx Include files
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <sys/net/net_prim.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip_stats.h>
#include <sys/net/ip_prim.h>
#include <netinet/route.h>
#include <sys/net/eth_prim.h>
#include <sys/net/arp_prim.h>
#include <sys/net/if_arp.h>
#include <sys/net/if_ether.h>

#include <fcntl.h>
#include <sys/errno.h>
#include <stdio.h>
#include "asn1.h"
#include "parse.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "mib.h"
#include "snmp_vars.h"
#include "var.h"
#include "snmp_api.h"
#include "debug.h"

#include <netdb.h>
#endif

extern	int	debug;
extern	int	errno;
