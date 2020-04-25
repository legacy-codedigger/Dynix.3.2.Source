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

/* $Header: misc.c 2.1 86/02/16 $ */

/*LINTLIBRARY*/
/*
 * Same as C library version but returns the ptr to null and end of s1
 */
char *
_strcpy( s1, s2 )
	register char *s1, *s2;
{
	while (*s1 = *s2) {
		s1++;
		s2++;
	}
	return(s1);
}

static char digits[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};


/*
 * convert a signed int to an ascii hex number (C format)
 */
char *
_itohex(p, h)
	register char *p;
	register int h;
{
	char hs[8];
	register char *cp = &hs[8];
	register int i = 0;

	if ( h < 0 ) {
		h = -h;
		*p++ = '-';
	}
	if ( h > 9 ) {
		*p++ = '0';
		*p++ = 'x';
	}
	do {
		*(--cp) = digits[h&0xf];
		i++;
		h >>= 4;
	} while (h > 0);
	while ( i-- > 0 ) {
		*p++ = *cp++;
	}
	return(p);
}
