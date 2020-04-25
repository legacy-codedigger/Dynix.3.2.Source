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

#ident	"$Header: snmp.h 1.1 1991/07/31 00:06:05 $"

/*
 * snmp.h
 *    Definitions for the Simple Network Management Protocol (RFC 1067).
 *
 *
 */

/* $Log: snmp.h,v $
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

#define SNMP_PORT	    161
#define SNMP_TRAP_PORT	    162

#define SNMP_MAX_LEN	    484

#define SNMP_VERSION_1	    0

#define GET_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x0)
#define GETNEXT_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x1)
#define GET_RSP_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x2)
#define SET_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x3)
#define TRP_REQ_MSG	    (ASN_CONTEXT | ASN_CONSTRUCTOR | 0x4)

#define SNMP_ERR_NOERROR    (0x0)
#define SNMP_ERR_TOOBIG	    (0x1)
#define SNMP_ERR_NOSUCHNAME (0x2)
#define SNMP_ERR_BADVALUE   (0x3)
#define SNMP_ERR_READONLY   (0x4)
#define SNMP_ERR_GENERR	    (0x5)

#define SNMP_TRAP_COLDSTART		(0x0)
#define SNMP_TRAP_WARMSTART		(0x1)
#define SNMP_TRAP_LINKDOWN		(0x2)
#define SNMP_TRAP_LINKUP		(0x3)
#define SNMP_TRAP_AUTHFAIL		(0x4)
#define SNMP_TRAP_EGPNEIGHBORLOSS	(0x5)
#define SNMP_TRAP_ENTERPRISESPECIFIC	(0x6)

