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

#ident	"$Header: var_if.c 1.1 1991/07/31 00:06:24 $"

/*
 * var_if.c 
 *   Resolve the variable pointed to by node.
 *
 */

/* $Log: var_if.c,v $
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

#define IFNUMBER		1
#define IFTABLE			2

#define IFENTRY			1

#define IFINDEX			1
#define IFDESCR			2
#define IFTYPE			3
#define IFMTU			4
#define IFSPEED			5
#define IFPHYSADDRESS		6
#define IFADMINSTATUS		7
#define IFOPERSTATUS		8
#define IFLASTCHANGE		9
#define IFINOCTETS		10
#define IFINUCASTPKTS		11
#define IFINNUCASTPKTS		12
#define IFINDISCARDS		13
#define IFINERRORS		14
#define IFINUNKNOWNPROTOS	15
#define IFOUTOCTETS		16
#define IFOUTUCASTPKTS		17
#define IFOUTNUCASTPKTS		18
#define IFOUTDISCARDS		19
#define IFOUTERRORS		20
#define IFOUTQLEN		21
#define IFSPECIFIC		22

struct if_entry{
    oid name[16];
    int namelen;
};

struct if_entry if_ent[] = {
    {{0}, 1 }
};

struct ifTable IfTable[MAX_IF]; /* initialize */
int if_num_entries = 0;

int
var_if(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct if_entry *if_entp = if_ent;
    struct ifTable *ifTablep = IfTable;
    int result;

    result = check_instance(name, *length, node->objid, node->objlen, if_entp->name, if_entp->namelen);
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
	*length = if_entp->namelen;
	memcpy(name, (char *)if_entp->name, if_entp->namelen * sizeof(oid));
    } else
	return FAILURE;

    *access_method = 0;
    switch (node->subid){
    case IFNUMBER:
	if (if_num_entries == 0)
	    if_num_entries = fill_ifTable(ifTablep);
	*((u_long *)node->value) = if_num_entries;
	node->val_len = sizeof(long); 
	return SUCCESS;
    case IFTABLE:
	return FAILURE;
    default:
	return FAILURE;
    }
    return FAILURE;
}
int
var_ifTable(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

int
var_ifEntry(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    struct ifTable *ifTablep = IfTable;
    struct mib_ifEntry_struct *mib_ifEntry;
    struct timeval tv;
    int result;
    int i;
    struct if_entry if_enttab;
    struct if_entry *if_enttabp = &if_enttab;
    
    if (if_num_entries == 0)
	if_num_entries = fill_ifTable(ifTablep);

/*    ifEntry = mib_ifEntry; */
    
    for (i=0; i < if_num_entries; i++, ifTablep++) {
	if_enttabp->name[0] = i + 1;
	result = check_instance(name, *length, node->objid, node->objlen, if_enttabp->name, 1);
	if (exact) {
	    if (result == 0)	/* we have a match */
		break;			
	} else { 	/* !exact */
	    if (result < 0) 
		break;
	}
    }
	
    if (i == if_num_entries) 
	return FAILURE;

    if (node->access == NOACCESS)
	return FAILURE;

    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - ifTablep->ifExpTime) > IF_EXPTIME) {
	ifTablep->ifExpTime = tv.tv_sec;
	if (get_if(ifTablep) < 0)
	    return FAILURE;
    }

    mib_ifEntry = ifTablep->mib_ifEntry;
    mib_ifEntry->ifIndex = i + 1;
	
    *access_method = 0;
    if (node->value){
	*length = 1;
	memcpy(name, (char *)&mib_ifEntry->ifIndex, sizeof(oid));
    } else
	return FAILURE;
    
    *access_method = 0;

    switch (node->subid){
    case IFINDEX:
	*((u_long *)node->value) = mib_ifEntry->ifIndex;
        return SUCCESS;
    case IFDESCR:
/*
	strcpy((char *)node->value, ifTablep->fname);
	node->val_len = strlen(ifTablep->fname);
*/
	strcpy((char *)node->value, mib_ifEntry->ifDescr);
	node->val_len = strlen(mib_ifEntry->ifDescr);
        return SUCCESS;
    case IFTYPE:
	*((u_long *)node->value) = mib_ifEntry->ifType;
        return SUCCESS;
    case IFMTU:
	*((u_long *)node->value) = mib_ifEntry->ifMtu;
        return SUCCESS;
    case IFSPEED:
	*((u_long *)node->value) = mib_ifEntry->ifSpeed;
        return SUCCESS;
    case IFPHYSADDRESS:
	memcpy(node->value, mib_ifEntry->ifPhysAddress, (int)mib_ifEntry->PhysAddrLen);
	node->val_len = (int)mib_ifEntry->PhysAddrLen;
        return SUCCESS;
    case IFADMINSTATUS:
	*((u_long *)node->value) = mib_ifEntry->ifOperStatus;
        return SUCCESS;
    case IFOPERSTATUS:
	*((u_long *)node->value) = mib_ifEntry->ifOperStatus;
        return SUCCESS;
    case IFINOCTETS:
	*((u_long *)node->value) = mib_ifEntry->ifInOctets;
        return SUCCESS;
    case IFINUCASTPKTS:
	*((u_long *)node->value) = mib_ifEntry->ifInUcastPkts;
        return SUCCESS;
    case IFINNUCASTPKTS:
	*((u_long *)node->value) = mib_ifEntry->ifInNUcastPkts;
        return SUCCESS;
    case IFINDISCARDS:
	*((u_long *)node->value) = mib_ifEntry->ifInDiscards;
        return SUCCESS;
    case IFINERRORS:
	*((u_long *)node->value) = mib_ifEntry->ifInErrors;
        return SUCCESS;
    case IFINUNKNOWNPROTOS:
	*((u_long *)node->value) = mib_ifEntry->ifInUnknownProtos;
        return SUCCESS;
    case IFOUTOCTETS:
	*((u_long *)node->value) = mib_ifEntry->ifOutOctets;
        return SUCCESS;
    case IFOUTUCASTPKTS:
	*((u_long *)node->value) = mib_ifEntry->ifOutUcastPkts;
        return SUCCESS;
    case IFOUTNUCASTPKTS:
	*((u_long *)node->value) = mib_ifEntry->ifOutNUcastPkts;
        return SUCCESS;
    case IFOUTDISCARDS:
	*((u_long *)node->value) = mib_ifEntry->ifOutDiscards;
        return SUCCESS;
    case IFOUTERRORS:
	*((u_long *)node->value) = mib_ifEntry->ifOutErrors;
        return SUCCESS;
    case IFOUTQLEN:
	*((u_long *)node->value) = mib_ifEntry->ifOutQLen;
        return SUCCESS;
    case IFSPECIFIC:
	realloc(node->value, 2 * sizeof(oid));
	memset(node->value, 0, 2 * sizeof(oid));
	node->val_len = 2 * sizeof(oid);
        return SUCCESS;
    case IFLASTCHANGE:
	*((u_long *)node->value) = mib_ifEntry->ifLastChange;
        return SUCCESS;
    default:
        return FAILURE;
    }
}

int
var_set_ifEntry(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct ifTable *ifTablep = IfTable;
    struct mib_ifEntry_struct mib_ifEntry;
    int result;
    int i;
    struct if_entry if_enttab;
    struct if_entry *if_enttabp = &if_enttab;
    struct tree *node;   /* pointer to variable entry that points here */
    struct variable_list *var;
    struct timeval tv;
    u_long data;
  
    node = slist->tp;
    var = slist->vp;
    
    if (if_num_entries == 0)
	if_num_entries = fill_ifTable(ifTablep);

    for (i=0; i < if_num_entries; i++, ifTablep++) {
	if_enttabp->name[0] = i + 1;
	result = check_instance(var->name, var->name_length, node->objid, node->objlen, if_enttabp->name, 1);
	if (result == 0)	/* we have a match */
	    break;			
    }
	
    if (i == if_num_entries) 
	return FAILURE;

    switch (node->subid){
    case IFADMINSTATUS:
	mib_ifEntry.ifAdminStatus = *var->val.integer;
	if (set_ifEntry(ifTablep, &mib_ifEntry) < 0)
	    return FAILURE;
	gettimeofday(&tv, (struct timezone *)0);
	data = (tv.tv_sec - StartTime) * 100;
	memcpy( &(ifTablep->mib_ifEntry->ifLastChange) , &data, sizeof(u_long));
	setlist_delete();
        return SUCCESS;
    default:
        return FAILURE;
    }
}


