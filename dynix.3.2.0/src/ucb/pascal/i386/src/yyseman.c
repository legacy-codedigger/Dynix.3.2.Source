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
static char rcsid[] = "$Id: yyseman.c,v 1.1 88/09/02 11:48:46 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree_ty.h"	/* must be included for yy.h */
#include "yy.h"

/*
 * Assign semantics to a generated token
 *
 * Most terminals have a semantic value the current
 * input line.  If they are generated they are flagged
 * by having this number negated.
 *
 * The terminals which have true semantics such
 * as identifiers and strings are instead given
 * semantic value NIL here - we do not attempt
 * to do repair, e.g. by giving generated integers
 * the value 1, etc.
 */
nullsem(ch)
	int ch;
{

	switch (ch) {
		case YID:
		case YINT:
		case YNUMB:
		case YBINT:
		case YSTRING:
			return (NIL);
		default:
			return (-yyeline);
	}
}
