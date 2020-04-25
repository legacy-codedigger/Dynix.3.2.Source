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

/* $Header: expo.c 1.1 89/03/12 $ */
/*
 *	pull exponent off real number srting
 */
long
EXPO(value)
double	value;
{
	register int retval;
	register char *cp;
	char sign, buf[30];
	extern char *index();

	if (value == 0.0)
		return 0;
	sprintf(buf, "%.1e", value);
	cp = index(buf, 'e') + 1;
	sign = *cp++;
	retval = 0;
	while (*cp) {
		retval = retval * 10 + *cp++ - '0';
	}
	return sign == '-' ? -retval : retval;
}
