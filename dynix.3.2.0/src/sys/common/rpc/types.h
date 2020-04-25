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

/*
 * $Header: types.h 1.3 88/03/14 $
 *
 * types.h
 *	Rpc additions to <sys/types.h>
 */

/* $Log:	types.h,v $
 */

#define	enum_t	int
#define	FALSE	(0)
#define	TRUE	(1)
#define __dontcare__	-1

#ifndef KERNEL
extern	char *malloc();
#define mem_alloc(bsize)	malloc(bsize)
#define mem_free(ptr, bsize)	free(ptr)
#ifndef major		/* ouch! */
#include <sys/types.h>
#endif	major
#else
#define mem_alloc(bsize)	kmem_alloc((u_int)bsize)
#define mem_free(ptr, bsize)	kmem_free((caddr_t)(ptr), (u_int)(bsize))
#endif	!KERNEL
