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


/*
 * display.h: version 1.2 of 1/7/83
 * 
 *
 * @(#)display.h	1.2	(National Semiconductor)	1/7/83
 */

/* output modes for outputmode */
#define INSTRUCT	0x1
#define NUMERIC		0x2
#define ABSNUMERIC	0x3
#define FLOATING	0x4
#define CHARACTER	0x5
#define STRING		0x6
#define DOUBLE		0x7

extern int firsthex;	/* flag for printing leading zeros on hex output */
extern int acontext;	/* last size(b,w,d) of a displayed object */

extern int snprint();	/* printnum as numeric of outradix */
extern char *regstr();	/* return register characters for printing */

