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

/* $Header: usclkc.h 1.3 87/03/23 $
 */

/*
 *	usclkc.h:  Balance Microsecond Clock
 */

typedef unsigned long	usclk_t;

void usclk_init();

usclk_t getusclk();

#if defined(i386 ) && !defined(KXX)
extern usclk_t *usclk_base;
#define GETUSCLK()	(*usclk_base)
#endif defined(i386 ) && !defined(KXX)

#if defined(ns32000) || defined(KXX)
#define GETUSCLK()	getusclk()
#endif defined(ns32000) || defined(KXX)
