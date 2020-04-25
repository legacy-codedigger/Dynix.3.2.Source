/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred. */

#ifndef lint
static char rcsid[] = "$Header: stdio.c 2.4 1991/04/19 21:01:57 $";
#endif

#include <stdio.h>
#include <signal.h>
#include <errno.h>

#undef bit
#define	bit(a)	(1<<(a))
#undef mask
#define mask(s) (1<<((s)-1))

extern FILE *output;
extern int errno;

/*
 * Our special interface to printf routines
 * so we can catch output if to a popen filter.
 */

printf(fmt, args)
char *fmt;
{
	int BIT;
	char localbuf[BUFSIZ];

	if (output->_flag & _IONBF) {
		BIT = bit(output - _iob);
		setbuf(output, localbuf);
		_doprnt(fmt, &args, output);
		fflush(output);
		setbuf(output, NULL);
	} else
		_doprnt(fmt, &args, output);
	fflush(stdout);
	return(ferror(output)? EOF: 0);
}

/*
 * Our own copy of popen here so we can use
 * vfork.
 */

#define	tst(a,b)	(*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1
static	int	popen_pid[_NFILE];

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
	fflush(stdout);
	fflush(output);
	if((pid = fork()) == 0) {
		/* myside and hisside reverse roles in child */
		close(myside);
		if (hisside != tst(0, 1)) {
			dup2(hisside, tst(0, 1));
			close(hisside);
		}
		if (*mode == 'w') {
			/* 
			 * arrange for output of pipe to the current
			 * output (stdout or filter).
			 */
			dup2(output->_file,1);
		}
		execl("/bin/sh", "sh", "-c", cmd, 0);
		_exit(1);
	}
	if(pid == -1) {
		close(myside);
		close(hisside);
		return NULL;
	}
	popen_pid[myside] = pid;
	close(hisside);
	return(fdopen(myside, mode));
}

pclose(ptr)
FILE *ptr;
{
	register f, r;
	int status, omask;

	f = fileno(ptr);
	fclose(ptr);
	omask = sigblock(mask(SIGINT)|mask(SIGQUIT)|mask(SIGHUP));
	while((r = wait(&status)) != popen_pid[f] && r != -1 && errno != EINTR)
		/* do nothing */;
	if(r == -1)
		status = -1;
	sigsetmask(omask);
	return(status);
}
