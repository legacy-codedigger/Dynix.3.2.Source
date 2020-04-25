/*	@(#)ftell.c	1.2	*/
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * Return file offset.
 * Coordinates with buffering.
 */
#include <stdio.h>


extern long lseek();

long
ftell(iop)
FILE	*iop;
{
	long	tres;

	if ((iop->_flag & _IOREAD) || (iop->_flag & (_IOWRT | _IORW))) {
		fflush(iop);
		tres = lseek(fileno(iop), 0L, 1);
		return(tres);
	} else 
		return(-1);
}
