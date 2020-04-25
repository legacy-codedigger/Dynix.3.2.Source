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

#ifndef lint
static char rcsid[] = "$Header: ventel.c 2.0 86/01/28 $";
#endif

/*
 * Routines for calling up on a Ventel Modem
 * The Ventel is expected to be strapped for "echo".
 */
#include "tip.h"

#define	MAXRETRY	5

static	int sigALRM();
static	int timeout = 0;
static	jmp_buf timeoutbuf;

/*ARGSUSED*/
ven_dialer(num, acu)
	register char *num;
	char *acu;
{
	register int connected = 0;
#ifdef ACULOG
	char line[80];
#endif
	/*
	 * Get in synch with a couple of carriage returns
	 */
	if (!vensync(FD)) {
		printf("can't synchronize with ventel\n");
#ifdef ACULOG
		logent(value(HOST), num, "ventel", "can't synch up");
#endif
		return (0);
	}
	if (boolean(value(VERBOSE)))
		printf("\ndialing...");
	fflush(stdout);
	ioctl(FD, TIOCHPCL, 0);
	write(FD, "<K", 2);
	write(FD, num, strlen(num));
	write(FD, ">", 1);
	if (gobble('\n') && gobble('\n') && gobble('\007'))
		echo("\r$\n");
	if (gobble('\n'))
		connected = gobble('!');
	ioctl(FD, TIOCFLUSH, (struct sgttyb *)NULL);
#ifdef ACULOG
	if (timeout) {
		sprintf(line, "%d second dial timeout",
			number(value(DIALTIMEOUT)));
		logent(value(HOST), num, "ventel", line);
	}
#endif
	if (timeout)
		ven_disconnect();	/* insurance */
	return (connected);
}

ven_disconnect()
{

	close(FD);
}

ven_abort()
{

	write(FD, "\03", 1);
	close(FD);
}

static int
echo(s)
	register char *s;
{
	char c;

	while (c = *s++) switch (c) {

	case '$':
		read(FD, &c, 1);
		s++;
		break;

	case '#':
		c = *s++;
		write(FD, &c, 1);
		break;

	default:
		write(FD, &c, 1);
		read(FD, &c, 1);
	}
}

static int
sigALRM()
{

	printf("\07timeout waiting for reply\n");
	timeout = 1;
	longjmp(timeoutbuf, 1);
}

static int
gobble(match)
	register char match;
{
	char c;
	int (*f)();
	extern int vflag;

	f = signal(SIGALRM, sigALRM);
	timeout = 0;
	do {
		if (setjmp(timeoutbuf)) {
			signal(SIGALRM, f);
			return (0);
		}
		alarm(number(value(DIALTIMEOUT)));
		read(FD, &c, 1);
		alarm(0);
		c &= 0177;
		if (vflag && boolean(value(VERBOSE)))
			putchar(c);
	} while (c != '\n' && c != match);
	signal(SIGALRM, f);
	return (c == match);
}

#define min(a,b)	((a)>(b)?(b):(a))
/*
 * This convoluted piece of code attempts to get
 * the ventel in sync.  If you don't have FIONREAD
 * there are gory ways to simulate this.
 */
static int
vensync(fd)
{
	int already = 0, nread;
	char buf[60];

	while (already < MAXRETRY) {
		/*
		 * After reseting the modem, send it two \r's to
		 * autobaud on. Make sure to delay between them
		 * so the modem can frame the incoming characters.
		 */
		write(fd, "\r", 1); 
		nap(10);
		write(fd, "\r", 1); 
		nap(10);
		ioctl(fd, TIOCFLUSH, (struct sgttyb *)NULL); 
		nap(50);
		if (ioctl(fd, FIONREAD, (caddr_t)&nread) < 0) {
			perror("tip: ioctl");
			continue;
		}
		while (nread > 0) {
			read(fd, buf, min(nread, 60));
			if ((buf[nread - 1] & 0177) == '$')
				return (1);
			nread -= min(nread, 60);
		}
		sleep(1);
		/*
		 * Toggle DTR to force anyone off that might have left
		 * the modem connected, and insure a consistent state
		 * to start from.
		 *
		 * If you don't have the ioctl calls to diddle directly
		 * with DTR, you can always try setting the baud rate to 0.
		 */
		if (already == 0) {
			ioctl(FD, TIOCCDTR, 0); 
			nap(50);
			ioctl(FD, TIOCSDTR, 0); 
			nap(50);
		}
		already++;
	}
	return (0);
}
