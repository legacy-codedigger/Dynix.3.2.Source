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

/* $Header: hypot.c 2.0 86/01/28 $ */

/*
 * sqrt(a^2 + b^2)
 *	(but carefully)
 */

double sqrt();
double
hypot(a,b)
double a,b;
{
	double t;
	if(a<0) a = -a;
	if(b<0) b = -b;
	if(a > b) {
		t = a;
		a = b;
		b = t;
	}
	if(b==0) return(0.);
	a /= b;
	/*
	 * pathological overflow possible
	 * in the next line.
	 */
	return(b*sqrt(1. + a*a));
}

struct	complex
{
	double	r;
	double	i;
};

double
cabs(arg)
struct complex arg;
{
	double hypot();

	return(hypot(arg.r, arg.i));
}
