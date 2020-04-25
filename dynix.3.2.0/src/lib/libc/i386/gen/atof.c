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

/* $Header: atof.c 1.1 86/02/24 $
 *
 * C library - ascii to floating (atof)
 */
/*LINTLIBRARY*/
#include <ctype.h>
#include <values.h>
#include <errno.h>

extern double ldexp();

double
atof(p)
register char *p;
{
	register int c;
	int exp = 0, neg_val = 0;
	double fl_val;
	extern int errno;

	errno = 0;
	while (isspace(c = *p)) /* eat leading white space */
		p++;
	switch (c) { /* process sign */
	case '-':
		neg_val++;
	case '+': /* fall-through */
		p++;
	}
	{	/* accumulate value */
		register long high = 0, low = 0, scale = 1;
		register int decpt = 0, nzeroes = 0;

		while (isdigit(c = *p++) || c == '.' && !decpt++) {
			if (c == '.')
				continue;
			if (decpt) { /* handle trailing zeroes specially */
				if (c == '0') { /* ignore zero for now */
					nzeroes++;
					continue;
				}
				while (nzeroes > 0) { /* put zeroes back in */
					exp--;
					if (high < MAXLONG/10) {
						high *= 10;
					} else if (scale < MAXLONG/10) {
						scale *= 10;
						low *= 10;
					} else
						exp++;
					nzeroes--;
				}
				exp--; /* decr exponent if decimal pt. seen */
			}
			if (high < MAXLONG/10) {
				high *= 10;
				high += c - '0';
			} else if (scale < MAXLONG/10) {
				scale *= 10;
				low *= 10;
				low += c - '0';
			} else
				exp++;
		}
		if (!high)
			return (0.0);
		fl_val = (double)high;
		if (scale > 1)
			fl_val = (double)scale * fl_val + (double)low;
	}
	if (c == 'E' || c == 'e') { /* accumulate exponent */
		register int e_exp = 0, neg_exp = 0;

		switch (*p) { /* process sign */
		case '-':
			neg_exp++;
		case '+': /* fall-through */
		case ' ': /* many FORTRAN environments generate this! */
			p++;
		}
		if (isdigit(c = *p)) { /* found a legitimate exponent */
			do {
				/* limit outrageously large exponents */
				if (e_exp < DMAXEXP)
					e_exp = 10 * e_exp + c - '0';
				else {
					errno = ERANGE;
					return MAXDOUBLE;
				}
			} while (isdigit(c = *++p));
			if (neg_exp)
				exp -= e_exp;
			else
				exp += e_exp;
		} else {
			errno = ERANGE;
			return 0.0;
		}
	}
	/*
	 * The following computation is done in two stages,
	 * first accumulating powers of (10/8), then jamming powers of 8,
	 * to avoid underflow in situations like the following (for
	 * the DEC representation): 1.2345678901234567890e-37,
	 * where exp would be about (-37 + -18) = -55, and the
	 * value 10^(-55) can't be represented, but 1.25^(-55) can
	 * be represented, and then 8^(-55) jammed via ldexp().
	 */
	if (exp != 0) { /* apply exponent */
		extern double pow1_25[];	/* powers of 1.25 */
		register double *powptr = pow1_25, fl_exp = fl_val;

		if ((c = exp) < 0) {
			c = -c;
			fl_exp = 1.0;
		}
		if (c > DMAXEXP/2) /* outrageously large exponents */
			c = DMAXEXP/2; /* will be handled by ldexp */
		for ( ; ; powptr++) {
			/* binary representation of ints assumed; otherwise
			 * replace (& 01) by (% 2) and (>>= 1) by (/= 2) */
			if (c & 01)
				fl_exp *= *powptr;
			if ((c >>= 1) == 0)
				break;
		}
		fl_val = ldexp(exp < 0 ? fl_val/fl_exp : fl_exp, 3 * exp);
	}
	return (neg_val ? -fl_val : fl_val); /* apply sign */
}
