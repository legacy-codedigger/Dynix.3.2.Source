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

#ifndef	lint
static char rcsid[] = "$Header: chksum.c 1.2 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)chksum.c	4.2 (Berkeley) 6/19/85";
#endif

#ifndef pdp11
chksum (s, n)
register char *s;
register n;
{
	register long sum, x, t;

	sum = 0xffff;
	x = 0;
	do {
		/* Rotate left, copying bit 15 to bit 0 */
		sum <<= 1;
		if (sum & 0x10000)
			sum ^= 0x10001;
		t = sum;
		sum = (sum + (*s++ & 0377)) & 0xffff;
		x += sum ^ n;
		if (sum <= t)
			sum = (sum ^ x) & 0xffff;
	} while (--n > 0);

	return (long) (short) sum;
}
#else pdp11
chksum(s,n)
register char *s;
register n;
{
	register unsigned sum, t;
	register x;

	sum = -1;
	x = 0;

	do {
		if (sum&0x8000) {
			sum <<= 1;
			sum++;
		} else
			sum <<= 1;
		t = sum;
		sum += (unsigned)*s++ & 0377;
		x += sum^n;
		if ((sum&0xffff) <= (t&0xffff)) {
			sum ^= x;
		}
	} while (--n > 0);

	return sum & 0xffff;
}

#endif pdp11
