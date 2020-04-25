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
static char rcsid[] = "$Header: line.c 2.0 86/01/28 $";
#endif

extern vti;
extern xnow,ynow;
line(x0,y0,x1,y1){
	struct{char x,c; int x0,y0,x1,y1;} p;
	p.c = 3;
	p.x0 = xsc(x0);
	p.y0 = ysc(y0);
	p.x1 = xnow = xsc(x1);
	p.y1 = ynow = ysc(y1);
	write(vti,&p.c,9);
}
cont(x0,y0){
	line(xnow,ynow,xsc(x0),ysc(y0));
	return;
}
