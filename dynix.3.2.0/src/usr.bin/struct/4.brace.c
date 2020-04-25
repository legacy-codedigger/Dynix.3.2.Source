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
static char rcsid[] = "$Header: 4.brace.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"
#include "4.def.h"
#include "3.def.h"

ndbrace(v)			/* determine whether braces needed around subparts of v */
				/* return TRUE if v ends with IF THEN not in braces */
VERT v;
	{
	VERT w;
	int i;
	LOGICAL endif;
	endif = FALSE;
	for (i = 0; i < CHILDNUM(v); ++i)
		{
		endif = FALSE;
		for (w = LCHILD(v,i); DEFINED(w); w = RSIB(w))
			endif = ndbrace(w);
		if (NTYPE(v) != DUMVX && NTYPE(v) != ITERVX &&
			(!DEFINED(LCHILD(v,i)) || compound(v,i) ||
			(endif && NTYPE(v) == IFVX && !IFTHEN(v) && i == THEN )))
				/* DUMVX doesn't nest, ITERVX doen't nest since
					nesting is done at LOOPNODE, etc., must
					check for IFTHEN followed by unrelated ELSE */
			{
			YESBRACE(v,i);
			endif = FALSE;
			}
		}
	return(endif || IFTHEN(v) );
	}


compound(v,ch)		/* return TRUE iff subpart ch of v has multiple statements */
VERT v;
int ch;
	{
	VERT w;
	w = LCHILD(v,ch);
	if (!DEFINED(w))
		return(FALSE);
	if (NTYPE(w) == ITERVX)
		{
		ASSERT(DEFINED(NXT(w)),compound);
		if (LABEL(NXT(w)))
			return(TRUE);		/* loop ends with labeled CONTINUE statement */
		else
			return(compound(w,0));
		}
	else if (DEFINED(RSIB(w)))
		return(TRUE);
	else if (NTYPE(w) == STLNVX && CODELINES(w) > 1)
		return(TRUE);
	else
		return(FALSE);
	}
