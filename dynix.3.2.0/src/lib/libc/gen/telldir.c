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
static char rcsid[] = "$Header: telldir.c 2.3 90/05/30 $";
#endif

/* $Log:	telldir.c,v $
 */

#include <sys/param.h>
#include <sys/dir.h>

/*
 * return a pointer into a directory
 */
long
telldir(dirp)
	register DIR *dirp;
{
	return((dirp->dd_bbase * (dirp->dd_bsize/8)) + dirp->dd_entno);
}
