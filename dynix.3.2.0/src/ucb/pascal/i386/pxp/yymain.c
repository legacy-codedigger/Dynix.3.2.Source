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
static char rcsid[] = "$Id: yymain.c,v 1.1 88/09/02 11:44:29 ksb Exp $";
#endif lint

/*
 * pi - Pascal interpreter code translator
 *
 * Charles Haley, Bill Joy UCB
 * Version 1.2 November 1978
 *
 *
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 November 1978
 */

#include "whoami.h"
#include "0.h"
#include "yy.h"

int	line = 1;

/*
 * Yymain initializes each of the utility
 * clusters and then starts the processing
 * by calling yyparse.
 */
yymain()
{

	/*
	 * Initialize the scanner
	 */
	if (bracket == 0) {
		if (getline() == -1) {
			Perror(filename, "No lines in file");
			pexit(NOSTART);
		}
	} else
		yyline = 0;

	/*
	 * Initialize the clusters
	 *
	initstring();
	 */
	inithash();
	inittree();

	/*
	 * Process the input
	 */
	yyparse();
	prttab();
	if (onefile) {
		extern int outcol;

		if (outcol)
			putchar('\n');
		flush();
		if (eflg) {
			writef(2, "File not rewritten because of errors\n");
			pexit(ERRS);
		}
		signal(1, 1);
		signal(2, 1);
		copyfile();
	}
	pexit(eflg ? ERRS : AOK);
}

copyfile()
{
	register int c;
	char buf[BUFSIZ];

	if (freopen(stdoutn, "r", stdin) == NULL) {
		perror(stdoutn);
		pexit(ERRS);
	}
	if (freopen(firstname, "w", stdout) == NULL) {
		perror(firstname);
		pexit(ERRS);
	}
	while ((c = getchar()) > 0)
		putchar(c);
	if (ferror(stdout))
		perror(stdout);
}

static
struct {
	int		magic;
	unsigned	txt_size;
	unsigned	data_size;
	unsigned	bss_size;
	unsigned	syms_size;
	unsigned	entry_point;
	unsigned	tr_size;
	unsigned	dr_size;
} header;

writef(i, cp)
{

	write(i, cp, strlen(cp));
}
