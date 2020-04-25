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
static char rcsid[] = "$Header: seekdir.c 2.2 90/05/30 $";
#endif

/* $Log:	seekdir.c,v $
 */

#include <sys/param.h>
#include <sys/dir.h>

/*
 * seek to an entry in a directory.
 * Only values returned by "telldir" should be passed to seekdir.
 */
void
seekdir(dirp, tell)
	register DIR *dirp;
	register long tell;
{
	register struct direct *dp;
	register long entno;
	long base;
	long curloc;
	extern long lseek();

	curloc = telldir(dirp);
	if (curloc == tell)
		return;
	base = tell / (dirp->dd_bsize/8);
	entno = tell % (dirp->dd_bsize/8);
	(void) lseek(dirp->dd_fd, base, 0);
	dirp->dd_loc = 0;
	dirp->dd_entno = 0;
	while (dirp->dd_entno < entno) {
		dp = readdir(dirp);
		if (dp == NULL)
			return;
	}
}
