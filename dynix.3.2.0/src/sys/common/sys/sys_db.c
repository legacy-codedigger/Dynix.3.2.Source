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
static	char	rcsid[] = "$Header: sys_db.c 1.3 90/02/20 $";
#endif

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/uio.h"

/*
 * sys_db.c - "interim" system calls to support
 * enhanced database performance.
 *
 * 0-1) read/write bypassing the buffer cache
 *   2) optional caching of bmap work
 * 3-4) fork with shared resources (file descriptors)
 *   5) tmp_affinity of specific pid
 */

db_support()
{
	register struct a {
		int	arg1;
		int	arg2;
		int	arg3;
		int	arg4;
		int	arg5;
	} *uap = (struct a *)u.u_ap;

	switch (uap->arg1) {

	case 0:					/* Direct (RAW) file read */
		u.u_error = DirectIO(
				uap->arg2,		/* fd */
				(char*) uap->arg3,	/* buffer */
				uap->arg4,		/* count */
				(long) uap->arg5,	/* file offset */
				UIO_READ
			);
		return;
	case 1:					/* Direct (RAW) file write */
		u.u_error = DirectIO(
				uap->arg2,		/* fd */
				(char*) uap->arg3,	/* buffer */
				uap->arg4,		/* count */
				(long) uap->arg5,	/* file offset */
				UIO_WRITE
			);
		return;
	case 2:					/* Bmap Cache On/Off */
		u.u_error = SysBmapCache(
				uap->arg2,	/* fd */
				uap->arg3	/* flavor */
			);
		return;
	case 3:					/* Shared fork */
		shfork(
			uap->arg2		/* resource mask */
		);
		return;
	case 4:					/* Shared vfork */
		shvfork(
			uap->arg2		/* resource mask */
		);
		return;
	case 5:					/* Affinity PID */
		affinitypid(
			uap->arg2,		/* pid */
			uap->arg3		/* engno */
		);
	}
}
