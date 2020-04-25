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
static char rcsid[] = "$Header: 4.main.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "def.h"
#include "4.def.h"

LOGICAL *brace;
output()
	{
	VERT w;
	int i;
	brace = challoc(nodenum * sizeof(*brace));
	for (i = 0; i < nodenum; ++i)
		brace[i] = FALSE;
	if (progress) fprintf(stderr,"ndbrace:\n");
	for (w = START; DEFINED(w); w = RSIB(w))
		ndbrace(w);
	if (progress) fprintf(stderr,"outrat:\n");
	for (w = START; DEFINED(w); w = RSIB(w))
		outrat(w,0,YESTAB);
	OUTSTR("END\n");
	chfree(brace,nodenum * sizeof(*brace));
	brace = 0;
	}
