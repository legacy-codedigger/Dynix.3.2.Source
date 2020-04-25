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

/* $Header: gcvt.c 2.0 86/01/28 $
 *
 * gcvt  - Floating output conversion to
 *         pleasant-looking string.
 */

extern char *ecvt();

char *
gcvt(number, ndigit, buf)
double	number;
int	ndigit;
char	*buf;
{
	int sign, decpt;
	register char *p1, *p2 = buf;
	register int i;

	p1 = ecvt(number, ndigit, &decpt, &sign);
	if (sign)
		*p2++ = '-';
	if (decpt > ndigit || decpt <= -4) {	/* E-style */
		decpt--;
		*p2++ = *p1++;
		*p2++ = '.';
		for (i = 1; i < ndigit; i++)
			*p2++ = *p1++;
#ifdef TRIMZERO
		while (p2[-1] == '0')
			p2--;
		if (p2[-1] == '.')
			p2--;
#endif
		*p2++ = 'e';
		if (decpt < 0) {
			decpt = -decpt;
			*p2++ = '-';
		} else
			*p2++ = '+';
		for (i = 1000; i != 0; i /= 10) /* 3B or CRAY, for example */
			if (i <= decpt || i <= 10) /* force 2 digits */
				*p2++ = (decpt / i) % 10 + '0';
	} else {
		if (decpt <= 0) {
			*p2++ = '0';
			*p2++ = '.';
			while (decpt < 0) {
				decpt++;
				*p2++ = '0';
			}
		}
		for (i = 1; i <= ndigit; i++) {
			*p2++ = *p1++;
			if (i == decpt)
				*p2++ = '.';
		}
		if (ndigit < decpt) {
			while (ndigit++ < decpt)
				*p2++ = '0';
			*p2++ = '.';
		}
#ifdef TRIMZERO
		while (*--p2 == '0' && p2 > buf)
			;
		if (*p2 != '.')
			p2++;
#endif
	}
	*p2 = '\0';
	return (buf);
}
