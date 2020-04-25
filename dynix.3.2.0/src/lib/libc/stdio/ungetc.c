/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: ungetc.c 2.1 91/02/06 $ */
/*	ungetc.c	4.2	83/09/25	*/

#include <stdio.h>

ungetc(c, iop)
	register FILE *iop;
{
	if (c == EOF)
		return (-1);
	if ((iop->_flag&_IOREAD) == 0 || iop->_ptr <= iop->_base)
		if (iop->_ptr == iop->_base && iop->_cnt == 0)
			*iop->_ptr++;
		else
			return (EOF);
	iop->_cnt++;
	if (*--iop->_ptr != c)
		*iop->_ptr = c;
	return (c);
}
