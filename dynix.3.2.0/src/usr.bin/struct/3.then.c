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
static char rcsid[] = "$Header: 3.then.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"
#include "3.def.h"

#define BRANCHTYPE(t)	(t == STOPVX || t == RETVX || t == BRKVX || t == NXTVX || t == GOVX)
#define MAXCHUNK	20
		/* if else clause smaller than MAXCHUNK and smaller than then clause,
			and there is no reason not to negate the if, negate the if */

getthen(v)		/* turn IFVX into THEN when appropriate, create else ifs where possible  */
VERT v;
	{
	VERT tch, fch;
	int tn,fn;
	int recvar;

	if (NTYPE(v) == IFVX)
		{
		tch = LCHILD(v,THEN);
		fch = LCHILD(v,ELSE);
		if (!DEFINED(fch))
			mkthen(v);
		else if (!DEFINED(tch))
			{
			negate(v);
			mkthen(v);
			}
		else if (BRANCHTYPE(NTYPE(tch)))
			mkthen(v);
		else if (BRANCHTYPE(NTYPE(fch)))
			{
			negate(v);
			mkthen(v);
			}
		else if (NTYPE(fch) != IFVX || DEFINED(RSIB(fch)))	/* not an else if */
			if ( NTYPE(tch) == IFVX && !DEFINED(RSIB(tch)))
					/* invert into else if */
				negate(v);
			else
				{
				/* asoc(v,n) returns number of statements associated with v
					if <= n, -1 otherwise */
				tn = asoc(tch,MAXCHUNK);
				fn = asoc(fch,MAXCHUNK);
				if (fn >= 0 && (tn < 0 || fn < tn))
					/* else clause smaller */
					negate(v);
				}
		}
	RECURSE(getthen,v,recvar);
	}

mkthen(v)
VERT v;
	{
	VERT w,tc;
	w = LCHILD(v,ELSE);
	tc = LCHILD(v,THEN);
	ASSERT(!DEFINED(w) || (DEFINED(tc) && BRANCHTYPE(NTYPE(tc)) ),mkthen);
	if (DEFINED(w))
		{
		insib(v,w);
		LCHILD(v,ELSE) = UNDEFINED;
		}
	ASSERT(IFTHEN(v),mkthen);
	}


negate(v)
VERT v;
	{
	ASSERT(NTYPE(v) == IFVX,negate);
	exchange(&LCHILD(v,THEN), &LCHILD(v,ELSE));
	NEG(v) = !NEG(v);
	}
