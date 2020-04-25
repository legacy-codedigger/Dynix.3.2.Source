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
static char rcsid[] = "$Header: mkdir.c 2.2 90/01/22 $";
#endif

/*
 * make directory
 */
#include <stdio.h>

main(argc, argv)
	char *argv[];
{
	int errors = 0;

	if (argc < 2) {
		fprintf(stderr, "usage: mkdir directory ...\n");
		exit(1);
	}
	while (--argc)
	{
#ifdef SCGACCT
		++argv;
		if (chk_access(*argv) < 0) {
			errors++;
			continue;       /* go to next argument */
		}
		if (mkdir(*argv, 0777) < 0) {
#else SCGACCT
		if (mkdir(*++argv, 0777) < 0) {
#endif SCGACCT
			fprintf(stderr, "mkdir: ");
			perror(*argv);
			errors++;
		}
#ifdef SCGACCT                /* accounting equipment */
		/* undo complications caused by needing to run setuid root */
		chown(*argv, getuid(), getgid());
		diracct(*argv);
#endif
	}
	exit(errors != 0);
}

#ifdef SCGACCT                /* accounting equipment */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <local/scgacct.h>

extern int errno;

chk_access(d)
char *d;
{
	char pname[MAXPATHLEN];
	register i, slash = 0;

	pname[0] = '\0';
	for(i = 0; d[i]; ++i)
		if(d[i] == '/')
			slash = i + 1;
	if(slash)
		strncpy(pname, d, slash);
	strcpy(pname+slash, ".");
	if (access(pname, W_OK)) {
		perror("mkdir");
		return(-1);
	}
	return(0);
}

diracct(d)
char *d;
{
	int fd;
	ino_t inode;
	struct stat statb;
	register char acctid[2], buf[MAXPATHLEN];

	if ( stat(d, &statb) != 0 ) {
		fprintf(stderr, "cannot stat:");
		perror(d);
		return(-1);
	}
        inode = statb.st_ino;

/* find root of this mounted file system */
	strcpy(buf,".");
	if ( stat(buf, &statb) != 0 ) {
		fprintf(stderr, "cannot stat:");
		perror(buf);
		return(-1);
	}

	while (statb.st_ino != ROOTINO) {
		strcat(buf,"/..");
		if (stat(buf,&statb) < 0) {
			fprintf(stderr, "%s: can't move up tree\n",d);
			return;
		}
	}
/* build pathname to diracct */
	strcat(buf,"/diracct");

/* write accounting record into diracct */

	if ((fd = open(buf,1)) >= 0)
	{	acctid[0] = getacct();
		lseek(fd,inode,0);
		write(fd,acctid,1);
		close(fd);
	}
	return(0);
}
#endif


