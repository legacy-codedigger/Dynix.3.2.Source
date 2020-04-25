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

#ifndef	lint
static char rcsid[] = "$Header: cfgets.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)cfgets.c	5.3 (Berkeley) 6/19/85";
#endif

/*
 * get nonblank, non-comment, (possibly continued) line. Alan S. Watt 
 */

#include <stdio.h>
#define COMMENT		'#'
#define CONTINUE	'\\'
#define EOLN		'\n'
#define EOS		'\0'

/*LINTLIBRARY*/

char *
cfgets(buf, siz, fil)
register char *buf;
int siz;
FILE *fil;
{
	register char *s;
	register i, c, len;
	char *fgets();

	for (i=0,s=buf; i = (fgets(s, siz-i, fil) != NULL); i = s - buf) {

		/* get last character of line */
		c = s[len = (strlen(s) - 1)];

		/* skip comments; make sure end of comment line seen */
		if (*s == COMMENT) {
			while (c != EOLN && c != EOF)
				c = getc(fil);
			*s = EOS;
		}

		/* skip blank lines */
		else if (*s != EOLN) {
			s += len;

			/* continue lines ending with CONTINUE */
			if (c != EOLN || *--s != CONTINUE)
				break;
		}
	}
	
	return i ? buf : NULL;
}

#ifdef TEST
main()
{
	char buf[512];

	while (cfgets(buf, sizeof buf, stdin))
		fputs(buf, stdout);
}
#endif TEST
