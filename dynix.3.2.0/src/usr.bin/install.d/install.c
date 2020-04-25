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

/*
 * $Header: install.c 2.4 90/09/14 $
 */

/*
 * C version of install
 */

#ifdef CCS
#include "/usr/include/sys/types.h"
#include "/usr/include/sys/stat.h"
#include "/usr/include/signal.h"
#include "/usr/include/sys/file.h"
#include "/usr/include/errno.h"
#include "/usr/include/stdio.h"
#include "/usr/include/grp.h"
#include "/usr/include/pwd.h"
#include "/usr/include/ctype.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/file.h>
#include <errno.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <ctype.h>
#endif

#include <strings.h>
#include <a.out.h>
#include <stdio.h>

#define	BSIZE	8192	/* copy files in this size chunks */

struct stat stdest, stsrc;
char *whoiam;
int stripf = 0, copyf = 0;	/* default is no copyf, no strip */
int owner = 0, group = 10;	/* default ownership (root, daemon) */
char *mode = "755";		/* default mode */
int dirflg = 0;			/* default destination is a file not a dir */
extern int errno;
int status = 0;			/* exit status */

main(argc, argv)
int argc;
char *argv[];
{
	struct group *getgrnam(), *grp;
	struct passwd *getpwnam(), *pwd;
	int c, err = 0, i; 
	extern int optind;
	extern char *optarg;

	whoiam = argv[0];
	while ((c = getopt(argc, argv, "cm:o:g:s")) != EOF) {
		switch (c) {
		case 'c':
			copyf++;
			break;
		case 'm':
			mode = optarg;
			break;
		case 'o':
			if( isdecimal(optarg) ) {
				owner = atoi(optarg);
			} else if((pwd=getpwnam(optarg)) == NULL) {
				printf("%s: unknown user id: %s\n",
					whoiam, optarg);
				exit(1);
			} else {
				owner = pwd->pw_uid;
			}
			break;
		case 'g':
			if( isdecimal(optarg) ) {
				owner = atoi(optarg);
			} else if ((grp  = getgrnam(optarg)) == NULL) {
				printf("%s: unknown group id: %s\n",
					whoiam, optarg);
				exit(1);
			} else {
				group = grp->gr_gid;
			}
			break;
		case 's':
			stripf++;
			break;
		case '?':
			err++;
			break;
		}
	}
	if (err) {
		fprintf(stderr, "install [ -c ] [ -m mode ] [ -o owner ] [ -g group ] [ -s ] file[s] destination\n");
		exit(1);
	}
	if (optind == argc-1) {
		fprintf(stderr, "%s: no destination specified\n", whoiam);
		exit(1);
	}
	if ((stat(argv[argc-1], &stdest) == 0)
		&& ((stdest.st_mode &S_IFMT) == S_IFDIR))
		dirflg++; /* Note if dest file exists and is a directory */
	if (argc - optind > 2) {
		if( dirflg ) {
			for( i = optind; i < argc -1; i++ )
				install(argv[i], argv[argc-1]);
			exit(status);
		}
		fprintf(stderr, "%s: too many files specified OR non-directory target specified ->", whoiam);
		for (i = optind; i < argc; i++)
			fprintf(stderr, " %s", argv[i]);
		fprintf(stderr, "\n");
		exit(1);
	}
	install(argv[optind], argv[optind+1]);
}

install(src, dest)
register char *src, *dest;
{
	int fdsrc;
	char *Eargv[20];
	char file[1024];	/* max file size */

	if ((fdsrc = open(src, 0)) < 0) {
		fprintf(stderr, "%s: can't open %s\n", whoiam, src);
		status = 1;
		return;
	}
	if (fstat(fdsrc, &stsrc) < 0) {
		fprintf(stderr, "%s: can't fstat %s\n", whoiam, src);
		close(fdsrc);
		status = 1;
		return;
	}
	if( dirflg ) {		/* concatenate src file to dest dir */
		strcpy(file, dest);
		strcat(file, "/");
		strcat(file, src);
		dest = file;
	}
	if (stat(dest, &stdest) >= 0) {
		if(stsrc.st_dev==stdest.st_dev && stsrc.st_ino==stdest.st_ino) {
			fprintf(stderr, "%s: can't move %s onto itself\n",
				whoiam, src);
			close(fdsrc);
			status = 1;
			return;
		}
	}
	unlink(dest);
	errno = 0;		/* Make sure errno is 0 so "if" will work */
	if (copyf || (rename(src, dest) != 0 && errno == EXDEV)) {
		if ( copyf && ((stsrc.st_mode & S_IFMT) == S_IFREG) ) {
			copy(fdsrc, src, dest);
		} else {
			Eargv[0] = copyf ? "/bin/cp" : "/bin/mv";
			Eargv[1] = src;
			Eargv[2] = dest;
			Eargv[3] = 0;
			status = (callsys(Eargv[0],Eargv) != 0);
		}
	} else if (errno != 0) {
		fprintf(stderr, "%s: can't install %s : ", whoiam, dest);
		perror("");
		close(fdsrc);
		status = 1;
		return;
	}
	close(fdsrc);
	if (stripf) {
		strip(dest);
	}
	chown(dest, owner, group);

	if( isoctal(mode) ) {
			chmod(dest, octal(mode));
	} else {
		Eargv[0] = "/bin/chmod";
		Eargv[1] = mode ? mode : "755";
		Eargv[2] = dest;
		Eargv[3] = 0;
		status = (callsys(Eargv[0],Eargv) != 0);
	}
}

isdecimal(s)
register char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}

isoctal(s)
register char *s;
{
	register c;

	while ((c = *s++) >= '0' && c <= '7');
	return(c == 0);
}

octal(s)
register char *s;
{
	register c, n;

	n = 0;
	while ((c = *s++) >= '0' && c <= '7')
		n = (n << 3) + (c - '0');
	return(n);
}

callsys(f, v)
	char *f, **v;
{
	register int t;
	int status;

	t = vfork();
	if (t == -1) {
		fprintf(stderr, "%s: No more processes\n", whoiam);
		return (100);
	}
	if (t == 0) {
		execvp(f, v);
		fprintf(stderr, "%s: can't exec %s : ", whoiam, f);
		perror("");
		(void) fflush(stdout);
		_exit(100);
	}
	while (t != wait(&status))
		;
	if ((t=(status&0377)) != 0 && t!=14) {
	    if (t!=2) {
		fprintf(stderr, "%s: Fatal error in %s: status = %d: ",
			whoiam, f, status);
		perror("");
	    }
	}
	return((status>>8) & 0377);
}

copy(fdsrc, src, dest)
int fdsrc;
char *dest, *src;
{
	int fdest, n;
	char buf[BSIZE];

	if ((fdest = creat(dest, 0666))< 0) {
		fprintf(stderr, "%s: can't creat %s\n", whoiam, dest);
		status = 1;
		return;
	}
	for (;;) {
		n = read(fdsrc, buf, BSIZE);
		if (n == 0)
			break;
		if (n < 0) {
			fprintf(stderr, "%s: read error %s\n", whoiam, src);
			close(fdest);
			status = 1;
			return;
		}
		if (write(fdest, buf, n) != n) {
			fprintf(stderr, "%s: write error %s\n", whoiam, src);
			close(fdest);
			status = 1;
			return;
		}
	}
	close(fdest);
}

strip(dest)
char *dest;
{
	struct	exec head;
	int size, f;

	if ((f = open(dest, 2)) < 0) {
		fprintf(stderr, "%s: can't re-open %s\n", whoiam, dest);
		status = 1;
		return;
	}
	if (read(f, &head, sizeof (head)) < 0 || N_BADMAG(head)) {
		fprintf(stderr, "%s: %s not in a.out format\n", whoiam, dest);
		close(f);
		status = 1;
		return;
	}
	if ((head.a_syms == 0) && (head.a_trsize == 0) && (head.a_drsize ==0)
	&&  (head.a_shdrsize ==0)) {
		fprintf(stderr, "%s: %s already stripped\n", whoiam, dest);
		close(f);
		return;
	}
	size = N_MINSIZ(head);
	head.a_syms = head.a_trsize = head.a_drsize = head.a_shdrsize = 0;
	if (ftruncate(f, size) < 0) {
		fprintf(stderr,"%: can't truncate %s\n", whoiam, dest);
		close(f);
		status = 1;
		return;
	}
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, &head, sizeof (head));
	close(f);
}
