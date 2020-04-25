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

#ifndef	lint
static char rcsid[] = "$Header: assert.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)assert.c	5.5 (Berkeley) 6/19/85";
#endif

#include "uucp.h"
#include <sys/time.h>
#include <errno.h>

/*LINTLIBRARY*/

/*
 *	print out assetion error
 */

assert(s1, s2, i1)
char *s1, *s2;
{
	register FILE *errlog;
	register struct tm *tp;
	extern struct tm *localtime();
	time_t clock;
	int pid;

	errlog = NULL;
	if (!Debug) {
		int savemask;
		savemask = umask(LOGMASK);
		errlog = fopen(ERRLOG, "a");
		umask(savemask);
	}
	if (errlog == NULL)
		errlog = stderr;

	pid = getpid();
	fprintf(errlog, "ASSERT ERROR (%.9s)  ", Progname);
	fprintf(errlog, "pid: %d  ", pid);
	(void) time(&clock);
	tp = localtime(&clock);
#ifdef USG
	fprintf(errlog, "(%d/%d-%2.2d:%2.2d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
#endif
#ifndef USG
	fprintf(errlog, "(%d/%d-%02d:%02d) ", tp->tm_mon + 1,
		tp->tm_mday, tp->tm_hour, tp->tm_min);
#endif
	fprintf(errlog, "%s %s (%d)\n", s1 ? s1 : "", s2 ? s2 : "", i1);
	if (errlog != stderr)
		(void) fclose(errlog);
	return;
}
