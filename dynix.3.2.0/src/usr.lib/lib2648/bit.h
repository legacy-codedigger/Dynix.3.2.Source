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

/* $Header: bit.h 2.0 86/01/28 $
 *
 * Bit matrix manipulations for font editor.
 *
 * General structure of a bit matrix: each row is packed into as few
 * bytes as possible, taking the bits from left to right within bytes.
 * The matrix is a sequence of such rows, i.e. up to 7 bits are wasted
 * at the end of each row.
 */

#include <stdio.h>
typedef char *	bitmat;
#ifdef TRACE
	FILE *trace;
#endif

#define max(x,y)	((x) > (y) ?   (x)  : (y))
#define min(x,y)	((x) < (y) ?   (x)  : (y))
