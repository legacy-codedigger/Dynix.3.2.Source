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

/* $Header: movecurs.c 2.0 86/01/28 $ */

#include "2648.h"

movecurs(x, y)
{
	char mes[20];

	if (x==_curx && y==_cury)
		return;
	sprintf(mes, "%d,%do", x, y);
	escseq(ESCD);
	outstr(mes);
	escseq(NONE);
	_curx = x;
	_cury = y;
}
