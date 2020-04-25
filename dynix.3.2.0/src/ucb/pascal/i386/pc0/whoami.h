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

/* $Header: whoami.h 1.1 89/03/12 $ */
/*
 *	Am I generating 
*	postfix binary input to the 2nd pass (PC)?
*	or pTrees (PTREE)?
 *	or am I pxp (PXP)?
 */
#define	PC
#undef	PTREE
#undef	PXP

/*
 *	Hardware Characteristics:
 *	address size (16 or 32 bits) and byte ordering (normal or dec11 family).
 */
#if defined(i386)
#undef	ADDR16
#define	ADDR32
#define	DEC11
#endif i386
