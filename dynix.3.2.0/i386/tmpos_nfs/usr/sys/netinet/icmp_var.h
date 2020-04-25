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
#ifndef _NETINET_ICMP_VAR_INCLUDED
#define _NETINET_ICMP_VAR_INCLUDED

/*
 * $Header: icmp_var.h 2.3 1991/04/30 17:17:10 $
 */

/* $Log: icmp_var.h,v $
 *
 */

/*
 * Variables related to this implementation
 * of the internet control message protocol.
 */
struct	icmpstat {
/* statistics related to icmp packets generated */
	int	icps_error;		/* # of calls to icmp_error */
	int	icps_oldshort;		/* no error 'cuz old ip too short */
	int	icps_oldicmp;		/* no error 'cuz old was icmp */
	int	icps_outhist[ICMP_MAXTYPE + 1];
/* statistics related to input messages processed */
 	int	icps_badcode;		/* icmp_code out of range */
	int	icps_tooshort;		/* packet < ICMP_MINLEN */
	int	icps_checksum;		/* bad checksum */
	int	icps_badlen;		/* calculated bound mismatch */
	int	icps_reflect;		/* number of responses */
	int	icps_inhist[ICMP_MAXTYPE + 1];
        int     icps_outerrors;         /* output errors other than above */
};

#ifdef KERNEL
extern	struct	icmpstat icmpstat;
#endif
#endif	/* _NETINET_ICMP_VAR_INCLUDED */
