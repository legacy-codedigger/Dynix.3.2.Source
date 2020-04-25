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
static	char	rcsid[] = "$Header: kern_xxx.c 2.4 90/06/10 $";
#endif

/*
 * kern_xxx.c
 *	Get/Set host-id/host-name.
 *
 * Also contains reboot().  Original had much COMPAT code; all such
 * removed from this version.
 */

/* $Log:	kern_xxx.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/reboot.h"
#include "../h/vfs.h"

#include "../balance/engine.h"

gethostid()
{
	u.u_r.r_val1 = hostid;
}

sethostid()
{
	struct a {
		int	hostid;
	} *uap = (struct a *)u.u_ap;

	if (suser())
		hostid = uap->hostid;
}

gethostname()
{
	register struct a {
		char	*hostname;
		int	len;
	} *uap = (struct a *)u.u_ap;
	register u_int len;

	len = uap->len;
	if (len > hostnamelen + 1)
		len = hostnamelen + 1;
	u.u_error = copyout((caddr_t)hostname, (caddr_t)uap->hostname, len);
}

sethostname()
{
	register struct a {
		char	*hostname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;

	if (!suser())
		return;
	if (uap->len > sizeof (hostname) - 1) {
		u.u_error = EINVAL;
		return;
	}
	hostnamelen = uap->len;
	u.u_error = copyin((caddr_t)uap->hostname, hostname, uap->len);
	hostname[hostnamelen] = 0;
}

reboot()
{
	register struct a {
		int	opt;
	} *a = (struct a *)u.u_ap;
	int flags = a->opt;

	if (suser()) {
		if (nonline == 1) {
			if (!(flags & RB_NOSYNC)) {
				extern struct vfs *rootvfs;
				extern int (*rootfsunmount)();

				xumount(rootvfs);
				/*
				 * No need to sync since boot() does it.
				 * rootfsunmount doesn't really flush the
				 * various caches; it just marks the
				 * superblock as having been cleanly
				 * unmounted so fsck won't need to run
				 * on it after rebooting
				 */
				(*rootfsunmount)();
			}
			boot(RB_BOOT, flags);
			panic("boot returned");
			/*
			 *+ The system failed to return to its firmware
			 *+ monitor to boot or reboot.
			 */
		}
		u.u_error = EINVAL;
	}
}

getdomainname()
{
	register struct a {
		char	*domainname;
		int	len;
	} *uap = (struct a *)u.u_ap;
	register u_int len;

	len = uap->len;
	if (len > domainnamelen + 1)
		len = domainnamelen + 1;
	u.u_error = copyout((caddr_t)domainname,(caddr_t)uap->domainname,len);
}

setdomainname()
{
	register struct a {
		char	*domainname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;

	if (!suser())
		return;
	if (uap->len > sizeof (domainname) - 1) {
		u.u_error = EINVAL;
		return;
	}
	domainnamelen = uap->len;
	u.u_error = copyin((caddr_t)uap->domainname, domainname, uap->len);
	domainname[domainnamelen] = 0;
}
