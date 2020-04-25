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

#ifndef lint
static char rcsid[] = "$Header: ts.c 2.0 86/01/28 $";
#endif

 /* ts.c: minor string processing subroutines */
match (s1, s2)
	char *s1, *s2;
{
	while (*s1 == *s2)
		if (*s1++ == '\0')
			return(1);
		else
			s2++;
	return(0);
}
prefix(small, big)
	char *small, *big;
{
int c;
while ((c= *small++) == *big++)
	if (c==0) return(1);
return(c==0);
}
letter (ch)
	{
	if (ch >= 'a' && ch <= 'z')
		return(1);
	if (ch >= 'A' && ch <= 'Z')
		return(1);
	return(0);
	}
numb(str)
	char *str;
	{
	/* convert to integer */
	int k;
	for (k=0; *str >= '0' && *str <= '9'; str++)
		k = k*10 + *str - '0';
	return(k);
	}
digit(x)
	{
	return(x>= '0' && x<= '9');
	}
max(a,b)
{
return( a>b ? a : b);
}
tcopy (s,t)
	char *s, *t;
{
	while (*s++ = *t++);
}
