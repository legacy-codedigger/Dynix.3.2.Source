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

#ifdef	RCS
static	char rcsid[] = "$Header: stop.c 2.1 86/02/27 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"

_stop(s)
	char *s;
{
	register int i;

	for (i = 0; i < NFILES; i++)
		if (iob[i].i_flgs != 0)
			close(i);
	if (s) 
		printf("%s\n", s);
	_rtt();
}
