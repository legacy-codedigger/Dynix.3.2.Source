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

/* $Header: atan.c 1.1 89/03/12 $ */
#include <math.h>
extern int errno;

double
ATAN(value)
double	value;
{
	register double result;

	errno = 0;
	result = atan(value);
	if (errno != 0) {
		ERROR("Argument %e is out of the domain of atan\n", value);
		/*NOTREACHED*/
	}
	return result;
}
