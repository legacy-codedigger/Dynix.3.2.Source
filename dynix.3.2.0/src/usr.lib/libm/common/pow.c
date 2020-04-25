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

/* $Header: pow.c 2.0 86/01/28 $ */

/*
	computes a^b.
	uses log and exp
*/

#include	<errno.h>
int errno;
double log(), exp();

double
pow(arg1,arg2)
double arg1, arg2;
{
	double temp;
	long l;

#ifdef vax
	asm("	bispsw	$0xe0");
#endif
	if(arg1 <= 0.) {
		if(arg1 == 0.) {
			if(arg2 <= 0.)
				goto domain;
			return(0.);
		}
		l = arg2;
		if(l != arg2)
			goto domain;
		temp = exp(arg2 * log(-arg1));
		if(l & 1)
			temp = -temp;
		return(temp);
	}
	return(exp(arg2 * log(arg1)));

domain:
	errno = EDOM;
	return(0.);
}
