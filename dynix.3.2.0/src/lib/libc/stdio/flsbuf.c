/* $Copyright: $
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

/* $Header: flsbuf.c 2.6 1991/08/13 21:30:44 $ */
/* @(#)flsbuf.c	4.7 (Berkeley) 6/6/84 */
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>

char	*malloc();

_flsbuf(c, iop)
register FILE *iop;
{
	register char *base;
	register n, rn;
	char c1;
	int size;
	struct stat stbuf;

	if (iop->_flag & _IORW) {
		iop->_flag |= _IOWRT;
		iop->_flag &= ~(_IOEOF|_IOREAD);
	}

	if ((iop->_flag&_IOWRT)==0)
		return(EOF);
tryagain:
	if (iop->_flag&_IOLBF) {
		base = iop->_base;
		*iop->_ptr++ = c;
		if (iop->_ptr >= base+iop->_bufsiz || c == '\n') {
			n = __writex(fileno(iop), base, rn = iop->_ptr - base);
			iop->_ptr = base;
		} else
			rn = n = 0;
		iop->_cnt = 0;
	} else if (iop->_flag&_IONBF) {
		c1 = c;
		rn = 1;
		n = __writex(fileno(iop), &c1, rn);
		iop->_cnt = 0;
	} else {
		if ((base=iop->_base)==NULL) {
			if ((n = isatty(fileno(iop))) || fstat(fileno(iop), &stbuf) < 0 ||
			    stbuf.st_blksize <= NULL)
				size = BUFSIZ;
			else
				size = stbuf.st_blksize;
			if ((iop->_base=base=malloc(size)) == NULL) {
				iop->_flag |= _IONBF;
				goto tryagain;
			}
			iop->_flag |= _IOMYBUF;
			iop->_bufsiz = size;
			if (iop==stdout && n) {
				iop->_flag |= _IOLBF;
				iop->_ptr = base;
				goto tryagain;
			}
			rn = n = 0;
		} else if ((rn = n = iop->_ptr - base) > 0) {
			iop->_ptr = base;
			n = __writex(fileno(iop), base, n);
		}
		iop->_cnt = iop->_bufsiz-1;
		*base++ = c;
		iop->_ptr = base;
	}
	if (rn != n) {
		iop->_flag |= _IOERR;
		return(EOF);
	}
	return(c);
}

fflush(iop)
register struct _iobuf *iop;
{
	register char *base;
	register n;

	if ((iop->_flag&(_IONBF|_IOWRT))==_IOWRT
	 && (base=iop->_base)!=NULL && (n=iop->_ptr-base)>0) {
		iop->_ptr = base;
		iop->_cnt = (iop->_flag&(_IOLBF|_IONBF)) ? 0 : iop->_bufsiz;
		if (__writex(fileno(iop), base, n)!=n) {
			iop->_flag |= _IOERR;
			return(EOF);
		}
	}
	return(0);
}

fclose(iop)
	register struct _iobuf *iop;
{
	register int r;

	r = EOF;
	if (iop->_flag&(_IOREAD|_IOWRT|_IORW) && (iop->_flag&_IOSTRG)==0) {
		r = fflush(iop);
		if (close(fileno(iop)) < 0)
			r = EOF;
		if (iop->_flag&_IOMYBUF)
			free(iop->_base);
	}
	iop->_cnt = 0;
	iop->_base = (char *)NULL;
	iop->_ptr = (char *)NULL;
	iop->_bufsiz = 0;
	iop->_flag = 0;
	iop->_file = 0;
	return(r);
}

/*
 * Flush buffers on exit
 */

_cleanup()
{
	extern int _fwalk();

	_fwalk(fclose);
}

/*
 * Write with gurantee that "n" chars will be written or error 
 * will be returned
 */

__writex(fd, buf, n)
	int fd;
	char *buf;
	int n;
{
	int nchar = 0;
	int ntry;
	char *bp;
	
	bp = buf;
	ntry = n;
	do {
		bp += nchar;
		ntry -= nchar;
		if ((nchar = write(fd, bp, ntry)) == -1)
			return(-1);
	} while (nchar < ntry);
	return(n);
}
