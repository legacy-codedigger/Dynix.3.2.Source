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

/* @(#)$Header: signals.c 1.3 84/12/18 $ */

#include <stdio.h>
#include <sgtty.h>
#include <signal.h>
#include "host.h"

extern struct sgtty otty;
extern struct sgtty ctty;

extern int shellflag;

quit(code)
int code;
{

	/* restore terminal modes */
	fflush(stdout);
	ioctl(0, TIOCSETP, &otty);
	if (!nflag)
		unlock();
	exit(code);
}

/*
 * We have just gotten a susp.  Suspend and prepare to resume.
 */
onsusp()
{
	ioctl(0, TIOCSETP, &otty);
	sigsetmask(0);
	signal(SIGTSTP, SIG_DFL);
	shellflag = 1;
	kill(0, SIGTSTP);

	/* the pc stops here, OH NO IT DOESN'T, (RAND) */

	signal(SIGTSTP, onsusp);
	shellflag = 0;
	ioctl(0, TIOCSETP, &ctty);
}

onalarm()
{
	printf("%s: link down\n", myname);
	quit(1);
}

onint()
{
	printf("\r\n%s: abort\r\n", myname);
	quit(1);
}

dosignals()
{
	/*
	 * Initialize interrupt handling.
	 */
	if (signal(SIGHUP, SIG_IGN) == SIG_DFL)
		signal(SIGHUP, quit);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, onint);
	if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
		signal(SIGTERM, quit);
	if (signal(SIGEMT, SIG_IGN) == SIG_DFL)
		signal(SIGEMT, quit);
	if (signal(SIGTSTP, SIG_IGN) == SIG_DFL)
		signal(SIGTSTP, onsusp);
}
