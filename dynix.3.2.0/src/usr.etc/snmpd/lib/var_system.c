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

#ident	"$Header: var_system.c 1.3 1991/08/02 16:19:02 $"

/*
 * var_system.c 
 *   Resolve the variable pointed to by node.
 *
 * filename
 *	one line description
 */

/* $Log: var_system.c,v $
 *
 *
 *
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#ifndef sequent
#include <sys/utsname.h>
#else
#include <sys/param.h>
#endif
#include <netdb.h>
#include "asn1.h"
#include "parse.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "mib.h"
#include "snmp_vars.h"
#include "var.h"
#include "snmp_api.h"
#include "debug.h"

#define SYSDESCR 1
#define SYSOBJID 2
#define SYSUPTIME 3
#define SYSCONTACT 4
#define SYSNAME 5
#define SYSLOCATION 6
#define SYSSERVICES 7

extern u_long StartTime;

u_long		uptime;
struct sys_entry{
    oid name[16];
    int namelen;
};

struct sys_entry sys_ent[] = {
    {{0}, 1 }
};

int
var_system(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid	*name;	    /* IN/OUT - input name requested, output name found */
    register int	*length;    /* IN/OUT - length of input and output oid's */
    int			exact;	    /* IN - TRUE if an exact match was requested. */
    int			*access_method;	/* OUT - 1 if function, 0 if char pointer. */
{
    struct sys_entry *sys_entp = sys_ent;
    int result;
    struct timeval tv;
    u_long data;
    char machname[MAXHOSTNAMELEN];
    int machnamelen = MAXHOSTNAMELEN;
    struct mib_system_struct *mib_system = &Mib_system;
    struct hostent *hp;

    result = check_instance(name, *length, node->objid, node->objlen, sys_entp->name, sys_entp->namelen);

    if (exact) {
	if (result != 0)
	    return FAILURE;
    } else {
	if (result >= 0)
	    return FAILURE;
    }
    if (node->access == NOACCESS)
	return FAILURE;

    if (node->value){
	*length = sys_entp->namelen;
	memcpy( name,(char *)sys_entp->name, sys_entp->namelen);
    } else
	return FAILURE;
    
    *access_method = 0;
    switch (node->subid){
    case SYSDESCR:
	memcpy((char *)node->value, mib_system->sysDescr, mib_system->DescrLen);
	node->val_len = mib_system->DescrLen;
	return SUCCESS;
    case SYSOBJID:
	node->value = (u_char *)realloc(node->value, mib_system->ObjIDLen * sizeof(oid));
	memcpy( (char *)node->value,(char *)mib_system->sysObjectID, mib_system->ObjIDLen * sizeof(oid));
	node->val_len = mib_system->ObjIDLen * sizeof(oid);
	return SUCCESS;
    case SYSUPTIME:
	gettimeofday(&tv, (struct timezone *)0);
	data = (tv.tv_sec - StartTime) * 100;
	memcpy( node->value,&data, sizeof(u_long));
	node->val_len = sizeof(u_long);
	return SUCCESS;
    case SYSCONTACT:
	memcpy((char *)node->value, mib_system->sysContact, mib_system->ContactLen);
	node->val_len = mib_system->ContactLen;
	return SUCCESS;
    case SYSNAME:
	if (gethostname(machname, machnamelen) < 0)
	    return FAILURE;
	/* get fully qualified name from nodename */
	hp = gethostbyname(machname);
	if (hp == NULL) {
	    memcpy( node->value, machname, strlen(machname));
        } else {
	    memcpy( node->value, hp->h_name, strlen(hp->h_name));
	}
	node->val_len = strlen(node->value);
	return SUCCESS;
    case SYSLOCATION:
	memcpy((char *)node->value, mib_system->sysLocation, mib_system->LocationLen);
	node->val_len = mib_system->LocationLen;
	return SUCCESS;
    case SYSSERVICES:
	*((u_long *)node->value) = mib_system->sysServices;
	return SUCCESS;
    default:
	return FAILURE;
    }
}


int
var_set_system(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    struct sys_entry *sys_entp = sys_ent;
    int result;
#ifdef sequent
    char name[31];
    int namelen = 31;
#else
    struct utsname machname;
    struct utsname *machnamep = &machname;
#endif
    struct mib_system_struct *mib_system = &Mib_system;
    struct tree *node;   /* pointer to variable entry that points here */
    struct variable_list *var;

    node = slist->tp;
    var = slist->vp;

    result = check_instance(var->name, var->name_length, node->objid, node->objlen, sys_entp->name, sys_entp->namelen);
    if (result != 0)
	return FAILURE;

    switch (node->subid){
    case SYSDESCR:
	memcpy( (char *)mib_system->sysDescr, var->val.string, var->val_len);
	mib_system->DescrLen = var->val_len;
	setlist_delete();
	return SUCCESS;
    case SYSCONTACT:
	memcpy( (char *)mib_system->sysContact, var->val.string, var->val_len);
	mib_system->ContactLen = var->val_len;
	setlist_delete();
	return SUCCESS;
/*
    case SYSNAME:
	if (sethostname(node->value, node->val_len) < 0) {
	    setlist_delete();
	    return FAILURE;
	} else {
	    setlist_delete();
	    return SUCCESS;
	}
*/
    case SYSLOCATION:
	memcpy( (char *)mib_system->sysLocation, var->val.string, var->val_len);
	mib_system->LocationLen = var->val_len;
	setlist_delete();
	return SUCCESS;
    default:
	return FAILURE;
    }
}

init_mib_system()
{
 
    struct mib_system_struct *mib_system = &Mib_system;
    extern oid default_enterprise[];  /* {1, 3, 6, 1, 4, 1, 147} enterprises.sequent */
    extern int default_enterprise_length;


    memcpy( (char *)mib_system->sysObjectID,(char *)default_enterprise, default_enterprise_length * sizeof(oid));
    mib_system->ObjIDLen = default_enterprise_length;

    mib_system->sysServices = 72;
}

	

