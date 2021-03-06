/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: talkd.c 1.1 89/10/09 $ */

/* The top level of the daemon, the format is heavily borrowed
   from rwhod.c. Basically: find out who and where you are; 
   disconnect all descriptors and ttys, and then endless
   loop on waiting for and processing requests
 */

#include "ctl.h"

#include <stdio.h>
#include <errno.h>
#include <sgtty.h>
#include <sys/ioctl.h>

#define MAX_ERR 20	/* Number of times to attempt to open the 
			   control socket */

extern	errno;

struct	sockaddr_in sin = { AF_INET };

CTL_MSG request;
CTL_RESPONSE response;
int sockt;
char hostname[HOST_NAME_LENGTH];

struct hostent *gethostbyname();
struct servent *getservbyname();

int debug = 0;

CTL_MSG swapmsg();

main(argc)
int argc;
{
    struct sockaddr_in from;
    int from_size;
    int cc;
    int name_length = sizeof(hostname);
    struct servent *sp;
    struct hostent *hp;


    if ( argc > 1 ) {
	debug = 1;
    }

    if ( !debug ) {
	if (fork()) {
	    exit(0);
	}
    }

    gethostname(hostname, &name_length);

    hp = gethostbyname(hostname);
    if (hp == (struct hostent *) 0) {
	    fprintf(stderr, "talkd: Cannot obtain local address\n");
	    exit(1);
    }

#ifdef NTALK
    sp = getservbyname("ntalk", "udp");
#else
    sp = getservbyname("talk", "udp");
#endif

    if (sp == 0) {
	    fprintf(stderr, "talkd: udp/talk: unknown service\n");
	    exit(1);
    }

    if (getuid()) {
	fprintf(stderr, "Talkd : not super user\n");
	exit(1);
    }

    setup_desc();

    sin.sin_port = sp->s_port;
    sockt = socket(AF_INET, SOCK_DGRAM, 0, 0);
    if (sockt < 0) {
	    print_error("talkd: socket");
	    exit(1);
    }

    if (bind(sockt, (caddr_t)&sin, sizeof (sin), 0) < 0) {
	print_error("bind");
	exit(1);
    }

    for (;;) {

	from_size = sizeof(from);
	cc = recvfrom(sockt, (char *)&request, sizeof (request), 0, 
		      &from, &from_size);

	if (cc != sizeof(request)) {
	    if (cc < 0 && errno != EINTR) {
		print_error("receive");
	    }
	} else {

	    if (debug) printf("Request received : \n");
	    if (debug) print_request(&request);

	    request = swapmsg(request);
	    process_request(&request, &response);

	    if (debug) printf("Response sent : \n");
	    if (debug) print_response(&response);

		/* can block here, is this what I want? */

	    cc = sendto(sockt, (char *) &response, sizeof(response), 0,
			&request.ctl_addr, sizeof(request.ctl_addr));

	    if (cc != sizeof(response)) {
		print_error("Send");
	    }
	}
    }
}

    /* disconnect all descriptors, remove ourself from the process
     * group that spawned us
     */

setup_desc()
{
    int s;

    if (debug) return;

    for (s = 0; s < 10; s++) {
	(void) close(s);
    }

    (void) open("/dev/null", 0);
    (void) dup2(0, 1);
    (void) dup2(0, 2);

    s = open("/dev/tty", 2);

    if (s >= 0) {
	ioctl(s, TIOCNOTTY, (struct sgttyb *) 0);
	(void) close(s);
    }

    (void) chdir("/dev");
}

extern int sys_nerr;
extern char *sys_errlist[];

print_error(string)
char *string;
{
    FILE *cons;
    char *err_dev = "/dev/console";
    char *sys;
    int val, pid;

    if (debug) err_dev = "/dev/tty";

    sys = "Unknown error";

    if(errno < sys_nerr) {
	sys = sys_errlist[errno];
    }

	/* don't ever open tty's directly, let a child do it */
    if ((pid = fork()) == 0) {
	cons = fopen(err_dev, "a");
	if (cons != NULL) {
	    fprintf(cons, "Talkd : %s : %s(%d)\n\r", string, sys,
		    errno);
	    fclose(cons);
	}
	exit(0);
    } else {
	    /* wait for the child process to return */
	do {
	    val = wait(0);
	    if (val == -1) {
		if (errno == EINTR) {
		    continue;
		} else if (errno == ECHILD) {
		    break;
		}
	    }
	} while (val != pid);
    }
}

#define swapshort(a) (((a << 8) | ((unsigned short) a >> 8)) & 0xffff)
#define swaplong(a) ((swapshort(a) << 16) | (swapshort(((unsigned)a >> 16))))

/*  
 * heuristic to detect if need to swap bytes
 */

CTL_MSG
swapmsg(req)
	CTL_MSG req;
{
 	CTL_MSG swapreq;
 	
 	if (req.ctl_addr.sin_family == swapshort(AF_INET)) {
 		swapreq = req;
 		swapreq.id_num = swaplong(req.id_num);
 		swapreq.pid = swaplong(req.pid);
 		swapreq.addr.sin_family = swapshort(req.addr.sin_family);
 		swapreq.ctl_addr.sin_family =
 			swapshort(req.ctl_addr.sin_family);
 		return swapreq;
 	}
 	else
 		return req;
}
