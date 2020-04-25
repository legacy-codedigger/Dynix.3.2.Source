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
static char rcsid[] = "$Header: conf_devsw.c 2.6 90/11/08 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"

int	nullsys(), nullioctl(), nulllseek();
int	rsstrategy(), rsopen(), rsclose(), rsioctl(), rslseek();
int	sdstrategy(), sdopen(), sdioctl();
int	wdstrategy(), wdopen(), wdioctl();
int     tmstrategy(), tmopen(), tmclose(), tmioctl();
int	tsstrategy(), tsopen(), tsclose(), tsioctl();
int	xpstrategy(), xpopen(), xpioctl();
int	xtstrategy(), xtopen(), xtclose();
int	zdstrategy(), zdopen(), zdioctl();

#if defined(BOOTXX)
struct devsw devsw[] = {
	{ "rs", rsstrategy, rsopen, 0, 0, 0, D_PACKET },
	{ "sd",	sdstrategy, sdopen, 0, 0, 0, D_DISK },
	{ "wd",	wdstrategy, wdopen, 0, 0, 0, D_DISK },
	{ "ts",	tsstrategy, tsopen, 0, 0, 0, D_TAPE },
	{ "xp",	xpstrategy, xpopen, 0, 0, 0, D_DISK },
	{ "xt",	xtstrategy, xtopen, 0, 0, 0, D_TAPE },
	{ "tm",tmstrategy, tmopen, tmclose, 0, 0, D_TAPE },
	{ "zd",	zdstrategy, zdopen, 0, 0, 0, D_DISK },
};
#else
struct devsw devsw[] = {
	{ "rs", rsstrategy, rsopen, rsclose, rsioctl, rslseek, D_PACKET },
	{ "sd",	sdstrategy, sdopen, nullsys, sdioctl, nulllseek, D_DISK },
	{ "wd",	wdstrategy, wdopen, nullsys, wdioctl, nulllseek, D_DISK },
	{ "ts",	tsstrategy, tsopen, tsclose, tsioctl, nulllseek, D_TAPE },
	{ "xp",	xpstrategy, xpopen, nullsys, xpioctl, nulllseek, D_DISK },
	{ "xt",	xtstrategy, xtopen, xtclose, nullsys, nulllseek, D_TAPE },
	{ "tm", tmstrategy, tmopen, tmclose, tmioctl, nulllseek, D_TAPE },
	{ "zd",	zdstrategy, zdopen, nullsys, zdioctl, nulllseek, D_DISK },
	{ 0, }
};
#endif
int n_devsw = sizeof(devsw)/sizeof(devsw[0]);
