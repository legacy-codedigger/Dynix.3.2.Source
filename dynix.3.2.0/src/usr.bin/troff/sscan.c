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
static	char rcsid[] = "$Header: sscan.c 2.0 86/01/28 $";
#endif

/*
 * Sscan:  Parses strings according to page 181 of K and R.
 *
 * Accepted strings start with ``"'' and end with ``"''.
 * Allowed escapes are:
 *	newline		NL	\n
 *	horizontal tab	HT	\t
 *	backspace	BS	\b
 *	carriage return	CR	\r
 *	form feed	FF	\f
 *	backslash	\	\\
 *	single quote	'	\'
 *	double quote	"	\"
 *	bit pattern	ddd	\ddd 	(0, 1, or 2 OCTAL digits)
 *	\ IGNORED	\*	where * is not one of the above (or newline)
 *	
 */
#include <stdio.h>

char *
sscan(s)
	char *s;
{
	static char buf[BUFSIZ];
	register char *ip,*op;
	register c,n;

	ip = s;
	while (ip && *ip && *ip != '"')
		++ip;
	if (ip == 0 || *ip != '"')
		return(NULL);
	buf[0] = NULL;
	++ip;
	for (op=buf; *ip && op < &buf[BUFSIZ] && *ip != '"'; /* void */) {
		switch (c = *ip) {
		/*
		 * Pass chars through until backslash escape seen
		 */
		default:
			if ((*op++ = *ip++) == NULL)
				return(buf);
			continue;
		case '\\':	/* seen backslash */
			c = *++ip;
			switch(c) {
			/*
			 * Handle \d, \dd, \ddd 
			 */
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				n = *ip++ - '0';
				if (*ip >= '0' && *ip <= '9') {
					n = (n<<3) + *ip++ - '0';
					if (*ip >= '0' && *ip <= '9') 
						n = (n<<3) + *ip++ - '0';
				}
				*op++ = n; continue;
			/*
			 * \newline is an ignored combination 
			 */
			case '\n':
				ip++; continue;
			/*
			 * Standard \n,\t,\b,\r,\f escapes
			 */
			case 'n':
				*op++ = '\n'; ip++; continue;
			case 't':
				*op++ = '\t'; ip++; continue;
			case 'b':
				*op++ = '\b'; ip++; continue;
			case 'r':
				*op++ = '\r'; ip++; continue;
			case 'f':
				*op++ = '\f'; ip++; continue;
			/*
			 * Anything else ignores the \ and
			 * passes straight through
			 */
			default:
				*op++ = *ip++; continue;
			}
		}
	}
	if (*ip == '"' && op < &buf[BUFSIZ]) {
		*op = NULL;
		return(buf);
	}
	return(NULL);
}
