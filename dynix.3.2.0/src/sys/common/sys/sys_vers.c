/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static	char	rcsid[] = "$Header: sys_vers.c 1.2 1991/04/25 20:55:15 $";
#endif

/*
 * return the version string and other information to the user.
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/user.h"

extern	char *bootname;
extern  int   boot_len;
extern  char version[];

get_vers()
{
	register struct a {
		int	arg1;
		u_int	arg2;
		caddr_t	arg3;
	} *uap = (struct a *)u.u_ap;
	u_int len;

	switch (uap->arg1) {

	case 0:		/* The version string */
		len = uap->arg2;
		u.u_error = copyout((caddr_t)version, 
					(caddr_t)uap->arg3, len);

		break;
	case 1:		/* The name of the last booted kernel */
		if (boot_len < uap->arg2)
			len = boot_len;
		else
			len = uap->arg2;
		u.u_error = copyout((caddr_t)bootname, 
					(caddr_t)uap->arg3, len);
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
}
