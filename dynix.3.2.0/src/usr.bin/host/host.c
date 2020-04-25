/* @(#)$Copyright:	$
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

#ifndef lint
static char *rcsid = "$Header: host.c 1.4 86/03/12 $";
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/ttychars.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "remote.h"
#include "host.h"

/*
 * initialize the command escape mechanism
 */
int escape = '~';
int newline = 1;
int esc = 0;

char *prompt = promptbuf;			/* ptr into prompt buffer */

/*
 * initialize the remote port characteristics
 */
struct tchars tchars = {-1, -1, CSTART, CSTOP, -1, -1};
int ldisc = NTTYDISC;
struct ltchars ltchars = {-1, -1, -1, -1, -1, -1};
int lmode = LLITOUT | LDECCTQ;

/*
 * terminal characteristics buffers
 */
struct sgttyb otty;		/* original modes */
struct sgttyb ctty;		/* run modes */
struct sgttyb pty;		/* port modes */

int shellflag;			/* true when stopped or in a subshell */

int process = 1;		/* process remote file server commands */

extern char *getenv();

extern int onint();
extern int onalarm();

main(ac, av)
int ac;
char *av[];
{
	register char *p;
	char buf[BUFSIZ];
	register i, n;
	int rdfs;
	extern char _sobuf[];

	setbuf(stdout, _sobuf);
	myname = *av++; ac--;

	/*
	 * Get our terminal parameters
	 */
	ioctl(0, TIOCGETP, &otty);

	/*
	 * Parse the command line arguments
	 */
	portname = getenv("HOSTPORT");
	getargs(ac, av);
	if (portname == NULL || *portname == '\0')
		usage();

	/*
	 * Set up signal handling
	 */
	dosignals();

	/*
	 * Lock the port
	 */
	signal(SIGINT, SIG_IGN);
	if (!nflag)
		setlock();
	signal(SIGINT, onint);

	/*
	 * Open the port
	 */
	signal(SIGALRM, onalarm);	/* time it, only wait 10 s */
	alarm(10);
	port = open(portname, 2);
	alarm(0);
	signal(SIGALRM, SIG_IGN);
	if (port < 0) {
		printf("error: cannot open port\n");
		quit(1);
	}
	printf("%s: connected\n", myname);
	fflush(stdout);

	/*
	 * set terminal modes
	 */
	ctty = otty;				/* copy it */
	ctty.sg_flags &= ~(ECHO | CRMOD);
	ctty.sg_flags |= CBREAK;
	ioctl(0, TIOCSETP, &ctty);

	/*
	 * set the port to proper modes
	 */
	ioctl(port, TIOCSETD, &ldisc);
	ioctl(port, TIOCGETP, &pty);	/* preserve only baud rate */
	pty.sg_flags = CBREAK | TANDEM | EVENP | ODDP;
	if (baud) {
		pty.sg_ispeed = baud;
		pty.sg_ospeed = baud;
	}
	pty.sg_erase = -1;
	pty.sg_kill = -1;
	ioctl(port, TIOCSETP, &pty);
	ioctl(port, TIOCSETC, &tchars);
	ioctl(port, TIOCLSET, &lmode);
	ioctl(port, TIOCSLTC, &ltchars);

	for(;;) {
		fflush(stdout);
		rdfs = 0;
		if (runfile  &&  *prompt == '\0') {
			if (feof(runfile)) {
				fclose(runfile);
				runfile = NULL;
				printf("cmdfile: EOF\r\n");
				if (fileonly) {
					printf("\r\n[EOT]\r\n");
					quit(0);
				}
				fflush(stdout);
				n = 0;
			} else {
				n = getrun(buf);
				p = buf;
				getrun(promptbuf);
				prompt = promptbuf;
			}
		} else {
			if (shellflag)
				rdfs = BIT(port);
			else
				rdfs = BIT(0)|BIT(port);
			i = select(sizeof port * 8, &rdfs, 0, 0, 0);
			if (i <= 0) {
				if (errno == EINTR) /* after a stopped job */
					continue;
				perror("host: select");
				sleep(1);
				continue;
			}
			if (rdfs & BIT(0)) {
				n = read(0, buf, sizeof buf);
				p = buf;
			} else n = 0;
		}
		while (n > 0) {
			n--;
			*p &= 0x7F;
			parse(*p++);
		}
		if (rdfs & BIT(port)) {
			rptr = rbuf;
			rcount = read(port, rptr, BUFSIZ);
			if (rcount <= 0) {
				printf("\r\n%s: lost DTR....", myname);
				fflush(stdout);
				close(port);
				reopenport();
				printf(" reconnected\r\n");
				fflush(stdout);
				continue;
			}
			while (rcount-- > 0) {
				if (process) {
					if (*rptr == SOH) {
						rptr++;
						fflush(stdout);
						docommand();
						continue;
					}
					if (*prompt != '\0' && *prompt++ != (*rptr&0x7f))
						prompt = promptbuf;
				}
				if (scriptfp)
					putc(*rptr, scriptfp);
				putchar(*rptr);
				rptr++;
			}
			fflush(stdout);
			if (bflag)
				fflush(scriptfp);
		}
	}
}

reopenport()
{
	port = open(portname, 2);

	ioctl(port, TIOCSETD, &ldisc);
	ioctl(port, TIOCSETP, &pty);
	ioctl(port, TIOCSETC, &tchars);
	ioctl(port, TIOCLSET, &lmode);
	ioctl(port, TIOCSLTC, &ltchars);
}
