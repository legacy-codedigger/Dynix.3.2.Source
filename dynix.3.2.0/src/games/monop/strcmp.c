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

/* $Header: strcmp.c 2.0 86/01/28 $ */

# include	<stdio.h>
# include	<ctype.h>

# define	reg	register

# define	makelower(c)	(isupper(c) ? tolower(c) : c)

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strcmp(s1, s2)
reg char	*s1, *s2; {

	while (makelower(*s1) == makelower(*s2)) {
		if (*s1 == '\0')
			return 0;
		s1++, s2++;
	}
	return *s1 - *s2;
}
