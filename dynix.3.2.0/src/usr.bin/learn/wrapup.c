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
static char rcsid[] = "$Header: wrapup.c 2.2 86/06/18 $";
#endif

#include "signal.h"
#include "stdio.h"
#include "lrnref.h"

wrapup(n)
int n;
{
/* this routine does not use 'system' because it wants interrupts turned off */

	signal(SIGINT, SIG_IGN);
	chdir("..");
	if (fork() == 0) {
		signal(SIGHUP, SIG_IGN);
#if defined(vax) || defined(ns32000) || defined(i386)
		if (fork() == 0) {
			close(1);
			open("/dev/tty", 1);
			execl("/bin/stty", "stty", "new", 0);
		}
#endif
		execl("/bin/rm", "rm", "-rf", dir, 0);
		execl("/usr/bin/rm", "rm", "-rf", dir, 0);
		perror("bin/rm");
		fprintf(stderr, "Wrapup:  can't find 'rm' command.\n");
		exit(0);
	}
	if (!n && todo)
		printf("To take up where you left off type \"learn %s %s\".\n", sname, todo);
	printf("Bye.\n");	/* not only does this reassure user but it
				stalls for time while deleting directory */
	fflush(stdout);
	wait(0);
	exit(n);
}
