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
static char rcsid[] = "$Header: tm.c 2.0 86/01/28 $";
#endif

 /* tm.c: split numerical fields */
# include "t..c"
maknew(str)
	char *str;
{
	/* make two numerical fields */
	int dpoint, c;
	char *p, *q, *ba;
	p = str;
	for (ba= 0; c = *str; str++)
		if (c == '\\' && *(str+1)== '&')
			ba=str;
	str=p;
	if (ba==0)
		{
		for (dpoint=0; *str; str++)
			{
			if (*str=='.' && !ineqn(str,p) &&
				(str>p && digit(*(str-1)) ||
				digit(*(str+1))))
					dpoint=str;
			}
		if (dpoint==0)
			for(; str>p; str--)
			{
			if (digit( * (str-1) ) && !ineqn(str, p))
				break;
			}
		if (!dpoint && p==str) /* not numerical, don't split */
			return(0);
		if (dpoint) str=dpoint;
		}
	else
		str = ba;
	p =str;
	if (exstore ==0 || exstore >exlim)
		{
		exstore = chspace();
		exlim= exstore+MAXCHS;
		}
	q = exstore;
	while (*exstore++ = *str++);
	*p = 0;
	return(q);
	}
ineqn (s, p)
	char *s, *p;
{
/* true if s is in a eqn within p */
int ineq = 0, c;
while (c = *p)
	{
	if (s == p)
		return(ineq);
	p++;
	if ((ineq == 0) && (c == delim1))
		ineq = 1;
	else
	if ((ineq == 1) && (c == delim2))
		ineq = 0;
	}
return(0);
}
