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
static char rcsid[] = "$Header: opendir.c 2.3 90/12/19 $";
#endif

/* $Log:	opendir.c,v $
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>

extern int errno;
/*
 * open a directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	register int fd;
	struct stat sb;
	extern char *malloc();

	if ((fd = open(name, 0)) == -1)
		return (NULL);
	if (fstat(fd, &sb) == -1) {
		close(fd);
		return (NULL);
	}
	if ((sb.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		close(fd);
		return (NULL);
	}
	if (((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) ||
	    ((dirp->dd_buf = malloc((int)sb.st_blksize)) == NULL)) {
		if (dirp) {
			if (dirp->dd_buf) {
				free(dirp->dd_buf);
			}
			free(dirp);
		}
		close(fd);
		return (NULL);
	}
	dirp->dd_bsize = sb.st_blksize;
	dirp->dd_bbase = 0;
	dirp->dd_entno = 0;
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	return (dirp);
}

DIR *
extend_fd_opendir(name)
	char *name;
{
	DIR *dir;
	int sz;

	dir = opendir(name);
	if (dir == NULL && errno == EMFILE) {
		/* 
		 * Attempt to extend file table size
		 */
		sz = getdtablesize();
		setdtablesize(sz*2);
		return(opendir(name));
	} else {
		return(dir);
	}
}
