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
static char rcsid[] = "$Id: error.c,v 1.1 88/09/02 11:47:59 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree_ty.h"		/* must be included for yy.h */
#include "yy.h"

char	errpfx	= 'E';
extern	int yyline;
/*
 * Panic is called when impossible
 * (supposedly, anyways) situations
 * are encountered.
 * Panic messages should be short
 * as they do not go to the message
 * file.
 */
panic(s)
	char *s;
{

#if defined(DEBUG)
	printf("Snark (%s) line=%d, yyline=%d\n", s, line, yyline);
	abort () ;	/* die horribly */
#endif
	Perror( "Snark in pi", s);
	pexit(DIED);
}

/*
 * Error is called for
 * semantic errors and
 * prints the error and
 * a line number.
 */

/*VARARGS1*/

error(a1, a2, a3, a4, a5)
	register char *a1;
{
	char errbuf[256]; 		/* was extern. why? ...pbk */
	register int i;

	if (errpfx == 'w' && opt('w') != 0) {
		errpfx = 'E';
		return;
	}
	Enocascade = FALSE;
	geterr((int) a1, errbuf);
	a1 = errbuf;
	if (line < 0)
		line = -line;
	if (opt('l'))
		yyoutline();
	yysetfile(filename);
	if (errpfx == ' ') {
		printf("  ");
		for (i = line; i >= 10; i /= 10)
			pchr( ' ' );
		printf("... ");
	} else if (Enoline)
		printf("  %c - ", errpfx);
	else
		printf("%c %d - ", errpfx, line);
	printf(a1, a2, a3, a4, a5);
	if (errpfx == 'E')
		eflg = TRUE, codeoff();
	errpfx = 'E';
	if (Eholdnl)
		Eholdnl = FALSE;
	else
		pchr( '\n' );
}

/*VARARGS1*/

cerror(a1, a2, a3, a4, a5)
    char *a1;
{

	if (Enocascade)
		return;
	setpfx(' ');
	error(a1, a2, a3, a4, a5);
}
