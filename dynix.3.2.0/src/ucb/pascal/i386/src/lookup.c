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
static char rcsid[] = "$Id: lookup.c,v 1.1 88/09/02 11:48:07 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"

struct nl *disptab[077+1];

/*
 * Lookup is called to
 * find a symbol in the
 * block structure symbol
 * table and returns a pointer to
 * its namelist entry.
 */
struct nl *
lookup(s)
	register char *s;
{
	register struct nl *p;
	register struct udinfo;

	if (s == NIL) {
		nocascade();
		return (NLNIL);
	}
	p = lookup1(s);
	if (p == NLNIL) {
		derror("%s is undefined", s);
		return (NLNIL);
	}
	if (p->class == FVAR) {
		p = p->chain;
		bn--;
	}
	return (p);
}

int	flagwas;
/*
 * Lookup1 is an internal lookup.
 * It is not an error to call lookup1
 * if the symbol is not defined.  Also
 * lookup1 will return FVARs while
 * lookup never will, thus asgnop
 * calls it when it thinks you are
 * assigning to the function variable.
 */

struct nl *
lookup1(s)
	register char *s;
{
	register struct nl *p;
	register struct nl *q;
	register int i;

	if (s == NIL)
		return (NLNIL);
	bn = cbn;
	/*
	 * We first check the field names
	 * of the currently active with
	 * statements (expensive since they
	 * are not hashed).
	 */
	for (p = withlist; p != NLNIL; p = p->nl_next) {
		q = p->type;
		if (q == NLNIL)
			continue;
		if (reclook(q, s) != NIL)
			/*
			 * Return the WITHPTR, lvalue understands.
			 */
			return (p);
	}
	/*
	 * Symbol table is a 64 way hash
	 * on the low bits of the character
	 * pointer value. (Simple, but effective)
	 */
	i = (int) s & 077;
	for (p = disptab[i]; p != NLNIL; p = p->nl_next)
		if (p->symbol == s && p->class != FIELD && p->class != BADUSE) {
			bn = (p->nl_block & 037);
			flagwas = p->nl_flags;
			p->nl_flags |= NUSED;
			return (p);
		}
	return (NLNIL);
}
