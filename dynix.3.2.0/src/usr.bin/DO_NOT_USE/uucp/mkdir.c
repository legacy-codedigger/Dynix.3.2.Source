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
static char rcsid[] = "$Header: mkdir.c 1.2 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)mkdir.c	5.2 (Berkeley) 6/20/85";
#endif

#ifndef BSD4_2
#include <stdio.h>

/*
 * make a directory. Also make sure that the directory is owned
 * by the right userid
 */
mkdir(path, mode)
char *path;
int mode;
{
	int pid, status, w;

	if (pid=fork()) {
		while ((w = wait(&status)) != pid && w != -1)
			;
		(void) chmod(path, mode);
	} else {
		(void) umask(~mode);
		(void) execlp("mkdir", "mkdir", path, (char *)NULL);
		perror(path);
		_exit(1);
	}
	return status;
}
#endif !BSD4_2
