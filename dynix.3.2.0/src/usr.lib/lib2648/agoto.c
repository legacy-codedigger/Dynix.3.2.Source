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

/* $Header: agoto.c 2.0 86/01/28 $
 *
 * Position the alphanumeric cursor to (x, y).
 */

#include "2648.h"

agoto(x, y)
int x, y;
{
	char mes[20];
	sprintf(mes, "\33*dE\33&a%dr%dC", x, y);
	outstr(mes);
}

/*
 * lower left corner of screen.
 */
lowleft()
{
	outstr("\33F");
}
