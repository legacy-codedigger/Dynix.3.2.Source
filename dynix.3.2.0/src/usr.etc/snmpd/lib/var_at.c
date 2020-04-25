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

#ident	"$Header: var_at.c 1.1 1991/07/31 00:06:20 $"
/*LINTLIBRARY*/

/*
 * var_at.c 
 *   Resolve the variable pointed to by node.
 *
 *
 */

/* $Log: var_at.c,v $
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

#define ATTABLE		1

#define ATENTRY		1

#define ATIFINDEX	1
#define ATPHYSADDRESS	2
#define ATNETADDRESS	3

unsigned long atTblExpTime = 0;

int
var_at(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
  return FAILURE;
}
int
var_atTable(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    return FAILURE;
}

int
var_atEntry(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */

{
    static struct mib_atEntry_struct *mib_atEntry;
    struct mib_atEntry_struct *atEntry;
    static int nentries;
    struct timeval tv;
    int result;
    int i;
    int j;
    
    gettimeofday(&tv, (struct timezone *)0);
    if ((tv.tv_sec - atTblExpTime) > AT_EXPTIME) {
	nentries = get_at(&mib_atEntry);
	atTblExpTime = tv.tv_sec;
    }

    atEntry = mib_atEntry;
    
    if (debug > 6) {
      for (i=0; i < nentries; i++, atEntry++) {
	fprintf(dfile, "address: %lx ether addr %lx object id: ", 
		atEntry->atNetAddress, atEntry->atPhysAddress);
	for (j=0; j<4; j++)
	  fprintf(dfile, "%d.", atEntry->objid[j]);
	fprintf(dfile, "\n");
      }
    }
	
    atEntry = mib_atEntry;
    
    for (i=0; i < nentries; i++, atEntry++) {
	result = check_instance(name, *length, node->objid, node->objlen, atEntry->objid, ATTBLSIZE);
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
	*length = ATTBLSIZE;
	memcpy(name, (char *)atEntry->objid, ATTBLSIZE * sizeof(oid));
    } else
	return FAILURE;
    
    switch (node->subid){
    case ATIFINDEX:
        *((u_long *)node->value) = atEntry->atIfIndex;
	return SUCCESS;
    case ATPHYSADDRESS:
	memcpy(node->value, atEntry->atPhysAddress, (int)atEntry->PhysAddrLen);
	node->val_len = (int)atEntry->PhysAddrLen;
	return SUCCESS;
    case ATNETADDRESS:
        memcpy(node->value, &atEntry->atNetAddress, node->val_len);
	return SUCCESS;
    default:
        return FAILURE;
    }
}

var_set_atEntry(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct setlist *setlist_delete();
    oid objid[ATTBLSIZE];
    int atEntryLen;
    int objlen;
    struct mib_atEntry_struct *atEntry;
    struct mib_atEntry_struct *new_atEntry;
    struct mib_atEntry_struct *atp;
    int size = SETTBLSIZE;
    int new_size;
    int atnum = 0;
    int i;
    unsigned char *cp;

    /* 
     * In order to set a table, the entire row must be present. Since the list is already sorted,
     * the we know the order the variables should be in.
     */
    /* allocate initial table */
    atEntry = (struct mib_atEntry_struct *)calloc(size, sizeof(struct mib_atEntry_struct));

    /*
     * First pass any atIfIndex's 
     */
    while ((slist) && 
	   (slist->tp->subid == ATIFINDEX)) {
	/* delete this slist entry and pick up the next one */
	slist = setlist_delete();
    }
	
    /*
     * Check for the atPhysAddress
     */
    if ((!slist) || (slist->tp->subid != ATPHYSADDRESS))
	goto at_error;

    /* build a table */
    atp = atEntry;
    while (slist->tp->subid == ATPHYSADDRESS) {

	/* extract the instance object id */
	if ((atEntryLen = get_instance(slist->vp->name, slist->vp->name_length, 
				       slist->tp->objid, slist->tp->objlen, 
				       atp->objid, ATTBLSIZE)) != ATTBLSIZE)
	    goto at_error;

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
	    
	    if (ether_aton(cp, atp->atPhysAddress)) {
		free(cp);
		goto at_error;
	    }
	    free(cp);
	} else {
	    memcpy(atp->atPhysAddress, slist->vp->val.string, slist->vp->val_len);
	}
	atp->PhysAddrLen = 6;
	
	atnum++;
	/* if table is too small allocate a bigger one and copy data to it */
	if (atnum >= size) {
	    new_size = size + SETTBLSIZE;
	    new_atEntry = (struct mib_atEntry_struct *)calloc(new_size, sizeof(struct mib_atEntry_struct));
	    memcpy(new_atEntry, atEntry, size * sizeof(struct mib_atEntry_struct));
	    free((char *)atEntry);
	    atEntry = new_atEntry;
	    atp = atEntry + size;
	    size = new_size;
	} else {
	    atp++;
	}

	/* delete this slist entry and pick up the next one */
	if ((slist = setlist_delete()) == NULL)
	    goto at_error;
    }

    atp = atEntry;
    for(i=0; i<atnum; i++) {
	/*
	 * Next  check for the atNetAddress
	 */
	if ((!slist) && (slist->tp->subid != ATNETADDRESS))
	    goto at_error;
	
	/* extract instance objid and compare it to previous one */
	if ((objlen = get_instance(slist->vp->name, slist->vp->name_length, 
				   slist->tp->objid, slist->tp->objlen, 
				   objid, ATTBLSIZE)) < 0)
	    goto at_error;
	if (compare(objid, objlen, atp->objid, atEntryLen) != 0)
	    goto at_error;

	memcpy(&(atp->atNetAddress), slist->vp->val.string, slist->vp->val_len);
	
	atp++;

	/* delete this slist entry */
	slist = setlist_delete();
    }

    /* 
     * Pass this off to the routine that actually sets the arp entry
     */
    atp = atEntry;
    for(i=0; i<atnum; i++) {
	if (set_atEntry(atp) < 0)
	    goto at_error;
	atp++;
	}

    free((char *)atEntry);

    return SUCCESS;

at_error:

    free((char *)atEntry);
    return FAILURE;
    
}



