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
static char rcsid[] = "$Header: 3.main.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"

structure()
	{
	VERT v, *head;

	if (progress)
		fprintf(stderr,"	getreach:\n");
	getreach();
	if (routerr) return;
	if (progress)
		fprintf(stderr,"	getflow:\n");
	getflow();
	if (progress)
		fprintf(stderr,"	getthen:\n");
	getthen(START);
	head = challoc(nodenum * sizeof(*head));
	for (v = 0; v < nodenum; ++v)
		head[v] = UNDEFINED;
	for (v = START; DEFINED(v); v = RSIB(v))
		fixhd(v,UNDEFINED,head);
			/* fixhd must be called before getloop so that
				it gets applied to IFVX which becomes NXT(w) for UNTVX w */
	if (progress)
		fprintf(stderr,"	getloop:\n");
	getloop();
	if (progress)
		fprintf(stderr,"	getbranch:\n");
	getbranch(head);
	chfree(head,nodenum * sizeof(*head));
	head = 0;
	}
