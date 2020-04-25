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
static char rcsid[] = "$Header: touch.c 2.0 86/01/28 $";
#endif

/*
 *	attempt to set the modify date of a file to the current date.
 *	if the file exists, read and write its first character.
 *	if the file doesn't exist, create it, unless -c option prevents it.
 *	if the file is read-only, -f forces chmod'ing and touch'ing.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int	dontcreate;	/* set if -c option */
int	force;		/* set if -f option */

char whoami[] = "touch";

main(argc,argv)
	int	argc;
	char	**argv;
{
	char	*argp;
	register int errors;

	dontcreate = 0;
	force = 0;
	errors = 0;
	for (argv++; **argv == '-'; argv++) {
		for (argp = &(*argv)[1]; *argp; argp++) {
			switch (*argp) {
			case 'c':
				dontcreate = 1;
				break;
			case 'f':
				force = 1;
				break;
			default:
				fprintf(stderr, "%s: bad option -%c\n",
					whoami, *argp);
				exit(1);
			}
		}
	}
	for (/*void*/; *argv; argv++) {
		errors += touch(*argv);
	}
	exit(errors);
}

touch(filename)
	char	*filename;
{
	struct stat	statbuffer;
	register int errors;

	errors = 0;
	if (stat(filename,&statbuffer) == -1) {
		if (!dontcreate) {
			errors += readwrite(filename,0);
		} else {
			fprintf(stderr, "%s: %s: does not exist\n",
				whoami, filename);
			++errors;
		}
		return (errors);
	}
	if ((statbuffer.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "%s: %s: can only touch regular files\n",
			whoami, filename);
		++errors;
		return (errors);
	}
	if (!access(filename,4|2)) {
		errors += readwrite(filename,statbuffer.st_size);
		return (errors);
	}
	if (force) {
		if (chmod(filename,0666)) {
			fprintf(stderr, "%s: %s: couldn't chmod: ",
				whoami, filename);
			++errors;
			perror("");
			return (errors);
		}
		errors += readwrite(filename,statbuffer.st_size);
		if (chmod(filename,statbuffer.st_mode)) {
			fprintf(stderr, "%s: %s: couldn't chmod back: ",
				whoami, filename);
			++errors;
			perror("");
			return (errors);
		}
	} else {
		fprintf(stderr, "%s: %s: cannot touch\n", whoami, filename);
		++errors;
		return (errors);
	}
	return (errors);
}

readwrite(filename,size)
	char	*filename;
	int	size;
{
	int	filedescriptor;
	char	first;

	if (size) {
		filedescriptor = open(filename,2);
		if (filedescriptor == -1) {
error:
			fprintf(stderr, "%s: %s: ", whoami, filename);
			perror("");
			return 1;
		}
		if (read(filedescriptor, &first, 1) != 1) {
			goto error;
		}
		if (lseek(filedescriptor,0l,0) == -1) {
			goto error;
		}
		if (write(filedescriptor, &first, 1) != 1) {
			goto error;
		}
	} else {
		filedescriptor = creat(filename,0666);
		if (filedescriptor == -1) {
			goto error;
		}
	}
	if (close(filedescriptor) == -1) {
		goto error;
	}
	return 0;
}
