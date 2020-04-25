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

/* $Header: 3.def.h 2.0 86/01/28 $ */

#define RECURSE(p,v,r)	{ for (r = 0; r < CHILDNUM(v); ++r) if (DEFINED(LCHILD(v,r))) p(LCHILD(v,r)); if (DEFINED(RSIB(v))) p(RSIB(v)); }

#define IFTHEN(v)		( NTYPE(v) == IFVX && !DEFINED(LCHILD(v,ELSE)))

#define BRK(v)	FATH(v)		/* lexical successor of v, for ITERVX only */
#define LABEL(v)	REACH(v)
