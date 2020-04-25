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

#ident "$Header: get_icmp.c 1.1 1991/07/31 00:02:44 $"

/*
 * Gather icmp stats
 */

/* $Log: get_icmp.c,v $
 *
 */

#include "defs.h"

extern char ipdev[];

/*
 * display ip/icmp specific stats
 */
get_icmp_mib_stats(mib_icmp)
	struct mib_icmp_struct *mib_icmp;
{
	init_mmap();		/* never know who's going to be the first */

	mib_icmp->icmpInErrors = 
	    icmpstat->icps_badcode + icmpstat->icps_tooshort + 
	    icmpstat->icps_checksum + icmpstat->icps_badlen;
	mib_icmp->icmpInMsgs = count_hist(icmpstat->icps_inhist) +
		mib_icmp->icmpInErrors;
	mib_icmp->icmpOutMsgs = count_hist(icmpstat->icps_outhist) +
		mib_icmp->icmpOutErrors;
	mib_icmp->icmpInDestUnreachs = icmpstat->icps_inhist[ICMP_UNREACH];
	mib_icmp->icmpInTimeExcds = icmpstat->icps_inhist[ICMP_TIMXCEED];
	mib_icmp->icmpInParmProbs = icmpstat->icps_inhist[ICMP_PARAMPROB];
	mib_icmp->icmpInSrcQuenchs = icmpstat->icps_inhist[ICMP_SOURCEQUENCH];
	mib_icmp->icmpInRedirects = icmpstat->icps_inhist[ICMP_REDIRECT] ;
	mib_icmp->icmpInEchos = icmpstat->icps_inhist[ICMP_ECHO];
	mib_icmp->icmpInEchoReps = icmpstat->icps_inhist[ICMP_ECHOREPLY];
	mib_icmp->icmpInTimestamps = icmpstat->icps_inhist[ICMP_TSTAMP];
	mib_icmp->icmpInTimestampReps = icmpstat->icps_inhist[ICMP_TSTAMPREPLY];
	mib_icmp->icmpInAddrMasks = icmpstat->icps_inhist[ICMP_MASKREQ];
	mib_icmp->icmpInAddrMaskReps = icmpstat->icps_inhist[ICMP_MASKREPLY];
	mib_icmp->icmpOutDestUnreachs = icmpstat->icps_outhist[ICMP_UNREACH];
	mib_icmp->icmpOutTimeExcds = icmpstat->icps_outhist[ICMP_TIMXCEED];
	mib_icmp->icmpOutParmProbs = icmpstat->icps_outhist[ICMP_PARAMPROB];
	mib_icmp->icmpOutSrcQuenchs = icmpstat->icps_outhist[ICMP_SOURCEQUENCH];
	mib_icmp->icmpOutRedirects = icmpstat->icps_outhist[ICMP_REDIRECT] ;
	mib_icmp->icmpOutEchos = icmpstat->icps_outhist[ICMP_ECHO];
	mib_icmp->icmpOutEchoReps = icmpstat->icps_outhist[ICMP_ECHOREPLY];
	mib_icmp->icmpOutTimestamps = icmpstat->icps_outhist[ICMP_TSTAMP];
	mib_icmp->icmpOutTimestampReps = icmpstat->icps_outhist[ICMP_TSTAMPREPLY];
	mib_icmp->icmpOutAddrMasks = icmpstat->icps_outhist[ICMP_MASKREQ];
	mib_icmp->icmpOutAddrMaskReps = icmpstat->icps_outhist[ICMP_MASKREPLY];

#ifdef	KERN3_2
	mib_icmp->icmpOutErrors = icmpstat->icps_outerrors +   
		icmpstat->icps_oldshort + icmpstat->icps_oldicmp;
#else
	mib_icmp->icmpOutErrors = 
		icmpstat->icps_oldshort + icmpstat->icps_oldicmp;
#endif

	return(0);
}

count_hist(h)
	int h[];
{
	int i, count = 0;

	for (i = 0; i < ICMP_MAXTYPE + 1; i++)
		count += h[i];
	return(count);
}
