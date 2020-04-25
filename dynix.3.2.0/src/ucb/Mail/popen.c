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
static char *rcsid = "$Header: popen.c 2.1 87/04/02 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)popen.c	5.2 (Berkeley) 6/21/85";
#endif not lint

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#define	tst(a,b)	(*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1
static	int	popen_pid[20];

#ifndef VMUNIX
#define vfork	fork
#endif VMUNIX
#ifndef	SIGRETRO
#define	sigchild()
#endif

FILE *
popen(cmd,mode)
char	*cmd;
char	*mode;
{
	int p[2];
	register myside, hisside, pid;

	if(pipe(p) < 0)
		return NULL;
	myside = tst(p[WTR], p[RDR]);
	hisside = tst(p[RDR], p[WTR]);
	if((pid = vfork()) == 0) {
		/* myside and hisside reverse roles in child */
		sigchild();
		close(myside);
		dup2(hisside, tst(0, 1));
		close(hisside);
		execl("/bin/csh", "sh", "-c", cmd, 0);
		_exit(1);
	}
	if(pid == -1)
		return NULL;
	popen_pid[myside] = pid;
	close(hisside);
	return(fdopen(myside, mode));
}

pclose(ptr)
FILE *ptr;
{
	register f, r;
	int status, omask;
	extern int errno;

	f = fileno(ptr);
	fclose(ptr);
# ifdef VMUNIX
	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGHUP));
# endif VMUNIX
	while((r = wait(&status)) != popen_pid[f] && r != -1 && errno != EINTR)
		;
	if(r == -1)
		status = -1;
# ifdef VMUNIX
	sigsetmask(omask);
# endif VMUNIX
	return(status);
}
