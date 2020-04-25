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

#ifndef	lint
static	char	rcsid[] = "$Header: gate_data.c 2.5 89/06/26 $";
#endif

/*
 * gate_data.c
 *	Data bytes to back up gates defined in machine/gate.h.
 *
 * K20 kernel hashes address of byte to determine SLIC gate to use.
 */

/* $Log:	gate_data.c,v $
 */

#include "../machine/gate.h"

pgate_t		g_time;			/* time and timeout queue */
pgate_t		g_cfree;		/* cfreelist gating */

pgate_t		g_runq;			/* dispatcher run-queues */
pgate_t		g_swap;			/* swapmap (vm_sw.c) */
pgate_t		g_netisr;		/* cfreelist gating */
pgate_t		g_fs;			/* file-system locking */
