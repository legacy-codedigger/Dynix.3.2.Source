/*	@(#)frexp.c	1.5	*/
/*LINTLIBRARY*/
/*
 * frexp(value, eptr)
 * returns a double x such that x = 0 or 0.5 <= |x| < 1.0
 * and stores an integer n such that value = x * 2 ** n
 * indirectly through eptr.
 *
 */
#include <nan.h>
#if ns32000 || i386
# include <values.h>
#endif

double
frexp(value, eptr)
double value; /* don't declare register, because of KILLNan! */
register int *eptr;
{
#if ns32000 || i386
	union dbl x;

	KILLNaN(value); /* raise exception on Not-a-Number (3b/ns32000 only) */
	if (value == 0.0) {
		*eptr = 0;	/* nothing to do for zero */
		return (value);
	}
	x.dd.d = value;
	*eptr = (int)x.db.ee-(DBIAS-1);
	x.db.ee = DBIAS-1;	/* 0.5 <= x < 1.0 */
	return x.dd.d;
#else
	register double absvalue;

	KILLNaN(value); /* raise exception on Not-a-Number (3b/ns32000 only) */
	*eptr = 0;
	if (value == 0.0) /* nothing to do for zero */
		return (value);
	absvalue = (value > 0.0) ? value : -value;
	for ( ; absvalue >= 1.0; absvalue *= 0.5)
		++*eptr;
	for ( ; absvalue < 0.5; absvalue += absvalue)
		--*eptr;
	return (value > 0.0 ? absvalue : -absvalue);
#endif
}
