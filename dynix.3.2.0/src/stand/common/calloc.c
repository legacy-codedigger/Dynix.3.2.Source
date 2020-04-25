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

#ifdef RCS
static char rcsid[] = "$Header: calloc.c 1.1 86/02/27 $";
#endif

/*
 * calloc()
 *	Allocate zeroed memory.
 *
 * Done via bumping "curmem" value.
 *
 * callocrnd() is used to round up so that next allocation occurs
 * on the given boundary.
 */

/* $Log:	calloc.c,v $
 */

#include <sys/param.h>
#define NULL 0

extern	end;
static	caddr_t curmem = NULL;

caddr_t
calloc(size)
	register int size;
{
	caddr_t	val;

	if(curmem == NULL)
		/* loader aligns on long boundary - so just assign */
		curmem = (caddr_t)&end;
	size = (size + (sizeof(int) - 1)) & ~(sizeof(int)-1);
	val = curmem;
	curmem += size;

	bzero(val, (unsigned)size);
	return(val);
}

callocrnd(bound)
	register int bound;
{
	if(curmem == NULL)
		curmem = (caddr_t)&end;
	curmem = (caddr_t)roundup((int)curmem, bound);
}
