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

#ident	"$Header: snmp_vars.h 1.1 1991/07/31 00:06:15 $"

/*
 * snmp_vars.h
 *    Definitions for SNMP (RFC 1067) agent variable finder.
 *
 *
 */

/* $Log: snmp_vars.h,v $
 *
 */

/***********************************************************
	Copyright 1988, 1989 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/
/*
#define		MAX_INTERFACES	2
extern	struct mib_ifEntry  mib_ifEntry[MAX_INTERFACES];
extern	struct mib_ip	    mib_ip;
#define		ROUTE_ENTRIES	2
extern	struct mib_udp	    mib_udp;
extern	long	mib_icmpInMsgs;
extern	long	mib_icmpOutMsgs;
extern	long	mib_icmpInErrors;
extern	long	mib_icmpOutErrors;
extern	long	mib_icmpInCount[];
extern	long	mib_icmpOutCount[];
*/

int	var_generic();
int     var_system();
int	var_set_generic();
int     var_set_system();

int	var_if();
int	var_ifTable();
int	var_ifEntry();
int	var_set_if();
int	var_set_ifTable();
int	var_set_ifEntry();

int	var_at();
int	var_atTable();
int	var_atEntry();
int	var_set_at();
int	var_set_atTable();
int	var_set_atEntry();

int	var_ip();
int	var_set_ip();
int	var_ipAddrTable();
int	var_set_ipAddrTable();
int	var_ipAddrEntry();
int	var_set_ipAddrEntry();
int	var_ipRouteEntry();
int	var_set_ipRouteEntry();
int	var_ipRouteTable();
int	var_set_ipRouteTable();
int	var_ipNetToMediaTable();
int	var_set_ipNetToMediaTable();
int	var_ipNetToMediaEntry();
int	var_set_ipNetToMediaEntry();

int	var_icmp();
int	var_set_icmp();

int	var_tcp();
int	var_set_tcp();
int	var_tcpConnTable();
int	var_set_tcpConnTable();
int	var_tcpConnEntry();
int	var_set_tcpConnEntry();

int	var_udp();
int	var_set_udp();
int	var_udpTable();
int	var_set_udpTable();
int	var_udpEntry();
int	var_set_udpEntry();

int	var_egp();
int	var_set_egp();
int	var_snmp();
int	var_set_snmp();

u_char	*getStatPtr();


