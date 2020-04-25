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
static char rcsid[] = "$Header: rtt.c 2.2 90/08/15 $";
#endif

#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/slic.h>
#include <machine/slicreg.h>
#include "sec.h"

/* 
 * return to diagnostics mode on SEC board
 */
_rtt()
{
	extern void sec_rtofw();
#ifdef BOOTXX
	sec_rtofw();
#else BOOTXX
	extern void ssm_rtofw();

	if (CD_LOC->c_cons->cd_type == SLB_SCSIBOARD) 
		sec_rtofw();
	else
		ssm_rtofw();
#endif BOOTXX
}

/*
 * SEC board dependent return to firmware.
 * Included in this module to inhibit inclusion of
 * rest of sec.c in some, but not all, bootstraps.
 */
static void
sec_rtofw()
{
	register struct config_desc *cd = CD_LOC;
	register struct sec_powerup *in;

	in = (struct sec_powerup *)cd->c_cons->cd_sc_init_queue;
	in->pu_cib.cib_inst = SINST_RETTODIAG;
	in->pu_cib.cib_status = (int *)0;
	mIntr(cd->c_cons->cd_slic, 5, SDEV_SCSIBOARD);
	for (;;)
		/* spin till shut down by firmware */;
}
