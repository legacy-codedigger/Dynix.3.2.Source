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

#ident	"$Header: snmp_impl.h 1.1 1991/07/31 00:06:14 $"

/*
 * snmp_impl.h
 *    Definitions for SNMP (RFC 1067) implementation.
 *
 *
 */

/* $Log: snmp_impl.h,v $
 *
 *
 */

/***********************************************************
	Copyright 1988, 1989 by Carnegie Mellon University

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


#if (defined vax) || (defined (mips)) || (defined (ns32000))
/*
 * This is a fairly bogus thing to do, but there seems to be no better way for
 * compilers that don't understand void pointers.
 */
#define void char
#endif

/*
 * Error codes:
 */
/*
 * These must not clash with SNMP error codes (all positive).
 */
#define PARSE_ERROR	-1
#define BUILD_ERROR	-2

#define SID_MAX_LEN	64
#define MAX_NAME_LEN	64  /* number of subid's in a objid */

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

struct comm_host_list {
    ulong inaddr;
    struct comm_host_list *next;
};

/* community linked list used to authenticate messages */
struct comms {
    struct comms *next;
    char *name;
    u_short access;
    struct comm_host_list *host_list;
};

/*  NOACCESS ==  0 -- see below */
#define READ	    1
#define WRITE	    2

#define RONLY	0xAAAA	/* read access for everyone */
#define RWRITE	0xAABA	/* add write access for community private */
#define WONLY	0xAABA	/* add write access for community private */
#define NOACCESS 0x0000	/* no access for anybody */

#define INTEGER	    ASN_INTEGER
#define STRING	    ASN_OCTET_STR
#define OBJID	    ASN_OBJECT_ID
#define NULLOBJ	    ASN_NULL

/* defined types (from the SMI, RFC 1065) */
#define IPADDRESS   (ASN_APPLICATION | 0)
#define COUNTER	    (ASN_APPLICATION | 1)
#define GAUGE	    (ASN_APPLICATION | 2)
#define TIMETICKS   (ASN_APPLICATION | 3)
#define OPAQUE	    (ASN_APPLICATION | 4)

#ifdef notdef
#define ERROR(string)	sendf("%s(%d): %s",__FILE__, __LINE__, string);
#else
#define ERROR(string)
#endif

/* from snmp.c*/
extern u_char	sid[];	/* size SID_MAX_LEN */

u_char	*snmp_parse_var_op();
u_char	*snmp_build_var_op();

u_char	*snmp_auth_parse();
u_char	*snmp_auth_build();
