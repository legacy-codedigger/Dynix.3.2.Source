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

/* $Header: dfdispose.c 1.1 89/03/12 $ */
/*
 * Close all active files within a dynamic record,
 * then dispose of the record.
 */

#include "h00vars.h"
#include "libpc.h"

DFDISPOSE(var, size)
char **var;		/* pointer to pointer being deallocated */
long size;		/* sizeof(bletch) */
{
	register IOREC *next, *prev;
	register IOREC *start, *end;

	start = (struct iorec *)(*var);
	end = (struct iorec *)(*var + size);
	prev = (struct iorec *)(&_fchain);
	next = _fchain.fchain;
	while(next != FILNIL && (next->flev < GLVL || next < start)) {
		prev = next;
		next = next->fchain;
	}
	while(next != FILNIL && next < end)
		next = PFCLOSE(next, TRUE);
	prev->fchain = next;
	DISPOSE(var, size);
}
