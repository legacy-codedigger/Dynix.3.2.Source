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

#ident	"$Header: snmp_config.c 1.2 1991/07/31 00:28:45 $"
/*LINTLIBRARY*/

/*
 * snmp_config.h
 *	Read config file (snmpd.conf) and parse and process it.
 */

/* $Log: snmp_config.c,v $
 *
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
#include <netdb.h>
#ifdef	sequent
#include <sys/socket.h>
#endif

#include "asn1.h"
#include "snmp.h"
#include "snmp_impl.h"
#include "snmp_api.h"
#include "snmp_client.h"
#include "utils.h"
#ifndef sequent
#include "select.h"
#endif
#include "debug.h"

extern int  errno;

struct comms	*Community = NULL;
struct traps    *Trap_List = NULL;

snmp_config(ConfigFile)
    char *ConfigFile;
{
    if (!snmp_openConfig(ConfigFile)) {
	syslog(LOG_ERR, "Config file open error");
	if (debug)
	    fprintf(dfile, "Config file open error\n");
	return -1;
    }
    snmp_configure();
    snmp_endConfig();
    return 0;
}

FILE	*fconfig = NULL;

snmp_openConfig(ConfigFile)
    char *ConfigFile;
{
    
    if (fconfig != NULL) {
	fseek(fconfig, 0L, 0);
	return 1;
    }
    fconfig = fopen(ConfigFile, "r");
    return (fconfig != NULL);
}

snmp_endConfig()
{
    
    if (fconfig == NULL)
	return;
    fclose(fconfig);
    fconfig = NULL;
}

/*
 *	<configuration>  ::=  <modules> <streams> eof
 *	<modules>        ::=  <modline> <modules> | null
 *	<modline>        ::=  <moddef> | <empty>
 *	<moddef>         ::=  STRING STRING STRING <comment> \n
 *	<empty>          ::=  <comment> \n
 *	<comment>        ::=  COMMENT | null
 *	<streams>        ::=  <delim> <nodes> | null
 *	<delim>          ::=  DELIM \n
 *	<nodes>          ::=  <nodeline> <nodes> | null
 *	<nodeline>       ::=  <nodedef> | <empty>
 *	<nodedef>        ::=  STRING STRING <info> <comment> \n
 *	<info>           ::=  <message> <info> | null
 *	<message>        ::=  <function> | <function> "=" <value>
 *	<function>       ::=  STRING
 *	<value>          ::=  <arg> | "{" <list> "}"
 *	<list>           ::=  <arg> <rest>
 *	<arg>            ::=  STRING
 *	<rest>           ::=  "," <list> | null
 */

static lexstate();
static match();

/*
 *  Lexical Tokens.
 */

/* #define STRING	256 */
#define COMMENT	'#'
#define NL	'\n'
#define DELIM	'%'

#define COMMAND_COMMUNITY "COMMUNITY"
#define COMMAND_SET       "SET"
#define COMMAND_SEND      "SEND"
#define COMMAND_TRAP      "TRAP"

static int	lexan();		/* returns next input token */

/*
 *  Local globals used by parser and lexical analyser.
 */

static int	token;			/* current input token */
static char	*word;			/* value of token */

/*
 *  configure()	--	build the configuration specified in a file
 */
snmp_configure()
{
	/*
	 *  Initially, equals signs are lexically special.
	 */
	lexstate('=');
	token = lexan();
	if (debug > 15)
	    fprintf(dfile, "TOKEN %d %s\n", token, token==STRING? word:"");

	/*
	 *
	 *  This section contains lines with three fields each,
	 *  i.e. command and 2 parameters.  Any additional tokens
	 *  should be comments.  Blank lines and whole-line
	 *  comments contain zero fields.  Any other number
	 *  of fields is an error.
	 */
	modules();
	match(EOF);
}

static
modules()
{
	for (;;) switch (token)
	{
	case STRING:
	    modline();
	    break;
	case COMMENT:
	    comment();
	    break;
	case NL:
	    match(NL); 
	    break;
	case EOF:
	    return;
	case DELIM:
	    return;
	}
}

static
modline()
{
	char	*command, *param1, *param2, *param3 = NULL;

	if (token == STRING) 
	    command = copy(word); 
	match(STRING); /* mandatory field */
	if (token == STRING) 
	    param1 = copy(word); 
	match(STRING); /* mandatory field */
	if (token == STRING) 
	    param2 = copy(word); 
	if (match(STRING) == 0) /* optional field */
		if (token == STRING) 
		    param3 = copy(word); 
	match(STRING);

	comment();
	if (debug > 5)
	    fprintf(dfile, "command: %s param1: %s param2: %s\n",command, param1, param2);
	processTokens(command, param1, param2, param3);
	match(NL);
}

static
comment()
{
    switch (token){
    case COMMENT:
	match(COMMENT); 
	break;
    default:
	return;
    }
}
/*
static
streams()
{
    switch (token) {
    case DELIM:	
	match(DELIM); 
	match(NL); 
	nodes(); 
	break;
    default:	
	return;
    }
}
*/
static
nodes()
{
    for (;;) switch (token) {
    case STRING:	
	nodeline(); 
	break;
    case COMMENT:
	comment(); 
	break;
    case NL:
	match(NL); 
	break;
    case EOF:
	return 0;
    case DELIM:	
	lerror("unexpected delimiter");
	return -1;
    }
}

static
nodeline()
{
    char	*upper, *lower;
    list_t	info = L_NULL, tail;
    list_t	message;
    
    if (token == STRING) 
	upper = copy(word); 
    match(STRING);
    if (token == STRING) 
	lower = copy(word); 
    match(STRING);
    
    /*
     *  Process control message list...
     */
    while (token == STRING) {
	/*
	 *  Control message identifier...
	 *
	 *  Store this at the head of a list, with arguments
	 *  (each of which is also a list) tagged afterwards.
	 */
	
	message = mklist(copy(word));
	
	if (info){	/* append to end of list */
	    tail->next = mklist((char *) message);
	    tail = tail->next;
	} else {	/* first in list */
	    info = mklist((char *) message);
	    tail = info;
	}
	
	match(STRING);
	
	/*
	 *  Argument list is optional...
	 *
	 *  After the '=' is either:
	 *
	 *   -	an empty list
	 *   -  a single argument
	 *   -  a brace-enclosed, comma-separated list
	 *      of possibly null arguments
	 */
	if (token == '=') {
	    match('=');
	    
	    switch (token) {
	    case STRING:		/* single argument */
		message->next = mklist(copy(word));
		match(STRING);
		break;
		
	    case '{':		/* brace-enclosed list */
		/*
		 *  Inside the braces in a control message
		 *  specification commas are special, equals
		 *  signs are not.
		 */
		lexstate(',');
		match('{');
		for (;;) {
		    if (token == STRING) {
			message->next =
			    mklist(copy(word));
			match(STRING);
		    } else
			message->next = mklist(NULL);
		    
		    if (token == '}')
			break;
		    message = message->next;
		    match(',');
		}
		
		/*
		 *  Restore lexical state.
		 */
		lexstate('=');
		match('}');
		break;
		
	    default:		/* empty list */
		break;
	    }
	}
    }
    
    comment();
    link(upper, lower, info);
    
    match(NL);
}

static
match(t)
int t;
{
    if (token == t) {
	if (token == EOF)
	    return;
	token = lexan();
    } else {
	lerror("bad format");
	return(-1);
	}
    
    if (debug > 15)
	fprintf(dfile, "TOKEN %d %s\n", token, token==STRING? word:"");

return(0);
}

/**********************************************************************/

static char	state;		/* lexical state */

/*
 *  lexstate()		--	set lexical state
 */
static
lexstate(c)
char	c;
{
    state = c;
}

#define MAXLEN	200		/* max. length of a token */

static int lineno = 1;		/* input line number */

/*
 *  lexan()	--	get the next input token
 */
static int
lexan()
{
    static char	buff[MAXLEN+1];
    int		len = 0;
    int		c;
    
 again:
    /* skip whitespace */
    while ((c = getc(fconfig)) == ' ' || c == '\t')
	;
    
    if (c == EOF)		/* end of file */
	return EOF;
    
    if (c == '\n') {		/* newline token */
	++lineno;
	return NL;
    }
    
    if (c == '#') {		/* comment - strip to end of line */
	while ((c = getc(fconfig)) != EOF && c != '\n')
	    ;
	if (c != EOF)
	    ungetc(c, fconfig);
	return COMMENT;
    }
    
    /* check for escaped newline */
     if (c == '\\') {
	if ((c = getc(fconfig)) == '\n')	/* got one */
	    goto again;
	else if (c != EOF)
	    ungetc(c, fconfig);
    }
    
    /*
     *  special characters...
     *  NB:  state is either '=' (normally) or ',' (inside braces) .
     *  State is set by the parser.
     */
    if (c == state || c == '{' || c == '}')
	return c;
    
    /*  quoted string - gather till matching quote or end of line */
     if (c == '"' || c == '\'') {
	char	q = c;		/* quote character */
	
	while ((c = getc(fconfig)) != q && c != EOF && c != '\n')
	    if (len < MAXLEN)
		buff[len++] = c;
	
	buff[len] = '\0';
	
	/* silently terminate quoted string at end of line... */
	if (c == '\n')
	    ungetc(c, fconfig);
	
	word = buff;
	return STRING;
    }
    
    /* unquoted string - gather till next special character or whitespace */
    while (c != state && c != '{' && c != '}' && c != EOF 
	   && c != ' ' && c != '\t' && c != '\n') {
	if (len < MAXLEN)
	    buff[len++] = c;
	
	c = getc(fconfig);
    }
    buff[len] = '\0';
    
    if (c != EOF)
	ungetc(c, fconfig);
    
    /* Check for delimiter */
    if (streq(buff, "%%"))
	return DELIM;
    
    word = buff;
    return STRING;
}

lerror(format, arg1, arg2)
char	*format;
char	*arg1, *arg2;
{
    syslog(LOG_WARNING, "snmpd.conf: parsing error line %d", lineno);
    if (debug) {
	fprintf(dfile, "%s: \"%s\", line %d: ","snmpd", "snmpd.conf", lineno);
	fprintf(dfile, format, arg1, arg2);
	fprintf(dfile, "\n");
    }
    return 1;
}

char *
copy(s)
char *s;
{
    char	*new;
    
    if ((new = (char *)malloc(strlen(s)+1)) == NULL) {
	syslog(LOG_ERR, "out of memory");
	if (debug)
	    fprintf(dfile ,"out of memory");
	exit(1);
    }
    strcpy(new, s);
    return new;
}

list_t
mklist(s)
char	*s;
{
    list_t	l = (list_t) memory(sizeof(struct list));
    
    l->data = s;
    l->next = L_NULL;
    return l;
}

char *
memory(size)
unsigned size;
{
    register char	*p;
    
    if ((p = (char *)malloc(size)) == NULL) {
	syslog(LOG_ERR, "out of memory");
	if (debug)
	    fprintf(dfile, "out of memory");
	exit(1);
    }
    return p;
}

processTokens(command, param1, param2, param3)
char *command;
char *param1;
char *param2;
char *param3;
{
    char *c;

/* convert command to upper case */
for(c = command; *c != '\0'; c++)
    *c = toupper((*c));

if (strcmp(command, COMMAND_COMMUNITY) == 0)
    processCommunity(param1, param2, param3);
else if (strcmp(command, COMMAND_SET) == 0)
    processSet(param1, param2);
else if (strcmp(command, COMMAND_SEND) == 0) {
    /* convert second token to upper case */
    for(c = param1; *c != '\0'; c++)
	*c = toupper((*c));
    if (strcmp(param1, COMMAND_TRAP) == 0)
	processTrap(param2);
    else
	lerror("INVALID COMMAND - %s\n", param1);
} else
    lerror("INVALID COMMAND - %s\n", command);
}


processCommunity(name, access, host)
char *name;
char *access;
char *host;
{
    struct comms *community;
    char *c;
    extern struct comms *getCommunity();
    ulong inaddr;

    if ((community = getCommunity(name, strlen(name))) == NULL) {
	/* community does not already exist */
	if (debug > 3)
	    fprintf(dfile, "Adding community %s (%s)\n", name, access);

	if ((community = (struct comms *)malloc(sizeof(struct comms))) == NULL) {
	    syslog(LOG_ERR, "out of memory");
	    if (debug)
		fprintf(dfile, "out of memory");
	    exit(1);
	}
	community->name = copy(name);
	community->host_list = NULL;

	/* convert access to upper case */
	for(c = access; *c != '\0'; c++)
	    if (isalpha(*c))
		*c = toupper((*c));
    
	if (strcmp(access, "READ-ONLY") == 0) {
	    if (debug > 3)
		fprintf(dfile, "Setting %s to %s\n", community->name, access);
	    community->access = READ;
	} else if (strcmp(access, "READ-WRITE") == 0) {
	    if (debug > 3)
		fprintf(dfile, "Setting %s to %s\n", community->name, access);
	    community->access = WRITE;
	} else if (strcmp(access, "NO-ACCESS") == 0) {
	    if (debug > 3)
		fprintf(dfile, "Setting %s to %s\n", community->name, access);
	    community->access = NOACCESS;
	} else {
	    syslog(LOG_ERR, "ERROR Setting %s to %s\n", community->name, access);
	    if (debug > 3)
		fprintf(dfile, "ERROR Setting %s to %s\n", community->name, access);
	    free(community);
	    return;
	}
	if (validateHost(host, &inaddr) < 0) {
	    lerror("Error setting community: Invalid host %s\n", host);
	    free((char *)community);
	    return;
	} else {
	    /* add this community to the linked list */
	    if (Community == NULL) {
		Community = community;
		community->next = NULL;
	    } else {
		community->next = Community;
		Community = community;
	    }
	}
    } else {
	if (debug > 3)
	    fprintf(dfile, "Updating community %s (%s)\n", name, access);
	if (validateHost(host, &inaddr) < 0) {
	    lerror("Error setting community: Invalid host %s\n", host);
	    return;
	}
    }

    /* add host to host list */
    addCommunityHost(community, inaddr);
}

processSet( var_name, var_val )
char *var_name;
char *var_val;
{
    if (debug > 3)
	fprintf(dfile, "Setting %s to %s\n", var_name, var_val);
    init_value(var_name, var_val, strlen(var_val));
}

processTrap(trap_ip_addr)
char *trap_ip_addr;
{
    struct traps *trap_list;
    struct snmp_session *session, *ss;

    if ((trap_list = (struct traps *)malloc(sizeof(struct traps))) == NULL) {
	syslog(LOG_ERR, "out of memory");
	if (debug)
	    fprintf(dfile, "out of memory");
	exit(1);
    }
    if ((session = (struct snmp_session *)malloc(sizeof(struct snmp_session))) == NULL) {
	syslog(LOG_ERR, "out of memory");
	if (debug)
	    fprintf(dfile, "out of memory");
	exit(1);
    }

    session->peername = copy(trap_ip_addr); 
    session->community = NULL;
    session->community_len = 0;
    session->retries = 0;
    session->timeout = 0;
    session->authenticator = NULL;
    session->callback = NULL;
    session->callback_magic = NULL;
    session->remote_port = SNMP_TRAP_PORT;
    session->local_port = 0;
    ss = snmp_open(session);
    if (ss == NULL){
	syslog(LOG_ERR, "Couldn't create trap endpoint for %s\n", trap_ip_addr);
	if (debug > 3)
	    fprintf(dfile, "Couldn't create trap endpoint for %s\n", trap_ip_addr);
	free((char *)trap_list);
	free((char *)session);
	return;
    } else {
	trap_list->session = ss;
    }
    
    if (debug > 3)
	fprintf(dfile, "Setting %s to list of trap receivers\n", trap_ip_addr);

    if (Trap_List == NULL) {
	Trap_List = trap_list;
	trap_list->next = NULL;
    } else {
	trap_list->next = Trap_List;
	Trap_List = trap_list;
    }
}

validateHost(host, inaddr)
char *host;
ulong *inaddr;
{
    struct hostent *hostp;
    ulong inetaddr;

    if ((host == NULL) || (inet_addr(host) == 0)) {
	*inaddr = 0;
	return(0);
    }

    hostp = gethostbyname(host);
    if (hostp) {
	/*
	 * If we find a host entry, set up the internet address
	 */
	*inaddr = *(ulong *)hostp->h_addr_list[0];
	return(0);
    } else {
	/*
	 * If we couldn't find a host entry, the string may still
	 * be a valid internet address. If it is, we try to match
	 * it to a host name.
	 */
	if ((inetaddr = inet_addr(host)) == -1)
	    return(-1);
	hostp = gethostbyaddr((char *)&inetaddr,sizeof(inetaddr),AF_INET);
	if (hostp) {
	    *inaddr = inetaddr;
	    return(0);
	}
    }
    return(-1);
}








