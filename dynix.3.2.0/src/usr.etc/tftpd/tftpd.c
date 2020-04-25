/* $Copyright:	$
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

#ifndef lint
static char rcsid[] = "$Header: tftpd.c 1.8 91/04/02 $";
#endif

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)tftpd.c	5.8 (Berkeley) 6/18/88";
#endif /* not lint */

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton <guyton@rand-unix>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <arpa/tftp.h>

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <syslog.h>

#define	TIMEOUT		5
#ifndef	SB_MAX
#define	SB_MAX	(64*1024)
#endif

extern	int errno;
struct	sockaddr_in sin = { AF_INET };
struct	sockaddr_in tftp_sin = { AF_INET };
int	peer;
int	rexmtval = TIMEOUT;
int	maxtimeout = 5*TIMEOUT;

#define	PKTSIZE	SEGSIZE+4
char	buf[PKTSIZE];
char	ackbuf[PKTSIZE];
struct	sockaddr_in from;
int	fromlen;
int	initted = 0;
int	securetftp = 0;
int	standalone = 0;
int	debug = 0;
int	f, i, j, pid;
int	rcvbuf_size = 0;

/*
 * Default directory for unqualified names
 * Used by TFTP boot procedures
 */
char	*homedir = "/tftpboot";

reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}

main(argc,argv)
	int argc;
	char **argv;
{
	struct sockaddr_in from;
	register struct tftphdr *tp;
	register int n;
	struct servent *sp;
	int on = 1;
	extern int optind, opterr;
	extern char *optarg;
	char ch;
	int i;

	openlog("tftpd", LOG_PID, LOG_DAEMON);
	while ((ch = getopt(argc, argv, "Sbds")) != EOF) {
		switch (ch) {
 		case 'S':
 			standalone = 1;
 			break;

 		case 'd':
 			debug = 1;
 			break;

 		case 's':
 			securetftp = 1;
 			break;

#ifdef	INTERNAL
 		case 'b':
 			rcvbuf_size = atoi(argv[1]);
			if (rcvbuf_size <= 0 || rcvbuf_size > SB_MAX) {
				syslog(LOG_ERR,
				    "tftpd: invalid buffer size %s\n", argv[1]);
				usage();
			}
 			break;
#endif

 		default:
 			usage ();
 			break;
 		}
 	}
 
 	/*
 	 * Do we wish to change the default home directory?
 	 * Must be an absolute path name.
 	 */
 	while (argc - optind > 0) {
 		if (argv[optind][0] == '/') {
 			homedir = argv[optind];
 			optind++;
 			break;
 		} else {
 			syslog(LOG_ERR, "ignored argument: %s", argv[1]);
 		}
 		optind++;
	}
	if (standalone == 0) {
		oldmain(argc, argv);
		exit(1);
	}
	if (debug == 0) {
		if (fork())
			exit(0);
		for (f = 0; f < 10; f++)
			(void) close(f);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		{ 
			int t = open("/dev/tty", 2);
			if (t >= 0) {
				ioctl(t, TIOCNOTTY, (char *)0);
				(void) close(t);
			}
		}
		openlog("tftpd", LOG_PID, LOG_DAEMON);
	}
	sp = getservbyname("tftp", "udp");
	if (sp == 0) {
		syslog(LOG_ERR, "tftpd: udp/tftp: unknown service\n");
		exit(1);
	}
	tftp_sin.sin_port = sp->s_port;
	signal(SIGCHLD, reapchild);

	if ((f = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "tftpd: socket");
		exit(1);
	}
	if (setsockopt(f, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		syslog(LOG_ERR, "tftpd: setsockopt (SO_REUSEADDR)");
	if (debug)
		if (setsockopt(f, SOL_SOCKET, SO_DEBUG, &on, sizeof(on)) < 0)
			syslog(LOG_ERR, "tftpd: setsockopt (SO_DEBUG)");
	if (rcvbuf_size)
		if (setsockopt(f, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, 
		    sizeof(rcvbuf_size)) < 0)
			syslog(LOG_ERR, "tftpd: setsockopt (SO_RCVBUF)");
	if (bind(f, (caddr_t)&tftp_sin, sizeof (tftp_sin), 0) < 0) {
		syslog(LOG_ERR, "tftpd: bind");
		exit(1);
	}
	for (;;) {
		do {
			fromlen = sizeof (from);
			n = recvfrom(f, buf, sizeof (buf), 0,
			    (caddr_t)&from, &fromlen);
		} while (n <= 0);

		pid = fork();
		if (pid < 0) {
			syslog(LOG_ERR, "fork: %m\n");
			sleep(2);
			continue;
		} else if (pid != 0) {
			continue;
		}
		alarm(0);
		close(0);
		close(1);
		peer = socket(AF_INET, SOCK_DGRAM, 0);
		if (peer < 0) {
			syslog(LOG_ERR, "tftpd: socket: %m\n");
			exit(1);
		}
		if (bind(peer, (caddr_t)&sin, sizeof (sin)) < 0) {
			syslog(LOG_ERR, "tftpd: bind: %m\n");
			exit(1);
		}
		if (connect(peer, (caddr_t)&from, sizeof(from)) < 0) {
			syslog(LOG_ERR, "tftpd: connect: %m\n");
			exit(1);
		}
		tp = (struct tftphdr *)buf;
		tp->th_opcode = ntohs(tp->th_opcode);
		if (tp->th_opcode == RRQ || tp->th_opcode == WRQ) {
			tftp(tp, n);
		} else {
			exit(1);
		}
	}
}

oldmain(argc,argv)
	int argc;
	char **argv;
{
	register struct tftphdr *tp;
	register int n;
	int on = 1;

	if (ioctl(0, FIONBIO, &on) < 0) {
		syslog(LOG_ERR, "ioctl(FIONBIO): %m\n");
		exit(1);
	}

	fromlen = sizeof (from);
	n = recvfrom(0, buf, sizeof (buf), 0,
	    (caddr_t)&from, &fromlen);
	if (n < 0) {
		syslog(LOG_ERR, "recvfrom: %m\n");
		exit(1);
	}
	/*
	 * Now that we have read the message out of the UDP
	 * socket, we fork and exit.  Thus, inetd will go back
	 * to listening to the tftp port, and the next request
	 * to come in will start up a new instance of tftpd.
	 *
	 * We do this so that inetd can run tftpd in "wait" mode.
	 * The problem with tftpd running in "nowait" mode is that
	 * inetd may get one or more successful "selects" on the
	 * tftp port before we do our receive, so more than one
	 * instance of tftpd may be started up.  Worse, if tftpd
	 * break before doing the above "recvfrom", inetd would
	 * spawn endless instances, clogging the system.
	 */
	{
		int pid;
		int i, j;

		for (i = 1; i < 20; i++) {
		    pid = fork();
		    if (pid < 0) {
				sleep(i);
				/*
				 * flush out to most recently sent request.
				 *
				 * This may drop some request, but those
				 * will be resent by the clients when
				 * they timeout.  The positive effect of
				 * this flush is to (try to) prevent more
				 * than one tftpd being started up to service
				 * a single request from a single client.
				 */
				j = sizeof from;
				i = recvfrom(0, buf, sizeof (buf), 0,
				    (caddr_t)&from, &j);
				if (i > 0) {
					n = i;
					fromlen = j;
				}
		    } else {
				break;
		    }
		}
		if (pid < 0) {
			syslog(LOG_ERR, "fork: %m\n");
			exit(1);
		} else if (pid != 0) {
			exit(0);
		}
	}
	from.sin_family = AF_INET;
	alarm(0);
	close(0);
	close(1);
	peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) {
		syslog(LOG_ERR, "socket: %m\n");
		exit(1);
	}
	if (bind(peer, (caddr_t)&sin, sizeof (sin)) < 0) {
		syslog(LOG_ERR, "bind: %m\n");
		exit(1);
	}
	if (connect(peer, (caddr_t)&from, sizeof(from)) < 0) {
		syslog(LOG_ERR, "connect: %m\n");
		exit(1);
	}
	tp = (struct tftphdr *)buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
	exit(1);
}

int	validate_access();
int	sendfile(), recvfile();

struct formats {
	char	*f_mode;
	int	(*f_validate)();
	int	(*f_send)();
	int	(*f_recv)();
	int	f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
#ifdef notdef
	{ "mail",	validate_user,		sendmail,	recvmail, 1 },
#endif
	{ 0 }
};

/*
 * Handle initial connection protocol.
 */
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	register char *cp;
	int first = 1, ecode;
	register struct formats *pf;
	char *filename, *mode;

	filename = cp = tp->th_stuff;
again:
	while (cp < buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	ecode = (*pf->f_validate)(filename, tp->th_opcode);
	if (ecode) {
		nak(ecode);
		exit(1);
	}
	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf);
	else
		(*pf->f_send)(pf);
	exit(0);
}


FILE *file;

/*
 * Validate file access.  Since we
 * have no uid or gid, for now require
 * file to exist and be publicly
 * readable/writable.
 * Note also, full path name must be
 * given as we have no login directory.
 */
validate_access(filename, mode)
	char *filename;
	int mode;
{
	struct stat stbuf;
	int	fd;

	if (!initted) {
		if (securetftp) {
			if (chroot(homedir) < 0) {
				syslog(LOG_ERR,
				       "cannot chroot to directory %s: %m",
					homedir);
				return (EACCESS);
			}
			(void) chdir("/");  /* cd to  new root */
		} else {
        		(void) chdir(homedir); /* don't care if this works */
		}
		/*
         	 * Need to perform access check as someone who will only
         	 * be allowed "public" access to the file.  There is no
         	 * such uid/gid reserved so we kludge it with -2/-2.
         	 * (Can't use -1/-1 'cause that means "don't change".)
         	 */
        	(void) setgid(-2);
        	(void) setuid(-2);
		initted = 1;
	}
	if (*filename != '/')
		return (EACCESS);
	if (stat(filename, &stbuf) < 0)
		return (errno == ENOENT ? ENOTFOUND : EACCESS);
	if (mode == RRQ) {
		if ((stbuf.st_mode&(S_IREAD >> 6)) == 0)
			return (EACCESS);
	} else {
		if ((stbuf.st_mode&(S_IWRITE >> 6)) == 0)
			return (EACCESS);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFREG)
                return (EACCESS);
	fd = open(filename, mode == RRQ ? 0 : 1);
	if (fd < 0)
		return (errno + 100);
	file = fdopen(fd, (mode == RRQ)? "r":"w");
	if (file == NULL) {
		return errno+100;
	}
	return (0);
}

int	timeout;
jmp_buf	timeoutbuf;

timer()
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
sendfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *r_init();
	register struct tftphdr *ap;    /* ack packet */
	register int block = 1, size, n;

	signal(SIGALRM, timer);
	dp = r_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		size = readit(file, &dp, pf->f_convert);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		timeout = 0;
		(void) setjmp(timeoutbuf);

send_data:
		if (send(peer, dp, size + 4, 0) != size + 4) {
			syslog(LOG_ERR, "tftpd: write to %s: %m\n", 
			    inet_ntoa(from.sin_addr));
			goto abort;
		}
		read_ahead(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf, sizeof (ackbuf), 0);
			alarm(0);
			if (n < 0) {
				syslog(LOG_ERR, "tftpd: read from %s: %m\n",
				    inet_ntoa(from.sin_addr));
				goto abort;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);

			if (ap->th_opcode == ERROR)
				goto abort;
			
			if (ap->th_opcode == ACK) {
				if (ap->th_block == block) {
					break;
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (ap->th_block == (block -1)) {
					goto send_data;
				}
			}

		}
		block++;
	} while (size == SEGSIZE);
abort:
	(void) fclose(file);
}

justquit()
{
	exit(0);
}


/*
 * Receive a file.
 */
recvfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *w_init();
	register struct tftphdr *ap;    /* ack buffer */
	register int block = 0, n, size;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		timeout = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		if (send(peer, ackbuf, 4, 0) != 4) {
			syslog(LOG_ERR, "tftpd: write to %s: %m\n",
			    inet_ntoa(from.sin_addr));
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, PKTSIZE, 0);
			alarm(0);
			if (n < 0) {            /* really? */
				syslog(LOG_ERR, "tftpd: read from %s: %m\n",
				    inet_ntoa(from.sin_addr));
				goto abort;
			}
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (dp->th_opcode == ERROR)
				goto abort;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
			}
		}
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == SEGSIZE);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(peer, ackbuf, 4, 0);

	signal(SIGALRM, justquit);      /* just quit on timeout */
	alarm(rexmtval);
	n = recv(peer, buf, sizeof (buf), 0); /* normally times out and quits */
	alarm(0);
	if (n >= 4 &&                   /* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(peer, ackbuf, 4, 0);     /* resend final ack */
	}
abort:
	return;
}

struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
nak(error)
	int error;
{
	register struct tftphdr *tp;
	int length;
	register struct errmsg *pe;
	extern char *sys_errlist[];

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = sys_errlist[error - 100];
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (send(peer, buf, length, 0) != length)
		syslog(LOG_ERR, "nak: %m\n");
}
usage()
{
  syslog(LOG_ERR, 
#ifdef	INTERNAL
      "Usage: tftpd [-s] [-S] [-d] [-b bufsize] [ home-directory ]\n");
#else
      "Usage: tftpd [-s] [-S] [ home-directory ]\n");
#endif
  exit (1);
}
