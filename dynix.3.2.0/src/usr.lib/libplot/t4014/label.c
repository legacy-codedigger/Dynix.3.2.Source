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
static char rcsid[] = "$Header: label.c 2.0 86/01/28 $";
#endif

#define N 0104
#define E 0101
#define NE 0105
#define S 0110
#define W 0102
#define SW 0112
/*	arrange by incremental plotting that an initial
 *	character such as +, X, *, etc will fall
 *	right on the point, and undo it so that further
 *	labels will fall properly in place
 */
char lbl_mv[] = {
	036,040,S,S,S,S,S,S,SW,SW,SW,SW,SW,SW,SW,SW,SW,SW,037,0
};
char lbl_umv[] = {
	036,040,N,N,N,N,N,N,NE,NE,NE,NE,NE,NE,NE,NE,NE,NE,037,0
};
label(s)
char *s;
{
	register i,c;
	for(i=0; c=lbl_mv[i]; i++)
		putch(c);
	for(i=0; c=s[i]; i++)
		putch(c);
	for(i=0; c=lbl_umv[i]; i++)
		putch(c);
}
