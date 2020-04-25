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

#ident	"$Header: var_icmp.c 1.1 1991/07/31 00:06:23 $"

/*
 * var_icmp.c 
 *   Resolve the variable pointed to by node.
 *
 */

/* $Log: var_icmp.c,v $
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "asn1.h"
#include "parse.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "mib.h"
#include "snmp_vars.h"
#include "var.h"
#include "debug.h"

#define ICMPINMSGS            1
#define ICMPINERRORS          2
#define ICMPINDESTUNREACHS    3
#define ICMPINTIMEEXCDS       4
#define ICMPINPARMPROBS       5
#define ICMPINSRCQUENCHS      6
#define ICMPINREDIRECTS       7
#define ICMPINECHOS           8
#define ICMPINECHOREPS        9
#define ICMPINTIMESTAMPS     10
#define ICMPINTIMESTAMPREPS  11
#define ICMPINADDRMASKS      12
#define ICMPINADDRMASKREPS   13
#define ICMPOUTMSGS          14
#define ICMPOUTERRORS        15
#define ICMPOUTDESTUNREACHS  16
#define ICMPOUTTIMEEXCDS     17
#define ICMPOUTPARMPROBS     18
#define ICMPOUTSRCQUENCHS    19
#define ICMPOUTREDIRECTS     20
#define ICMPOUTECHOS         21
#define ICMPOUTECHOREPS      22
#define ICMPOUTTIMESTAMPS    23
#define ICMPOUTTIMESTAMPREPS 24
#define ICMPOUTADDRMASKS     25
#define ICMPOUTADDRMASKREPS  26

struct icmp_entry{
    oid name[16];
    int namelen;
};

struct icmp_entry icmp_ent[] = {
    {{0}, 1 }
};

unsigned long icmpExpTime = 0;

int
var_icmp(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */
{
    struct icmp_entry *icmp_entp = icmp_ent;
    int result;
    static struct mib_icmp_struct Mib_icmp;
    struct mib_icmp_struct *mib_icmp = &Mib_icmp;
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - icmpExpTime) > ICMP_EXPTIME) {
	if (get_icmp_mib_stats(mib_icmp) < 0)
	    return FAILURE;
	icmpExpTime = tv.tv_sec;
    }

    result = check_instance(name, *length, node->objid, node->objlen, icmp_entp->name, icmp_entp->namelen);
    if (exact) {
	if (result != 0)
	    return FAILURE;
    } else {
	if (result >= 0)
	    return FAILURE;
    }
    if (node->access == NOACCESS)
	return FAILURE;

    if (node->value) {
	*length = icmp_entp->namelen;
	memcpy(name, (char *)icmp_entp->name, icmp_entp->namelen * sizeof(oid));
    } else
	return FAILURE;
    
    *access_method = 0;
    node->val_len = sizeof(long); /* all following variables are sizeof long */

    switch (node->subid){
    case ICMPINMSGS:
	*((u_long *)node->value) = mib_icmp->icmpInMsgs;
	return SUCCESS;
    case ICMPINERRORS:
	*((u_long *)node->value) = mib_icmp->icmpInErrors;
	return SUCCESS;
    case ICMPINDESTUNREACHS:
	*((u_long *)node->value) = mib_icmp->icmpInDestUnreachs;
	return SUCCESS;
    case ICMPINTIMEEXCDS:
	*((u_long *)node->value) = mib_icmp->icmpInTimeExcds;
	return SUCCESS;
    case ICMPINPARMPROBS:
	*((u_long *)node->value) = mib_icmp->icmpInParmProbs;
	return SUCCESS;
    case ICMPINSRCQUENCHS:
	*((u_long *)node->value) = mib_icmp->icmpInSrcQuenchs;
	return SUCCESS;
    case ICMPINREDIRECTS:
	*((u_long *)node->value) = mib_icmp->icmpInRedirects;
	return SUCCESS;
    case ICMPINECHOS:
	*((u_long *)node->value) = mib_icmp->icmpInEchos;
	return SUCCESS;
    case ICMPINECHOREPS:
	*((u_long *)node->value) = mib_icmp->icmpInEchoReps;
	return SUCCESS;
    case ICMPINTIMESTAMPS:
	*((u_long *)node->value) = mib_icmp->icmpInTimestamps;
	return SUCCESS;
    case ICMPINTIMESTAMPREPS:
	*((u_long *)node->value) = mib_icmp->icmpInTimestampReps;
	return SUCCESS;
    case ICMPINADDRMASKS:
	*((u_long *)node->value) = mib_icmp->icmpInAddrMasks;
	return SUCCESS;
    case ICMPINADDRMASKREPS:
	*((u_long *)node->value) = mib_icmp->icmpInAddrMaskReps;
	return SUCCESS;
    case ICMPOUTMSGS:
	*((u_long *)node->value) = mib_icmp->icmpOutMsgs;
	return SUCCESS;
    case ICMPOUTERRORS:
	*((u_long *)node->value) = mib_icmp->icmpOutErrors;
	return SUCCESS;
    case ICMPOUTDESTUNREACHS:
	*((u_long *)node->value) = mib_icmp->icmpOutDestUnreachs;
	return SUCCESS;
    case ICMPOUTTIMEEXCDS:
	*((u_long *)node->value) = mib_icmp->icmpOutTimeExcds;
	return SUCCESS;
    case ICMPOUTPARMPROBS:
	*((u_long *)node->value) = mib_icmp->icmpOutParmProbs;
	return SUCCESS;
    case ICMPOUTSRCQUENCHS:
	*((u_long *)node->value) = mib_icmp->icmpOutSrcQuenchs;
	return SUCCESS;
    case ICMPOUTREDIRECTS:
	*((u_long *)node->value) = mib_icmp->icmpOutRedirects;
	return SUCCESS;
    case ICMPOUTECHOS:
	*((u_long *)node->value) = mib_icmp->icmpOutEchos;
	return SUCCESS;
    case ICMPOUTECHOREPS:
	*((u_long *)node->value) = mib_icmp->icmpOutEchoReps;
	return SUCCESS;
    case ICMPOUTTIMESTAMPS:
	*((u_long *)node->value) = mib_icmp->icmpOutTimestamps;
	return SUCCESS;
    case ICMPOUTTIMESTAMPREPS:
	*((u_long *)node->value) = mib_icmp->icmpOutTimestampReps;
	return SUCCESS;
    case ICMPOUTADDRMASKS:
	*((u_long *)node->value) = mib_icmp->icmpOutAddrMasks;
	return SUCCESS;
    case ICMPOUTADDRMASKREPS:
	*((u_long *)node->value) = mib_icmp->icmpOutAddrMaskReps;
	return SUCCESS;
    default:
	return FAILURE;
    }
}



