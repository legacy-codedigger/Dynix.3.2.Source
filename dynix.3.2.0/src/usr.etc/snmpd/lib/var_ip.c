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

#ident	"$Header: var_ip.c 1.1 1991/07/31 00:06:26 $"

/*
 * var_at.c 
 *   Resolve the variable pointed to by node.
 *
 */

/* $Log: var_ip.c,v $
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
#include "snmp_api.h"
#include "debug.h"

#define IPFORWARDING		1
#define IPDEFAULTTTL		2
#define IPINRECEIVES		3
#define IPINHDRERRORS		4
#define IPINADDRERRORS		5
#define IPFORWDATAGRAMS		6
#define IPINUNKNOWNPROTOS	7
#define IPINDISCARDS		8
#define IPINDELIVERS		9
#define IPOUTREQUESTS		10
#define IPOUTDISCARDS		11
#define IPOUTNOROUTES		12
#define IPREASMTIMEOUT		13
#define IPREASMREQDS		14
#define IPREASMOKS		15
#define IPREASMFAILS		16
#define IPFRAGOKS		17
#define IPFRAGFAILS		18
#define IPFRAGCREATES		19
#define IPADDRTABLE		20
#define IPROUTINGTABLE		21
#define IPNETTOMEDIATABLE	22

#define IPADDRENTRY		1

#define IPADENTADDR		1
#define IPADENTIFINDEX		2
#define IPADENTNETMASK		3
#define IPADENTBCASTADDR	4
#define IPADENTREASMMAXSIZ	5

#define IPROUTEENTRY		1

#define IPROUTEDEST		1
#define IPROUTEIFINDEX		2
#define IPROUTEMETRIC1		3
#define IPROUTEMETRIC2		4
#define IPROUTEMETRIC3		5
#define IPROUTEMETRIC4		6
#define IPROUTENEXTHOP		7
#define IPROUTETYPE		8
#define IPROUTEPROTO		9
#define IPROUTEAGE		10
#define IPROUTEMASK		11

#define IPNETTOMEDIAENTRY      	1

#define IPNETTOMEDIAIFINDEX    	1
#define IPNETTOMEDIAPHYSADDRESS	2
#define IPNETTOMEDIANETADDRESS 	3
#define IPNETTOMEDIATYPE       	4

struct ip_entry{
    oid name[16];
    int namelen;
};

struct ip_entry ip_ent[] = {
    {{0}, 1 }
};

unsigned long ipExpTime = 0;

int
    var_ip(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct ip_entry *ip_entp = ip_ent;
    int result;
    static struct mib_ip_struct Mib_ip;
    struct mib_ip_struct *mib_ip = &Mib_ip;
    struct timeval tv;
    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - ipExpTime) > IP_EXPTIME) {
	if (get_ip_mib_stats(mib_ip) < 0)
	    return FAILURE;
	ipExpTime = tv.tv_sec;
    }
    
    result = check_instance(name, *length, node->objid, node->objlen, ip_entp->name, ip_entp->namelen);
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
	*length = ip_entp->namelen;
	memcpy(name, (char *)ip_entp->name, ip_entp->namelen * sizeof(oid));
    } else
	return FAILURE;
    
    *access_method = 0;
    node->val_len = sizeof(long); /* all following variables are sizeof long */
    
    switch (node->subid){
    case IPFORWARDING:
	*((u_long *)node->value) = mib_ip->ipForwarding;
	return SUCCESS;
    case IPDEFAULTTTL:
	*((u_long *)node->value) = mib_ip->ipDefaultTTL;
	return SUCCESS;
    case IPINRECEIVES:
	*((u_long *)node->value) = mib_ip->ipInReceives;
	return SUCCESS;
    case IPINHDRERRORS:
	*((u_long *)node->value) = mib_ip->ipInHdrErrors;
	return SUCCESS;
    case IPINADDRERRORS:
	*((u_long *)node->value) = mib_ip->ipInAddrErrors;
	return SUCCESS;
    case IPFORWDATAGRAMS:
	*((u_long *)node->value) = mib_ip->ipForwDatagrams;
	return SUCCESS;
    case IPINUNKNOWNPROTOS:
	*((u_long *)node->value) = mib_ip->ipInUnknownProtos;
	return SUCCESS;
    case IPINDISCARDS:
	*((u_long *)node->value) = mib_ip->ipInDiscards;
	return SUCCESS;
    case IPINDELIVERS:
	*((u_long *)node->value) = mib_ip->ipInDelivers;
	return SUCCESS;
    case IPOUTREQUESTS:
	*((u_long *)node->value) = mib_ip->ipOutRequests;
	return SUCCESS;
    case IPOUTDISCARDS:
	*((u_long *)node->value) = mib_ip->ipOutDiscards;
	return SUCCESS;
    case IPOUTNOROUTES:
	*((u_long *)node->value) = mib_ip->ipOutNoRoutes;
	return SUCCESS;
    case IPREASMTIMEOUT:
	*((u_long *)node->value) = mib_ip->ipReasmTimeout;
	return SUCCESS;
    case IPREASMREQDS:
	*((u_long *)node->value) = mib_ip->ipReasmReqds;
	return SUCCESS;
    case IPREASMOKS:
	*((u_long *)node->value) = mib_ip->ipReasmOKs;
	return SUCCESS;
    case IPREASMFAILS:
	*((u_long *)node->value) = mib_ip->ipReasmFails;
	return SUCCESS;
    case IPFRAGOKS:
	*((u_long *)node->value) = mib_ip->ipFragOKs;
	return SUCCESS;
    case IPFRAGFAILS:
	*((u_long *)node->value) = mib_ip->ipFragFails;
	return SUCCESS;
    case IPFRAGCREATES:
	*((u_long *)node->value) = mib_ip->ipFragCreates;
	return SUCCESS;
    case IPADDRTABLE:
	return FAILURE;
    case IPROUTINGTABLE:
	return FAILURE;
    case IPNETTOMEDIATABLE:
	return FAILURE;
    default:
	return FAILURE;
    }
    return FAILURE;
}
int
    var_ipAddrTable(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

unsigned long ipAddrTblExpTime = 0;
struct mib_ipAddrEntry_struct *mib_ipAddrEntry = NULL;
int mib_ipAddrEntry_nentries = 0;

int
var_ipAddrEntry(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct mib_ipAddrEntry_struct *ipAddrEntry;
    struct timeval tv;
    int result;
    int i;
    int j;
    oid *tmpname;
    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - ipAddrTblExpTime) > IPADDR_EXPTIME) {
	mib_ipAddrEntry_nentries = get_ipAddr(&mib_ipAddrEntry);
	ipAddrTblExpTime = tv.tv_sec;
    }
    
    ipAddrEntry = mib_ipAddrEntry;
    
    if (debug > 6) {
	for (i=0; i < mib_ipAddrEntry_nentries; i++, ipAddrEntry++) {
	    fprintf(dfile, "address: %lx net mask %lx object id: ", 
		    ipAddrEntry->ipAdEntAddr, ipAddrEntry->ipAdEntNetMask);
	    for (j=0; j<4; j++)
		fprintf(dfile, "%d.", ipAddrEntry->objid[j]);
	    fprintf(dfile, "\n");
	}
    }
    
    ipAddrEntry = mib_ipAddrEntry;
    
    for (i=0; i < mib_ipAddrEntry_nentries; i++, ipAddrEntry++) {
	result = check_instance(name, *length, node->objid, node->objlen, ipAddrEntry->objid, IPADDRTBLSIZE);
	if (exact) {
	    if (result == 0)	/* we have a match */
		break;			
	} else { 	/* !exact */
	    if (result < 0) 
		break;
	}
    }
    
    if (i == mib_ipAddrEntry_nentries) 
	return FAILURE;
    
    if (node->access == NOACCESS)
	return FAILURE;

    *access_method = 0;
    if (node->value){
	*length = IPADDRTBLSIZE;
	memcpy(name, (char *)ipAddrEntry->objid, IPADDRTBLSIZE * sizeof(oid));
    } else
	return FAILURE;
    
    switch (node->subid){
    case IPADENTADDR:
        memcpy(node->value, &ipAddrEntry->ipAdEntAddr, node->val_len);
	return SUCCESS;
    case IPADENTIFINDEX:
	*((u_long *)node->value) = ipAddrEntry->ipAdEntIfIndex;
	return SUCCESS;
    case IPADENTNETMASK:
        memcpy(node->value, &ipAddrEntry->ipAdEntNetMask, node->val_len);
	return SUCCESS;
    case IPADENTBCASTADDR:
	*((u_long *)node->value) = ipAddrEntry->ipAdEntBcastAddr;
	return SUCCESS;
    case IPADENTREASMMAXSIZ:
	*((u_long *)node->value) = ipAddrEntry->ipAdEntReasmMaxSize;
	return SUCCESS;
    default:
        return FAILURE;
    }
    return FAILURE;
}

int
    var_ipRouteTable(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

unsigned long ipRouteTblExpTime = 0;

int
    var_ipRouteEntry(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    static struct mib_ipRouteEntry_struct *mib_ipRouteEntry;
    struct mib_ipRouteEntry_struct *ipRouteEntry;
    static int nentries;
    struct timeval tv;
    int result;
    int i;
    int j;
    oid *tmpname;
    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - ipRouteTblExpTime) > IPROUTE_EXPTIME) {
	nentries = get_ipRoute(&mib_ipRouteEntry);
	ipRouteTblExpTime = tv.tv_sec;
    }
    
    ipRouteEntry = mib_ipRouteEntry;
    
    if (debug > 6) {
	for (i=0; i < nentries; i++, ipRouteEntry++) {
	    fprintf(dfile, "address: %s next hop: %s  object id: ", 
		    inet_ntoa(ipRouteEntry->ipRouteDest), inet_ntoa(ipRouteEntry->ipRouteNextHop));
	    for (j=0; j<4; j++)
		fprintf(dfile, "%d.", ipRouteEntry->objid[j]);
	    fprintf(dfile, "\n");
	}
    }
    
    ipRouteEntry = mib_ipRouteEntry;
    
    for (i=0; i < nentries; i++, ipRouteEntry++) {
	result = check_instance(name, *length, node->objid, node->objlen, ipRouteEntry->objid, IPROUTETBLSIZE);
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
	*length = IPROUTETBLSIZE;
	memcpy(name, (char *)ipRouteEntry->objid, IPROUTETBLSIZE * sizeof(oid));
    } else
	return FAILURE;
    
    switch (node->subid){
    case IPROUTEDEST:
        memcpy(node->value, &ipRouteEntry->ipRouteDest, node->val_len);
	return SUCCESS;
    case IPROUTEIFINDEX:
        *((u_long *)node->value) = ipRouteEntry->ipRouteIfIndex;
	return SUCCESS;
    case IPROUTEMETRIC1:
        *((u_long *)node->value) = ipRouteEntry->ipRouteMetric1;
	return SUCCESS;
    case IPROUTEMETRIC2:
        *((u_long *)node->value) = ipRouteEntry->ipRouteMetric2;
	return SUCCESS;
    case IPROUTEMETRIC3:
        *((u_long *)node->value) = ipRouteEntry->ipRouteMetric3;
	return SUCCESS;
    case IPROUTEMETRIC4:
        *((u_long *)node->value) = ipRouteEntry->ipRouteMetric4;
	return SUCCESS;
    case IPROUTENEXTHOP:
        memcpy(node->value, &ipRouteEntry->ipRouteNextHop, node->val_len);
	return SUCCESS;
    case IPROUTETYPE:
        *((u_long *)node->value) = ipRouteEntry->ipRouteType;
	return SUCCESS;
    case IPROUTEPROTO:
        *((u_long *)node->value) = ipRouteEntry->ipRouteProto;
	return SUCCESS;
    case IPROUTEAGE:
        *((u_long *)node->value) = ipRouteEntry->ipRouteAge;
	return SUCCESS;
    case IPROUTEMASK:
        memcpy(node->value, &ipRouteEntry->ipRouteMask, node->val_len);
	return SUCCESS;
    default:
        return FAILURE;
    }
    return FAILURE;
}

int
    var_ipNetToMediaTable(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

unsigned long ipNetToMediaTblExpTime = 0;

int
    var_ipNetToMediaEntry(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    static struct mib_ipNetToMediaEntry_struct *mib_ipNetToMediaEntry;
    struct mib_ipNetToMediaEntry_struct *ipNetToMediaEntry;
    static int nentries;
    struct timeval tv;
    int result;
    int i;
    int j;
    oid *tmpname;
    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - ipNetToMediaTblExpTime) > IPNETTOMEDIA_EXPTIME) {
	nentries = get_ipNetToMedia(&mib_ipNetToMediaEntry);
	ipNetToMediaTblExpTime = tv.tv_sec;
    }
    
    ipNetToMediaEntry = mib_ipNetToMediaEntry;
    
    if (debug > 6) {
	for (i=0; i < nentries; i++, ipNetToMediaEntry++) {
	    fprintf(dfile, "address: %lx ether addr %lx object id: ", 
		    ipNetToMediaEntry->ipNetToMediaNetAddress, ipNetToMediaEntry->ipNetToMediaPhysAddress);
	    for (j=0; j<4; j++)
		fprintf(dfile, "%d.", ipNetToMediaEntry->objid[j]);
	    fprintf(dfile, "\n");
	}
    }
    
    ipNetToMediaEntry = mib_ipNetToMediaEntry;
    
    for (i=0; i < nentries; i++, ipNetToMediaEntry++) {
	result = check_instance(name, *length, node->objid, node->objlen, ipNetToMediaEntry->objid, IPNETTOMEDIATBLSIZE);
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
	*length = IPNETTOMEDIATBLSIZE;
	memcpy(name, (char *)ipNetToMediaEntry->objid, IPNETTOMEDIATBLSIZE * sizeof(oid));
    } else
	return FAILURE;
    
    switch (node->subid){
    case IPNETTOMEDIAIFINDEX:
        *((u_long *)node->value) = ipNetToMediaEntry->ipNetToMediaIfIndex;
	return SUCCESS;
    case IPNETTOMEDIAPHYSADDRESS:
	memcpy(node->value, ipNetToMediaEntry->ipNetToMediaPhysAddress, (int)ipNetToMediaEntry->PhysAddrLen);
	node->val_len = (int)ipNetToMediaEntry->PhysAddrLen;
	return SUCCESS;
    case IPNETTOMEDIANETADDRESS:
        memcpy(node->value, &ipNetToMediaEntry->ipNetToMediaNetAddress, node->val_len);
	return SUCCESS;
    case IPNETTOMEDIATYPE:
	*((u_long *)node->value) = ipNetToMediaEntry->ipNetToMediaType;
	return SUCCESS;
    default:
        return FAILURE;
    }
    return FAILURE;
}


var_set_ip(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct setlist *setlist_delete();
    struct ip_entry *ip_entp = ip_ent;
    int result;
    struct tree *node;   /* pointer to variable entry that points here */
    struct variable_list *var;
    struct mib_ip_struct mib_ip;

    node = slist->tp;
    var = slist->vp;

    result = check_instance(var->name, var->name_length, node->objid, node->objlen, ip_entp->name, ip_entp->namelen);
    if (result != 0)
	return FAILURE;
    
    if (get_ip_mib_stats(&mib_ip) < 0)
	return FAILURE;
    
    switch (node->subid){
    case IPFORWARDING:
	mib_ip.ipForwarding = *var->val.integer;
	setlist_delete();
	break;
    case IPDEFAULTTTL:
	mib_ip.ipDefaultTTL = *var->val.integer;
	setlist_delete();
	break;
    default:
	return FAILURE;
    }
    if (set_ip(&mib_ip) < 0)
	return FAILURE;
    
    return SUCCESS;
}

var_set_ipNetToMediaEntry(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct setlist *setlist_delete();
    oid objid[IPNETTOMEDIATBLSIZE];
    int ipNetToMediaLen;
    int objlen;
    struct mib_ipNetToMediaEntry_struct *ipNetToMedia;
    struct mib_ipNetToMediaEntry_struct *new_ipNetToMedia;
    struct mib_ipNetToMediaEntry_struct *ipntmp;
    int size = SETTBLSIZE;
    int new_size;
    int ipntmnum = 0;
    int i;
    unsigned char *cp;

    /* 
     * In order to set a table, the entire row must be present. Since the list is already sorted,
     * the we know the order the variables should be in.
     */
    /* allocate initial table */
    ipNetToMedia = (struct mib_ipNetToMediaEntry_struct *)calloc(size, sizeof(struct mib_ipNetToMediaEntry_struct));
    /*
     * First pass any ipNetToMediaIfIndex's 
     */
    while ((slist) && 
	   (slist->tp->subid == IPNETTOMEDIAIFINDEX)) {
	/* delete this slist entry and pick up the next one */
	slist = setlist_delete();
    }
	
    /*
     * Check for the ipNetToMediaPhysAddress
     */
    if ((!slist) || (slist->tp->subid != IPNETTOMEDIAPHYSADDRESS))
	goto ipntm_error;

    /* build a table */
    ipntmp = ipNetToMedia;
    while (slist->tp->subid == IPNETTOMEDIAPHYSADDRESS) {

	/* extract the instance object id */
	if ((ipNetToMediaLen = get_instance(slist->vp->name, slist->vp->name_length, 
				       slist->tp->objid, slist->tp->objlen, 
				       ipntmp->objid, IPNETTOMEDIATBLSIZE)) != IPNETTOMEDIATBLSIZE)
	    goto ipntm_error;

	/*
	 * Heads Up - kludge to follow...
	 * It appears that some net managers send out the ether address in binary
	 * and others (e.g. Sun) send it in ascii! So, if the length is greater
	 * than 6 I'll assume it's ascii otherwise it's binary. The magic number 6
	 * comes from arp_prim.
	 */
	if (slist->vp->val_len > 6) {
	    /* Convert from ascii to binary */
	    cp = (unsigned char *)calloc(1, slist->vp->val_len + 1);
	    memcpy(cp, slist->vp->val.string, slist->vp->val_len);

	    if (ether_aton(cp, ipntmp->ipNetToMediaPhysAddress)) {
		free(cp);
		goto ipntm_error;
	    }
	    free(cp);
	} else {
	    memcpy(ipntmp->ipNetToMediaPhysAddress, slist->vp->val.string, slist->vp->val_len);
	}
	ipntmp->PhysAddrLen = 6;

	ipntmnum++;
	/* if table is too small allocate a bigger one and copy data to it */
	if (ipntmnum >= size) {
	    new_size = size + SETTBLSIZE;
	    new_ipNetToMedia = (struct mib_ipNetToMediaEntry_struct *)calloc(new_size, sizeof(struct mib_ipNetToMediaEntry_struct));
	    memcpy(new_ipNetToMedia, ipNetToMedia, size * sizeof(struct mib_ipNetToMediaEntry_struct));
	    free((char *)ipNetToMedia);
	    ipNetToMedia = new_ipNetToMedia;
	    ipntmp = ipNetToMedia + size;
	    size = new_size;
	} else {
	    ipntmp++;
	}

	/* delete this slist entry and pick up the next one */
	if ((slist = setlist_delete()) == NULL)
	    goto ipntm_error;
    }

    
    ipntmp = ipNetToMedia;
    
    for(i=0; i<ipntmnum; i++) {
	/*
	 * Next  check for the ipNetToMediaNetAddress
	 */
	if ((!slist) || (slist->tp->subid != IPNETTOMEDIANETADDRESS))
	    goto ipntm_error;
	
	/* extract instance objid and compare it to previous one */
	if ((objlen = get_instance(slist->vp->name, slist->vp->name_length, 
				   slist->tp->objid, slist->tp->objlen, 
				   objid, IPNETTOMEDIATBLSIZE)) < 0)
	    goto ipntm_error;
	if (compare(objid, objlen, ipntmp->objid, ipNetToMediaLen) != 0)
	    goto ipntm_error;

	memcpy(&(ipntmp->ipNetToMediaNetAddress), slist->vp->val.string, slist->vp->val_len);
	
	ipntmp++;

	/* delete this slist entry */
	slist = setlist_delete();
    }

    ipntmp = ipNetToMedia;
    
    for(i=0; i<ipntmnum; i++) {
	/*
	 * Next  check for the ipNetToMediaType
	 */
	if ((!slist) || (slist->tp->subid != IPNETTOMEDIATYPE))
	    goto ipntm_error;
	
	/* extract instance objid and compare it to previous one */
	if ((objlen = get_instance(slist->vp->name, slist->vp->name_length, 
				   slist->tp->objid, slist->tp->objlen, 
				   objid, IPNETTOMEDIATBLSIZE)) < 0)
	    goto ipntm_error;
	if (compare(objid, objlen, ipntmp->objid, ipNetToMediaLen) != 0)
	    goto ipntm_error;

	ipntmp->ipNetToMediaType = *slist->vp->val.integer;
	
	ipntmp++;

	/* delete this slist entry */
	slist = setlist_delete();
    }

    /* 
     * Pass this off to the routine that actually sets the arp entry
     */
    ipntmp = ipNetToMedia;
    for(i=0; i<ipntmnum; i++) {
	if (set_ipNetToMedia(ipntmp) < 0)
	    goto ipntm_error;
	ipntmp++;
	}

    free((char *)ipNetToMedia);

    return SUCCESS;

ipntm_error:

    free((char *)ipNetToMedia);
    return FAILURE;
    
}

var_set_ipRouteEntry(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct setlist *setlist_delete();
    oid objid[IPROUTETBLSIZE];
    int ipRouteLen;
    int objlen;
    struct mib_ipRouteEntry_struct *ipRoute;
    struct mib_ipRouteEntry_struct *new_ipRoute;
    struct mib_ipRouteEntry_struct *iprp;
    int size = SETTBLSIZE;
    int new_size;
    int iprnum = 0;
    int i;
    unsigned char *cp;

    /* 
     * In order to set a table, the entire row must be present. Since the list is already sorted,
     * the we know the order the variables should be in.
     */
    /* allocate initial table */
    ipRoute = (struct mib_ipRouteEntry_struct *)calloc(size, sizeof(struct mib_ipRouteEntry_struct));
    /*
     * First check for the ipRouteDest's and build a list
     */
    if (slist->tp->subid != IPROUTEDEST)
	goto ipr_error;

    iprp = ipRoute;
    while (slist->tp->subid == IPROUTEDEST) { 

	/* extract the instance object id */
	if ((ipRouteLen = get_instance(slist->vp->name, slist->vp->name_length, 
				       slist->tp->objid, slist->tp->objlen, 
				       iprp->objid, IPROUTETBLSIZE)) != IPROUTETBLSIZE)
	    goto ipr_error;

	memcpy(&(iprp->ipRouteDest), slist->vp->val.string, slist->vp->val_len);

	iprnum++;
	/* if table is too small allocate a bigger one and copy data to it */
	if (iprnum >= size) {
	    new_size = size + SETTBLSIZE;
	    new_ipRoute = (struct mib_ipRouteEntry_struct *)calloc(new_size, sizeof(struct mib_ipRouteEntry_struct));
	    memcpy(new_ipRoute, ipRoute, size * sizeof(struct mib_ipRouteEntry_struct));
	    free((char *)ipRoute);
	    ipRoute = new_ipRoute;
	    iprp = ipRoute + size;
	    size = new_size;
	} else {
	    iprp++;
	}

	/* delete this slist entry and pick up the next one */
	if ((slist = setlist_delete()) == NULL)
	    goto ipr_error;
    }

    while((slist) && 
	  ((slist->tp->subid == IPROUTEIFINDEX) ||
	  (slist->tp->subid == IPROUTEMETRIC1) ||
	  (slist->tp->subid == IPROUTEMETRIC2) ||
	  (slist->tp->subid == IPROUTEMETRIC3) ||
	  (slist->tp->subid == IPROUTEMETRIC4))) {
	
	/* delete this slist entry */
	slist = setlist_delete();
    }

    iprp = ipRoute;
    for(i=0; i<iprnum; i++) {
	/*
	 * Next  check for the ipRouteNextHop
	 */
	if ((!slist) || (slist->tp->subid != IPROUTENEXTHOP))
	    goto ipr_error;
	
	/* extract instance objid and compare it to previous one */
	if ((objlen = get_instance(slist->vp->name, slist->vp->name_length, 
				   slist->tp->objid, slist->tp->objlen, 
				   objid, IPROUTETBLSIZE)) < 0)
	    goto ipr_error;
	if (compare(objid, objlen, iprp->objid, ipRouteLen) != 0)
	    goto ipr_error;

	memcpy(&(iprp->ipRouteNextHop), slist->vp->val.string, slist->vp->val_len);
	
	iprp++;

	/* delete this slist entry */
	slist = setlist_delete();
    }

    iprp = ipRoute;
    for(i=0; i<iprnum; i++) {
	/*
	 * Next  check for the ipRouteType
	 */
	if ((!slist) || (slist->tp->subid != IPROUTETYPE))
	    goto ipr_error;
	
	/* extract instance objid and compare it to previous one */
	if ((objlen = get_instance(slist->vp->name, slist->vp->name_length, 
				   slist->tp->objid, slist->tp->objlen, 
				   objid, IPROUTETBLSIZE)) < 0)
	    goto ipr_error;
	if (compare(objid, objlen, iprp->objid, ipRouteLen) != 0)
	    goto ipr_error;

	iprp->ipRouteType = *slist->vp->val.integer;
	
	iprp++;

	/* delete this slist entry */
	slist = setlist_delete();
    }

    while((slist) && 
	  ((slist->tp->subid == IPROUTEAGE) ||
	  (slist->tp->subid == IPROUTEMASK))) {
	/*
	 * Next  check for the ipRouteAge or ipRouteMask
	 */
	/* 
	 * delete this slist entry (it may be the end of the list, so don't 
	 * error on a NULL
	 */
	slist = setlist_delete();
    }

    /* 
     * Pass this off to the routine that actually sets the arp entry
     */
    iprp = ipRoute;
    for(i=0; i<iprnum; i++) {
	if (set_ipRoute(iprp) < 0)
	    goto ipr_error;
	iprp++;
	}

    free((char *)ipRoute);

    return SUCCESS;

ipr_error:

    free((char *)ipRoute);
    return FAILURE;
    
}






