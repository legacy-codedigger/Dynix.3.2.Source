/*
 * $Copyright:	$
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

#ifndef lint
static char rcsid[] = "$Header: custA.c 2.7 87/02/13 $";
#endif

/*
 * This file contains stub entry points for configuring custom b8k bus
 * controller boards.  These entries would be replaced with real code
 * suitable for that controller.
 */

/*
 * $Log:	custA.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/clist.h"
#include "../h/buf.h"

#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/cfg.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/hwparam.h"
#include "../machine/intctl.h"
#include "../machine/trap.h"
#include "../machine/mftpr.h"

#include "../custA/ioconf.h"
#include "../custA/custA.h"

/*
 * custA_map()
 *
 * This function is called from sysinit() after all other kernel data
 * structures have been allocated and mapped.  Its purpose is to allow
 * controller software to do initializations which depend upon where
 * these structures are and what their mapping is.  For example, the
 * C-list has been allocated phys==virt, contiguous, in sysinit().
 */

custA_map()
{
}

/*
 * conf_custA()
 *	Allocate and fill out descriptors.
 *
 * Sysinit() insures we have same virtual addresses for the
 * boards after real page-tables turned on.
 * 
 * Can deal with deconfigured boards (suggest using a bit vector).
 *
 * Program the group interrupt mask.  Establish the physical
 * addresses of the boards, and establish virtual addresses for
 * those that are configured.
 *
 */

conf_custA()
{
}

/*
 * probe_custA_devices()
 *	Probe for devices attached to the controller
 *
 */

probe_custA_devices()
{
}

