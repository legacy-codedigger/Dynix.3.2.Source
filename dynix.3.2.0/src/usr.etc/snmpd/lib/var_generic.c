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

#ident	"$Header: var_generic.c 1.1 1991/07/31 00:06:22 $"

/*
 * var_generic.c - return a pointer to the named variable.
 *
 */

/* $Log: var_generic.c,v $
 *
 */

#include <sys/types.h>
#include <netinet/in.h>
#include "asn1.h"
#include "parse.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "mib.h"
#include "snmp_vars.h"
#include "var.h"

struct generic_entry{
    char *name;
    int namelen;
};

int
var_generic(node, name, length, exact, access_method)
    register struct tree *node;   /* IN - pointer to variable entry that points here */
    register oid        *name;      /* IN/OUT - input name requested, output name found */
    register int        *length;    /* IN/OUT - length of input and output oid's */
    int                 exact;      /* IN - TRUE if an exact match was requested. */
    int                 *access_method; /* OUT - 1 if function, 0 if char pointer. */
{
    /* There is no data at this level */
    return FAILURE;
}
var_set_generic(slist)
    register struct setlist *slist;   /* IN - first entry in a sorted list of variables to be set */
{
    /* There is no setable data at this level */
    return FAILURE;
}
