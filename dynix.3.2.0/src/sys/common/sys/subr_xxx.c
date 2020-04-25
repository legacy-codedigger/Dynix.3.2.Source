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
static	char	rcsid[] = "$Header: subr_xxx.c 2.3 90/09/14 $";
#endif

/*
 * subr_xxx.c
 *	Miscellaneous Functions.
 */

/* $Log:	subr_xxx.c,v $
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */

nodev()
{
	return (ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */

nulldev()
{
	return (0);
}

imin(a, b)
{
	return (a < b ? a : b);
}

imax(a, b)
{
	return (a > b ? a : b);
}

unsigned
min(a, b)
	u_int a, b;
{
	return (a < b ? a : b);
}

unsigned
max(a, b)
	u_int a, b;
{
#ifdef lint
	a = max(a, b);
#endif lint
	return (a > b ? a : b);
}

/*
 * scanc()
 *	does vax scanc instruction.
 *
 * That is, scan a character string until the AND of mask and character
 * is non-zero.
 *
 * No need for asm for this; compiler gen's good code.
 */

scanc(size, cp, table, mask)
	register unsigned size;
	register char *cp, table[];
	register int mask;
{
	register unsigned i = 0;

	while ((table[*(u_char *)(cp + i)]&mask) == 0 && i < size)
		i++;
	return (size - i);
}

/* C language portable versions */

#ifndef	ns32000
#ifndef	i386
#ifndef	vax
ffs(mask)
	register long mask;
{
	register int i;

	/*
	 * should really be Number of Bits in Word
	 * instead of NSIG! but this routine isn't 
	 * really used since NS32000 has the ffs instruction
	 */
	for(i = 1; i < NSIG; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	return (0);
}

bcmp(s1, s2, len)
	register char *s1, *s2;
	register int len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (1);
	return (0);
}

strlen(s1)
	register char *s1;
{
	register int len;

	for (len = 0; *s1++ != '\0'; len++)
		/* void */;
	return (len);
}
#endif	vax
#endif	i386
#endif	ns32000

/*
 * strcmp()
 *	Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strcmp(s1, s2)
	register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return (0);
	return (*s1 - *--s2);
}

/*
 * strncmp()
 *      Compare strings (at most n bytes).
 *      Returns: s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strncmp(s1, s2, n)
	register char *s1, *s2;
	register n;
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(n<0 ? 0 : *s1 - *--s2);
}

/*
 * strncpy()
 *	Copy s2 to s1, truncating to copy n bytes.
 *
 * Return ptr to null in s1 or s1 + n
 */

char *
strncpy(s1, s2, n)
	register char *s1, *s2;
{
	register i;

	for (i = 0; i < n; i++) {
		if ((*s1++ = *s2++) == '\0') {
			return (s1 - 1);
		}
	}
	return (s1);
}
