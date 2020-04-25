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
static char rcsid[] = "$Header: arc.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
arc(xi,yi,x0,y0,x1,y1){
	putc('a',stdout);
	putsi(xi);
	putsi(yi);
	putsi(x0);
	putsi(y0);
	putsi(x1);
	putsi(y1);
}
