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

#ident	"$Header: var.c 1.1 1991/07/31 00:06:17 $"
/*LINTLIBRARY*/

/*
 * var.c - 
 *   Process a variable list and other useful routines.
 *
 *
 */

/* $Log: var.c,v $
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
 

#include "asn1.h"
#include "snmp.h"
#include "parse.h"
#include "snmp_impl.h"
#include "snmp_api.h"
#include "snmp_client.h"
#include "mib.h"
#include "snmp_vars.h"
#include "var.h"
#include "debug.h"

#define MIN(a,b)        ((a)<(b)?(a):(b))
#define LOOKAHEAD 2

/*
 *	Each variable name is placed in the variable table, without the terminating
 * substring that determines the instance of the variable.  When a string is found that
 * is lexicographicly preceded by the input string, the function for that entry is
 * called to find the method of access of the instance of the named variable.  If
 * that variable is not found, NULL is returned, and the search through the table
 * continues (it should stop at the next entry).  If it is found, the function returns
 * a character pointer and a length or a function pointer.  The former is the address
 * of the operand, the latter is an access routine for the variable.
 *
 * u_char *
 * findVar(name, length, exact, var_len, access_method)
 * oid	    *name;	    IN/OUT - input name requested, output name found
 * int	    length;	    IN/OUT - number of sub-ids in the in and out oid's
 * int	    exact;	    IN - TRUE if an exact match was requested.
 * int	    len;	    OUT - length of variable or 0 if function returned.
 * int	    access_method; OUT - 1 if function, 0 if char pointer.
 *
 * accessVar(rw, var, varLen)
 * int	    rw;	    IN - request to READ or WRITE the variable
 * u_char   *var;   IN/OUT - input or output buffer space
 * int	    *varLen;IN/OUT - input and output buffer len
 */


extern struct tree *Mib;
extern struct mib_snmp_struct *mib_snmp;
struct setlist *Setlist = NULL;
struct tree  *cache_subtree = NULL;

/*
 * parseVariableList goes through the list of variables and retrieves each one,
 * placing it's value in the variable.  If doSet is non-zero, the variable is set
 * with the value in the packet.  If any error occurs, an error code is returned.
 */
int
parseVariableList(pdu, index)

    struct snmp_pdu     *pdu;
    register long	*index;

{
    struct variable_list *vars;
    int	    		exact;
    struct tree         *subtree = Mib;
    struct tree 	*get_value();
    struct tree 	*set_value();
    struct tree 	*getnext_value();
    struct tree 	*check_cache();
    struct setlist 	*setlist_delete();
    oid 		newname[MAX_OID_LEN];
    oid 		*np = newname;
    oid 		*tmp;
    int 		nplen;
    int 		found = 0;
    int 		first = 1;
    struct setlist 	*slistp;

    Setlist = NULL;
    *index = 1;
    
    for(vars = pdu->variables; vars; vars = vars->next_variable){
	 /* now attempt to retrieve the variable on the local entity */

	 /* Copy object id and length since the get functions change them */
	 memcpy(np, vars->name, vars->name_length * sizeof(oid));
	 nplen = vars->name_length;
	 
	 found = 0;
	 first = 1;
	 subtree = Mib;

	 switch(pdu->command) {
	 case GET_REQ_MSG:
	     exact = TRUE;
	     if ((subtree = check_cache(np, &nplen, exact)) == NULL) {
		 if (debug > 5)
		     fprintf(dfile, "Cache failed, doing tree search\n");
		 subtree = Mib;
		 subtree = get_value(np, &nplen, subtree, &found);
	     }
	     break;
	 case GETNEXT_REQ_MSG:
	     exact = FALSE;
	     if ((subtree = check_cache(np, &nplen, exact)) == NULL) {
		 if (debug > 5)
		     fprintf(dfile, "Cache failed, doing tree search\n");
		 subtree = Mib;
		 subtree = getnext_value(np, &nplen, subtree, exact, &found, &first);
	     }
	     break;
	 case SET_REQ_MSG:
	     exact = TRUE;
	     subtree = set_value(np, &nplen, subtree, &found);
	     break;
	 default:
	     if (debug)
		 fprintf(dfile, "Unknown message type %d\n", pdu->command);
	     return SNMP_ERR_GENERR;
	 }
	   
	 if (subtree != NULL) {
	     /* safety catch */
	     if (subtree->value == NULL) {
		 return SNMP_ERR_NOSUCHNAME;
	     }
	     
	     if (pdu->command != SET_REQ_MSG){
		 /* 
		  * Take an educated guess that the next request will be the 
		  * close to this one so we save it in a cache 
		  */
		  cache_subtree = subtree;
		  /* 
		   *If the variable was found, the object id returned is the 
		   * extension for this instance of the variable, so concatenate it
		   * to the original object id
		   */
		  tmp = vars->name;
		  memcpy(tmp, subtree->objid, subtree->objlen * sizeof(oid));
		  tmp += subtree->objlen;
		  memcpy(tmp, np, nplen * sizeof(oid));
		  vars->name_length = subtree->objlen + nplen;
		  /*
		   * fill in the values and lengths in the pdu struct
		   */
		  vars->type = subtree->type;;
		  vars->val_len = subtree->val_len;
		  vars->val.string = (u_char *)malloc((unsigned)vars->val_len);
		  memcpy(vars->val.string, subtree->value, vars->val_len);
		  mib_snmp->snmpInTotalReqVars++;

	      } else {   /* it is a set command */

		   /* 
		    *see if the variable is writeable 
		    */
		   if (subtree->access != RWRITE) {
			return SNMP_ERR_READONLY;
		   }
		   
		   /* 
		    * see if the type and value is consistent with this entities variable 
		    */
		   if (goodValue(vars, subtree) < 0) {
			return SNMP_ERR_BADVALUE;
		   }
		   /*
		    * add an entry to the list of variables to set
		    */
		   if (setlist_add(vars, subtree, *index) < 0) {
		       /* free the list and return an error */
		       if (debug > 5)
			   fprintf(dfile, "Error adding to setlist\n");
		       slistp = Setlist;
		       while ((slistp = setlist_delete(slistp)) != NULL) {
			   if (debug > 5)
			       fprintf(dfile, "Freeing %lx\n", slistp);
		       }
		       return SNMP_ERR_GENERR;
		   }
		   mib_snmp->snmpInTotalSetVars++;
	       }  
	     (*index)++;
	 } else {
	     return SNMP_ERR_NOSUCHNAME;
	 }
     }
    
    /*
     * If we get here and we are do a set then all variables have checked out, 
     * so we set them. 
     */
     if (pdu->command == SET_REQ_MSG){
	 while (Setlist) {
	     if (debug > 5)
		 fprintf(dfile, "Setting %s\n", Setlist->tp->label);
	     /* On return from function, Setlist will have been changed to point further down the 
	      * list. This way the set function can pull off as many of the variables from the list
	      * as it needs to do the set and also discover if any are missing.
	      */
	     subtree = Setlist->tp;
 	     if ((*subtree->setVar)(Setlist) == FAILURE) { 
		 if (debug)
		     fprintf(dfile, "Set failed\n"); 
		 /*
		  * pick up the index of the current head of the list
		  */
		 *index = Setlist->index;
		 /* 
		  * Free up any other potential sets
		  */
		 slistp = Setlist;
		 while ((slistp = setlist_delete(slistp)) != NULL) {
		     if (debug > 5)
			 fprintf(dfile, "Freeing %lx\n", slistp);
		 }
		 return SNMP_ERR_GENERR;
	     }
	 }
     }
    return SNMP_ERR_NOERROR;
}

int
compare(name1, len1, name2, len2)
    register oid	    *name1, *name2;
    register int	    len1, len2;
{
    register int    len;

    /* len = minimum of len1 and len2 */
    len = MIN(len1, len2);

    while(len-- > 0){
        if (*name1 < *name2)
            return -1;
        if (*name2++ < *name1++)
            return 1;
    }
/****
    register int    r;
    / * find first non-matching byte * /
    r = memcmp( name1, name2, len * sizeof(oid));
    / * if r != 0 return * /
    if (r)
	return r;
*****/

    /* bytes match up to length of shorter string */
    if (len1 < len2)
	return -1;  /* name1 shorter, so it is "less" */
    if (len2 < len1)
	return 1;
    return 0;	/* both strings are equal */
}
/*
 * scompare - short compare that only compares up to the length
 * of the shorter string
*/
int
scompare(name1, len1, name2, len2)
    register oid	    *name1, *name2;
    register int	    len1, len2;
{
    register int    len;

    /* len = minimum of len1 and len2 */
    len = MIN(len1, len2);

    /* find first non-matching byte */
/*
    return(memcmp(name1, name2, len * sizeof(oid)));
*/
    while(len-- > 0){
        if (*name1 < *name2)
            return -1;
        if (*name2++ < *name1++)
            return 1;
    }
    /* bytes match up to length of shorter string */
     return 0;  /* both strings are equal */

}

int
check_instance(name1, len1, name2, len2, name3, len3)
    register oid	    *name1, *name2, *name3;
    register int	    len1, len2, len3;
{
    oid *name4, *tempname;
    register int len4;
    int result;

    len4 = len2 + len3;
    tempname = name4 = (oid *)malloc(len4*sizeof(oid));
    /* build complete object id */
    memcpy( tempname,name2, len2 * sizeof(oid));
    tempname += len2;
    memcpy( tempname,name3, len3 * sizeof(oid));

    result = compare(name1, len1, name4, len4);
    free((char *)name4);

    return(result);
}
/*
 * get_instance  -  subtract objids name3 = name1 - name2
*/
int
get_instance(name1, len1, name2, len2, name3, len3)
    register oid	    *name1, *name2, *name3;
    register int	    len1, len2, len3;
{
    /* len = minimum of len1 and len2 */
    if ((len1 - len2) > len3)
	return(-1);

    /* move past name2 objects 
    register int    len;

    len = len2;
    while(len-- > 0){
	name1++;
    }
    */
    name1 += len2;

    /* copy the instance objects */
    memcpy( name3, name1, (len1 - len2) * sizeof(oid));

    return (len1 - len2);
}



/*
 * Check to see if the type specified matches the actual variable type
 */
int
goodValue(var, treenode)
    struct variable_list *var; 	/* structure recv'd from pdu */
    struct tree *treenode;	/* tree node found thru search */
{
    struct enum_list *enump;

    /* check type */
    if (var->type != treenode->type) {
	if (debug > 3)
	    fprintf(dfile, "value check: bad type %d/%d\n", var->type, treenode->type);
	return(-1);
    }

    /* check size */
    switch(var->type){
    case INTEGER:
    case COUNTER:
    case GAUGE:
    case TIMETICKS:
	if (var->val_len > sizeof(u_long)) {
	    if (debug > 3)
		fprintf(dfile, "value check: bad size %d/%d\n", var->val_len, sizeof(u_long));
	    return(-1);
	}
	break;
    case STRING:
	if (var->val_len > DESC_MAX) {
	    if (debug > 3)
		fprintf(dfile, "value check: bad size %d/%d\n", var->val_len, DESC_MAX);
	    return(-1);
	}
	break;
    case IPADDRESS:
	if (var->val_len > IPADDRESS_SIZE) {
	    if (debug > 3)
		fprintf(dfile, "value check: bad size %d/%d\n", var->val_len, IPADDRESS_SIZE);
	    return(-1);
	}
	break;
    case OBJID:
	if (var->val_len > MAX_NAME_LEN) {
	    if (debug > 3)
		fprintf(dfile, "value check: bad size %d/%d\n", var->val_len, MAX_NAME_LEN);
	    return(-1);
	}
	break;
    case OPAQUE:
    case NULLOBJ:
    default:
	if (debug > 3)
	    fprintf(dfile, "value check: bad size %d\n", var->val_len);
	return(-1);
    }

    /* check size imits (ignored for now) */

    /* 
     * check enums
     *if enum != NULL, check that val is a valid enum 
     */
    if (treenode->enums) {
	for (enump = treenode->enums; enump; enump = enump->next) {
	    if (*var->val.integer == enump->value) 
		break;
	}
	if (!enump) {
	    if (debug > 3)
		fprintf(dfile, "value check: bad enum %d\n", *var->val.integer);
	    return(-1);
	}
    }
    return(0);
}

#ifndef sequent
gettimeofday(ttp, tzp)
     struct timeval *ttp;
     char *tzp; /* this is a dummy */

{
	 time_t time();
	 time_t tloc;
	 
	 if(time(&tloc) < 0)
	     if (debug)
		 perror("time failed");
	 
	 ttp->tv_sec = tloc;
	 ttp->tv_usec = 0L;
	 return;
}

bcopy(s1,s2,s3)
    char *s1, *s2;
    int s3;
{
return(memcpy(s2, s1, s3));
}

bzero(s1, s2)
    char *s1;
    int s2;
{
return(memset(s1, 0, s2));
}

bcmp(s1, s2, n)
     char *s1, *s2;
     int n;
{
return(memcmp(s1, s2, n));
}
#else
memcpy(s1,s2,s3)
    char *s1, *s2;
    int s3;
{
return(bcopy(s2, s1, s3));
}

memset(s1, s2, s3)
    char *s1;
    int s2, s3;
{
if (s2 == 0)
    return(bzero(s1, s3));
}

memcmp(s1, s2, n)
     char *s1, *s2;
     int n;
{
return(bcmp(s2, s1, n));
}

char *strchr (s, c)
    char *s;
    int c;
{
extern char *index();
return(index(s, c));
}
#endif

/*
 * setlist_add - this routine is used to build a sorted link list of 
 * variables that are to be set. This list is built for each set request
 * received.
 */
int
setlist_add(var, treenode, index)
    struct variable_list *var; 	/* structure recv'd from pdu */
    struct tree *treenode;	/* tree node found thru search */
    int index;
{
    struct setlist *slistp;
    int result;
    struct setlist *listp;
    struct setlist *prevp;

    if ((slistp = (struct setlist *)malloc(sizeof(struct setlist))) == NULL)
	return(-1);
    
    slistp->vp = var;
    slistp->tp = treenode;
    slistp->index = index;

    /* add it to the list */
    if (Setlist == NULL) {
	/*
	 * If the list is empty, add it to the beginning
	 */
	Setlist = slistp;
	slistp->next = NULL;
	return(0);
    }
    /* Check to see if it goes at the beginning */
    result = compare(slistp->vp->name, slistp->vp->name_length, 
		     Setlist->vp->name, Setlist->vp->name_length);
    if (result < 0) {
	slistp->next = Setlist;
	Setlist = slistp;
	return(0);
    }
    
    /*
     * Search the list for the correct spot and link it in.
     */
    prevp = Setlist;
    listp = Setlist->next;
    while (listp) {
	result = compare(slistp->vp->name, slistp->vp->name_length, 
			 listp->vp->name, listp->vp->name_length);
	if (result < 0) {
	    /* add it here */
	    prevp->next = slistp;
	    slistp->next = listp;
	    return(0);
	} else {
	    prevp = listp;
	    listp = listp->next;
	}
    }
    /* if we made it here, add it to the end */
    prevp->next = slistp;
    slistp->next = NULL;
    return(0);
}

struct setlist 
*setlist_delete()
{
    struct setlist *slistp;


    /* always delete from the front of the list */

    if (Setlist == NULL) {
	if (debug)
	    fprintf(dfile, "Freeing from null setlist\n");
	return NULL;
    }

    slistp = Setlist;
    
    Setlist = slistp->next;
    free(slistp);

    return(Setlist);
}

struct tree 
*check_cache(np, nplen, exact)
    oid *np;
    int *nplen;
    int exact;
{
    oid newname[MAX_OID_LEN];
    oid *namep = newname;
    int nameplen;
    int access_method;
    struct tree *subtree;
    int i;
    int lookahead = LOOKAHEAD;
    int result;

    if (!cache_subtree) {
	if (debug > 5)
	    fprintf(dfile, "cache_subtree is NULL\n");
	return NULL;
    }

    /* Copy object id and length since the get functions change them */
    memcpy(namep, np, *nplen * sizeof(oid));
    nameplen = *nplen;
	
    for (i=0, subtree = cache_subtree; (i < lookahead) && (subtree); i++, subtree = subtree->next_peer) {
	if (exact)
	    /* this should catch sequential gets */
	    result = scompare(np, *nplen, subtree->objid, subtree->objlen);
        else
	    /* this will only catch table data */
	    result = compare(np, *nplen, subtree->objid, subtree->objlen);
	if (result >= 0) {
	    if (((*subtree->findVar)(subtree, np, nplen, exact, &access_method)) == SUCCESS) {
		if (debug > 5)
		    fprintf(dfile, "Found %s in cache\n", subtree->label);
		return(subtree);
	    } else {
		/* restore object id and length since the get functions change them */
		memcpy(np, namep, nameplen * sizeof(oid));
		*nplen = nameplen;
	    }
	}
    }
	
    return(NULL);
}
