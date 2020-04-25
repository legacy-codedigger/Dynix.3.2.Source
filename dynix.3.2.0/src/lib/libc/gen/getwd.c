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

/* $Header: getwd.c 2.1 87/07/20 $ */

/*
 * getwd() returns the pathname of the current working directory. On error
 * an error message is copied to pathname and null pointer is returned.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define CURDIR		"."
#define GETWDERR(s)	strcpy(pathname, (s));
#define PARENTDIR	".."
#define PATHSEP		"/"
#define ROOTDIR		"/"

char *strcpy();
static int pathsize;			/* pathname length */

char *
getwd(pathname)
	char *pathname;
{
	char pathbuf[MAXPATHLEN];		/* temporary pathname buffer */
	char *pnptr = &pathbuf[(sizeof pathbuf)-1]; /* pathname pointer */
	char *prepend();		/* prepend dirname to pathname */
	dev_t rdev;			/* root device number */
	DIR *dirp;			/* directory stream */
	ino_t rino;			/* root inode number */
	struct direct *dir;		/* directory entry struct */
	struct stat d ,dd;		/* file status struct */

	pathsize = 0;
	*pnptr = '\0';
	stat(ROOTDIR, &d);
	rdev = d.st_dev;
	rino = d.st_ino;
	for (;;) {
		stat(CURDIR, &d);
		if (d.st_ino == rino && d.st_dev == rdev)
			break;		/* reached root directory */
		if ((dirp = opendir(PARENTDIR)) == NULL) {
			GETWDERR("getwd: can't open ..");
			goto fail;
		}
		if (chdir(PARENTDIR) < 0) {
			GETWDERR("getwd: can't chdir to ..");
			goto fail;
		}
		fstat(dirp->dd_fd, &dd);
		if (d.st_dev == dd.st_dev) {
			if (d.st_ino == dd.st_ino) {
				/* reached root directory */
				closedir(dirp);
				break;
			}
			do {
				if ((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					GETWDERR("getwd: read error in ..");
					goto fail;
				}
			} while (dir->d_ino != d.st_ino);
		} else
			do {
				if((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					GETWDERR("getwd: read error in ..");
					goto fail;
				}
				/*
				 * Do not need to follow symbolic links.
				 * So use lstat() instead of stat.
				 *
				 * NFS note:
				 * This allows one to hide nfs mounts in
				 * sub-directory from the local filesystem
				 * mount points and use symbolic links to
				 * point at the nfs mounted on directories.
				 * This setup can help prevent users using
				 * local resources from being hung up
				 * stat'ing a remote resource where the server
				 * is down. This happens when the remote
				 * resource directory entry occurs before
				 * the local one. 
				 */
				lstat(dir->d_name, &dd);
			} while(dd.st_ino != d.st_ino || dd.st_dev != d.st_dev);
		closedir(dirp);
		pnptr = prepend(PATHSEP, prepend(dir->d_name, pnptr));
	}
	if (*pnptr == '\0')		/* current dir == root dir */
		strcpy(pathname, ROOTDIR);
	else {
		strcpy(pathname, pnptr);
		if (chdir(pnptr) < 0) {
			GETWDERR("getwd: can't change back to .");
			return (NULL);
		}
	}
	return (pathname);

fail:
	chdir(prepend(CURDIR, pnptr));
	return (NULL);
}

/*
 * prepend() tacks a directory name onto the front of a pathname.
 */
static char *
prepend(dirname, pathname)
	register char *dirname;
	register char *pathname;
{
	register int i;			/* directory name size counter */

	for (i = 0; *dirname != '\0'; i++, dirname++)
		continue;
	if ((pathsize += i) < MAXPATHLEN)
		while (i-- > 0)
			*--pathname = *--dirname;
	return (pathname);
}
