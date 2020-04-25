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

/* $Header: asin.c 2.0 86/01/28 $ */

/*
	asin(arg) and acos(arg) return the arcsin, arccos,
	respectively of their arguments.

	Arctan is called after appropriate range reduction.
*/

#include	<errno.h>
int errno;
double atan();
double sqrt();
static double pio2	= 1.570796326794896619;

double
asin(arg) double arg; {

	double sign, temp;

	sign = 1.;
	if(arg <0){
		arg = -arg;
		sign = -1.;
	}

	if(arg > 1.){
		errno = EDOM;
		return(0.);
	}

	temp = sqrt(1. - arg*arg);
	if(arg > 0.7)
		temp = pio2 - atan(temp/arg);
	else
		temp = atan(arg/temp);

	return(sign*temp);
}

double
acos(arg) double arg; {

	if((arg > 1.) || (arg < -1.)){
		errno = EDOM;
		return(0.);
	}

	return(pio2 - asin(arg));
}
