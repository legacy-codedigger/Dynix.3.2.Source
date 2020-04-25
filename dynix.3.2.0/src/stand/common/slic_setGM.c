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

/* $Header: slic_setGM.c 2.2 86/05/01 $ */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <machine/hwparam.h>
#include <machine/slic.h>
#include "saio.h"

/*
 * Set the group mask.
 */

int
setGM(destination, mask)
unsigned char destination, mask;
{
	register struct cpuslic *sl = (struct cpuslic *)LOAD_CPUSLICADDR;
	register int error = 0;
	register int retry = 4;
	register unsigned char stat;

	sl->sl_dest = destination;
	sl->sl_smessage = mask;
loop:
	sl->sl_cmd_stat = SL_SETGM;
	while (sl->sl_cmd_stat & SL_BUSY)
		;
	stat = sl->sl_cmd_stat;
	if ((stat & SL_PARITY) == 0) {
		slicerror(SL_PERROR, SL_SETGM, destination, mask, stat);
		if (--retry)
			goto loop;
		error = SL_BAD_PAR;
	}
	if ((stat & SL_EXISTS) == 0) {
		slicerror(SL_DERROR, SL_SETGM, destination, mask, stat);
		error += SL_BAD_DEST;
	}
	if ((stat & SL_OK) == 0) {
		slicerror(SL_NOTOK, SL_SETGM, destination, mask, stat);
		error += SL_NOT_OK;
	}
	return(error);
}
