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
static char rcsid[] = "$Id: yyoptions.c,v 1.1 88/09/02 11:48:40 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree_ty.h"	/* must be included for yy.h */
#include "yy.h"

/*
 * Options processes the option
 * strings which can appear in
 * comments and returns the next character.
 */
options()
{
	register c;
	register char *optp;

	c = readch();
	if (c != '$')
		return (c);
	do {
		c = readch();
		switch (c) {
			case 'b':
				optp = &opt( 'b' );
				c = readch();
				if (!digit(c))
					return (c);
				*optp = c - '0';
				c = readch();
				break;
#if defined(PC)
			case 'C':
				    /*
				     *	C is a replacement for t, fake it.
				     */
				c = 't';
				/* and fall through */
			case 'g':
#endif PC
			case 'k':
			case 'l':
			case 'n':
			case 'p':
			case 's':
			case 't':
			case 'u':
			case 'w':
			case 'z':
				optp = &opt( c );
				c = readch();
				if (c == '+') {
					*optp = 1;
					c = readch();
				} else if (c == '-') {
					*optp = 0;
					c = readch();
				} else {
					return (c);
				}
				break;
			default:
				    return (c);
			}
	} while (c == ',');
	if ( opt( 'u' ) )
		setuflg();
	return (c);
}
