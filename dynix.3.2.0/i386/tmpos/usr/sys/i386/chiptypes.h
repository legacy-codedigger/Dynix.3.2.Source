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
 * $Header: chiptypes.h 1.2 88/10/25 $
 *
 * chiptypes.h
 *	Definitions of Intel chips on the processor board.
 *
 */

/* $Log:	chiptypes.h,v $
 *
 */


/*
 * Intel components on the processor board
 */
#ifndef __CHIPTYPES__

#ifdef i386
#define CPU_TYPE  i386
#define MMU_TYPE  i386 
#define FPU_TYPE  i387
#endif i386

#define __CHIPTYPES__
#endif __CHIPTYPES__




