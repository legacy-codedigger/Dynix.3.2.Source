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
static char rcsid[] = "$Header: closedir.c 2.1 86/04/15 $";
#endif

/* $Log:	closedir.c,v $
 */

#include <sys/param.h>
#include <sys/dir.h>

/*
 * close a directory.
 */
void
closedir(dirp)
	register DIR *dirp;
{
	close(dirp->dd_fd);
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
	free(dirp->dd_buf);
	free(dirp);
}
