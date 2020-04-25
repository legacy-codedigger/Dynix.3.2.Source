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

#ident	"$Header: snmp.c 1.1 1991/07/31 00:06:04 $"

/*
 * snmp.c
 *    Simple Network Management Protocol (RFC 1067).
 *
 */

/* $Log: snmp.c,v $
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
#include <stdio.h>

#ifdef KINETICS
#include "gw.h"
#include "ab.h"
#include "inet.h"
#include "fp4/cmdmacro.h"
#include "fp4/pbuf.h"
#include "glob.h"
#endif

#if (defined(unix) && !defined(KINETICS))
#include <sys/types.h>
#include <netinet/in.h>
#ifndef NULL
#define NULL 0
#endif
#endif

#include "asn1.h"
#include "snmp.h"
#include "snmp_impl.h"

#include "mib.h"
#include "snmp_vars.h"
#include "debug.h"

u_char *
snmp_parse_var_op(data, var_name, var_name_len, var_val_type, var_val_len, var_val, listlength)
    register u_char *data;  /* IN - pointer to the start of object */
    oid	    *var_name;	    /* OUT - object id of variable */
    int	    *var_name_len;  /* OUT - length of variable name */
    u_char  *var_val_type;  /* OUT - type of variable (int or octet string) (one byte) */
    int	    *var_val_len;   /* OUT - length of variable */
    u_char  **var_val;	    /* OUT - pointer to ASN1 encoded value of variable */
    int	    *listlength;    /* IN/OUT - number of valid bytes left in var_op_list */
{
    u_char	    var_op_type;
    int		    var_op_len = *listlength;
    u_char	    *var_op_start = data;

    data = asn_parse_header(data, &var_op_len, &var_op_type);
    if (data == NULL){
	ERROR("");
	return NULL;
    }
    if (var_op_type != (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR))
	return NULL;
    data = asn_parse_objid(data, &var_op_len, &var_op_type, var_name, var_name_len);
    if (data == NULL){
	ERROR("");
	return NULL;
    }
    if (var_op_type != (u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID))
	return NULL;
    *var_val = data;	/* save pointer to this object */
    /* find out what type of object this is */
    data = asn_parse_header(data, &var_op_len, var_val_type);
    if (data == NULL){
	ERROR("");
	return NULL;
    }
    *var_val_len = var_op_len;
    data += var_op_len;
    *listlength -= (int)(data - var_op_start);
    return data;
}

u_char *
snmp_build_var_op(data, var_name, var_name_len, var_val_type, var_val_len, var_val, listlength)
    register u_char *data;	/* IN - pointer to the beginning of the output buffer */
    oid		*var_name;	/* IN - object id of variable */
    int		*var_name_len;	/* IN - length of object id */
    u_char	var_val_type;	/* IN - type of variable */
    int		var_val_len;	/* IN - length of variable */
    u_char	*var_val;	/* IN - value of variable */
    register int *listlength;    /* IN/OUT - number of valid bytes left in output buffer */
{
    int		    dummyLen, headerLen;
    u_char	    *dataPtr;

    dummyLen = *listlength;
    dataPtr = data;
    data = asn_build_header(data, &dummyLen, (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR), 0);
    if (data == NULL){
	ERROR("");
	return NULL;
    }
    headerLen = data - dataPtr;
    *listlength -= headerLen;
    data = asn_build_objid(data, listlength,
	    (u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID),
	    var_name, *var_name_len);
    if (data == NULL){
	ERROR("");
	return NULL;
    }
    switch(var_val_type){
	case ASN_INTEGER:
	case GAUGE:
	case COUNTER:
	case TIMETICKS:
	    data = asn_build_int(data, listlength, var_val_type,
		    (long *)var_val, var_val_len);
	    break;
	case ASN_OCTET_STR:
	case IPADDRESS:
	case OPAQUE:
	    data = asn_build_string(data, listlength, var_val_type,
		    var_val, var_val_len);
	    break;
	case ASN_OBJECT_ID:
	    data = asn_build_objid(data, listlength, var_val_type,
		    (oid *)var_val, var_val_len / sizeof(oid));
	    break;
	case ASN_NULL:
	    data = asn_build_null(data, listlength, var_val_type);
	    break;
	default:
	    ERROR("wrong type");
	    return NULL;
    }
    if (data == NULL){
	ERROR("");
	return NULL;
    }
    dummyLen = (data - dataPtr) - headerLen;
    if (asn_build_header(dataPtr, &dummyLen, (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR), dummyLen) == NULL){
	ERROR("");
	return NULL;
    }
    return data;
}



int
snmp_build_trap(out_data, length, sysOid, sysOidLen, myAddr, trapType, specificType, time, varName, varNameLen, varType, varLen, varVal)
    register u_char  *out_data;
    int	    length;
    oid	    *sysOid;
    int	    sysOidLen;
    u_long  myAddr;
    int	    trapType;
    int	    specificType;
    u_long  time;
    oid	    *varName;
    int	    varNameLen;
    u_char  varType;
    int	    varLen;
    u_char  *varVal;
{
    long    version = SNMP_VERSION_1;
    int	    sidLen = strlen("public");
    int	    dummyLen;
    u_char  *out_auth, *out_header, *out_pdu, *out_varHeader, *out_varlist, *out_end;


    out_auth = out_data;
    out_header = snmp_auth_build(out_data, &length, (u_char *)"public", &sidLen, &version, 90);
    if (out_header == NULL){
	ERROR("auth build failed");
	return 0;
    }
    out_pdu = asn_build_header(out_header, &length, (u_char)TRP_REQ_MSG, 90);
    if (out_pdu == NULL){
	ERROR("header build failed");
	return 0;
    }
    out_data = asn_build_objid(out_pdu, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID),
		(oid *)sysOid, sysOidLen);
    if (out_data == NULL){
	ERROR("build enterprise failed");
	return 0;
    }
    out_data = asn_build_string(out_data, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OCTET_STR),
		(u_char *)&myAddr, sizeof(myAddr));
    if (out_data == NULL){
	ERROR("build agent_addr failed");
	return 0;
    }
    out_data = asn_build_int(out_data, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&trapType, sizeof(trapType));
    if (out_data == NULL){
	ERROR("build trap_type failed");
	return 0;
    }
    out_data = asn_build_int(out_data, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&specificType, sizeof(specificType));
    if (out_data == NULL){
	ERROR("build specificType failed");
	return 0;
    }
    out_varHeader = asn_build_int(out_data, &length,
		(u_char)(TIMETICKS),
		(long *)&time, sizeof(time));
    if (out_varHeader == NULL){
	ERROR("build timestampfailed");
	return 0;
    }
    out_varlist = asn_build_header(out_varHeader,  &length, (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR), 90);
    out_end = snmp_build_var_op(out_varlist, varName, &varNameLen, varType, varLen, varVal, &length);
    if (out_end == NULL){
	ERROR("build varop failed");
	return 0;
    }
    /* Now rebuild header with the actual lengths */
    dummyLen = out_end - out_varlist;
    if (asn_build_header(out_varHeader, &dummyLen, (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR), dummyLen) != out_varlist)
	return 0;
    dummyLen = out_end - out_pdu;
    if (asn_build_header(out_header, &dummyLen, (u_char)TRP_REQ_MSG, dummyLen) != out_pdu)
	return 0;
    dummyLen = out_end - out_header;
    if (snmp_auth_build(out_auth, &dummyLen, (u_char *)"public", &sidLen, &version, dummyLen) != out_header)
	return 0;
    return out_end - out_auth;
}





