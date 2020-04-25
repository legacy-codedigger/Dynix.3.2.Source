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

/* $Id: config.h,v 2.8 88/09/02 11:46:08 ksb Exp $ */
/*
 *	Berkeley Pascal Compiler	(config.h)
 */

#if !defined(_CONFIG_)
#define	_CONFIG_

/*
 * Compiler configuration definitions.
 */

/*
 * These flags control global compiler operation.
 */
#define	BUFSTDERR	1		/* buffer output to stderr */
#define NESTCALLS	1		/* disallow two concurrent store()'s */
#define	FLEXNAMES	1		/* arbitrary length identifiers */

/*
 * Table sizes.
 */
#define TREESZ		4000		/* parse tree table size 1000 */
#define	DELAYS		20		/* delayed evaluation table size */
#define NRECUR		(10*TREESZ)	/* maximum eval recursion depth */

/* in case anyone still uses fixed length names */
#if !defined(FLEXNAMES)
#define	NCHNAM		8		/* significant chars of identifier */
#endif /* flex names */
#endif /* config twice */
