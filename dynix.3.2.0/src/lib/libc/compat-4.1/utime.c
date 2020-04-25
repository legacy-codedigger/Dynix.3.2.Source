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

/* $Header: utime.c 2.0 86/01/28 $ */

#include <sys/time.h>
/*
 * Backwards compatible utime.
 */

utime(name, otv)
	char *name;
	int otv[];
{
	struct timeval tv[2];

	tv[0].tv_sec = otv[0]; tv[0].tv_usec = 0;
	tv[1].tv_sec = otv[1]; tv[1].tv_usec = 0;
	return (utimes(name, tv));
}
