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

/* $Header: frexp.c 1.1 86/02/24 $
 *
 * frexp(value, eptr)
 * returns a double x such that x = 0 or 0.5 <= |x| < 1.0
 * and stores an integer n such that value = x * 2 ** n
 * indirectly through eptr.
 */

#include <values.h>

double
frexp(value, eptr)
register double value;
register int *eptr;
{
	union dbl x;

	if (value == 0.0) {
		*eptr = 0;	/* nothing to do for zero */
		return (value);
	}
	x.dd.d = value;
	*eptr = (int)x.db.ee-(DBIAS-1);
	x.db.ee = DBIAS-1;	/* 0.5 <= x < 1.0 */
	return x.dd.d;
}
