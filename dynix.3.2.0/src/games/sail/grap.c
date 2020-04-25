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

/* $Header: grap.c 2.0 86/01/28 $ */

#ifndef lint
static	char *sccsid = "@(#)grap.c	1.1 83/03/17";
#endif
#include "externs.h"

ungrap(from,to)
int from, to;
{
	int k;

	if(grapple(from, to)){
		for (k = 0; k < 10; k++){
			if (scene[game].ship[from].file -> grapples[k].turnfoul && to == scene[game].ship[from].file -> grapples[k].toship && (die() < 3 || scene[game].ship[from].nationality == scene[game].ship[to].nationality)){
				cleangrapple(from, to, k);
				makesignal("ungrappling %s (%c%c)", to, from);
			}
		}
	}
}

grap(from, to)
int from,to;
{
	int number, captured, l;

	if ((captured = scene[game].ship[to].file -> captured) < 0)
		captured = to;
	number = die() < 3;
	if (!number && scene[game].ship[from].nationality == scene[game].ship[captured].nationality)
		number = 1;
	if (number){
		for (l=0; l < 10 && scene[game].ship[from].file -> grapples[l].turnfoul; l++);
		if (l < 10){
			Write(FILES + from, 0, 124 + l*4, turn);
			Write(FILES + from, 0, 124 + l*4 + 2, to);
		}
		for (l=0; l < 10 && scene[game].ship[to].file -> grapples[l].turnfoul; l++);
		if (l < 10){
			Write(FILES + to, 0, 124 + l*4, turn);
			Write(FILES + to, 0, 124 + l*4 + 2, from);
		}
		makesignal("grappled with %s (%c%c)", to, from);
	}
}


