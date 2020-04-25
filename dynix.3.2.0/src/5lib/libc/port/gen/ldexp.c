#include <math.h>

#if ns32000 || i386
/* $Header: ldexp.c 2.2 86/07/26 $
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
		return value < 0 ? -HUGE : HUGE;	/* SVID */
	} else if (new_exp < MINBEXP) {
		errno = ERANGE;		/* underflow */
		return 0.0;
	}
	value = 0.0;	/* since optimizer doesn't recognize that next
			   statement makes x.dd.d != value .. */
	x.db.ee += exp;
	return x.dd.d;
}
#else
/*	@(#)ldexp.c	2.7	*/
/*LINTLIBRARY*/
/*
 *	double ldexp (value, exp)
 *		double value;
 *		int exp;
 *
 *	Ldexp returns value * 2**exp, if that result is in range.
 *	If underflow occurs, it returns zero.  If overflow occurs,
 *	it returns a value of appropriate sign and largest single-
 *	precision magnitude.  In case of underflow or overflow,
 *	the external int "errno" is set to ERANGE.  Note that errno is
 *	not modified if no error occurs, so if you intend to test it
 *	after you use ldexp, you had better set it to something
 *	other than ERANGE first (zero is a reasonable value to use).
 */

#include <values.h>
#include <errno.h>
/* Largest signed long int power of 2 */
#define MAXSHIFT	(BITSPERBYTE * sizeof(long) - 2)

extern double frexp();

double
ldexp(value, exp)
register double value;
register int exp;
{
	int old_exp;

	if (exp == 0 || value == 0.0) /* nothing to do for zero */
		return (value);
#if	!(pdp11 || u3b5)	/* pdp11 "cc" can't handle cast of
				   double to void on pdp11 or 3b5 */
	(void)
#endif
	frexp(value, &old_exp);
	if (exp > 0) {
		if (exp + old_exp > MAXBEXP) { /* overflow */
			errno = ERANGE;
			return (value < 0 ? -MAXFLOAT : MAXFLOAT);
		}
		for ( ; exp > MAXSHIFT; exp -= MAXSHIFT)
			value *= (1L << MAXSHIFT);
		return (value * (1L << exp));
	}
	if (exp + old_exp < MINBEXP) { /* underflow */
		errno = ERANGE;
		return (0.0);
	}
	for ( ; exp < -MAXSHIFT; exp += MAXSHIFT)
		value *= 1.0/(1L << MAXSHIFT); /* mult faster than div */
	return (value / (1L << -exp));
}
#endif
