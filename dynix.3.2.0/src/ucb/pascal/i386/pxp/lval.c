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
static char rcsid[] = "$Id: lval.c,v 1.1 88/09/02 11:44:19 ksb Exp $";
#endif lint

/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"
#include "tree.h"

/*
 * A "variable"
 */
lvalue(r)
	register int *r;
{
	register *c, *co;

	ppid(r[2]);
	for (c = r[3]; c != NIL; c = c[2]) {
		co = c[1];
		if (co == NIL)
			continue;
		switch (co[0]) {
			case T_PTR:
				ppop("^");
				continue;
			case T_ARY:
				arycod(co[1]);
				continue;
			case T_FIELD:
				ppop(".");
				ppid(co[1]);
				continue;
			case T_ARGL:
				ppid("{unexpected argument list}");
				break;
			default:
				panic("lval2");
		}
	}
}

/*
 * Subscripting
 */
arycod(el)
	register int *el;
{

	ppbra("[");
	if (el != NIL)
		for (;;) {
			rvalue(el[1], NIL);
			el = el[2];
			if (el == NIL)
				break;
			ppsep(", ");
		}
	else
		rvalue(NIL, NIL);
	ppket("]");
}
