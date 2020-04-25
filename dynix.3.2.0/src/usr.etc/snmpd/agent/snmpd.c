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

#ident	"$Header: snmpd.c 1.1 1991/07/31 00:11:26 $"

/*
/*
 * snmpd.c 
 *   - receive and process snmp requests
 *
 */

/* $Log: snmpd.c,v $
 *
 *
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/file.h>

#include "../lib/asn1.h"
#include "../lib/snmp.h"
#include "../lib/snmp_impl.h"
#include "../lib/snmp_api.h"
#ifndef sequent
#include "../lib/snmp/select.h"
#endif

#ifndef BSD4_3

typedef long	fd_mask;

#ifdef sequent
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#else
#define FD_ZERO(p)	memset((char *)(p), 0, sizeof(*(p)))
#endif /* sequent */
#endif

extern int  errno;
int snmp_dump_packet = 0;
FILE *dfile;
#define DEBUG_FILE "/usr/tmp/snmpd.out"
char *debugfile = DEBUG_FILE;
int Print = 1;
int debug = 0;

/*
 * The following routine is invoked each time a message is received 
 * and snmp_read is called. The function is specified in the session
 * structure passed to snmp_open. The pdu is freed by the calling routine
 * upon return.
 */
int snmpInput(op, session, reqid, pdu, magic)
    int op;
    struct snmp_session *session;
    int reqid;
    struct snmp_pdu *pdu;
    void *magic;
{
    int index;
    struct snmp_pdu *duppdu;
    
    if (op == RECEIVED_MESSAGE){
	if (debug > 2) {
	    printPdu(pdu);
	    fprintf(dfile, "Community: %s\n", session->community);
	}
	/*
	 * Parse the variable list of the pdu. Upon return the variable list will
	 * contain values for the variables requested and an error status will be 
	 * returned. 
	 */
	duppdu = snmp_dup_pdu(pdu);
	if ((pdu->errstat = parseVariableList(pdu, &index, session)) != 0) {
	    duppdu->errindex = index;
	    duppdu->errstat = pdu->errstat;
	    /* don't worry about pdu, it is freed upon return */
	    pdu = duppdu;
	} else
	    pdu->errindex = 0;

	pdu->command = GET_RSP_MSG;  	/* set msnmp msg type tp GET_RESPONSE */
	if (debug > 2) {
	    printPdu(pdu);
	    fprintf(dfile, "Community: %s\n", session->community);
	}
	/* 
	 * Send a response. The snmp_send routine parses the var list and formats
	 * the SNMP message.
	 */
	if (snmp_send(session, pdu) != reqid) {
	    if (debug)
		fprintf(dfile, "snmp_send failure\n");
	    /* if send failed try it again with the original packet 
	     * snmp_errno is set by snmp_send
	     */
	    duppdu->errindex = pdu->errindex;
	    duppdu->errstat = snmp_errno;
	    duppdu->command = GET_RSP_MSG;  	/* set msnmp msg type tp GET_RESPONSE */
	    snmp_send(session, duppdu); 	/* send orig msg with errstat */
	}
    } else if (op == TIMED_OUT) {
	if (debug)
	    fprintf(dfile, "Timeout: This shouldn't happen!\n");
    }
    snmp_free_pdu(duppdu);
}

/*
 * The following routine is invoked each time a message is received 
 * and snmp_read is called. The function is specified in the session
 * structure passed to snmp_open.
 *
 * Returns the authenticated pdu, or NULL if authentication failed.
 * If null authentication is used, the authenticator in snmp_session can be
 * set to NULL(0).
 */

u_char * 
snmpAuth(data, length, community, community_len, msg_type, address)
    u_char *data;	/* The rest of the unparsed PDU to be authenticated */
    int *length;	/* The length of the PDU (updated by the authenticator) */
    u_char *community;	/* The community name to authenticate under. */
    int community_len;	/* The length of the community name. */
    ipaddr address;	/* The ip address of the requesting staion */
{
if (debug > 5)
    fprintf(dfile, "message from address %lx\n", address.sin_addr.s_addr);

if (authenticateCommunity( community, community_len, msg_type, address.sin_addr.s_addr) < 0)
    return(0);
else
    return(data);
}

int done = 0;

main(argc, argv)
    int	    argc;
    char    **argv;
{
    struct snmp_session session, *ss;
    int	arg;
    int count, numfds, block;
    fd_set fdset;
    struct timeval timeout, *tvp;
    int i;
    int f;
    char *ConfigFile = "/usr/etc/snmpd.conf";
    extern void setIncrDbgFlg(), setNoDbgFlg(), setQuitFlg();
    extern char *optarg;
    extern int optind, opterr;

    setdtablesize(64); /* up # fds allowed open (note: this is based */
                           /* on the base NOFILES value */

    openlog("snmpd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);
    /*
     * usage: snmpagentd [-p]
     */
    while ((f = getopt(argc, argv, "pd:f:")) != EOF) {
        switch(f) {
	case 'd':
	    debug = atoi(optarg);
	    snmp_dump_packet++;
	    break;
	case 'p':
	    Print++;
	    break;
	case 'f':
	    ConfigFile = optarg;
	    break;
	default:
	    /*	    printf("invalid option: -%c\n", argv[arg][1]);*/
	    printf("Usage: snmpagentd [-p] [-f config file] [-d debug level]\n");
	    break;
	}
    }

    if (getuid()) {
	fprintf(stderr, "snmpd: not super user\n");
	exit(1);
    }

#if defined(SIGUSR1) && defined(SIGUSR2)
    (void) signal(SIGUSR1, setIncrDbgFlg);
    (void) signal(SIGUSR2, setNoDbgFlg);
#else	/* SIGUSR1&&SIGUSR2 */
    (void) signal(SIGEMT, setIncrDbgFlg);
    (void) signal(SIGFPE, setNoDbgFlg);
#endif /* SIGUSR1&&SIGUSR2 */
    (void) signal(SIGQUIT, setQuitFlg);
    setdebug(debug);

    /* Read the mib file and build a tree structure */
    init_mib();
    /* Read snmpd.conf and build community list */
    if (snmp_config(ConfigFile) < 0)
	printf("ERROR: doing config\n");
    
    if (!debug) {
	if (fork())
	    exit(0);
	{ int s;
	  for (s = 0; s < 3; s++)
	      (void) close(s);
	  (void) open("/", 0);
	  (void) dup2(0, 1);
	  (void) dup2(0, 2);
	  (void) setpgrp(0);
      }
    }
    
#ifdef sequent
    bzero((char *)&session, sizeof(struct snmp_session));
#else
    memset((char *)&session, 0, sizeof(struct snmp_session));
#endif /* sequent */    
    session.peername = NULL;
    session.community = NULL;
    session.community_len = 0;
    session.retries = SNMP_DEFAULT_RETRIES;
    session.timeout = SNMP_DEFAULT_TIMEOUT;
    session.authenticator = snmpAuth;
    session.callback = snmpInput;
    session.callback_magic = NULL;
    session.local_port = SNMP_PORT;
    ss = snmp_open(&session);
    if (ss == NULL){
	printf("Couldn't open snmp\n");
	exit(-1);
    }

    if (debug)
	fprintf(dfile, "Sending Coldstart trap\n");

    snmp_send_trap(SNMP_TRAP_COLDSTART, 0);

    if (debug)
	fprintf(dfile, "Sent Coldstart trap\n");

    while(!done){
	numfds = 0;
	FD_ZERO(&fdset);
	block = 1;
	tvp = &timeout;
	timerclear(tvp);
	snmp_select_info(&numfds, &fdset, tvp, &block);
	if (block == 1)
	    tvp = NULL;	/* block without timeout */
	count = select(numfds, &fdset, 0, 0, tvp);
	if (count > 0){
		snmp_read(&fdset);
	} else switch(count){
	    case 0:
		snmp_timeout();
		break;
	    case -1:
		if (errno == EINTR){
		    continue;
		} else {
		     syslog(LOG_ERR, "Select error");
		}
		break;
	    default:
		syslog(LOG_ERR, "Select error");
		break;
	     }
    }
if (debug)
    fprintf(dfile, "Exiting upon request\n");
}


/*
 * Debug routine to print a PDU
 */
printPdu(pdu)
    struct snmp_pdu *pdu;
{
    struct variable_list *vars;
    char buf[512];

    fprintf(dfile, "Address of peer:\n");
    fprintf(dfile, "   family: %d\n", pdu->address.sin_family);
    fprintf(dfile, "   port: %u\n", pdu->address.sin_port);
#ifdef sequent
    fprintf(dfile, "   ip address: %s\n", inet_ntoa(pdu->address.sin_addr.S_un.S_addr));
#else
    fprintf(dfile, "   ip address: %s\n", inet_ntoa(pdu->address.sin_addr.s_addr));
#endif /* sequent */
    fprintf(dfile, "Type of PDU: %d\n", pdu->command);
    fprintf(dfile, "Request id: %lu\n", pdu->reqid);
    fprintf(dfile, "Error status: %lu\n", pdu->errstat);
    fprintf(dfile, "Error index: %lu\n", pdu->errindex);
    if (pdu->errstat == SNMP_ERR_NOERROR){
	for(vars = pdu->variables; vars; vars = vars->next_variable) {
	    sprint_variable(buf, vars->name, vars->name_length, vars);
	    fprintf(dfile, "%s", buf);
	  }
    }
}



/*
 ** Catch a special signal and set debug level.
 **
 **  If debuging is off then turn on debuging else increment the level.
 **
 ** Handy for looking in on long running servers.
 */

void
setIncrDbgFlg()
{
    (void)signal(SIGUSR1, setIncrDbgFlg);
    if (debug == 0) {
	debug++;
	snmp_dump_packet++;
	setdebug(1);
    }
    else {
	debug++;
    }
    fprintf(dfile,"Debug turned ON, Level %d\n",debug);
}

/*
 ** Catch a special signal to turn off debugging
 */

void
setNoDbgFlg()
{
    (void)signal(SIGUSR2, setNoDbgFlg);
    setdebug(0);
}

/*
 ** Turn on or off debuging by open or closeing the debug file
 */

setdebug(code)
     int code;
{
    if (code) {
	dfile = freopen(debugfile, "w+", stderr);
	if ( dfile == NULL) {
	    syslog(LOG_WARNING, "can't open debug file %s: %m",
		   debugfile);
	    debug = 0;
	} else {
#ifdef sequent
	    setlinebuf(dfile);
#else
	    setvbuf(dfile, NULL, _IOLBF, BUFSIZ);
#endif
	    (void) fcntl(fileno(dfile), F_SETFL, FAPPEND);
	}
    } else {
	snmp_dump_packet = 0;
	debug = 0;
    }

    if (dfile && debug == 0) {
	fprintf(dfile,"Debug turned OFF\n");
	(void) fclose(dfile);
	dfile = 0;
    }
}

/*
 ** Catch a special signal to turn off debugging
 */

void
setQuitFlg()
{
    extern done;

    (void)signal(SIGQUIT, setQuitFlg);
    done++;
}









