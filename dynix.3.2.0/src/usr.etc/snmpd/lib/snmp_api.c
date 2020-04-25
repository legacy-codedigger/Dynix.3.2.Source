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

#ident	"$Header: snmp_api.c 1.1 1991/07/31 00:06:06 $"

/*
 * snmp_api.c
 *    API for access to snmp.
 */

/* $Log: snmp_api.c,v $
 *
 *
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

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#ifdef sequent
#include <sys/socket.h>
#else
#include <sys/utsname.h>
#include <tiuser.h>
#include <fcntl.h>
#include "select.h"
#endif
#include <netdb.h>
#include "asn1.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "snmp_api.h"
#include "mib.h"
#include "var.h"
#include "debug.h"

#define PACKET_LENGTH	1500

#ifndef BSD4_3
#define BSD4_2
#endif

#ifndef BSD4_3

#ifndef sequent
/* typedef long	fd_mask; */
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	memset((char *)(p), 0, sizeof(*(p)))
#endif /* notdef sequent */
#endif

oid default_enterprise[] = {1, 3, 6, 1, 4, 1, 147}; /* enterprises.sequent */
int default_enterprise_length = 7;


#define DEFAULT_COMMUNITY   "public"
#define DEFAULT_RETRIES	    4
#define DEFAULT_TIMEOUT	    1000000L
#define DEFAULT_REMPORT	    SNMP_PORT
#define DEFAULT_ENTERPRISE  default_enterprise
#define DEFAULT_TIME	    0

struct mib_snmp_struct *mib_snmp = &Mib_snmp;

/*
 * Internal information about the state of the snmp session.
 */
struct snmp_internal_session {
    int	    sd;		/* socket descriptor for this connection */
    ipaddr  addr;	/* address of connected peer */
    struct request_list *requests;/* Info about outstanding requests */
};

/*
 * A list of all the outstanding requests for a particular session.
 */
struct request_list {
    struct request_list *next_request;
    u_long  request_id;	/* request id */
    int	    retries;	/* Number of retries */
    u_long timeout;	/* length to wait for timeout */
    struct timeval time; /* Time this request was made */
    struct timeval expire;  /* time this request is due to expire */
    struct snmp_pdu *pdu;   /* The pdu for this request (saved so it can be retransmitted */
};

/*
 * The list of active/open sessions.
 */
struct session_list {
    struct session_list *next;
    struct snmp_session *session;
    struct snmp_internal_session *internal;
};

struct session_list *Sessions = NULL;

u_long Reqid = 0;
int snmp_errno = 0;
extern int t_errno;
extern int errno;
extern int debug;

char *api_errors[4] = {
    "Unknown session",
    "Unknown host",
    "Invalid local port",
    "Unknown Error"
};

static char *
api_errstring(snmp_errnumber)
    int	snmp_errnumber;
{
    if (snmp_errnumber <= SNMPERR_BAD_SESSION && snmp_errnumber >= SNMPERR_GENERR){
	return api_errors[snmp_errnumber + 4];
    } else {
	return "Unknown Error";
    }
}


/*
 * Gets initial request ID for all transactions and opens debug file.
 */

unsigned long StartTime;

static
init_snmp(){
    struct timeval tv;

    gettimeofday(&tv, (struct timezone *)0);
    StartTime = tv.tv_sec;
#ifdef sequent
    srandom(tv.tv_sec ^ tv.tv_usec);
    Reqid = random();
#else
    srand(tv.tv_sec ^ tv.tv_usec);
    Reqid = rand();
#endif
}

/*
 * Sets up the session with the snmp_session information provided
 * by the user.  Then opens and binds the necessary UDP port.
 * A handle to the created session is returned (this is different than
 * the pointer passed to snmp_open()).  On any error, NULL is returned
 * and snmp_errno is set to the appropriate error code.
 */
struct snmp_session *
snmp_open(session)
    struct snmp_session *session;
{
    struct session_list *slp;
    struct snmp_internal_session *isp;
    u_char *cp;
    int sd;
    u_long addr;
    struct sockaddr_in	me;
    struct hostent *hp;
    struct servent *servp;
#ifndef sequent
    struct t_bind bindreq;
#endif

    if (Reqid == 0)
	init_snmp();

    /* Copy session structure and link into list */
    slp = (struct session_list *)malloc(sizeof(struct session_list));
    slp->internal = isp = (struct snmp_internal_session *)malloc(sizeof(struct snmp_internal_session));
    memset((char *)isp, 0, sizeof(struct snmp_internal_session));

    slp->internal->sd = -1; /* mark it not set */
    slp->session = (struct snmp_session *)malloc(sizeof(struct snmp_session));
    memcpy( (char *)slp->session,(char *)session, sizeof(struct snmp_session));
    session = slp->session;
    /* now link it in. */
    slp->next = Sessions;
    Sessions = slp;
    /*
     * session now points to the new structure that still contains pointers to
     * data allocated elsewhere.  Some of this data is copied to space malloc'd
     * here, and the pointer replaced with the new one.
     */

    if (session->peername != NULL){
	cp = (u_char *)malloc((unsigned)strlen(session->peername) + 1);
	strcpy((char *)cp, session->peername);
	session->peername = (char *)cp;
    }

    /* Fill in defaults if necessary */
    if (session->community_len != SNMP_DEFAULT_COMMUNITY_LEN){
	cp = (u_char *)malloc((unsigned)session->community_len);
	memcpy( (char *)cp,(char *)session->community, session->community_len);
    } else {
	session->community_len = strlen(DEFAULT_COMMUNITY);
	cp = (u_char *)malloc((unsigned)session->community_len);
	memcpy( (char *)cp,(char *)DEFAULT_COMMUNITY, session->community_len);
    }
    session->community = cp;	/* replace pointer with pointer to new data */

    if (session->retries == SNMP_DEFAULT_RETRIES)
	session->retries = DEFAULT_RETRIES;
    if (session->timeout == SNMP_DEFAULT_TIMEOUT)
	session->timeout = DEFAULT_TIMEOUT;
    isp->requests = NULL;

    /* Set up connections */
#ifdef sequent
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0){
	syslog(LOG_ERR, "socket: %m");
	if (debug)
	    fprintf(dfile, "Socket error %d\n", errno);
#else
    sd = t_open(TLI_UDP, O_RDWR, (struct t_info *)NULL);
    if (sd < 0){
	syslog(LOG_ERR, "t_open: %d", t_errno);
	if (debug)
	    fprintf(dfile, "t_open error %d\n", t_errno);
#endif /* sequent */
	snmp_errno = SNMP_ERR_GENERR;
	if (!snmp_close(session)){
	    syslog(LOG_ERR, "Couldn't abort session: %s.\n", api_errstring(SNMPERR_GENERR));
	    if (debug)
		fprintf(dfile, "Couldn't abort session: %s.\n", api_errstring(SNMPERR_GENERR));
	    return 0;
	}
	return 0;
    }
    isp->sd = sd;
    if (session->peername != SNMP_DEFAULT_PEERNAME){
	if ((addr = inet_addr(session->peername)) != -1){
	    memcpy( (char *)&isp->addr.sin_addr,(char *)&addr, sizeof(isp->addr.sin_addr));
	} else {
	    hp = gethostbyname(session->peername);
	    if (hp == NULL){
		if (debug)
		    fprintf(dfile, "unknown host: %s\n", session->peername);
		snmp_errno = SNMP_ERR_GENERR;
		if (!snmp_close(session)){
		    syslog(LOG_ERR, "Couldn't abort session: %s.\n", api_errstring(SNMPERR_BAD_ADDRESS));
		    if (debug)
			fprintf(dfile, "Couldn't abort session: %s.\n", api_errstring(SNMPERR_BAD_ADDRESS));
		    return 0;
		}
		return 0;
	    } else {
		memcpy( (char *)&isp->addr.sin_addr,(char *)hp->h_addr, hp->h_length);
	    }
	}
	isp->addr.sin_family = AF_INET;
	if (session->remote_port == SNMP_DEFAULT_REMPORT){
	    servp = getservbyname("snmp", "udp");
	    if (servp != NULL){
		isp->addr.sin_port = servp->s_port;
	    } else {
		isp->addr.sin_port = htons(SNMP_PORT);
	    }
	} else {
	    isp->addr.sin_port = htons(session->remote_port);
	}
    } else {
	isp->addr.sin_addr.s_addr = SNMP_DEFAULT_ADDRESS;
    }

    me.sin_family = AF_INET;
    me.sin_addr.s_addr = INADDR_ANY;
    me.sin_port = htons(session->local_port);
#ifdef sequent
    if (bind(sd, (struct sockaddr *)&me, sizeof(me)) != 0){
	if (debug)
	    perror("bind");
#else
    bindreq.addr.buf = (caddr_t)&me;
    bindreq.addr.len = sizeof(struct sockaddr_in);
    bindreq.addr.maxlen = sizeof(struct sockaddr_in);
    if (t_bind(sd, &bindreq, &bindreq) < 0) {
	if (debug)
	    t_error("t_bind");
#endif /* sequent */

	snmp_errno = SNMP_ERR_GENERR;
	if (!snmp_close(session)){
	    syslog(LOG_ERR, "Couldn't abort session: %s.\n", api_errstring(SNMPERR_BAD_LOCPORT));
	    if (debug)
		fprintf(dfile, "Couldn't abort session: %s.\n", api_errstring(SNMPERR_BAD_LOCPORT));
	    return 0;
	}
	return 0;
    }
    return session;
}


/*
 * Free each element in the input request list.
 */
static
free_request_list(rp)
    struct request_list *rp;
{
    struct request_list *orp;

    while(rp){
	orp = rp;
	rp = rp->next_request;
	if (orp->pdu != NULL)
	    snmp_free_pdu(orp->pdu);
	free((char *)orp);
    }
}

/*
 * Close the input session.  Frees all data allocated for the session,
 * dequeues any pending requests, and closes any sockets allocated for
 * the session.  Returns 0 on error, 1 otherwise.
 */
int 
snmp_close(session)
    struct snmp_session *session;
{
    struct session_list *slp = NULL, *oslp = NULL;

    if (Sessions->session == session){	/* If first entry */
	slp = Sessions;
	Sessions = slp->next;
    } else {
	for(slp = Sessions; slp; slp = slp->next){
	    if (slp->session == session){
		if (oslp)   /* if we found entry that points here */
		    oslp->next = slp->next;	/* link around this entry */
		break;
	    }
	    oslp = slp;
	}
    }
    /* If we found the session, free all data associated with it */
    if (slp){
	if (slp->session->community != NULL)
	    free((char *)slp->session->community);
	if(slp->session->peername != NULL)
	    free((char *)slp->session->peername);
	free((char *)slp->session);
	if (slp->internal->sd != -1)
	    close(slp->internal->sd);
	free_request_list(slp->internal->requests);
	free((char *)slp->internal);
	free((char *)slp);
    } else {
/*	snmp_errno = SNMPERR_BAD_SESSION; */
	return 0;
    }
    return 1;
}

/*
 * Takes a session and a pdu and serializes the ASN PDU into the area
 * pointed to by packet.  out_length is the size of the data area available.
 * Returns the length of the completed packet in out_length.  If any errors
 * occur, -1 is returned.  If all goes well, 0 is returned.
 */
static int
snmp_build(session, pdu, packet, out_length)
    struct snmp_session	*session;
    struct snmp_pdu	*pdu;
    register u_char	*packet;
    int			*out_length;
{
    u_char  buf[PACKET_LENGTH];
    register u_char  *cp;
    struct variable_list *vp;
    int	    length;
    long    zero = 0;
    int	    totallength;

    length = *out_length;
    cp = packet;
    for(vp = pdu->variables; vp; vp = vp->next_variable){
	cp = snmp_build_var_op(cp, vp->name, &vp->name_length, vp->type, vp->val_len, (u_char *)vp->val.string, &length);
	if (cp == NULL)
	    return -1;
    }
    totallength = cp - packet;

    length = PACKET_LENGTH;
    cp = asn_build_header(buf, &length, (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR), totallength);
    if (cp == NULL)
	return -1;
    memcpy( (char *)cp,(char *)packet, totallength);
    totallength += cp - buf;

    length = *out_length;

    switch(pdu->command) {
    case GET_REQ_MSG:
	mib_snmp->snmpOutGetRequests++;
	break;
    case GETNEXT_REQ_MSG:
	mib_snmp->snmpOutGetNexts++;
	break;
    case GET_RSP_MSG:
	mib_snmp->snmpOutGetResponses++;
	break;
    case SET_REQ_MSG:
	mib_snmp->snmpOutSetRequests++;
	break;
    case TRP_REQ_MSG:
	mib_snmp->snmpOutTraps++;
	break;
    default:
	break;
    };

    if (pdu->command != TRP_REQ_MSG){
	/* request id */
	cp = asn_build_int(packet, &length,
	    (u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
	    (long *)&pdu->reqid, sizeof(pdu->reqid));
	if (cp == NULL)
	    return -1;
	/* error status */
	switch(pdu->errstat){
	case SNMP_ERR_NOERROR:
	    break;
	case SNMP_ERR_TOOBIG:
	    mib_snmp->snmpOutTooBigs++;
	    break;
	case SNMP_ERR_NOSUCHNAME:
	    mib_snmp->snmpOutNoSuchNames++;
	    break;
	case SNMP_ERR_BADVALUE:
	    mib_snmp->snmpOutBadValues++;
	    break;
	case SNMP_ERR_READONLY:
	    mib_snmp->snmpOutReadOnlys++;
	    break;
	case SNMP_ERR_GENERR:
	    mib_snmp->snmpOutGenErrs++;
	    break;
	default:
	    break;
	};

	cp = asn_build_int(cp, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&pdu->errstat, sizeof(pdu->errstat));
	if (cp == NULL)
	    return -1;
	/* error index */
	cp = asn_build_int(cp, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&pdu->errindex, sizeof(pdu->errindex));
	if (cp == NULL)
	    return -1;
    } else {	/* this is a trap message */
	/* enterprise */
	cp = asn_build_objid(packet, &length,
	    (u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OBJECT_ID),
	    (oid *)pdu->enterprise, pdu->enterprise_length);
	if (cp == NULL)
	    return -1;
	/* agent-addr */
	cp = asn_build_string(cp, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_OCTET_STR),
		(u_char *)&pdu->agent_addr.sin_addr.s_addr, sizeof(pdu->agent_addr.sin_addr.s_addr));
	if (cp == NULL)
	    return -1;
	/* generic trap */
	cp = asn_build_int(cp, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&pdu->trap_type, sizeof(pdu->trap_type));
	if (cp == NULL)
	    return -1;
	/* specific trap */
	cp = asn_build_int(cp, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&pdu->specific_type, sizeof(pdu->specific_type));
	if (cp == NULL)
	    return -1;
	/* timestamp  */
	cp = asn_build_int(cp, &length,
		(u_char)(ASN_UNIVERSAL | ASN_PRIMITIVE | ASN_INTEGER),
		(long *)&pdu->time, sizeof(pdu->time));
	if (cp == NULL)
	    return -1;
    }
    if (length < totallength)
	return -1;
    memcpy( (char *)cp,(char *)buf, totallength);
    totallength += cp - packet;

    length = PACKET_LENGTH;
    cp = asn_build_header(buf, &length, (u_char)pdu->command, totallength);
    if (cp == NULL)
	return -1;
    if (length < totallength)
	return -1;
    memcpy( (char *)cp,(char *)packet, totallength);
    totallength += cp - buf;

    length = *out_length;
    cp = snmp_auth_build(packet, &length, session->community, &session->community_len, &zero, totallength);
    if (cp == NULL)
	return -1;
    if ((*out_length - (cp - packet)) < totallength)
	return -1;
    memcpy( (char *)cp,(char *)buf, totallength);
    totallength += cp - packet;
    *out_length = totallength;
    return 0;
}

/*
 * Parses the packet recieved on the input session, and places the data into
 * the input pdu.  length is the length of the input packet.  If any errors
 * are encountered, -1 is returned.  Otherwise, a 0 is returned.
 */
static int
snmp_parse(session, pdu, data, length)
    struct snmp_session *session;
    struct snmp_pdu *pdu;
    u_char  *data;
    int	    length;
{
    u_char  msg_type;
    u_char  type;
    u_char  *var_val;
    long    version;
    int	    len, four;
    u_char community[128];
    int community_length = 128;
    struct variable_list *vp;
    oid	    objid[MAX_NAME_LEN], *op;

    /* authenticates message and returns length if valid */
    data = snmp_auth_parse(data, &length, community, &community_length, &version);
    if (data == NULL)
	return -1;
    if (version != SNMP_VERSION_1){
	mib_snmp->snmpInBadVersions++;
	if (debug) {
	    fprintf(dfile, "Wrong version: %d\n", version);
	    fprintf(dfile, "Continuing anyway\n");
	}
    }

    data = asn_parse_header(data, &length, &msg_type);
    if (data == NULL) {
	mib_snmp->snmpInASNParseErrs++;
	return -1;
    }

    if (session->authenticator){
	data = session->authenticator(data, &length, community, community_length, msg_type, pdu->address);
	if (data == NULL)
	    return 0;
    }

    /* fill in the community that sent the message into the session struct */
    if (session->community != NULL)
	free(session->community);
    session->community = (u_char *)malloc((unsigned)community_length);
    memcpy( (char *)session->community,(char *)community, community_length);
    session->community_len = community_length;
    
    pdu->command = msg_type;
    switch(pdu->command) {
    case GET_REQ_MSG:
	mib_snmp->snmpInGetRequests++;
	break;
    case GETNEXT_REQ_MSG:
	mib_snmp->snmpInGetNexts++;
	break;
    case GET_RSP_MSG:
	mib_snmp->snmpInGetResponses++;
	break;
    case SET_REQ_MSG:
	mib_snmp->snmpInSetRequests++;
	break;
    case TRP_REQ_MSG:
	mib_snmp->snmpInTraps++;
	break;
    default:
	mib_snmp->snmpInBadTypes++;
	break;
    };

    if (pdu->command != TRP_REQ_MSG){
	data = asn_parse_int(data, &length, &type, (long *)&pdu->reqid, sizeof(pdu->reqid));
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
	data = asn_parse_int(data, &length, &type, (long *)&pdu->errstat, sizeof(pdu->errstat));
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	} else {
	    switch(pdu->errstat){
	    case SNMP_ERR_NOERROR:
		break;
	    case SNMP_ERR_TOOBIG:
		mib_snmp->snmpInTooBigs++;
		break;
	    case SNMP_ERR_NOSUCHNAME:
		mib_snmp->snmpInNoSuchNames++;
		break;
	    case SNMP_ERR_BADVALUE:
		mib_snmp->snmpInBadValues++;
		break;
	    case SNMP_ERR_READONLY:
		mib_snmp->snmpInReadOnlys++;
		break;
	    case SNMP_ERR_GENERR:
		mib_snmp->snmpInGenErrs++;
		break;
	    default:
		break;
	    };
	}
	
	data = asn_parse_int(data, &length, &type, (long *)&pdu->errindex, sizeof(pdu->errindex));
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
    } else {
	pdu->enterprise_length = MAX_NAME_LEN;
	data = asn_parse_objid(data, &length, &type, objid, &pdu->enterprise_length);
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
	pdu->enterprise = (oid *)malloc(pdu->enterprise_length * sizeof(oid));
	memcpy( (char *)pdu->enterprise,(char *)objid, pdu->enterprise_length * sizeof(oid));

	four = 4;
	data = asn_parse_string(data, &length, &type, (u_char *)&pdu->agent_addr.sin_addr.s_addr, &four);
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
	data = asn_parse_int(data, &length, &type, (long *)&pdu->trap_type, sizeof(pdu->trap_type));
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
	data = asn_parse_int(data, &length, &type, (long *)&pdu->specific_type, sizeof(pdu->specific_type));
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
	data = asn_parse_int(data, &length, &type, (long *)&pdu->time, sizeof(pdu->time));
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
    }

    data = asn_parse_header(data, &length, &type);
    if (data == NULL) {
	mib_snmp->snmpInASNParseErrs++;
	return -1;
    }
    if (type != (u_char)(ASN_SEQUENCE | ASN_CONSTRUCTOR)) {
	mib_snmp->snmpInASNParseErrs++;
	return -1;
    }
    while((int)length > 0){
	if (pdu->variables == NULL){
	    pdu->variables = vp = (struct variable_list *)malloc(sizeof(struct variable_list));
	} else {
	    vp->next_variable = (struct variable_list *)malloc(sizeof(struct variable_list));
	    vp = vp->next_variable;
	}
	vp->next_variable = NULL;
	vp->val.string = NULL;
	vp->name = NULL;
	vp->name_length = MAX_NAME_LEN;
	data = snmp_parse_var_op(data, objid, &vp->name_length, &vp->type, &vp->val_len, &var_val, (int *)&length);
	if (data == NULL) {
	    mib_snmp->snmpInASNParseErrs++;
	    return -1;
	}
	/* op = (oid *)malloc((unsigned)vp->name_length * sizeof(oid)); */
	op = (oid *)malloc(MAX_OID_LEN * sizeof(oid));
	memcpy( (char *)op,(char *)objid, vp->name_length * sizeof(oid));
	vp->name = op;

	len = PACKET_LENGTH;
	switch((short)vp->type){
	    case ASN_INTEGER:
	    case COUNTER:
	    case GAUGE:
	    case TIMETICKS:
		vp->val.integer = (long *)malloc(sizeof(long));
		vp->val_len = sizeof(long);
		asn_parse_int(var_val, &len, &vp->type, (long *)vp->val.integer, sizeof(vp->val.integer));
		break;
	    case ASN_OCTET_STR:
	    case IPADDRESS:
	    case OPAQUE:
		vp->val.string = (u_char *)malloc((unsigned)vp->val_len);
		asn_parse_string(var_val, &len, &vp->type, vp->val.string, &vp->val_len);
		break;
	    case ASN_OBJECT_ID:
		vp->val_len = MAX_NAME_LEN;
		asn_parse_objid(var_val, &len, &vp->type, objid, &vp->val_len);
		vp->val_len *= sizeof(oid);
		vp->val.objid = (oid *)malloc((unsigned)vp->val_len);
		memcpy( (char *)vp->val.objid,(char *)objid, vp->val_len);
		break;
	    case ASN_NULL:
		break;
	    default:
		if (debug)
		    fprintf(dfile, "bad type returned (%x)\n", vp->type);
		break;
	}
    }
    return 0;
}

/*
 * Sends the input pdu on the session after calling snmp build to create
 * a serialized packet.  If necessary, set some of the pdu data from the
 * session defaults.  Add a request corresponding to this pdu to the list
 * of outstanding requests on this session, then send the pdu.
 * Returns the request id of the generated packet if applicable, otherwise 1.
 * On any error, 0 is returned.
 * The pdu is freed by snmp_send() unless a failure occured.
 */
int
snmp_send(session, pdu)
    struct snmp_session *session;
    struct snmp_pdu	*pdu;
{
    struct session_list *slp;
    struct snmp_internal_session *isp = NULL;
    u_char  packet[PACKET_LENGTH];
    int length = PACKET_LENGTH;
    struct request_list *rp;
    struct timeval tv;
#ifndef sequent
    struct t_unitdata sndbuf;
#endif

    for(slp = Sessions; slp; slp = slp->next){
	if (slp->session == session){
	    isp = slp->internal;
	    break;
	}
    }
    if (isp == NULL){
/*	snmp_errno = SNMPERR_BAD_SESSION; */
	return 0;
    }
    if (pdu->command == GET_REQ_MSG || 
	pdu->command == GETNEXT_REQ_MSG || 
/*	pdu->command == GET_RSP_MSG || */
	pdu->command == SET_REQ_MSG){
	if (pdu->reqid == SNMP_DEFAULT_REQID)
	    pdu->reqid = ++Reqid;
	if (pdu->errstat == SNMP_DEFAULT_ERRSTAT)
	    pdu->errstat = 0;
	if (pdu->errindex == SNMP_DEFAULT_ERRINDEX)
	    pdu->errindex = 0;
    } else if (pdu->command == TRP_REQ_MSG ) {
	/* fill in trap defaults */
	pdu->reqid = 1;	/* give a bogus non-error reqid for traps */
	if (pdu->enterprise_length == SNMP_DEFAULT_ENTERPRISE_LENGTH){
	    pdu->enterprise = (oid *)malloc(sizeof(DEFAULT_ENTERPRISE));
	    memcpy( (char *)pdu->enterprise,(char *)DEFAULT_ENTERPRISE, sizeof(DEFAULT_ENTERPRISE));
	    pdu->enterprise_length = sizeof(DEFAULT_ENTERPRISE)/sizeof(oid);
	}
	if (pdu->time == SNMP_DEFAULT_TIME)
	    pdu->time = DEFAULT_TIME;
    }
    if (pdu->address.sin_addr.s_addr == SNMP_DEFAULT_ADDRESS){
	if (isp->addr.sin_addr.s_addr != SNMP_DEFAULT_ADDRESS){
	    memcpy( (char *)&pdu->address,(char *)&isp->addr, sizeof(pdu->address));
	} else {
	    if (debug)
		fprintf(dfile, "No remote IP address specified\n");
/*	    snmp_errno = SNMPERR_BAD_ADDRESS; */
	    return 0;
	}
    }
	

    if (snmp_build(session, pdu, packet, &length) < 0){
	if (debug > 5)
	    fprintf(dfile, "Error building packet\n");
	if (length >= PACKET_LENGTH)
	    snmp_errno = SNMP_ERR_TOOBIG;
	else
	    snmp_errno = SNMP_ERR_GENERR;
	return 0;
    }
    if (snmp_dump_packet){
	int count;

	for(count = 0; count < length; count++){
	    fprintf(dfile, "%02X ", packet[count]);
	    if ((count % 16) == 15)
		fprintf(dfile, "\n");
	}
	fprintf(dfile, "\n\n");
    }

    mib_snmp->snmpOutPkts++;
    gettimeofday(&tv, (struct timezone *)0);
#ifdef sequent
    if (sendto(isp->sd, (char *)packet, length, 0, (struct sockaddr *)&pdu->address, sizeof(pdu->address)) < 0){
	if (debug)
	    perror("sendto");
#else
	sndbuf.addr.buf = (char *)&pdu->address;
	sndbuf.addr.len = sizeof(pdu->address);
	sndbuf.opt.len = 0;
	sndbuf.udata.buf = (char *)packet;
	sndbuf.udata.len = length;

    if (t_sndudata(isp->sd, &sndbuf) < 0){
	if (debug)
	    t_error("t_sndudata");
#endif
	snmp_errno = SNMP_ERR_GENERR;
	return 0;
    }
    if (pdu->command == GET_REQ_MSG || pdu->command == GETNEXT_REQ_MSG || pdu->command == SET_REQ_MSG){
	/* set up to expect a response */
	rp = (struct request_list *)malloc(sizeof(struct request_list));
	rp->next_request = isp->requests;
	isp->requests = rp;
	rp->pdu = pdu;
	rp->request_id = pdu->reqid;

	rp->retries = 1;
	rp->timeout = session->timeout;
	rp->time = tv;
	tv.tv_usec += rp->timeout;
	tv.tv_sec += tv.tv_usec / 1000000L;
	tv.tv_usec %= 1000000L;
	rp->expire = tv;
    }
    return pdu->reqid;
}

/*
 * Frees the pdu and any malloc'd data associated with it.
 */
void
snmp_free_pdu(pdu)
    struct snmp_pdu *pdu;
{
    struct variable_list *vp, *ovp;

    vp = pdu->variables;
    while(vp){
	if (vp->name)
	    free((char *)vp->name);
	if (vp->val.string)
	    free((char *)vp->val.string);
	ovp = vp;
	vp = vp->next_variable;
	free((char *)ovp);
    }
    if (pdu->enterprise)
	free((char *)pdu->enterprise);
    free((char *)pdu);
}


/*
 * Checks to see if any of the fd's set in the fdset belong to
 * snmp.  Each socket with it's fd set has a packet read from it
 * and snmp_parse is called on the packet received.  The resulting pdu
 * is passed to the callback routine for that session.  If the callback
 * routine returns successfully, the pdu and it's request are deleted.
 */
void
snmp_read(fdset)
    fd_set  *fdset;
{
    struct session_list *slp;
    struct snmp_session *sp;
    struct snmp_internal_session *isp;
    u_char packet[PACKET_LENGTH];
    struct sockaddr_in	from;
    int length, fromlength;
    struct snmp_pdu *pdu;
    struct request_list *rp, *orp;
#ifndef sequent
    struct t_unitdata recvbuf;
    int flags = 0;
#endif /* sequent */

    for(slp = Sessions; slp; slp = slp->next){
	if (FD_ISSET(slp->internal->sd, fdset)){
	    sp = slp->session;
	    isp = slp->internal;
	    fromlength = sizeof from;
#ifdef sequent
	    length = recvfrom(isp->sd, (char *)packet, PACKET_LENGTH, 0, (struct sockaddr *)&from, &fromlength);
	    if (length == -1)
		if (debug)
		    perror("recvfrom");
#else
	    recvbuf.addr.buf = (char *)&from;
	    recvbuf.addr.maxlen = fromlength;
	    recvbuf.opt.maxlen = 0;
	    recvbuf.udata.buf = (char *)packet;
	    recvbuf.udata.maxlen = PACKET_LENGTH;
	    length = t_rcvudata(isp->sd, &recvbuf, &flags);
	    if (length == -1) {
		if (t_errno == TLOOK){
		    t_rcvuderr(isp->sd, NULL);
		}
		if (debug)
		    t_error("t_rcvudata");
		continue;
	    } else 
		length = recvbuf.udata.len;
#endif /* sequent */

	    if (snmp_dump_packet){
		int count;

		fprintf(dfile, "recieved %d bytes from %s:\n", length, inet_ntoa(from.sin_addr));
		for(count = 0; count < length; count++){
		    fprintf(dfile, "%02X ", packet[count]);
		    if ((count % 16) == 15)
			fprintf(dfile, "\n");
		}
		fprintf(dfile, "\n\n");
	    }
	    
	    mib_snmp->snmpInPkts++;
	    pdu = (struct snmp_pdu *)malloc(sizeof(struct snmp_pdu));
	    pdu->address = from;
	    pdu->reqid = 0;
	    pdu->variables = NULL;
	    pdu->enterprise = NULL;
	    pdu->enterprise_length = 0;
	    if (snmp_parse(sp, pdu, packet, length) != SNMP_ERR_NOERROR){
		syslog(LOG_WARNING, "Mangled packet");
		if (debug)
		    fprintf(dfile, "Mangled packet\n");
		snmp_free_pdu(pdu);
		return;
	    }

	    if (pdu->command == GET_RSP_MSG){
		for(rp = isp->requests; rp; rp = rp->next_request){
		    if (rp->request_id == pdu->reqid){
			if (((*sp->callback)(RECEIVED_MESSAGE, sp, pdu->reqid, pdu, sp->callback_magic)) == 1){
			    /* successful, so delete request */
			    orp = rp;
			    if (isp->requests == orp){
				/* first in list */
				isp->requests = orp->next_request;
			    } else {
				for(rp = isp->requests; rp; rp = rp->next_request){
				    if (rp->next_request == orp){
					rp->next_request = orp->next_request;	/* link around it */
					break;
				    }
				}
			    }
			    snmp_free_pdu(orp->pdu);
			    free((char *)orp);
			    break;  /* there shouldn't be any more request with the same reqid */
			}
		    }
		}
	    } else if (pdu->command == GET_REQ_MSG || pdu->command == GETNEXT_REQ_MSG
		    || pdu->command == TRP_REQ_MSG || pdu->command == SET_REQ_MSG){
		(*sp->callback)(RECEIVED_MESSAGE, sp, pdu->reqid, pdu, sp->callback_magic);
	    }
	    snmp_free_pdu(pdu);
	}
    }
}

/*
 * Returns info about what snmp requires from a select statement.
 * numfds is the number of fds in the list that are significant.
 * All file descriptors opened for SNMP are OR'd into the fdset.
 * If activity occurs on any of these file descriptors, snmp_read
 * should be called with that file descriptor set
 *
 * The timeout is the latest time that SNMP can wait for a timeout.  The
 * select should be done with the minimum time between timeout and any other
 * timeouts necessary.  This should be checked upon each invocation of select.
 * If a timeout is received, snmp_timeout should be called to check if the
 * timeout was for SNMP.  (snmp_timeout is idempotent)
 *
 * Block is 1 if the select is requested to block indefinitely, rather than time out.
 * If block is input as 1, the timeout value will be treated as undefined, but it must
 * be available for setting in snmp_select_info.  On return, if block is true, the value
 * of timeout will be undefined.
 *
 * snmp_select_info returns the number of open sockets.  (i.e. The number of sessions open)
 */
int
snmp_select_info(numfds, fdset, timeout, block)
    int	    *numfds;
    fd_set  *fdset;
    struct timeval *timeout;
    int	    *block; /* should the select block until input arrives (i.e. no input) */
{
    struct session_list *slp;
    struct snmp_internal_session *isp;
    struct request_list *rp;
    struct timeval now, earliest;
    int active = 0, requests = 0;

    timerclear(&earliest);
    /*
     * For each request outstanding, add it's socket to the fdset,
     * and if it is the earliest timeout to expire, mark it as lowest.
     */
    for(slp = Sessions; slp; slp = slp->next){
	active++;
	isp = slp->internal;
	if ((isp->sd + 1) > *numfds)
	    *numfds = (isp->sd + 1);
	FD_SET(isp->sd, fdset);
	if (isp->requests){
	    /* found another session with outstanding requests */
	    requests++;
	    for(rp = isp->requests; rp; rp = rp->next_request){
		if (!timerisset(&earliest) || timercmp(&rp->expire, &earliest, <))
		    earliest = rp->expire;
	    }
	}
    }
    if (requests == 0)	/* if none are active, skip arithmetic */
	return active;

    /*
     * Now find out how much time until the earliest timeout.  This
     * transforms earliest from an absolute time into a delta time, the
     * time left until the select should timeout.
     */
    gettimeofday(&now, (struct timezone *)0);
    earliest.tv_sec--;	/* adjust time to make arithmetic easier */
    earliest.tv_usec += 1000000L;
    earliest.tv_sec -= now.tv_sec;
    earliest.tv_usec -= now.tv_usec;
    while (earliest.tv_usec >= 1000000L){
	earliest.tv_usec -= 1000000L;
	earliest.tv_sec += 1;
    }
    if (earliest.tv_sec < 0){
	earliest.tv_sec = 0;
	earliest.tv_usec = 0;
    }

    /* if it was blocking before or our delta time is less, reset timeout */
    if (*block == 1 || timercmp(&earliest, timeout, <)){
	*timeout = earliest;
	*block = 0;
    }
    return active;
}

/*
 * snmp_timeout should be called whenever the timeout from snmp_select_info expires,
 * but it is idempotent, so snmp_timeout can be polled (probably a cpu expensive
 * proposition).  snmp_timeout checks to see if any of the sessions have an
 * outstanding request that has timed out.  If it finds one (or more), and that
 * pdu has more retries available, a new packet is formed from the pdu and is
 * resent.  If there are no more retries available, the callback for the session
 * is used to alert the user of the timeout.
 */
void
snmp_timeout(){
    struct session_list *slp;
    struct snmp_session *sp;
    struct snmp_internal_session *isp;
    struct request_list *rp, *orp, *freeme = NULL;
    struct timeval now;
#ifndef sequent
    struct t_unitdata sndbuf;
#endif /* sequent */


    gettimeofday(&now, (struct timezone *)0);
    /*
     * For each request outstanding, check to see if it has expired.
     */
    for(slp = Sessions; slp; slp = slp->next){
	sp = slp->session;
	isp = slp->internal;
	orp = NULL;
	for(rp = isp->requests; rp; rp = rp->next_request){
	    if (freeme != NULL){    /* frees rp's after the for loop goes on to the next_request */
		free((char *)freeme);
		freeme = NULL;
	    }
	    if (timercmp(&rp->expire, &now, <)){
		/* this timer has expired */
		if (rp->retries >= sp->retries){
		    /* No more chances, delete this entry */
		    (*sp->callback)(TIMED_OUT, sp, rp->pdu->reqid, rp->pdu, (void *)sp->callback_magic);
		    if (orp == NULL){
			isp->requests = rp->next_request;
		    } else {
			orp->next_request = rp->next_request;
		    }
		    snmp_free_pdu(rp->pdu);
		    freeme = rp;
		    continue;	/* don't update orp below */
		} else {
		    u_char  packet[PACKET_LENGTH];
		    int length = PACKET_LENGTH;
		    struct timeval tv;

		    /* retransmit this pdu */
		    rp->retries++;
		    rp->timeout <<= 1;
		    if (snmp_build(sp, rp->pdu, packet, &length) < 0){
			syslog(LOG_WARNING, "Error building packet");
			if (debug)
			    fprintf(dfile, "Error building packet\n");
		    }
		    if (snmp_dump_packet){
			int count;

			for(count = 0; count < length; count++){
			    fprintf(dfile, "%02X ", packet[count]);
			    if ((count % 16) == 15)
				fprintf(dfile, "\n");
			}
			fprintf(dfile, "\n\n");
		    }
		    gettimeofday(&tv, (struct timezone *)0);

#ifdef sequent
		    if (sendto(isp->sd, (char *)packet, length, 0, (struct sockaddr *)&rp->pdu->address, sizeof(rp->pdu->address)) < 0){
			if (debug)
			    perror("sendto");
		    }
#else
		    sndbuf.addr.buf = (char *)&rp->pdu->address;
		    sndbuf.addr.len = sizeof(rp->pdu->address);
		    sndbuf.opt.len = 0;
		    sndbuf.udata.buf = (char *)packet;
		    sndbuf.udata.len = length;

		    if (t_sndudata(isp->sd, &sndbuf) < 0){
			if (debug)
			    t_error("t_sndudata");
		    }
#endif

		    rp->time = tv;
		    tv.tv_usec += rp->timeout;
		    tv.tv_sec += tv.tv_usec / 1000000L;
		    tv.tv_usec %= 1000000L;
		    rp->expire = tv;
		}
	    }
	    orp = rp;
	}
	if (freeme != NULL){
	    free((char *)freeme);
	    freeme = NULL;
	}
    }
}

extern struct traps *Trap_List;

snmp_send_trap(trap_type, specific_trap_type)
    int trap_type;
    int specific_trap_type;
{
    struct traps *trap_list;
    struct snmp_pdu *pdu;
    struct timeval tv;
    char name[MAXHOSTNAMELEN];
    int namelen = MAXHOSTNAMELEN;
    struct hostent *hp;
    
    for(trap_list = Trap_List; trap_list; trap_list = trap_list->next){
	pdu = (struct snmp_pdu *)malloc(sizeof(struct snmp_pdu));
	memset((char *)pdu, 0, sizeof(struct snmp_pdu));
	pdu->address.sin_addr.s_addr = SNMP_DEFAULT_ADDRESS;
	pdu->command = TRP_REQ_MSG;
	pdu->enterprise_length = SNMP_DEFAULT_ENTERPRISE_LENGTH;

	if (gethostname(name, namelen) < 0) {
	    pdu->agent_addr.sin_addr.s_addr = 0;
	} else {
	    hp = gethostbyname(name);
	    if (hp == NULL)
		pdu->agent_addr.sin_addr.s_addr = 0;
	    else
		memcpy((char *)&pdu->agent_addr.sin_addr, (char *)hp->h_addr, sizeof(struct in_addr));
	}
	pdu->trap_type = trap_type;
	pdu->specific_type = specific_trap_type;
	
	gettimeofday(&tv, (struct timezone *)0);
	pdu->time = (tv.tv_sec - StartTime) * 100;
	
	snmp_send(trap_list->session, pdu);

	snmp_free_pdu(pdu);
    }
}

struct snmp_pdu *
snmp_dup_pdu(pdu)
    struct snmp_pdu *pdu;
{

    struct variable_list *var, *newvar;
    struct snmp_pdu *newpdu;
    
    /* clone the pdu */
    newpdu = (struct snmp_pdu *)malloc(sizeof(struct snmp_pdu));
    memcpy( (char *)newpdu,(char *)pdu, sizeof(struct snmp_pdu));
    newpdu->variables = 0;
    newpdu->command = pdu->command;
    newpdu->reqid = pdu->reqid;
    newpdu->errstat = pdu->errstat;
    newpdu->errindex = pdu->errindex;
    var = pdu->variables;
    
    /* clone the variable list */
    if (var != NULL){
        newpdu->variables = newvar = (struct variable_list *)malloc(sizeof(struct variable_list));
        memcpy( (char *)newvar,(char *)var, sizeof(struct variable_list));
        if (var->name != NULL){
            newvar->name = (oid *)malloc(var->name_length * sizeof(oid));
            memcpy( (char *)newvar->name,(char *)var->name, var->name_length * sizeof(oid));
        }
        if (var->val.string != NULL){
            newvar->val.string = (u_char *)malloc(var->val_len);
            memcpy( (char *)newvar->val.string,(char *)var->val.string, var->val_len);
        }
        newvar->next_variable = 0;

        while(var->next_variable){
            var = var->next_variable;
            newvar->next_variable = (struct variable_list *)malloc(sizeof(struct variable_list));
            newvar = newvar->next_variable;
            memcpy( (char *)newvar,(char *)var, sizeof(struct variable_list));
            if (var->name != NULL){
                newvar->name = (oid *)malloc(var->name_length * sizeof(oid));
                memcpy( (char *)newvar->name,(char *)var->name, var->name_length * sizeof(oid));
            }
            if (var->val.string != NULL){
                newvar->val.string = (u_char *)malloc(var->val_len);
                memcpy( (char *)newvar->val.string,(char *)var->val.string, var->val_len);
            }
            newvar->next_variable = 0;
        }
    }
    return newpdu;
}



