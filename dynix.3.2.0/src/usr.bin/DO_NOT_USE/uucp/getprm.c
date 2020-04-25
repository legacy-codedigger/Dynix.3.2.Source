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
static char rcsid[] = "$Header: getprm.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)getprm.c	5.4 (Berkeley) 10/9/85";
#endif

#include "uucp.h"

#define LQUOTE	'('
#define RQUOTE ')'

/*LINTLIBRARY*/

/*
 *	get next parameter from s
 *
 *	return - pointer to next character in s
 */

char *
getprm(s, prm)
register char *s, *prm;
{
	register char *c;

	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;

	*prm = '\0';
	if (*s == '\0')
		return NULL;

	if (*s == '>' || *s == '<' || *s == '|'
	  || *s == ';' || *s == '&') {
		*prm++ = *s++;
		*prm = '\0';
		return s;
	}

	/* look for quoted argument */
	if (*s == LQUOTE) {
		if ((c = index(s + 1, RQUOTE)) != NULL) {
			c++;
			while (c != s)
				*prm++ = *s++;
			*prm = '\0';
			return s;
		}
	}

	/* look for `  ` string */
	if (*s == '`') {
		if ((c = index(s + 1, '`')) != NULL) {
			c++;
			while (c != s)
				*prm++ = *s++;
			*prm = '\0';
			return s;
		}
	}

	while (*s != ' ' && *s != '\t' && *s != '<'
		&& *s != '>' && *s != '|' && *s != '\0'
		&& *s != '&' && *s != ';' && *s != '\n')
		*prm++ = *s++;
	*prm = '\0';

	return s;
}
