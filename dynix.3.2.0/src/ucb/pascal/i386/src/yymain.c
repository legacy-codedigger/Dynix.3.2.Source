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
static char rcsid[] = "$Id: yymain.c,v 1.1 88/09/02 11:48:40 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree_ty.h"	/* must be included for yy.h */
#include "yy.h"
#include <a.out.h>
#include "objfmt.h"
#include <signal.h>
#include "config.h"

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
#if defined(PXP)
	if (bracket == 0) {
#endif
		if (getline() == -1) {
			Perror(filename, "No lines in file");
			pexit(NOSTART);
		}
#if defined(PXP)
	} else
		yyline = 0;
#endif
	line = 1;
	errpfx = 'E';
	/*
	 * Initialize the clusters
	 *
	initstring();
	 */
	inithash();
	inittree();
#if defined(PC)
	initnl();
#endif

	/*
	 * Process the input
	 */
	yyparse();
#if defined(PC)
#if defined(DEBUG)
	dumpnl(NLNIL);
#endif
#endif

#if defined(PXP)
	prttab();
	if (onefile) {
		extern int outcol;

		if (outcol)
			pchr('\n');
		flush();
		if (eflg) {
			writef(2, "File not rewritten because of errors\n");
			pexit(ERRS);
		}
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGINT, SIG_IGN);
		copyfile();
	}
#endif
	pexit(eflg ? ERRS : AOK);
}

#if defined(PXP)
copyfile()
{
	extern int fout[];
	register int c;

	(void) close(1);
	if (creat(firstname, 0644) != 1) {
		perror(firstname);
		pexit(ERRS);
	}
	(void) lseek(fout[0], 0l, 0);
	while ((c = read(fout[0], &fout[3], 512)) > 0) {
		if (write(1, &fout[3], c) != c) {
			perror(firstname);
			pexit(ERRS);
		}
	}
}
#endif


#if defined(PXP)
writef(i, cp)
{

	write(i, cp, strlen(cp));
}
#endif
