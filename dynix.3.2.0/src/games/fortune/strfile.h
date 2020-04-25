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

/* $Header: strfile.h 2.0 86/01/28 $ */

/* @(#)strfile.h	1.2 (Berkeley) 5/14/81 */

# define	MAXDELIMS	3

struct	strfile {		/* information table			*/
	int	str_numstr;		/* number of strings in the file */
	int	str_longlen;		/* length of longest string	*/
	int	str_shortlen;		/* length of shortest string	*/
	long	str_delims[MAXDELIMS];	/* delimiter markings		*/
	int	str_unused;		/* reserve space for later needs */
};

typedef struct strfile	STRFILE;
