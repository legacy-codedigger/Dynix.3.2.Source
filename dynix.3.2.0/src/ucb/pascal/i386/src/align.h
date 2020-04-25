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

/* $Id: align.h,v 1.1 88/09/02 11:47:53 ksb Exp $ */
    /*
     *	alignment of various types in bytes.
     *	sizes are found using sizeof( type ).
     */
#if defined(i386)
#define A_CHAR	1
#define A_INT	4
#define A_FLOAT	4
#define A_DOUBLE	4
#define A_LONG	4
#define A_SHORT	2
#define A_POINT	4
#define A_STRUCT	1
#define A_STACK	4
#define A_FILET	4
#define A_SET	4
#define A_MIN	1
#define A_MAX	4
#endif
