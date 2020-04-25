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

#ident	"$Header: parse.h 1.1 1991/07/31 00:06:03 $"

/*
 * parse.h
 *	Define tree structure used for the mib.
 */

/* $Log: parse.h,v $
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
 * parse.h
 */

/*
 * A linked list of tag-value pairs for enumerated integers.
 */
struct enum_list {
    struct enum_list *next;
    int	value;
    char *label;
};

/*
 * A linked list of nodes.
 */
struct node {
    struct node *next;
    char label[32]; /* This node's (unique) textual name */
    int	subid;	    /* This node's integer subidentifier */
    char parent[32];/* The parent's textual name */
    int type;	    /* The type of object this represents */
    int access; /* This node's access type */
    struct enum_list *enums;	/* (optional) list of enumerated integers (otherwise NULL) */
};

/*
 * A tree in the format of the tree structure of the MIB.
 */
struct tree {
    struct tree *child_list;	/* list of children of this node */
    struct tree *next_peer;	/* Next node in list of peers */
    struct tree *parent;
    char label[32];		/* This node's textual name */
    int subid;			/* This node's integer subidentifier */
    oid *objid;			/* This node's object id */
    int objlen;			/* The object id's length */
    u_short type;		/* This node's object type */
    u_char magic;               /* passed to function as a hint */
    u_short access;             /* This node's access type */
    struct enum_list *enums;	/* (optional) list of enumerated integers (otherwise NULL) */
    void (*printer)();     	/* Value printing function */
    int (*findVar)();      	/* function that finds variable */
    int (*setVar)();      	/* function that sets variable */
    u_char *value;              /* ptr to value */
    u_short val_len;            /* len of value in bytes */
};

struct setlist {
    struct variable_list *vp; 	/* structure recv'd from pdu */
    struct tree *tp;		/* tree node found thru search */
    int index;
    struct setlist *next;
};

/* non-aggregate types for tree end nodes */
#define TYPE_OTHER	    0
#define TYPE_OBJID	    1
#define TYPE_OCTETSTR	    2
#define TYPE_INTEGER	    3
#define TYPE_NETADDR	    4
#define	TYPE_IPADDR	    5
#define TYPE_COUNTER	    6
#define TYPE_GAUGE	    7
#define TYPE_TIMETICKS	    8
#define TYPE_OPAQUE	    9
#define TYPE_NULL	    10

#define INSTANCE	0
#define TABLEVAR	1

struct tree *read_mib();

/* flags for dumping the mib tree */
#define TP_PTRS		0x1
#define TP_NAMES	0x2
#define TP_OBJID	0x4
#define TP_TYPE		0x8
#define TP_VERBOSE	0x10
