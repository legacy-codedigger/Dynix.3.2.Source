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

/* $Header: praliases.c 2.0 86/01/28 $ */

# include "sendmail.h"

static	char sccsid[] = "%W%	%G%";

typedef struct { char *dptr; int dsize; } datum;
datum	firstkey(), nextkey(), fetch();
char	*filename = ALIASFILE;

main(argc, argv)
	char **argv;
{
	datum content, key;

	if (argc > 2 && strcmp(argv[1], "-f") == 0)
	{
		argv++;
		filename = *++argv;
		argc -= 2;
	}

	if (dbminit(filename) < 0)
		exit(EX_OSFILE);
	argc--, argv++;
	if (argc == 0) {
		for (key = firstkey(); key.dptr; key = nextkey(key)) {
			content = fetch(key);
			printf("\n%s:%s\n", key.dptr, content.dptr);
		}
		exit(EX_OK);
	}
	while (argc) {
		key.dptr = *argv;
		key.dsize = strlen(*argv)+1;
		content = fetch(key);
		if (content.dptr == 0)
			printf("%s: No such key\n");
		else
			printf("\n%s:%s\n", key.dptr, content.dptr);
		argc--, argv++;
	}
	exit(EX_OK);
}
