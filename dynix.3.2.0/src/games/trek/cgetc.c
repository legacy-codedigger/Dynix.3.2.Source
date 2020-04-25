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

/* $Header: cgetc.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)cgetc.c	1.1	(Berkeley) 5/27/83";
#endif not lint

# include	<stdio.h>

char	cgetc(i)
int	i;
{
	return ( getchar() );
}
