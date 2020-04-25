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
static	char rcsid[] = "$Header: slic_init.c 1.2 87/02/13 $";
#endif

/*
 * SLIC initialization
 */
#include <sys/types.h>
#include <machine/hwparam.h>
#include <machine/slic.h>

slic_init()
{
	register struct cpuslic *sl = (struct cpuslic *) LOAD_CPUSLICADDR;

	/*
	 * Set up SLIC to everyone is in GROUP1,
	 * using BIN1 for console I/O 
	 * and set GROUP MASK to 0xFF (clear group mask).
	 */
	sl->sl_procgrp = 1;
	sl->sl_lmask = 2;
	setGM(sl->sl_procid & SL_PROCID, 0xff);
}
