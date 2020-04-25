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

/* $Header: ldexp.c 1.1 86/02/24 $
 *
 *	double ldexp (value, exp)
 *		double value;
 *		int exp;
 *
 *	Ldexp returns value * 2**exp, if that result is in range.
 *	If underflow occurs, it returns zero.  If overflow occurs,
 *	it returns a value of appropriate sign and largest double-
 *	precision magnitude.  In case of underflow or overflow,
 *	the external int "errno" is set to ERANGE.  Note that errno is
 *	not modified if no error occurs, so if you intend to test it
 *	after you use ldexp, you had better set it to something
 *	other than ERANGE first (zero is a reasonable value to use).
 */

#include <values.h>
#include <errno.h>

double
ldexp(value, exp)
register double value;
register int exp;
{
	extern int errno;
	register int new_exp;
	union dbl x;

	if (exp == 0 || value == 0.0) /* nothing to do for zero */
		return value;
	x.dd.d = value;
	new_exp = (int)x.db.ee-(DBIAS-1) + exp;		/* frexp */

	if (new_exp > MAXBEXP) {
		errno = ERANGE; 	/* overflow */
		return value < 0 ? -MAXDOUBLE : MAXDOUBLE;
	} else if (new_exp < MINBEXP) {
		errno = ERANGE;		/* underflow */
		return 0.0;
	}
	value = 0.0;	/* since optimizer doesn't recognize that next
			   statement makes x.dd.d != value .. */
	x.db.ee += exp;
	return x.dd.d;
}
