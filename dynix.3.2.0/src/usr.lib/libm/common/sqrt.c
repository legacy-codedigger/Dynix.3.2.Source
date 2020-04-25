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

/* $Header: sqrt.c 2.0 86/01/28 $ */

/*
	sqrt returns the square root of its floating
	point argument. Newton's method.

	calls frexp
*/

#include <errno.h>

int errno;
double frexp();

double
sqrt(arg)
double arg;
{
	double x, temp;
	int exp;
	int i;

	if(arg <= 0.) {
		if(arg < 0.)
			errno = EDOM;
		return(0.);
	}
	x = frexp(arg,&exp);
	while(x < 0.5) {
		x *= 2;
		exp--;
	}
	/*
	 * NOTE
	 * this wont work on 1's comp
	 */
	if(exp & 1) {
		x *= 2;
		exp--;
	}
	temp = 0.5*(1.0+x);

	while(exp > 60) {
		temp *= (1L<<30);
		exp -= 60;
	}
	while(exp < -60) {
		temp /= (1L<<30);
		exp += 60;
	}
	if(exp >= 0)
		temp *= 1L << (exp/2);
	else
		temp /= 1L << (-exp/2);
	for(i=0; i<=4; i++)
		temp = 0.5*(temp + arg/temp);
	return(temp);
}
