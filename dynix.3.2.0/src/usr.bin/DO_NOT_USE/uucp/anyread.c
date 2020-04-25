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
static char rcsid[] = "$Header: anyread.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)anyread.c	5.4 (Berkeley) 6/19/85";
#endif

#include "uucp.h"
#include <sys/stat.h>

/*LINTLIBRARY*/

/*
 *	anyread		check if anybody can read
 *	return SUCCESS ok: FAIL not ok
 */
anyread(file)
char *file;
{
	struct stat s;

	if (stat(subfile(file), &s) < 0)
		/* for security check a non existant file is readable */
		return SUCCESS;
	if (!(s.st_mode & ANYREAD))
		return FAIL;
	return SUCCESS;
}
