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

#ident	"$Header: snmp_auth.c 1.1 1991/07/31 00:06:09 $"

/*
 * snmp_auth.c -
 *   Authentication for SNMP (RFC 1067).  This implements a null
 * authentication layer.
 *
 *
 */

/* $Log: snmp_auth.c,v $
 *
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
#include <stdio.h>

#ifdef KINETICS
#include "gw.h"
#include "fp4/cmdmacro.h"
#endif

#if (defined(unix) && !defined(KINETICS))
#include <sys/types.h>
#include <netinet/in.h>
#ifndef NULL
#define NULL 0
#endif
#endif

#include "asn1.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "mib.h"
#include "debug.h"

extern struct comms	*Community;
extern struct mib_snmp_struct *mib_snmp;	

u_char *
snmp_auth_parse(data, length, community, clen, version)
    u_char	    *data;
    int		    *length;
    u_char	    *community;
    int		    *clen;
    long	    *version;
{
    u_char    type;

    data = asn_parse_header(data, length, &type);
    if (data == NULL){
	ERROR("bad header");
	return NULL;
    }
    if (type != (ASN_SEQUENCE | ASN_CONSTRUCTOR)){
	ERROR("wrong auth header type");
	return NULL;
    }
    data = asn_parse_int(data, length, &type, version, sizeof(*version));
    if (data == NULL){
	ERROR("bad parse of version");
	return NULL;
    }
    data = asn_parse_string(data, length, &type, community, clen);
    if (data == NULL){
	ERROR("bad parse of community");
	return NULL;
    }
    community[*clen] = '\0';
    return (u_char *)data;
}

u_char *
snmp_auth_build(data, length, community, clen, version, messagelen)
    u_char	    *data;
    int		    *length;
    u_char	    *community;
    int		    *clen;
    long	    *version;
    int		    messagelen;
{
    data = asn_build_header(data, length, (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR), messagelen + *clen + 5);
    if (data == NULL){
	mib_snmp->snmpInASNParseErrs++;
	ERROR("buildheader");
	return NULL;
    }
    data = asn_build_int(data, length,
	    (u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
	    (long *)version, sizeof(*version));
    if (data == NULL){
	mib_snmp->snmpInASNParseErrs++;
	ERROR("buildint");
	return NULL;
    }
    data = asn_build_string(data, length,
	    (u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OCTET_STR), 
	    community, *clen);
    if (data == NULL){
	mib_snmp->snmpInASNParseErrs++;
	ERROR("buildstring");
	return NULL;
    }
    return (u_char *)data;
}

int
authenticateCommunity(community_name, length, msg_type, sourceip)
    char *community_name;
    int length;
    int msg_type;
    ulong sourceip;
{
    struct comms *community;
    struct comms *getCommunity();
    extern struct mib_snmp_struct *mib_snmp;

    community = getCommunity(community_name, length);

    if ((community == NULL) || (community->access == NOACCESS)) {
	mib_snmp->snmpInBadCommunityNames++;
	if (mib_snmp->snmpEnableAuthTraps == 1)
	    snmp_send_trap(SNMP_TRAP_AUTHFAIL, 0);
	return(-1);
    }

    if (checkCommunityHost(community, sourceip) < 0) {
	mib_snmp->snmpInBadCommunityNames++;
	if (mib_snmp->snmpEnableAuthTraps == 1)
	    snmp_send_trap(SNMP_TRAP_AUTHFAIL, 0);
	return(-1);
    }

    if (msg_type == SET_REQ_MSG) {
	if (community->access != WRITE) {
	    mib_snmp->snmpInBadCommunityUses++;
	    if (mib_snmp->snmpEnableAuthTraps == 1)
		snmp_send_trap(SNMP_TRAP_AUTHFAIL, 0);
	    return(-1);
	}
    }

    return(0);
}
struct comms *
getCommunity(community_name, length)
    u_char	*community_name;
{
    struct comms *community;

    for(community = Community; community; community = community->next){
	if (length == strlen(community->name))
	    if (!memcmp(community->name, community_name, length))
		return community;
    }
    return NULL;
}

addCommunityHost(community, inaddr)
    struct comms *community;
    ulong inaddr;
{
    struct comm_host_list *hostp;
    
    /* allocate struct */
    if ((hostp = (struct comm_host_list *)malloc(sizeof(struct comm_host_list))) == NULL)
	return(-1);

    hostp->inaddr = inaddr;
    hostp->next = community->host_list;
    community->host_list = hostp;
    return(0);
}

checkCommunityHost(community, inaddr)
    struct comms *community;
    ulong inaddr;
{
    struct comm_host_list *hostp;

    /* move to the end of the list */
    for(hostp = community->host_list; hostp; hostp = hostp->next){
	if ((hostp->inaddr == 0) || (hostp->inaddr == inaddr))
	    return(0);
    }
    return(-1);
}



