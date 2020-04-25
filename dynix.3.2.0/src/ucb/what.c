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
static char rcsid[] = "$Header: what.c 2.0 86/01/28 $";
#endif

#include <stdio.h>

/*
 * what
 */

char	*infile = "Standard input";

main(argc, argv)
	int argc;
	char *argv[];
{

	argc--, argv++;
	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			infile = argv[0];
			printf("%s\n", infile);
			argc--, argv++;
		}
		fseek(stdin, (long) 0, 0);
		find();
	} while (argc > 0);
	exit(0);
}

find()
{
	static char buf[BUFSIZ];
	register char *cp;
	register int c, cc;
	register char *pat;

contin:
	while ((c = getchar()) != EOF)
		if (c == '@') {
			for (pat = "(#)"; *pat; pat++)
				if ((c = getchar()) != *pat)
					goto contin;
			putchar('\t');
			while ((c = getchar()) != EOF && c && c != '"' &&
			    c != '>' && c != '\n')
				putchar(c);
			putchar('\n');
		}
}
