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
static char rcsid[] = "$Id: yyerror.c,v 1.1 88/09/02 11:48:37 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree_ty.h"	/* must be included for yy.h */
#include "yy.h"

/*
 * Yerror prints an error
 * message and then returns
 * NIL for the tree if needed.
 * The error is flagged on the
 * current line which is printed
 * if the listing is turned off.
#if defined(PXP)
 *
 * As is obvious from the fooling around
 * with fout below, the Pascal system should
 * be changed to use the new library "lS".
#endif
 */
/*VARARGS*/
yerror(s, a1, a2, a3, a4, a5)
	char *s;
	char *a1, *a2, *a3, *a4, *a5;
{
#if defined(PC)
	char buf[256];
#endif
	register int i;
	static yySerrs;
#if defined(PXP)
	int ofout;
#endif

	if (errpfx == 'w' && opt('w') != 0) {
		errpfx = 'E';
		return;
	}
	/* no continuations allowed here */
	if (errpfx == ' ')
		errpfx = 'E';
#if defined(PXP)
	flush();
	ofout = fout[0];
	fout[0] = errout;
#endif
	yyResume = 0;
#if defined(PC)
	geterr((int) s, buf);
	s = buf;
#endif
	yysync();
	pchr(errpfx);
	pchr(' ');
	for (i = 3; i < yyecol; i++)
		pchr('-');
	printf("^--- ");
/*
	if (yyecol > 60)
		printf("\n\t");
*/
	printf(s, a1, a2, a3, a4, a5);
	pchr('\n');
	if (errpfx == 'E')
#if defined(PC)
		eflg = TRUE, codeoff();
#endif
#if defined(PXP)
		eflg = TRUE;
#endif
	errpfx = 'E';
	yySerrs++;
	if (yySerrs >= MAXSYNERR) {
		yySerrs = 0;
		yerror("Too many syntax errors - QUIT");
		pexit(ERRS);
	}
#if defined(PXP)
	flush();
	fout[0] = ofout;
	return (0);
#endif
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
		error("End matched %s on line %d", what, (char *) where);
		return;
	}
	if (where < 0)
		where = -where;
	yerror("Inserted keyword end matching %s on line %d", what, (char *) where);
}
