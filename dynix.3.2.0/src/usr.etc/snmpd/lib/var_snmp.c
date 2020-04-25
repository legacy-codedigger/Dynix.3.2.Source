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

#ident	"$Header: var_snmp.c 1.1 1991/07/31 00:06:27 $"

/*
 * var_snmp.c 
 *   Resolve the variable pointed to by node.
 *
 */

/* $Log: var_snmp.c,v $
 *
 */

#include <stdio.h>
#include <sys/types.h>
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

#define SNMPINPKTS 		1
#define SNMPOUTPKTS 		2
#define SNMPINBADVERSIONS 	3
#define SNMPINBADCOMMUNITYNAMES 4
#define SNMPINBADCOMMUNITYUSES 	5
#define SNMPINASNPARSEERRS     	6
#define SNMPINBADTYPES 		7
#define SNMPINTOOBIGS 		8
#define SNMPINNOSUCHNAMES      	9
#define SNMPINBADVALUES        	10
#define SNMPINREADONLYS        	11
#define SNMPINGENERRS 		12
#define SNMPINTOTALREQVARS     	13
#define SNMPINTOTALSETVARS     	14
#define SNMPINGETREQUESTS      	15
#define SNMPINGETNEXTS 		16
#define SNMPINSETREQUESTS      	17
#define SNMPINGETRESPONSES     	18
#define SNMPINTRAPS 		19
#define SNMPOUTTOOBIGS 		20
#define SNMPOUTNOSUCHNAMES     	21
#define SNMPOUTBADVALUES       	22
#define SNMPOUTREADONLYS       	23
#define SNMPOUTGENERRS 		24
#define SNMPOUTGETREQUESTS     	25
#define SNMPOUTGETNEXTS        	26
#define SNMPOUTSETREQUESTS     	27
#define SNMPOUTGETRESPONSES    	28
#define SNMPOUTTRAPS 		29
#define SNMPENABLEAUTHTRAPS    	30


struct snmp_entry{
  oid name[16];
  int namelen;
};

struct snmp_entry snmp_ent[] = {
  {{0}, 1 }
};

extern struct mib_snmp_struct *mib_snmp;

int
var_snmp(node, name, length, exact, access_method)
register struct tree *node;   /* IN - pointer to variable entry that points here */
register oid        *name;      /* IN/OUT - input name requested, output name found */
register int        *length;    /* IN/OUT - length of input and output oid's */
int                 exact;      /* IN - TRUE if an exact match was requested. */
int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */
{
  struct snmp_entry *snmp_entp = snmp_ent;
  int result;
  
    result = check_instance(name, *length, node->objid, node->objlen, snmp_entp->name, snmp_entp->namelen);
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
	*length = snmp_entp->namelen;
	memcpy(name, (char *)snmp_entp->name, snmp_entp->namelen * sizeof(oid));
    } else
	return FAILURE;
  
  *access_method = 0;
  node->val_len = sizeof(long); /* all following variables are sizeof long */

  *access_method = 0;
  switch (node->subid){
  case SNMPINPKTS:
      *((u_long *)node->value) = mib_snmp->snmpInPkts;
      return SUCCESS;
  case SNMPOUTPKTS:
      *((u_long *)node->value) = mib_snmp->snmpOutPkts;
      return SUCCESS;
  case SNMPINBADVERSIONS:
      *((u_long *)node->value) = mib_snmp->snmpInBadVersions;
      return SUCCESS;
  case SNMPINBADCOMMUNITYNAMES:
      *((u_long *)node->value) = mib_snmp->snmpInBadCommunityNames;
      return SUCCESS;
  case SNMPINBADCOMMUNITYUSES:
      *((u_long *)node->value) = mib_snmp->snmpInBadCommunityUses;
      return SUCCESS;
  case SNMPINASNPARSEERRS:
      *((u_long *)node->value) = mib_snmp->snmpInASNParseErrs;
      return SUCCESS;
  case SNMPINBADTYPES:
      *((u_long *)node->value) = mib_snmp->snmpInBadTypes;
      return SUCCESS;
  case SNMPINTOOBIGS:
      *((u_long *)node->value) = mib_snmp->snmpInTooBigs;
      return SUCCESS;
  case SNMPINNOSUCHNAMES:
      *((u_long *)node->value) = mib_snmp->snmpInNoSuchNames;
      return SUCCESS;
  case SNMPINBADVALUES:
      *((u_long *)node->value) = mib_snmp->snmpInBadValues;
      return SUCCESS;
  case SNMPINREADONLYS:
      *((u_long *)node->value) = mib_snmp->snmpInReadOnlys;
      return SUCCESS;
  case SNMPINGENERRS:
      *((u_long *)node->value) = mib_snmp->snmpInGenErrs;
      return SUCCESS;
  case SNMPINTOTALREQVARS:
      *((u_long *)node->value) = mib_snmp->snmpInTotalReqVars;
      return SUCCESS;
  case SNMPINTOTALSETVARS:
      *((u_long *)node->value) = mib_snmp->snmpInTotalSetVars;
      return SUCCESS;
  case SNMPINGETREQUESTS:
      *((u_long *)node->value) = mib_snmp->snmpInGetRequests;
      return SUCCESS;
  case SNMPINGETNEXTS:
      *((u_long *)node->value) = mib_snmp->snmpInGetNexts;
      return SUCCESS;
  case SNMPINSETREQUESTS:
      *((u_long *)node->value) = mib_snmp->snmpInSetRequests;
      return SUCCESS;
  case SNMPINGETRESPONSES:
      *((u_long *)node->value) = mib_snmp->snmpInGetResponses;
      return SUCCESS;
  case SNMPINTRAPS:
      *((u_long *)node->value) = mib_snmp->snmpInTraps;
      return SUCCESS;
  case SNMPOUTTOOBIGS:
      *((u_long *)node->value) = mib_snmp->snmpOutTooBigs;
      return SUCCESS;
  case SNMPOUTNOSUCHNAMES:
      *((u_long *)node->value) = mib_snmp->snmpOutNoSuchNames;
      return SUCCESS;
  case SNMPOUTBADVALUES:
      *((u_long *)node->value) = mib_snmp->snmpOutBadValues;
      return SUCCESS;
  case SNMPOUTREADONLYS:
      *((u_long *)node->value) = mib_snmp->snmpOutReadOnlys;
      return SUCCESS;
  case SNMPOUTGENERRS:
      *((u_long *)node->value) = mib_snmp->snmpOutGenErrs;
      return SUCCESS;
  case SNMPOUTGETREQUESTS:
      *((u_long *)node->value) = mib_snmp->snmpOutGetRequests;
      return SUCCESS;
  case SNMPOUTGETNEXTS:
      *((u_long *)node->value) = mib_snmp->snmpOutGetNexts;
      return SUCCESS;
  case SNMPOUTSETREQUESTS:
      *((u_long *)node->value) = mib_snmp->snmpOutSetRequests;
      return SUCCESS;
  case SNMPOUTGETRESPONSES:
      *((u_long *)node->value) = mib_snmp->snmpOutGetResponses;
      return SUCCESS;
  case SNMPOUTTRAPS:
      *((u_long *)node->value) = mib_snmp->snmpOutTraps;
      return SUCCESS;
  case SNMPENABLEAUTHTRAPS:
      *((u_long *)node->value) = mib_snmp->snmpEnableAuthTraps;
      return SUCCESS;
  default:
    return FAILURE;
  }
}

int
var_set_snmp(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct snmp_entry *snmp_entp = snmp_ent;
    int result;
    struct tree *node;   /* pointer to variable entry that points here */
    struct variable_list *var;
  
    node = slist->tp;
    var = slist->vp;

    result = check_instance(var->name, var->name_length, node->objid, node->objlen, snmp_entp->name, snmp_entp->namelen);
    if (result != 0)
	return FAILURE;

    switch (node->subid){
    case SNMPENABLEAUTHTRAPS:
	mib_snmp->snmpEnableAuthTraps = *var->val.integer;
	setlist_delete();
	return SUCCESS;
    default:
	return FAILURE;
    }
}


init_mib_snmp()

{
    memset((char *)mib_snmp, 0, sizeof(struct mib_snmp_struct));
    mib_snmp->snmpEnableAuthTraps = 1;
}
