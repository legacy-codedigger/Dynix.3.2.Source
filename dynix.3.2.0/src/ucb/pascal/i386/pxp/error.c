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

#if !defined(lint)
static char rcsid[] = "$Id: error.c,v 1.1 88/09/02 11:44:17 ksb Exp $";
#endif lint

/*
 * pi - Pascal interpreter code translator
 *
 * Charles Haley, Bill Joy UCB
 * Version 1.2 January 1979
 *
 *
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "whoami.h"
#include "0.h"
#include "yy.h"

extern	int yyline;
extern	char errout;

char	errpfx	= 'E';
extern	int yyline;
/*
 * Panic is called when impossible
 * (supposedly, anyways) situations
 * are encountered.
 */
panic(s)
	char *s;
{

#if defined(DEBUG)
	fprintf(stderr, "Snark (%s) line=%d yyline=%d\n", s, line, yyline);
#endif
	Perror( "Snark in pxp", s);
	pexit(DIED);
}

extern	char *errfile;
/*
 * Error is called for
 * semantic errors and
 * prints the error and
 * a line number.
 */
error(a1, a2, a3, a4)
{
	if (errpfx == 'w' && opt('w') != 0) {
		errpfx == 'E';
		return;
	}
	if (line < 0)
		line = -line;
	yySsync();
	yysetfile(filename);
		fprintf(stderr, "%c %d - ", errpfx, line);
	fprintf(stderr, a1, a2, a3, a4);
	if (errpfx == 'E')
		eflg++;
	errpfx = 'E';
		putc('\n', stderr);
}
