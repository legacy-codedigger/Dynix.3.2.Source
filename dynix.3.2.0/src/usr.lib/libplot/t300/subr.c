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
static char rcsid[] = "$Header: subr.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include "con.h"
abval(q)
{
	return (q>=0 ? q : -q);
}

xconv (xp)
{
	/* x position input is -2047 to +2047, output must be 0 to PAGSIZ*HORZRES */
	xp += 2048;
	/* the computation is newx = xp*(PAGSIZ*HORZRES)/4096 */
	return (xoffset + xp /xscale);
}

yconv (yp)
{
	/* see description of xconv */
	yp += 2048;
	return (yp / yscale);
}

inplot()
{
	stty(OUTF, &PTTY);
	spew (ACK);
}

outplot()
{
	spew(ESC);
	spew(ACK);
	fflush(stdout);
	stty(OUTF, &ITTY);
}

spew(ch)
{
	if(ch == UP)putc(ESC,stdout);
	putc(ch, stdout);
}

tobotleft ()
{
	move(-2048,-2048);
}
reset()
{
	outplot();
	exit();
}

float
dist2 (x1, y1, x2, y2)
{
	float t,v;
	t = x2-x1;
	v = y1-y2;
	return (t*t+v*v);
}

swap (pa, pb)
int *pa, *pb;
{
	int t;
	t = *pa;
	*pa = *pb;
	*pb = t;
}
movep (xg, yg)
{
	int i,ch;
	if((xg == xnow) && (yg == ynow))return;
	/* if we need to go to left margin, just CR */
	if (xg < xnow/2)
	{
		spew(CR);
		xnow = 0;
	}
	i = (xg-xnow)/HORZRES;
	if(xnow < xg)ch = RIGHT;
	else ch = LEFT;
	xnow += i*HORZRES;
	i = abval(i);
	while(i--)spew(ch);
	i = abval(xg-xnow);
	inplot();
	while(i--) spew(ch);
	outplot();
	i=(yg-ynow)/VERTRES;
	if(ynow < yg)ch = UP;
	else ch = DOWN;
	ynow += i*VERTRES;
	i = abval(i);
	while(i--)spew(ch);
	i=abval(yg-ynow);
	inplot();
	while(i--)spew(ch);
	outplot();
	xnow = xg; ynow = yg;
}

xsc(xi){
	int xa;
	xa = (xi - obotx) * scalex + botx;
	return(xa);
}
ysc(yi){
	int ya;
	ya = (yi - oboty) *scaley +boty;
	return(ya);
}
