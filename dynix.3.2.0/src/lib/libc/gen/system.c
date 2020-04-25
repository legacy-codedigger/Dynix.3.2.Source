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

/* $Header: system.c 2.0 86/01/28 $ */

#include	<signal.h>

system(s)
char *s;
{
	int status, pid, w;
	register int (*istat)(), (*qstat)();

	if ((pid = vfork()) == 0) {
		execl("/bin/sh", "sh", "-c", s, 0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1)
		status = -1;
	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
	return(status);
}
