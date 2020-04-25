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
static char rcsid[] = "$Header: string.c 2.0 86/01/28 $";
#endif

#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	general purpose string handling ======== */


STRING	movstr(a,b)
	REG STRING	a, b;
{
	WHILE *b++ = *a++ DONE
	return(--b);
}

INT	any(c,s)
	REG CHAR	c;
	STRING		s;
{
	REG CHAR d;

	WHILE d = *s++
	DO	IF d==c
		THEN	return(TRUE);
		FI
	OD
	return(FALSE);
}

INT	cf(s1, s2)
	REG STRING s1, s2;
{
	WHILE *s1++ == *s2
	DO	IF *s2++==0
		THEN	return(0);
		FI
	OD
	return(*--s1 - *s2);
}

INT	length(as)
	STRING as;
{
	REG STRING s;

	IF s=as THEN WHILE *s++ DONE FI
	return(s-as);
}
