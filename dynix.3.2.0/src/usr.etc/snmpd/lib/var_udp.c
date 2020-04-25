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

#ident	"$Header: var_udp.c 1.1 1991/07/31 00:06:31 $"

/*
 * var_udp.c 
 *   Resolve the variable pointed to by node.
 *
 */

/* $Log: var_udp.c,v $
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

#define UDPINDATAGRAMS 1
#define UDPNOPORTS 2
#define UDPOUTDATAGRAMS 3
#define UDPINERRORS 4
#define UDPTABLE 5

#define UDPENTRY 1

#define UDPLOCALADDRESS 1
#define UDPLOCALPORT 2

struct udp_entry{
    oid name[16];
    int namelen;
};

struct udp_entry udp_ent[] = {
    {{0}, 1 }
};

unsigned long udpExpTime = 0;

int
var_udp(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct udp_entry *udp_entp = udp_ent;
    int result;
    static struct mib_udp_struct Mib_udp;
    struct mib_udp_struct *mib_udp = &Mib_udp;
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - udpExpTime) > UDP_EXPTIME) {
	if (get_udp_stats(mib_udp) < 0)
	    return FAILURE;
	udpExpTime = tv.tv_sec;
    }

    result = check_instance(name, *length, node->objid, node->objlen, udp_entp->name, udp_entp->namelen);
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
	*length = udp_entp->namelen;
	memcpy(name, (char *)udp_entp->name, udp_entp->namelen * sizeof(oid));
    } else
	return FAILURE;
    
    *access_method = 0;
    node->val_len = sizeof(long); /* all following variables are sizeof long */

    switch (node->subid){
    case UDPINDATAGRAMS:
	*((u_long *)node->value) = mib_udp->udpInDatagrams;
        return SUCCESS;
    case UDPNOPORTS:
	*((u_long *)node->value) = mib_udp->udpNoPorts;
        return SUCCESS;
    case UDPOUTDATAGRAMS:
	*((u_long *)node->value) = mib_udp->udpInErrors;
        return SUCCESS;
    case UDPINERRORS:
	*((u_long *)node->value) = mib_udp->udpOutDatagrams;
        return SUCCESS;
    case UDPTABLE:
        return FAILURE;
    default:
        return FAILURE;
    }
    return FAILURE;
}
int
var_udpTable(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

unsigned long udpTblExpTime = 0;

int
var_udpEntry(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    static struct mib_udpEntry_struct *mib_udpEntry;
    struct mib_udpEntry_struct *udpEntry;
    static int nentries;
    struct timeval tv;
    int result;
    int i;
    int j;
    oid *tmpname;
    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - udpTblExpTime) > UDP_EXPTIME) {
	nentries = get_udp_pcb(&mib_udpEntry);
	udpTblExpTime = tv.tv_sec;
    }

    udpEntry = mib_udpEntry;
    
    for (i=0; i < nentries; i++, udpEntry++) {
	result = check_instance(name, *length, node->objid, node->objlen, udpEntry->objid, UDPTBLSIZE);
	if (exact) {
	    if (result == 0)	/* we have a match */
		break;			
	} else { 	/* !exact */
	    if (result < 0) 
		break;
	}
    }

    if (i == nentries) 
	return FAILURE;
	
    if (node->access == NOACCESS)
	return FAILURE;

    *access_method = 0;
    if (node->value){
	*length = UDPTBLSIZE;
	memcpy(name, (char *)udpEntry->objid, UDPTBLSIZE * sizeof(oid));
    } else
	return FAILURE;
    
    switch (node->subid){
    case UDPLOCALADDRESS:
        memcpy(node->value, &udpEntry->udpLocalAddress, node->val_len);
	return SUCCESS;
    case UDPLOCALPORT:
	memcpy(node->value, &udpEntry->udpLocalPort, sizeof(udpEntry->udpLocalPort));
        return SUCCESS;
    default:
        return FAILURE;
    }
    return FAILURE;
}

