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
static char rcsid[] = "$Header: main.c 2.1 87/02/06 $";
#endif

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

/*
 * Config builds a set of files for building a UNIX
 * system given a description of the desired system.
 */
main(argc, argv)
	int argc;
	char **argv;
{

again:
	if (argc > 1 && eq("-p", argv[1])) {
		printf("Sorry, profiling not supported\n");
		/* profiling++; */
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && eq("-src", argv[1])) {
		srcconfig++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && eq("-oldfw", argv[1])) {
		oldfw++;
		argc--, argv++;
		goto again;
	}
	if (argc != 2) {
		fprintf(stderr, "usage: config [ -p ] [ -src ] sysname\n");
		exit(1);
	}
	PREFIX = argv[1];
	if (freopen("controllers.balance", "r", stdin) == NULL) {
		perror("controllers.balance");
		exit(2);
	}
	dtab = NULL;
	confp = &conf_list;
	if (yyparse())
		exit(3);

	conf();			/* build conf.c */
	balance_ioconf();
	makefile();			/* build Makefile */
	swapconf();			/* swap config files */
	printf("Don't forget to run \"make depend\"\n");
}

/*
 * get_word
 *	returns EOF on end of file
 *	NULL on end of line
 *	pointer to the word otherwise
 */
char *
get_word(fp)
	register FILE *fp;
{
	static char line[80];
	register int ch;
	register char *cp;

	while ((ch = getc(fp)) != EOF)
		if (ch != ' ' && ch != '\t')
			break;
	if (ch == EOF)
		return ((char *)EOF);
	if (ch == '\n')
		return (NULL);
	cp = line;
	*cp++ = ch;
	while ((ch = getc(fp)) != EOF) {
		if (isspace(ch))
			break;
		*cp++ = ch;
	}
	*cp = 0;
	if (ch == EOF)
		return ((char *)EOF);
	(void) ungetc(ch, fp);
	return (line);
}

/*
 * prepend the path to a filename
 */
char *
path(file)
	char *file;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(PREFIX)+strlen(file)+5));
	(void) strcpy(cp, "../");
	(void) strcat(cp, PREFIX);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

int first_time = 0;

yywrap()
{
	if (first_time == 0) {
		if (freopen(PREFIX, "r", stdin) == NULL) {
			perror(PREFIX);
			exit(2);
		}
		first_time = 1;
		return(0);
	}
	return(1);
}
