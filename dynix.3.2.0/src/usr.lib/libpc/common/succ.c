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

/* $Header: succ.c 1.1 89/03/12 $ */
long
SUCC(value, lower, upper)
long value;
long lower;
long upper;
{
	if (value == upper) {
		ERROR("Cannot take succ of last element of a range\n");
		/*NOTREACHED*/
		return value;
	}
	value++;
	if (value < lower || value > upper) {
		ERROR("Value of %D is out of range\n", value);
		/*NOTREACHED*/
	}
	return value;
}
