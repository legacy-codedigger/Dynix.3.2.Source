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

#ident	"$Header: var_tcp.c 1.1 1991/07/31 00:06:30 $"

/*
 * var_tcp.c 
 *   Resolve the variable pointed to by node.
 *
 */

/* $Log: var_tcp.c,v $
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

#define TCPRTOALGORITHM		1
#define TCPRTOMIN		2
#define TCPRTOMAX		3
#define TCPMAXCONN		4
#define TCPACTIVEOPENS		5
#define TCPPASSIVEOPENS		6
#define TCPATTEMPTFAILS		7
#define TCPESTABRESETS		8
#define TCPCURRESTAB		9
#define TCPINSEGS		10
#define TCPOUTSEGS		11
#define TCPRETRANSSEGS		12
#define TCPCONNTABLE		13
#define TCPINERRS		14
#define TCPOUTRSTS		15

#define TCPCONNENTRY		1

#define TCPCONNSTATE		1
#define TCPCONNLOCALADDRESS	2
#define TCPCONNLOCALPORT	3
#define TCPCONNREMADDRESS	4
#define TCPCONNREMPORT		5

struct tcp_entry{
    oid name[16];
    int namelen;
};

struct tcp_entry tcp_ent[] = {
    {{0}, 1 }
};

unsigned long tcpExpTime = 0;

int
var_tcp(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct tcp_entry *tcp_entp = tcp_ent;
    int result;
    static struct mib_tcp_struct Mib_tcp;
    struct mib_tcp_struct *mib_tcp = &Mib_tcp;
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - tcpExpTime) > TCP_EXPTIME) {
	if (get_tcp_mib_stats(mib_tcp) < 0)
	    return FAILURE;
	tcpExpTime = tv.tv_sec;
    }

    result = check_instance(name, *length, node->objid, node->objlen, tcp_entp->name, tcp_entp->namelen);
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
	*length = tcp_entp->namelen;
	memcpy(name, (char *)tcp_entp->name, tcp_entp->namelen * sizeof(oid));
    } else
	return FAILURE;
    
    *access_method = 0;
    node->val_len = sizeof(long); /* all following variables are sizeof long */

    switch (node->subid){
    case TCPRTOALGORITHM:
	*((u_long *)node->value) = mib_tcp->tcpRtoAlgorithm;
	return SUCCESS;
    case TCPRTOMIN:
	*((u_long *)node->value) = mib_tcp->tcpRtoMin;
	return SUCCESS;
    case TCPRTOMAX:
	*((u_long *)node->value) = mib_tcp->tcpRtoMax;
	return SUCCESS;
    case TCPMAXCONN:
	*((u_long *)node->value) = mib_tcp->tcpMaxConn;
	return SUCCESS;
    case TCPACTIVEOPENS:
	*((u_long *)node->value) = mib_tcp->tcpActiveOpens;
	return SUCCESS;
    case TCPPASSIVEOPENS:
	*((u_long *)node->value) = mib_tcp->tcpPassiveOpens;
	return SUCCESS;
    case TCPATTEMPTFAILS:
	*((u_long *)node->value) = mib_tcp->tcpAttemptFails;
	return SUCCESS;
    case TCPESTABRESETS:
	*((u_long *)node->value) = mib_tcp->tcpEstabResets;
	return SUCCESS;
    case TCPCURRESTAB:
	*((u_long *)node->value) = mib_tcp->tcpCurrEstab;
	return SUCCESS;
    case TCPINSEGS:
	*((u_long *)node->value) = mib_tcp->tcpInSegs;
	return SUCCESS;
    case TCPOUTSEGS:
	*((u_long *)node->value) = mib_tcp->tcpOutSegs;
	return SUCCESS;
    case TCPRETRANSSEGS:
	*((u_long *)node->value) = mib_tcp->tcpRetransSegs;
	return SUCCESS;
    case TCPCONNTABLE:
        return FAILURE;
    case TCPINERRS:
	*((u_long *)node->value) = mib_tcp->tcpInErrs;
	return SUCCESS;
    case TCPOUTRSTS:
	*((u_long *)node->value) = mib_tcp->tcpOutRsts;
	return SUCCESS;
    default:
        return FAILURE;
    }
    return FAILURE;
}
int
var_tcpConnTable(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

int
var_tcpConnEntry(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct tcp_entry *tcp_connentp = tcp_ent;
    static struct mib_tcpConnEntry_struct *mib_tcpConnEntry;
    struct mib_tcpConnEntry_struct *tcpConnEntry;
    static int nentries;
    static unsigned long tcpConnExpTime;
    struct timeval tv;
    int result;
    int i;

    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - tcpConnExpTime) > 10) {
	nentries = get_tcp_pcb(&mib_tcpConnEntry);
	tcpConnExpTime = tv.tv_sec;
    }

    tcpConnEntry = mib_tcpConnEntry;
    
    for (i=0; i < nentries; i++, tcpConnEntry++) {
	result = check_instance(name, *length, node->objid, node->objlen, tcpConnEntry->objid, TCPCONNSIZE);
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
	*length = TCPCONNSIZE;
	memcpy(name, (char *)tcpConnEntry->objid, TCPCONNSIZE * sizeof(oid));
    } else
	return FAILURE;
    
    switch (node->subid){
    case TCPCONNSTATE:
	    *((u_long *)node->value) = tcpConnEntry->tcpConnState;
	    return SUCCESS;
    case TCPCONNLOCALADDRESS:
	    memcpy(node->value, &tcpConnEntry->tcpConnLocalAddress, node->val_len); 
	    return SUCCESS;
    case TCPCONNLOCALPORT:
	    memcpy(node->value, &tcpConnEntry->tcpConnLocalPort, sizeof(tcpConnEntry->tcpConnLocalPort));
	    return SUCCESS;
    case TCPCONNREMADDRESS:
	    memcpy(node->value, &tcpConnEntry->tcpConnRemAddress, node->val_len);
	    return SUCCESS;
    case TCPCONNREMPORT:
	    memcpy(node->value, &tcpConnEntry->tcpConnRemPort, sizeof(tcpConnEntry->tcpConnRemPort));
	    return SUCCESS;
    default:
        return FAILURE;
    }
    return FAILURE;
}

