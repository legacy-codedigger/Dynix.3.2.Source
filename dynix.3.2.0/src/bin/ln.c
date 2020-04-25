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
static char rcsid[] = "$Header: ln.c 2.1 89/12/18 $";
#endif

/*
 * ln
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

struct	stat stb;
int	fflag;		/* force flag set? */
int	sflag;
int	cflag;
char	name[BUFSIZ];
char	*rindex();
extern	int errno;

main(argc, argv)
	int argc;
	register char **argv;
{
	register int i, r;

	argc--, argv++;
again:
	if (argc && strcmp(argv[0], "-f") == 0) {
		fflag++;
		argv++;
		argc--;
	}
	if (argc && strcmp(argv[0], "-s") == 0) {
		sflag++;
		argv++;
		argc--;
	}
	if (argc && strcmp(argv[0], "-c") == 0) {
		cflag++;
		argv++;
		argc--;
	}
	if (cflag) {
		if (argc != 3)
			usage();
		exit( cslinkit( argv[0], argv[1], argv[2] ) );
	}
	if (argc == 0) 
		usage();
	else if (argc == 1) {
		argv[argc] = ".";
		argc++;
	}
	if (argc > 2) {
		if (stat(argv[argc-1], &stb) < 0)
			usage();
		if ((stb.st_mode&S_IFMT) != S_IFDIR) 
			usage();
	}
	r = 0;
	for(i = 0; i < argc-1; i++)
		r |= linkit(argv[i], argv[argc-1]);
	exit(r);
}

usage()
{
	fprintf(stderr, "Usage: ln [ -s ] f1\nor: ln [ -s ] f1 f2\nln [ -s ] f1 ... fn d2\n");
	fprintf(stderr, "or: ln -c ucb=f1 att=f2 f3\n");
	exit(1);
}

int	link(), symlink();

linkit(from, to)
	char *from, *to;
{
	char *tail;
	int (*linkf)() = sflag ? symlink : link;

	/* is target a directory? */
	if (sflag == 0 && fflag == 0 && stat(from, &stb) >= 0
	    && (stb.st_mode&S_IFMT) == S_IFDIR) {
		printf("%s is a directory\n", from);
		return (1);
	}
	if (stat(to, &stb) >= 0 && (stb.st_mode&S_IFMT) == S_IFDIR) {
		tail = rindex(from, '/');
		if (tail == 0)
			tail = from;
		else
			tail++;
		sprintf(name, "%s/%s", to, tail);
		to = name;
	}
	if ((*linkf)(from, to) < 0) {
		if (errno == EEXIST)
			perror(to);
		else if (access(from, 0) < 0)
			perror(from);
		else
			perror(to);
		return (1);
	}
	return (0);
}

cslinkit(f1, f2, to)
	char *f1, *f2, *to;
{
	char *ucb, *att;

	ucb = att = NULL;
	if (strncmp(f1, "ucb=", 4) == 0) ucb = f1+4;
	if (strncmp(f2, "ucb=", 4) == 0) ucb = f2+4;
	if (strncmp(f1, "att=", 4) == 0) att = f1+4;
	if (strncmp(f2, "att=", 4) == 0) att = f2+4;
	if (att == NULL || ucb == NULL)
		usage();
	if (csymlink(ucb, att, to) < 0) {
		if (errno == EEXIST)
			perror(to);
		else if (access(ucb, 0) < 0)
			perror(ucb);
		else if (access(att, 0) < 0)
			perror(att);
		else
			perror(to);
		return(1);
	}
	return (0);
}
