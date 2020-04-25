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
static char rcsid[] = "$Header: error.c 2.1 90/12/19 $";
#endif

#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	<sys/signal.h>
#include	"defs.h"


/* ========	error handling	======== */

exitset()
{
	assnum(&exitadr,exitval);
}

sigchk()
{
	/* Find out if it is time to go away.
	 * `trapnote' is set to SIGSET when fault is seen and
	 * no trap has been set.
	 */
	IF trapnote&SIGSET
	THEN	exitsh(SIGFAIL);
	FI
}

failed(s1,s2)
	STRING	s1, s2;
{
	prp(); prs(s1); 
	IF s2
	THEN	prs(colon); prs(s2);
	FI
	newline(); exitsh(ERROR);
}

error(s)
	STRING	s;
{
	failed(s,NIL);
}

exitsh(xno)
	INT	xno;
{
	/* Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exitval=xno;
	IF (flags & (forked|errflg|ttyflg)) != ttyflg
	THEN	done();
	ELSE	clearup();
		longjmp(errshell,1);
	FI
}

done()
{
	REG STRING	t;
	IF t=trapcom[0]
	THEN	trapcom[0]=0; /*should free but not long */
		signal(SIGINT,SIG_DFL);
		execexp(t,0);
	FI
	rmtemp(0);
	exit(exitval);
}

rmtemp(base)
	IOPTR		base;
{
	WHILE iotemp>base
	DO  unlink(iotemp->ioname);
	    iotemp=iotemp->iolst;
	OD
}
