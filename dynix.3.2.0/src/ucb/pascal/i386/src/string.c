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

#if !defined(lint)
static char rcsid[] = "$Id: string.c,v 1.1 88/09/02 11:48:29 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"

/*
 * STRING SPACE DECLARATIONS
 *
 * Strng is the base of the current
 * string space and strngp the
 * base of the free area therein.
 * Strp is the array of descriptors.
 */
STATIC	char strings[STRINC];
STATIC	char *strng = strings;
STATIC	char *strngp = strings;

/*
 * Copy a string into the string area.
 */
char *
savestr(cp)
	register char *cp;
{
	register int i;

	i = strlen(cp) + 1;
	if (strngp + i >= strng + STRINC) {
		strngp = malloc(STRINC);
		if (strngp == 0) {
			yerror("Ran out of memory (string)");
			pexit(DIED);
		}
		strng = strngp;
	}
	(void) pstrcpy(strngp, cp);
	cp = strngp;
	strngp = cp + i;
	return (cp);
}

#if !defined(PXP)
char *
esavestr(cp)
	char *cp;
{
	strngp = ( (char *) ( ( (int) (strngp + 1) ) &~ 1 ) );
	return (savestr(cp));
}
#endif
