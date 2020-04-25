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
static char rcsid[] = "$Header: getpass.c 2.2 89/07/31 $";
#endif


/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)getpass.c	5.6 (Berkeley) 6/29/88";
#endif /* not lint */

#include <stdio.h>
#include <signal.h>
#include <sgtty.h>

static	struct sgttyb ttyb;
static	int flags;
static	FILE *fi;

static intfix()
{
	ttyb.sg_flags = flags;
	if (fi != NULL)
		(void) stty(fileno(fi), &ttyb);
	exit(SIGINT);
}

char *
mygetpass(prompt)
char *prompt;
{
	register char *p;
	register c;
	static char pbuf[50+1];
	int (*signal())();
	int (*sig)();

	if ((fi = fopen("/dev/tty", "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	sig = signal(SIGINT, intfix);
	(void) gtty(fileno(fi), &ttyb);
	flags = ttyb.sg_flags;
	ttyb.sg_flags &= ~ECHO;
	(void) stty(fileno(fi), &ttyb);
	fprintf(stderr, "%s", prompt); (void) fflush(stderr);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[sizeof(pbuf)-1])
			*p++ = c;
	}
	*p = '\0';
	fprintf(stderr, "\n"); (void) fflush(stderr);
	ttyb.sg_flags = flags;
	(void) stty(fileno(fi), &ttyb);
	(void) signal(SIGINT, sig);
	if (fi != stdin)
		(void) fclose(fi);
	return(pbuf);
}
