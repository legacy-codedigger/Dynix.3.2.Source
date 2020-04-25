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
static char rcsid[] = "$Header: space.c 2.1 86/06/17 $";
#endif

extern float boty;
extern float botx;
extern float oboty;
extern float obotx;
extern float scalex;
extern float scaley;
float deltx = 4095.;
float delty = 4095.;
space(x0,y0,x1,y1){
	botx = -2047.;
	boty = -2047;
	obotx = x0;
	oboty = y0;
	scalex = deltx/(x1-x0);
	scaley = delty/(y1-y0);
}
