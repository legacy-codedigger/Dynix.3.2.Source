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
static char rcsid[] = "$Id: yyerror.c,v 1.1 88/09/02 11:44:29 ksb Exp $";
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

/*
 * Yerror prints an error
 * message and then returns
 * NIL for the tree if needed.
 * The error is flagged on the
 * current line which is printed
 * if the listing is turned off.
 *
 * As is obvious from the fooling around
 * with fout below, the Pascal system should
 * be changed to use the new library "lS".
 */
yerror(s, a1, a2, a3, a4, a5)
	char *s;
{
	register int i, j;
	static yySerrs;

	if (errpfx == 'w' && opt('w') != 0)
		return;
	yyResume = 0;
	yysync();
	putc(errpfx, stderr);
	putc(' ', stderr);
	for (i = 3; i < yyecol; i++)
		putc('-', stderr);
	fprintf(stderr, "^--- ");
/*
	if (yyecol > 60)
		fprintf(stderr,"\n\t");
*/
	fprintf(stderr, s, a1, a2, a3, a4, a5);
	putc('\n', stderr);
	if (errpfx == 'E')
		eflg++;
	errpfx = 'E';
	yySerrs++;
	if (yySerrs >= MAXSYNERR) {
		yySerrs = 0;
		yerror("Too many syntax errors - QUIT");
		pexit(ERRS);
	}
	return (0);
}

/*
 * A bracketing error message
 */
brerror(where, what)
	int where;
	char *what;
{

	if (where == 0) {
		line = yyeline;
		setpfx(' ');
		error("End matched %s on line %d", what, where);
		return;
	}
	if (where < 0)
		where = -where;
	yerror("Inserted keyword end matching %s on line %d", what, where);
}
