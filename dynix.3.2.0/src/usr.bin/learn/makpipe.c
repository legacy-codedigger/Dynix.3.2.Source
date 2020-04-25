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
static char sccsid[] = "@(#)makpipe.c	4.2	(Berkeley)	4/25/83";
#endif not lint

#include "stdio.h"

makpipe()
{
	int f[2];

	pipe(f);
	if (fork()==0) {
		close(f[1]);
		close(0);
		dup(f[0]);
		close(f[0]);
#if defined(vax) || defined(ns32000) || defined(i386)
		execl ("/bin/sh", "sh", "-i", 0);
		execl ("/usr/ucb/bin/sh", "sh", "-i", 0);
#else
		execlp("/bin/csh", "csh", "-if", 0);
		/*execl ("/usr/ucb/bin/csh", "csh", "-if", 0);*/
#endif
		write(2, "Exec error\n", 11);
	}
	close(f[0]);
	sleep(2);	/* so shell won't eat up too much input */
	return(f[1]);
}
