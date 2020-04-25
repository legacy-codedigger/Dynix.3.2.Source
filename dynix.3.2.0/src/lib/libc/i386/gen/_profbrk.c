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

/* $Header: _profbrk.c 1.1 87/04/10 $
 *
 * _profbrk(incr)
 *	allocate buffer space for profiling.
 *
 * Called by [mg]crt0.o before profiling.
 */

/*
 * $Log:	_profbrk.c,v $
 */

_profbrk(incr)
	int	incr;
{
	return(sbrk(incr));
}
