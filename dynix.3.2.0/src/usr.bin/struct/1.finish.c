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
static char rcsid[] = "$Header: 1.finish.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"
#include "1.incl.h"

fingraph()
	{
	/* if any entry statements, add a DUMVX with arcs to all entry statements */
	if (ENTLST)
		{
		ARC(START,0) = addum(ARC(START,0),ENTLST);
		freelst(ENTLST);
		}
	/* if any FMTVX, add a DUMVX with arcs to all FMTVX's */
	if (FMTLST)
		{
		ARC(START,0) = addum(ARC(START,0),FMTLST);
		freelst(FMTLST);
		}
	}

addum(v,lst)
VERT v;
struct list *lst;
	{
	VERT new;
	int count,i;
	struct list *ls;
	count = lslen(lst);		/* length of lst */
	new = create(DUMVX,1+count);
	ARC(new,0) = v;
	for (i = count, ls = lst; i >= 1; --i, ls = ls->nxtlist)
		{
		ASSERT(ls,addum);
		ARC(new,i) = ls->elt;
		}
	ASSERT(!ls, addum);
	return(new);
	}
