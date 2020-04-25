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
static char rcsid[] = "$Header: whatnow.c 2.0 86/01/28 $";
#endif

#include "stdio.h"
#include "lrnref.h"

extern	char	togo[];
extern	int	review;

whatnow()
{
	if (again) {
		if (!review)
			printf("\nOK.  That was lesson %s.\n\n", todo);
		fflush(stdout);
		strcpy(level, togo);
		return;
	}
	if (skip) {
		printf("\nOK.  That was lesson %s.\n", todo);
		printf("Skipping to next lesson.\n\n");
		fflush(stdout);
		strcpy(level, todo);
		skip = 0;
		return;
	}
	if (todo == 0) {
		more=0;
		return;
	}
	if (didok) {
		strcpy(level,todo);
		if (speed<=9) speed++;
	}
	else {
		speed -= 4;
		/* the 4 above means that 4 right, one wrong leave
		    you with the same speed. */
		if (speed <0) speed=0;
	}
	if (wrong) {
		speed -= 2;
		if (speed <0 ) speed = 0;
	}
	if (didok && more) {
		printf("\nGood.  That was lesson %s.\n\n",level);
		fflush(stdout);
	}
}
