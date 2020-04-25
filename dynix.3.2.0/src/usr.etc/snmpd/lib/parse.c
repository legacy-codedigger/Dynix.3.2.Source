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

#ident	"$Header: parse.c 1.1 1991/07/31 00:06:01 $"

/*
 * parse.c
 *	parse asn1 objects, also build mib tree from mib file
 */

/* $Log: parse.c,v $
 *
 */

/***********************************************************
	Copyright 1989 by Carnegie Mellon University

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
/*
 * parse.c
 */

/*
* added displaystring
*/
/*
* added parsing of display string size (actually ignore the size)
*/

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "asn1.h"
#include "parse.h"
#include "snmp_impl.h"
#include "snmp_vars.h"
#include "mib.h"
#include "func.h"
#include "snmp_api.h"
#include "debug.h"
/*
 * This is one element of an object identifier with either an integer subidentifier,
 * or a textual string label, or both.
 * The subid is -1 if not present, and label is NULL if not present.
 */
struct subid {
    int subid;
    char *label;
};

int Line = 1;

/* types of tokens */
/* Changed the names of these constants because they conflicted with constants
 * of the same name declared in other .h files */
#define	TOKEN_CONTINUE    -1
#define TOKEN_LABEL	  1
#define TOKEN_SUBTREE	  2
#define TOKEN_SYNTAX	  3
#define TOKEN_OBJID	  4
#define TOKEN_OCTETSTR    5
#define TOKEN_INTEGER	  6
#define TOKEN_NETADDR	  7
#define	TOKEN_IPADDR	  8
#define TOKEN_COUNTER	  9
#define TOKEN_GAUGE	  10
#define TOKEN_TIMETICKS   11
#define TOKEN_OPAQUE	  12
#define TOKEN_NUL	  13
#define TOKEN_SEQUENCE    14
#define TOKEN_OF	  15	/* SEQUENCE OF */
#define TOKEN_OBJTYPE	  16
#define TOKEN_ACCESS	  17
#define TOKEN_READONLY    18
#define TOKEN_READWRITE   19
#define	TOKEN_WRITEONLY   20
#define TOKEN_NOACCESS    21
#define TOKEN_STATUS	  22
#define TOKEN_MANDATORY   23
#define TOKEN_OPTIONAL    24
#define TOKEN_OBSOLETE    25
#define TOKEN_PUNCT	  26
#define TOKEN_EQUALS	  27
#define TOKEN_DEPRECATED  28

struct tok {
	char *name;			/* token name */
	int len;			/* length not counting nul */
	int token;			/* value */
	int hash;			/* hash of name */
	struct tok *next;		/* pointer to next in hash table */
};


struct tok tokens[] = {
	{ "obsolete", sizeof ("obsolete")-1, TOKEN_OBSOLETE },
	{ "Opaque", sizeof ("Opaque")-1, TOKEN_OPAQUE },
	{ "optional", sizeof ("optional")-1, TOKEN_OPTIONAL },
	{ "mandatory", sizeof ("mandatory")-1, TOKEN_MANDATORY },
	{ "not-accessible", sizeof ("not-accessible")-1, TOKEN_NOACCESS },
	{ "deprecated", sizeof ("deprecated")-1, TOKEN_DEPRECATED },
	{ "write-only", sizeof ("write-only")-1, TOKEN_WRITEONLY },
	{ "read-write", sizeof ("read-write")-1, TOKEN_READWRITE },
	{ "TimeTicks", sizeof ("TimeTicks")-1, TOKEN_TIMETICKS },
	{ "OBJECTIDENTIFIER", sizeof ("OBJECTIDENTIFIER")-1, TOKEN_OBJID },
	/*
	 * This CONTINUE appends the next word onto OBJECT,
	 * hopefully matching OBJECTIDENTIFIER above.
	 */
	{ "OBJECT", sizeof ("OBJECT")-1, TOKEN_CONTINUE },
	{ "NetworkAddress", sizeof ("NetworkAddress")-1, TOKEN_NETADDR },
	{ "Gauge", sizeof ("Gauge")-1, TOKEN_GAUGE },
	{ "OCTETSTRING", sizeof ("OCTETSTRING")-1, TOKEN_OCTETSTR },
	{ "OCTET", sizeof ("OCTET")-1, -1 },
	{ "OF", sizeof ("OF")-1, TOKEN_OF },
	{ "SEQUENCE", sizeof ("SEQUENCE")-1, TOKEN_SEQUENCE },
	{ "NULL", sizeof ("NULL")-1, TOKEN_NUL },
	{ "IpAddress", sizeof ("IpAddress")-1, TOKEN_IPADDR },
	{ "INTEGER", sizeof ("INTEGER")-1, TOKEN_INTEGER },
	{ "Counter", sizeof ("Counter")-1, TOKEN_COUNTER },
	{ "read-only", sizeof ("read-only")-1, TOKEN_READONLY },
	{ "ACCESS", sizeof ("ACCESS")-1, TOKEN_ACCESS },
	{ "STATUS", sizeof ("STATUS")-1, TOKEN_STATUS },
	{ "SYNTAX", sizeof ("SYNTAX")-1, TOKEN_SYNTAX },
	{ "OBJECT-TYPE", sizeof ("OBJECT-TYPE")-1, TOKEN_OBJTYPE },
	{ "DisplayString", sizeof ("DisplayString")-1, TOKEN_OCTETSTR },
	{ "{", sizeof ("{")-1, TOKEN_PUNCT },
	{ "}", sizeof ("}")-1, TOKEN_PUNCT },
	{ "::=", sizeof ("::=")-1, TOKEN_EQUALS },
	{ NULL }
};

#define	HASHSIZE	34
#define	BUCKET(x)	(x & 0x021)

struct tok	*buckets[HASHSIZE];

static
hash_init()
{
	register struct tok	*tp;
	register char	*cp;
	register int	h;
	register int	b;

	for (tp = tokens; tp->name; tp++) {
		for (h = 0, cp = tp->name; *cp; cp++)
			h += *cp;
		tp->hash = h;
		b = BUCKET(h);
		if (buckets[b])
			tp->next = buckets[b];
		buckets[b] = tp;
	}
}


static char *
Malloc(num)
    unsigned num;
{
    char *cp;
    char *malloc();
    
    /* this is to fix (what seems to be) a problem with the IBM RT C library malloc */
    if (num < 16)
	num = 16;
    cp = malloc(num);
    return cp;
}

static
print_error(string, token)
    char *string;
    char *token;
{
    if (token) {
	syslog(LOG_WARNING, "Mib parsing error: %s(%s): On or around line %d", string, token, Line);
	if (debug)
	    fprintf(dfile, "Mib parsing error: %s(%s): On or around line %d\n", string, token, Line);
    } else {
	syslog(LOG_WARNING, "Mib parsing error: %s: On or around line %d", string, Line);
	if (debug)
	    fprintf(dfile, "Mib parsing error: %s: On or around line %d\n", string, Line);
    }
}

/* #ifdef TEST */
print_subtree(tree, count, flags)
    struct tree *tree;
    int count;
    int flags;
{
    struct tree *tp;
    int i, j;

    if (flags & TP_VERBOSE) {
	for(i = 0; i < count; i++)
	    printf("  ");
	printf("Children of %s:\n", tree->label);
    }
    count++;
    for(tp = tree->child_list; tp; tp = tp->next_peer){
	if (flags & TP_VERBOSE) {
	    for(i = 0; i < count; i++)
		printf("  ");
	}
	if (flags & TP_NAMES)
	    printf("%-23.23s ", tp->label );
	if (flags & TP_TYPE) {
	    switch (tp->type) {
	    case TYPE_OTHER:
		printf("%-10.10s", "other");
		break;
	    case OBJID:
		printf("%-10.10s", "objid");
		break;
	    case STRING:
		printf("%-10.10s", "octetstr");
		break;
	    case INTEGER:
		printf("%-10.10s", "integer");
		break;
	    case IPADDRESS:
		printf("%-10.10s", "ipaddr");
		break;
	    case COUNTER:
		printf("%-10.10s", "counter");
		break;
	    case GAUGE:
		printf("%-10.10s", "gauge");
		break;
	    case TIMETICKS:
		printf("%-10.10s", "timeticks");
		break;
	    case OPAQUE:
		printf("%-10.10s", "opaque");
		break;
	    case TYPE_NULL:
		printf("%-10.10s", "null");
		break;
	    default:
		printf("%-10.10s", "unknown");
		break;
	    }
	}
	if (flags & TP_OBJID) {
	    printf("(" );
	    for (j=0;j<tp->objlen;j++) {
		printf("%d", tp->objid[j]);
		printf (".");
	    }
	    printf(")");
	}
	if (flags & TP_PTRS)
	    printf(" (left=%s right=%s parent=%s)", (tp->child_list) ? tp->child_list->label : "NULL" , 
		   (tp->next_peer) ? tp->next_peer->label : "NULL" , (tp->parent) ? tp->parent->label : "NULL" );
	printf("\n");
    }
    for(tp = tree->child_list; tp; tp = tp->next_peer){
	print_subtree(tp, count, flags);
    }
}
/* #endif */ /* TEST */


static struct tree *
build_tree(nodes)
    struct node *nodes;
{
    struct node *np;
    struct tree *tp;
    
    /* build root node */
    tp = (struct tree *)Malloc(sizeof(struct tree));
    tp->parent = NULL;
    tp->next_peer = NULL;
    tp->child_list = NULL;
    tp->enums = NULL;
    strcpy(tp->label, "iso");
    tp->subid = 1;

    tp->objid = (oid *)malloc(sizeof(oid));
    *(tp->objid) = 1;
    
    tp->objlen = 1;
    tp->type = 0;
    tp->access = TOKEN_READONLY;
    tp->findVar = var_generic;
    tp->setVar = var_set_generic;
    /* grow tree from this root node */
    do_subtree(tp, &nodes);
#ifdef TEST
    print_subtree(tp, 0);
#endif /* TEST */
    /* If any nodes are left, the tree is probably inconsistent */
    if (nodes){
	syslog(LOG_WARNING, "The mib description doesn't seem to be consistent.");
	syslog(LOG_WARNING, "Some nodes couldn't be linked under the \"iso\" tree.\n");
	if (debug) {
	    fprintf(dfile, "The mib description doesn't seem to be consistent.\n");
	    fprintf(dfile, "Some nodes couldn't be linked under the \"iso\" tree.\n");
	    fprintf(dfile, "these nodes are left:\n");
	    for(np = nodes; np; np = np->next)
		fprintf(dfile, "%s ::= { %s %d } (%d)\n", np->label, np->parent, np->subid,
		    np->type);
	}
    }
    return tp;
}


/*
 * Find all the children of root in the list of nodes.  Link them into the
 * tree and out of the nodes list.
 */
static
do_subtree(root, nodes)
    struct tree *root;
    struct node **nodes;
{
    register struct tree *tp;
    struct tree *peer = NULL;
    register struct node *np;
    struct node *oldnp = NULL, *child_list = NULL, *childp = NULL;
    struct varToFunc *funcp;
    int x;
    extern struct varToFunc varToFuncTable[];

    tp = root;
    /*
     * Search each of the nodes for one whose parent is root, and
     * move each into a seperate list.
     */
    for(np = *nodes; np; np = np->next){
	if ((tp->label[0] == np->parent[0]) && !strcmp(tp->label, np->parent)){
	    if (child_list == NULL){
		child_list = childp = np;   /* first entry in child list */
	    } else {
		childp->next = np;
		childp = np;
	    }
	    /* take this node out of the node list */
	    if (oldnp == NULL){
		*nodes = np->next;  /* fix root of node list */
	    } else {
		oldnp->next = np->next;	/* link around this node */
	    }
	} else {
	    oldnp = np;
	}
    }
    if (childp)
	childp->next = 0;	/* re-terminate list */
    /*
     * Take each element in the child list and place it into the tree.
     */
    for(np = child_list; np; np = np->next){
	tp = (struct tree *)Malloc(sizeof(struct tree));
	tp->parent = root;
	tp->next_peer = NULL;
	tp->child_list = NULL;
	strcpy(tp->label, np->label);
	tp->subid = np->subid;
	tp->objid = (oid *)malloc((root->objlen + 1) * sizeof(oid));
	memcpy( (char *)tp->objid,(char *)root->objid, (int)root->objlen * sizeof(oid));
	*(tp->objid + root->objlen) = np->subid;
	tp->objlen = root->objlen + 1;
	switch(np->type){
	    case TOKEN_OBJID:
		tp->type = OBJID;
		/* allocate single oid -- realloc later if needed */
		tp->value = (u_char *)malloc(sizeof(oid));
		tp->val_len = sizeof(oid);
		break;
	    case TOKEN_OCTETSTR:
		tp->type = STRING;
		tp->value = (u_char *)malloc(DESC_MAX);
		tp->val_len = DESC_MAX;
		break;
	    case TOKEN_INTEGER:
		tp->type = INTEGER;
		tp->value = (u_char *)malloc(sizeof(u_long));
		tp->val_len = sizeof(u_long);
		break;
	    case TOKEN_NETADDR:
		tp->type = IPADDRESS;
		tp->value = (u_char *)malloc(IPADDRESS_SIZE);
		tp->val_len = IPADDRESS_SIZE;
		break;
	    case TOKEN_IPADDR:
		tp->type = IPADDRESS;
		tp->value = (u_char *)malloc(IPADDRESS_SIZE);
		tp->val_len = IPADDRESS_SIZE;
		break;
	    case TOKEN_COUNTER:
		tp->type = COUNTER;
		tp->value = (u_char *)malloc(sizeof(u_long));
		tp->val_len = sizeof(u_long);
		break;
	    case TOKEN_GAUGE:
		tp->type = GAUGE;
		tp->value = (u_char *)malloc(sizeof(u_long));
		tp->val_len = sizeof(u_long);
		break;
	    case TOKEN_TIMETICKS:
		tp->type = TIMETICKS;
		tp->value = (u_char *)malloc(sizeof(u_long));
		tp->val_len = sizeof(u_long);
		break;
	    case TOKEN_OPAQUE:
		tp->type = OPAQUE;
		tp->value = NULL;
		tp->val_len = 0;
		break;
	    case TOKEN_NUL:
		tp->type = TYPE_NULL;
		tp->value = NULL;
		tp->val_len = 0;
		break;
	    default:
		tp->type = TYPE_OTHER;
		tp->value = NULL;
		tp->val_len = 0;
		break;
	}
	tp->access = np->access;

	tp->enums = np->enums;
	np->enums = NULL;	/* so we don't free them later */

	/* A very inefficient procedure...
	 * search through the varToFunc table and place the function
	 * ptr for this variable in the structure
	 */
	for(x = 0, funcp = varToFuncTable; x < sizeof(varToFuncTable)/sizeof(struct varToFunc); funcp++, x++){
	    if (strcmp(funcp->var, tp->label) == 0) {
		tp->findVar = funcp->getfunc;
		tp->setVar = funcp->setfunc;
		break;
	    }
	}
	if (x == sizeof(varToFuncTable)/sizeof(struct varToFunc))
	    tp->findVar = var_generic;

	if (root->child_list == NULL){
	    root->child_list = tp;
	} else {
	    peer->next_peer = tp;
	}
	peer = tp;
	do_subtree(tp, nodes);	/* recurse on this child */
    }
    /* free all nodes that were copied into tree */
    for(np = child_list; np;){
	oldnp = np;
	np = np->next;
	free_node(oldnp);
    }
}


/*
 * Takes a list of the form:
 * { iso org(3) dod(6) 1 }
 * and creates several nodes, one for each parent-child pair.
 * Returns NULL on error.
 */
static int
getoid(fp, obid,  length)
    register FILE *fp;
    register struct subid *obid;	/* an array of subids */
    int length;	    /* the length of the array */
{
    register int count;
    int type;
    char token[64], label[32];
    register char *cp, *tp;

    if ((type = get_token(fp, token)) != TOKEN_PUNCT){
	if (type == -1)
	    print_error("Unexpected EOF", (char *)NULL);
	else
	    print_error("Unexpected", token);
	return NULL;
    }
    if (*token != '{'){
	print_error("Unexpected", token);
	return NULL;
    }
    for(count = 0; count < length; count++, obid++){
	obid->label = 0;
	obid->subid = -1;
	if ((type = get_token(fp, token)) != TOKEN_LABEL){
	    if (type == -1){
		print_error("Unexpected EOF", (char *)NULL);
		return NULL;
	    }
	    else if (type == TOKEN_PUNCT && *token == '}'){
		return count;
	    } else {
		print_error("Unexpected", token);
		return NULL;
	    }
	}
	tp = token;
	if (!isdigit(*tp)){
	    /* this entry has a label */
	    cp = label;
	    while(*tp && *tp != '(')
		*cp++ = *tp++;
	    *cp = 0;
	    cp = (char *)Malloc((unsigned)strlen(label));
	    strcpy(cp, label);
	    obid->label = cp;
	    if (*tp == '('){
		/* this entry has a label-integer pair in the form label(integer). */
		cp = ++tp;
		while(*cp && *cp != ')')
		    cp++;
		if (*cp == ')')
		    *cp = 0;
		else {
		    print_error("No terminating parenthesis", (char *)NULL);
		    return NULL;
		}
		obid->subid = atoi(tp);
	    }
	} else {
	    /* this entry  has just an integer sub-identifier */
	    obid->subid = atoi(tp);
	}
    }
    return count;


}

static
free_node(np)
    struct node *np;
{
    struct enum_list *ep, *tep;

    ep = np->enums;
    while(ep){
	tep = ep;
	ep = ep->next;
	free((char *)tep);
    }
    free((char *)np);
}

/*
 * Parse an entry of the form:
 * label OBJECT IDENTIFIER ::= { parent 2 }
 * The "label OBJECT IDENTIFIER" portion has already been parsed.
 * Returns 0 on error.
 */
static struct node *
parse_objectid(fp, name)
    FILE *fp;
    char *name;
{
    int type;
    char token[64];
    register int count;
    register struct subid *op, *nop;
    int length;
    struct subid obid[16];
    struct node *np, *root;

    type = get_token(fp, token);
    if (type != TOKEN_EQUALS){
	print_error("Bad format", token);
	return 0;
    }
    if (length = getoid(fp, obid, 16)){
	np = root = (struct node *)Malloc(sizeof(struct node));
	/*
	 * For each parent-child subid pair in the subid array,
	 * create a node and link it into the node list.
	 */
	for(count = 0, op = obid, nop=obid+1; count < (length - 2); count++,
	    op++, nop++){
	    /* every node must have parent's name and child's name or number */
	    if (op->label && (nop->label || (nop->subid != -1))){
		strcpy(np->parent, op->label);
		if (nop->label)
		    strcpy(np->label, nop->label);
		if (nop->subid != -1)
		    np->subid = nop->subid;
		np ->type = 0;
		np->enums = 0;
		/* set up next entry */
		np->next = (struct node *)Malloc(sizeof(*np->next));
		np = np->next;
	    }
	}
	/*
	 * The above loop took care of all but the last pair.  This pair is taken
	 * care of here.  The name for this node is taken from the label for this
	 * entry.
	 * np still points to an unused entry.
	 */
	if (count == (length - 2)){
	    if (op->label){
		strcpy(np->parent, op->label);
		strcpy(np->label, name);
		if (nop->subid != -1)
		    np->subid = nop->subid;
		else
		    print_error("Warning: This entry is pretty silly", token);
	    } else {
		free_node(np);
	    }
	} else {
	    print_error("Missing end of obid", (char *)NULL);
	    free_node(np);   /* the last node allocated wasn't used */
	    return NULL;
	}
	/* free the obid array */
	for(count = 0, op = obid; count < length; count++, op++){
	    if (op->label)
		free(op->label);
	    op->label = 0;
	}
	return root;
    } else {
	print_error("Bad object identifier", (char *)NULL);
	return 0;
    }
}

/*
 * Parses an asn type.  This structure is ignored by this parser.
 * Returns NULL on error.
 */
static int
parse_asntype(fp)
    FILE *fp;
{
    int type;
    char token[64];

    type = get_token(fp, token);
    if (type != TOKEN_SEQUENCE){
	print_error("Not a sequence", (char *)NULL); /* should we handle this */
	return NULL;
    }
    while((type = get_token(fp, token)) != NULL){
	if (type == -1)
	    return NULL;
	if (type == TOKEN_PUNCT && (token[0] == '}' && token[1] == '\0'))
	    return -1;
    }
    print_error("Premature end of file", (char *)NULL);
    return NULL;
}

/*
 * Parses an OBJECT TYPE macro.
 * Returns 0 on error.
 */
static struct node *
parse_objecttype(fp, name)
    register FILE *fp;
    char *name;
{
    register int type;
    char token[64];
    int count, length;
    struct subid obid[16];
    char syntax[32];
    int nexttype;
    char nexttoken[64];
    register struct node *np;
    register struct enum_list *ep;
    register char *cp;
    register char *tp;

    type = get_token(fp, token);
    if (type != TOKEN_SYNTAX){
	print_error("Bad format for OBJECT TYPE", token);
	return 0;
    }
    np = (struct node *)Malloc(sizeof(struct node));
    np->next = 0;
    np->enums = 0;
    type = get_token(fp, token);
    nexttype = get_token(fp, nexttoken);
    np->type = type;
    switch(type){
	case TOKEN_SEQUENCE:
	    strcpy(syntax, token);
	    if (nexttype == TOKEN_OF){
		strcat(syntax, " ");
		strcat(syntax, nexttoken);
		nexttype = get_token(fp, nexttoken);
		strcat(syntax, " ");
		strcat(syntax, nexttoken);
		nexttype = get_token(fp, nexttoken);
	    }
	    break;
	case TOKEN_INTEGER:
	    strcpy(syntax, token);
	    if (nexttype == TOKEN_PUNCT &&
		(nexttoken[0] == '{' && nexttoken[1] == '\0')) {
		/* if there is an enumeration list, parse it */
		while((type = get_token(fp, token)) != NULL){
		    if (type == -1){
			free_node(np);
			return 0;
		    }
		    if (type == TOKEN_PUNCT &&
			(token[0] == '}' && token[1] == '\0'))
			break;
		    if (type == 1){
			/* this is an enumerated label */
			if (np->enums == 0){
			    ep = np->enums = (struct enum_list *)
					Malloc(sizeof(struct enum_list));
			} else {
			    ep->next = (struct enum_list *)
					Malloc(sizeof(struct enum_list));
			    ep = ep->next;
			}
			ep->next = 0;
			/* a reasonable approximation for the length */
			ep->label = (char *)Malloc((unsigned)strlen(token));
			cp = ep->label;
			tp = token;
			while(*tp != '(')
			    *cp++ = *tp++;
			*cp = 0;
			cp = ++tp;    /* start of number */
			while(*tp != ')')
			    tp++;
			*tp = 0;    /* terminate number */
			ep->value = atoi(cp);
		    }
		}
		if (type == NULL){
		    print_error("Premature end of file", (char *)NULL);
		    free_node(np);
		    return 0;
		}
		nexttype = get_token(fp, nexttoken);
	    } else if (nexttype == TOKEN_LABEL && *nexttoken == '('){
		/* ignore the "constrained integer" for now */
		nexttype = get_token(fp, nexttoken);
	    }
	    break;
	case TOKEN_OBJID:
	    strcpy(syntax, token);
	    break;
	case TOKEN_OCTETSTR:
	    strcpy(syntax, token);
	    while (nexttype == TOKEN_LABEL && *nexttoken == '('){
		/* ignore the "string size" for now */
		nexttype = get_token(fp, nexttoken);
	    }
	    break;
	case TOKEN_NETADDR:
	case TOKEN_IPADDR:
	case TOKEN_COUNTER:
	case TOKEN_GAUGE:
	case TOKEN_TIMETICKS:
	case TOKEN_OPAQUE:
	case TOKEN_NUL:
	case TOKEN_LABEL:
	    strcpy(syntax, token);
	    break;
	default:
	    print_error("Bad syntax", token);
	    free_node(np);
	    return 0;
    }
    if (nexttype != TOKEN_ACCESS){
	print_error("Should be ACCESS", nexttoken);
	free_node(np);
	return 0;
    }
    type = get_token(fp, token);
    switch(type){
    case TOKEN_READONLY:
      np->access = RONLY; /* set access */
      break;
    case TOKEN_READWRITE:
      np->access = RWRITE; /* set access */
      break;
    case TOKEN_WRITEONLY:
      np->access = WONLY; /* set access */
      break;
    case TOKEN_NOACCESS:
      np->access = NOACCESS; /* set access */
      break;
    default:
      print_error("Bad access type", nexttoken);
      free_node(np);
      return 0;
    };
/*
    if (type != TOKEN_READONLY && type != TOKEN_READWRITE && type != TOKEN_WRITEONLY
	&& type != TOKEN_NOACCESS){
	print_error("Bad access type", nexttoken);
	free_node(np);
	return 0;
    } else 
	np->access = type; / * set access * /
*/
    type = get_token(fp, token);
    if (type != TOKEN_STATUS){
	print_error("Should be STATUS", token);
	free_node(np);
	return 0;
    }
    type = get_token(fp, token);
    if (type != TOKEN_MANDATORY && type != TOKEN_OPTIONAL && type != TOKEN_OBSOLETE && type != TOKEN_DEPRECATED){
	print_error("Bad status", token);
	free_node(np);
	return 0;
    }
    type = get_token(fp, token);
    if (type != TOKEN_EQUALS){
	print_error("Bad format", token);
	free_node(np);
	return 0;
    }
    length = getoid(fp, obid, 16);
    if (length > 1 && length <= 16){
	/* just take the last pair in the obid list */
	if (obid[length - 2].label)
	    strncpy(np->parent, obid[length - 2].label, 32);
	strcpy(np->label, name);
	if (obid[length - 1].subid != -1)
	    np->subid = obid[length - 1].subid;
	else
	    print_error("Warning: This entry is pretty silly", (char *)NULL);
    } else {
	print_error("No end to obid", (char *)NULL);
	free_node(np);
	np = 0;
    }
    /* free obid array */
    for(count = 0; count < length; count++){
	if (obid[count].label)
	    free(obid[count].label);
	obid[count].label = 0;
    }
    return np;
}


/*
 * Parses a mib file and returns a linked list of nodes found in the file.
 * Returns NULL on error.
 */
static struct node *
parse(fp)
    FILE *fp;
{
    char token[64];
    char name[32];
    int	type = 1;
    struct node *np, *root = NULL;

    hash_init();

    while(type != NULL){
	type = get_token(fp, token);
	if (type != TOKEN_LABEL){
	    if (type == NULL){
		return root;
	    }
	    print_error(token, "is a reserved word");
	    return NULL;
	}
	strncpy(name, token, 32);
	type = get_token(fp, token);
	if (type == TOKEN_OBJTYPE){
	    if (root == NULL){
		/* first link in chain */
		np = root = parse_objecttype(fp, name);
		if (np == NULL){
		    print_error("Bad parse of object type", (char *)NULL);
		    return NULL;
		}
	    } else {
		np->next = parse_objecttype(fp, name);
		if (np->next == NULL){
		    print_error("Bad parse of objecttype", (char *)NULL);
		    return NULL;
		}
	    }
	    /* now find end of chain */
	    while(np->next)
		np = np->next;

	} else if (type == TOKEN_OBJID){
	    if (root == NULL){
		/* first link in chain */
		np = root = parse_objectid(fp, name);
		if (np == NULL){
		    print_error("Bad parse of object id", (char *)NULL);
		    return NULL;
		}
	    } else {
		np->next = parse_objectid(fp, name);
		if (np->next == NULL){
		    print_error("Bad parse of object type", (char *)NULL);
		    return NULL;
		}
	    }
	    /* now find end of chain */
	    while(np->next)
		np = np->next;
	} else if (type == TOKEN_EQUALS){
	    type = parse_asntype(fp);
	} else if (type == NULL){
	    break;
	} else {
	    print_error("Bad operator", (char *)NULL);
	    return NULL;
	}
    }
#ifdef TEST
{
    if (debug) {
	struct enum_list *ep;
    
	for(np = root; np; np = np->next){
	    fprintf(dfile, "%s ::= { %s %d } (%d)\n", np->label, np->parent, np->subid,
		    np->type);
	    if (np->enums){
		fprintf(dfile, "Enums: \n");
		for(ep = np->enums; ep; ep = ep->next){
		    fprintf(dfile, "%s(%d)\n", ep->label, ep->value);
		}
	    }
	}
    }
}
#endif /* TEST */
    return root;
}

/*
 * Parses a token from the file.  The type of the token parsed is returned,
 * and the text is placed in the string pointed to by token.
 */
static int
get_token(fp, token)
    register FILE *fp;
    register char *token;
{
    static char last = ' ';
    register int ch;
    register char *cp = token;
    register int hash = 0;
    register struct tok *tp;

    *cp = 0;
    ch = last;
    /* skip all white space */
    while(isspace(ch) && ch != -1){
	ch = getc(fp);
	if (ch == '\n')
	    Line++;
    }
    if (ch == -1)
	return NULL;

    /*
     * Accumulate characters until white space is found.  Then attempt to match this
     * token as a reserved word.  If a match is found, return the type.  Else it is
     * a label.
     */
    do {
	if (!isspace(ch)){
	    hash += ch;
	    *cp++ = ch;
	    if (ch == '\n')
		Line++;
	} else {
	    last = ch;
	    *cp = '\0';

	    for (tp = buckets[BUCKET(hash)]; tp; tp = tp->next) {
		if ((tp->hash == hash) && (strcmp(tp->name, token) == 0))
			break;
	    }
	    if (tp){
		if (tp->token == TOKEN_CONTINUE)
		    continue;
		return (tp->token);
	    }

	    if (token[0] == '-' && token[1] == '-'){
		/* strip comment */
		while ((ch = getc(fp)) != -1)
		    if (ch == '\n'){
			Line++;
			break;
		    }
		if (ch == -1)
		    return NULL;
		last = ch;
		return get_token(fp, token);		
	    }
	    return TOKEN_LABEL;
	}
    
    } while ((ch = getc(fp)) != -1);
    return NULL;
}

struct tree *
read_mib(filename)
    char *filename;
{
    FILE *fp;
    struct node *nodes;
    struct tree *tree;
    struct node *parse();

    fp = fopen(filename, "r");
    if (fp == NULL)
	return NULL;
    nodes = parse(fp);
    if (!nodes){
	syslog(LOG_ERR, "Mib table is bad.  Exiting");
	if (debug)
	    fprintf(dfile, "Mib table is bad.  Exiting\n");
	return NULL;
    }
    tree = build_tree(nodes);
    fclose(fp);
    return tree;
}

