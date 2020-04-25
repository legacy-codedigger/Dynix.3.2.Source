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

/* $Header: execvp.c 2.3 90/12/18 $
 *
 *	execlp(name, arg,...,0)	(like execl, but does path search)
 *	execvp(name, argv)	(like execv, but does path search)
 */
#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#define	NULL	0

static	char shell[] =	"/bin/sh";
char	*execat(), *getenv();
extern	errno;

execlp(name, argv)
char *name, *argv;
{
	return(execvp(name, &argv));
}

execvp(name, argv)
char *name, **argv;
{
	char *pathstr;
	register char *cp;
	char fname[512];
	char *newargs[512];
	int i, fd;
	unsigned int magic;
	register unsigned etxtbsy = 1;
	register eacces = 0;

	if ((pathstr = getenv("PATH")) == NULL)
		pathstr = ":/bin:/usr/bin";
	cp = index(name, '/')? "": pathstr;

	do {
		cp = execat(cp, name, fname);
	retry:
		execv(fname, argv);
		switch(errno) {
		case ENOEXEC:
			if ((fd = open(fname, O_RDONLY)) < 0)
				continue;  /* should not happen */
			if (read(fd, &magic, sizeof(magic)) == sizeof(magic)) {
				(void) close(fd);
				switch(magic) {
#ifndef i386
				case 0x12eb:	/* i386 OMAGIC */
				case 0x22eb:	/* i386 ZMAGIC */
				case 0x32eb:	/* i386 XMAGIC */
				case 0x42eb:	/* i386 SMAGIC */
					fprintf(stderr,
				"execvp: %s: Cannot execute i386 binary\n",
						fname);
					return(-1);
					/* NOTREACHED */
					break;
#endif i386

#ifndef ns32000
				case 0x00ea:	/* ns32000 OMAGIC */
				case 0x10ea:	/* ns32000 OMAGIC */
				case 0x20ea:	/* ns32000 OMAGIC */
				case 0x30ea:	/* ns32000 OMAGIC */
					fprintf(stderr,
				"execvp: %s: Cannot execute ns32000 binary\n",
						fname);
					return(-1);
					/* NOTREACHED */
					break;
#endif ns32000
				default:
					break;
				}
			} else
				(void) close(fd);
			newargs[0] = "sh";
			newargs[1] = fname;
			for (i=1; newargs[i+1]=argv[i]; i++) {
				if (i>=254) {
					errno = E2BIG;
					return(-1);
				}
			}
			execv(shell, newargs);
			return(-1);
		case ETXTBSY:
			if (++etxtbsy > 5)
				return(-1);
			sleep(etxtbsy);
			goto retry;
		case EACCES:
			eacces++;
			break;
		case ENOMEM:
		case E2BIG:
			return(-1);
		}
	} while (cp);
	if (eacces)
		errno = EACCES;
	return(-1);
}

static char *
execat(s1, s2, si)
register char *s1, *s2;
char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}
