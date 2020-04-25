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
static char rcsid[] = "$Header: learn.c 2.0 86/01/28 $";
#endif

#include "stdio.h"
#include "lrnref.h"
#include "signal.h"

char	*direct	= "/usr/lib/learn";	/* CHANGE THIS ON YOUR SYSTEM */
int	more;
char	*level;
int	speed;
char	*sname;
char	*todo;
FILE	*incopy	= NULL;
int	didok;
int	sequence	= 1;
int	comfile	= -1;
int	status;
int	wrong;
char	*pwline;
char	*dir;
FILE	*scrin;
int	logging	= 1;	/* set to 0 to turn off logging */
int	ask;
int	again;
int	skip;
int	teed;
int	total;

main(argc,argv)
int argc;
char *argv[];
{
	extern hangup(), intrpt();
	extern char * getlogin();
	char *malloc();

	speed = 0;
	more = 1;
	pwline = getlogin();
	setbuf(stdout, malloc(BUFSIZ));
	setbuf(stderr, malloc(BUFSIZ));
	selsub(argc, argv);
	chgenv();
	signal(SIGHUP, hangup);
	signal(SIGINT, intrpt);
	while (more) {
		selunit();
		dounit();
		whatnow();
	}
	wrapup(0);
}

hangup()
{
	wrapup(1);
}

intrpt()
{
	char response[20], *p;

	signal(SIGINT, hangup);
	write(2, "\nInterrupt.\nWant to go on?  ", 28);
	p = response;
	*p = 'n';
	while (read(0, p, 1) == 1 && *p != '\n')
		p++;
	if (response[0] != 'y')
		wrapup(0);
	ungetc('\n', stdin);
	signal(SIGINT, intrpt);
}
