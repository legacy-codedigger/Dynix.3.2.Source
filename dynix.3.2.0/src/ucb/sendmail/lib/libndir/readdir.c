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

/* $Header: readdir.c 2.0 86/01/28 $ */

/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)readdir.c 4.3 8/8/82";

#include <sys/types.h>
#include <dir.h>

/*
 * read an old stlye directory entry and present it as a new one
 */
#define	ODIRSIZ	14

struct	olddirect {
	ino_t	od_ino;
	char	od_name[ODIRSIZ];
};

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register DIR *dirp;
{
	register struct olddirect *dp;
	static struct direct dir;

	for (;;) {
		if (dirp->dd_loc == 0) {
			dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, 
			    DIRBLKSIZ);
			if (dirp->dd_size <= 0)
				return NULL;
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct olddirect *)(dirp->dd_buf + dirp->dd_loc);
		dirp->dd_loc += sizeof(struct olddirect);
		if (dp->od_ino == 0)
			continue;
		dir.d_ino = dp->od_ino;
		strncpy(dir.d_name, dp->od_name, ODIRSIZ);
		dir.d_name[ODIRSIZ] = '\0'; /* insure null termination */
		dir.d_namlen = strlen(dir.d_name);
		dir.d_reclen = DIRBLKSIZ;
		return (&dir);
	}
}
