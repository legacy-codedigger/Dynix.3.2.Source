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

/* $Header: ftell.c 2.0 86/01/28 $ */
/* @(#)ftell.c	4.2 (Berkeley) 3/9/81 */
/*
 * Return file offset.
 * Coordinates with buffering.
 */

#include	<stdio.h>
long	lseek();


long ftell(iop)
FILE *iop;
{
	long tres;
	register adjust;

	if (iop->_cnt < 0)
		iop->_cnt = 0;
	if (iop->_flag&_IOREAD)
		adjust = - iop->_cnt;
	else if (iop->_flag&(_IOWRT|_IORW)) {
		adjust = 0;
		if (iop->_flag&_IOWRT && iop->_base && (iop->_flag&_IONBF)==0)
			adjust = iop->_ptr - iop->_base;
	} else
		return(-1);
	tres = lseek(fileno(iop), 0L, 1);
	if (tres<0)
		return(tres);
	tres += adjust;
	return(tres);
}
