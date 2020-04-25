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
static char sccsid[] = "@(#)fseek.c	5.1 (Berkeley) 6/5/85";
static char rcsid[] = "$Header: fseek.c 2.2 86/12/16 $";
#endif not lint

/*
 * Seek for standard library.  Coordinates with buffering.
 */

#include	<stdio.h>

long lseek();

fseek(iop, offset, ptrname)
	register FILE *iop;
	long offset;
{
	register resync, c;
	long p = -1, curpos = -1;

	iop->_flag &= ~_IOEOF;
	if (iop->_flag&_IOREAD) {
		if (ptrname<2 && iop->_base &&
			!(iop->_flag&_IONBF)) {
			c = iop->_cnt;
			p = offset;
			if (ptrname==0) {
				curpos = lseek(fileno(iop), 0L, 1);
				if (curpos == -1)
					return (-1);
				p += c - curpos;
			} else
				offset -= c;
			if(!(iop->_flag&_IORW) && c>0&&p<=c
			    && p>=iop->_base-iop->_ptr){
				iop->_ptr += (int)p;
				iop->_cnt -= (int)p;
				if (curpos == -1)
					curpos = lseek(fileno(iop), 0L, 1);
#ifdef	notdef	/* 43bsd */
				return (curpos == -1? -1: curpos - iop->_cnt);
#else
				return (curpos == -1? -1 : 0);
#endif
			}
			resync = offset&01;
		} else 
			resync = 0;
		if (iop->_flag & _IORW) {
			iop->_ptr = iop->_base;
			iop->_flag &= ~_IOREAD;
			resync = 0;
		}
		p = lseek(fileno(iop), offset-resync, ptrname);
		iop->_cnt = 0;
		if (resync && getc(iop) != EOF && p != -1)
			p++;
	}
	else if (iop->_flag & (_IOWRT|_IORW)) {
		fflush(iop);
		if (iop->_flag & _IORW) {
			iop->_cnt = 0;
			iop->_flag &= ~_IOWRT;
			iop->_ptr = iop->_base;
		}
		p = lseek(fileno(iop), offset, ptrname);
	}
#ifdef	notdef	/* 43bsd */
	return(p);
#else
	return(p==-1?-1:0);
#endif
}
