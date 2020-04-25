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
static char rcsid[] = "$Header: printenv.c 2.0 86/01/28 $";
#endif

/*
 * printenv
 *
 * Bill Joy, UCB
 * February, 1979
 */

extern	char **environ;

main(argc, argv)
	int argc;
	char *argv[];
{
	register char **ep;
	int found = 0;

	argc--, argv++;
	if (environ)
		for (ep = environ; *ep; ep++)
			if (argc == 0 || prefix(argv[0], *ep)) {
				register char *cp = *ep;

				found++;
				if (argc) {
					while (*cp && *cp != '=')
						cp++;
					if (*cp == '=')
						cp++;
				}
				printf("%s\n", cp);
			}
	exit (!found);
}

prefix(cp, dp)
	char *cp, *dp;
{

	while (*cp && *dp && *cp == *dp)
		cp++, dp++;
	if (*cp == 0)
		return (*dp == '=');
	return (0);
}
