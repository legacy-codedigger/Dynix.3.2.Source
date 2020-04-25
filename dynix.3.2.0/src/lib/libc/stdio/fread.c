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

/* $Header: fread.c 2.1 86/12/12 $ */
/* @(#)fread.c	4.1 (Berkeley) 5/29/84 */
#include	<stdio.h>

fread(ptr, size, count, iop)
	register char *ptr;
	unsigned size, count;
	register FILE *iop;
{
	register int s, n;

	if (size == 0 || count == 0)
		return(0);
	s = size * count;
	while (s > 0) {
		if (iop->_cnt < s) {
			if (iop->_cnt > 0) {
				bcopy(iop->_ptr, ptr, iop->_cnt);
				ptr += iop->_cnt;
				s -= iop->_cnt;
			}
			/*
			 * filbuf clobbers _cnt & _ptr,
			 * so don't waste time setting them.
			 */
			if ((n = _filbuf(iop)) == EOF)
				break;
			*ptr++ = n;
			s--;
		}
		if (iop->_cnt >= s) {
			bcopy(iop->_ptr, ptr, s);
			iop->_ptr += s;
			iop->_cnt -= s;
			return (count);
		}
	}
	return (count - ((s + size - 1) / size));
}
