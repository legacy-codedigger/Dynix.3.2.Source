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
static char rcsid[] = "$Header: 0.list.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"

struct list *consls(v,ls)		/* make list */
VERT v;
struct list *ls;
	{
	struct list *temp;
	temp = challoc(sizeof(*temp));
	temp->elt = v;
	temp->nxtlist = ls;
	return(temp);
	}

struct list *append(v,ls)		/* return ls . v */
VERT v;
struct list *ls;
	{
	struct list *temp;
	if (!ls) return(consls(v,0));
	for (temp = ls; temp -> nxtlist; temp = temp->nxtlist)
		;
	temp->nxtlist = consls(v,0);
	return(ls);
	}


freelst(ls)
struct list *ls;
	{
	if (!ls) return;
	if (ls->nxtlist)
		freelst(ls->nxtlist);
	chfree(ls,sizeof(*ls));
	}


oneelt(ls)		/* return w if w is only elt of ls, UNDEFINED otherwise */
struct list *ls;
	{
	if (!ls) return(UNDEFINED);
	if (ls->nxtlist) return(UNDEFINED);
	return(ls->elt);
	}


lslen(ls)		/* return number of elements in list ls */
struct list *ls;
	{
	int count;
	struct list *lp;
	count = 0;
	for (lp = ls; lp; lp = lp->nxtlist)
		++count;
	return(count);
	}


prlst(ls)
struct list *ls;
	{
	struct list *lp;
	for (lp = ls; lp; lp = lp->nxtlist)
		printf("%d,",lp->elt);
	fprintf(stderr,"\n");
	}
