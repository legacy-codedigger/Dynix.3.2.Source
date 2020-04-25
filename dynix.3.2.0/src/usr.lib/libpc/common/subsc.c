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

/* $Header: subsc.c 1.1 89/03/12 $ */
char ESUBSC[] = "Subscript value of %D is out of range\n";

long
SUBSC(i, lower, upper)
long i, lower, upper;
{
	if (i < lower || i > upper) {
		ERROR(ESUBSC, i);
		/*NOTREACHED*/
	}
	return i;
}
