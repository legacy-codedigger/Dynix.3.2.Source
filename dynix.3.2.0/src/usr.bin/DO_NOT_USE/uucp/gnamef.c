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

#ifndef	lint
static char rcsid[] = "$Header: gnamef.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)gnamef.c	5.4 (Berkeley) 6/20/85";
#endif

#include "uucp.h"
#ifdef	NDIR
#include "ndir.h"
#else
#include <sys/dir.h>
#endif

/*LINTLIBRARY*/

/*
 *	get next file name from directory
 *
 *	return codes:
 *		0  -  end of directory read
 *		1  -  returned name
 */

gnamef(dirp, filename)
register DIR *dirp;
register char *filename;
{
	register struct direct *dentp;

	for (;;) {
		if ((dentp = readdir(dirp)) == NULL) {
			return 0;
		}
		if (dentp->d_ino != 0)
			break;
	}

	/* Truncate filename.  This may become a problem someday. */
	strncpy(filename, dentp->d_name, NAMESIZE-1);
	filename[NAMESIZE-1] = '\0';
	DEBUG(99,"gnamef returns %s\n",filename);
	return 1;
}
