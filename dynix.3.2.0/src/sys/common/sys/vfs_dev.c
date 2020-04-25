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
static	char	rcsid[] = "$Header: vfs_dev.c 2.7 90/06/10 $";
#endif

/*
 * vfs_dev.c
 *	Virtual File-System device-node handling.
 */

/* $Log:	vfs_dev.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/time.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"

/*
 * Convert a dev into a vnode pointer suitable for bio.
 */

static 
devtovp_badop()
{
	panic("devtovp_badop");
	/*
	 *+ A bad dev to vnode VOP has been requested.
	 */
}

static int
devtovp_inactive(vp)
	struct vnode *vp;
{
	VN_UNLOCK(vp);
}

static int
devtovp_strategy(bp)
	struct buf *bp;
{
	(*bdevsw[major(bp->b_vp->v_rdev)].d_strategy)(bp);
	return (0);
}

static int
devtovp_minphys(bp)
	struct buf *bp;
{
	(*bdevsw[major(bp->b_dev)].d_minphys)(bp);
	return (0);
}

struct vnodeops dev_vnode_ops = {
	devtovp_badop,			/* vn_open */
	devtovp_badop,			/* vn_close */
	devtovp_badop,			/* vn_rdwr */
	devtovp_badop,			/* vn_ioctl */
	devtovp_badop,			/* vn_select */
	devtovp_badop,			/* vn_getattr */
	devtovp_badop,			/* vn_setattr */
	devtovp_badop,			/* vn_access */
	devtovp_badop,			/* vn_lookup */
	devtovp_badop,			/* vn_create */
	devtovp_badop,			/* vn_remove */
	devtovp_badop,			/* vn_link */
	devtovp_badop,			/* vn_rename */
	devtovp_badop,			/* vn_mkdir */
	devtovp_badop,			/* vn_rmdir */
	devtovp_badop,			/* vn_readdir */
	devtovp_badop,			/* vn_symlink */
	devtovp_badop,			/* vn_csymlink */
	devtovp_badop,			/* vn_readlink */
	devtovp_badop,			/* vn_readclink */
	devtovp_badop,			/* vn_fsync */
	devtovp_inactive,		/* vn_inactive */
	devtovp_badop,			/* vn_bmap */
	devtovp_strategy,		/* vn_strategy */
	devtovp_badop,			/* vn_bread */
	devtovp_badop,			/* vn_brelse */
	devtovp_minphys			/* vn_minphys */
};

lock_t		devnode_mutex;			/* mutex for adding to list */
struct	vnode	*devnode;			/* array of devnode's */
struct	vnode	*devnode_max;			/* current max alloc'd one */

/*
 * devnode_init()
 *	Init array of device-vnodes.
 */

devnode_init()
{
	init_lock(&devnode_mutex, G_NFS);
	devnode_max = devnode - 1;		/* ==> empty list */
}

/*
 * devtovp()
 *	Allocate a new device-vnode.
 *
 * The list can grow, but never shrink. This makes mutex simple,
 * because we can search the list without locking it (see DEVTOVP()). 
 * We must only ensure that a new vnode to be added is completely
 * valid before we add it to the list.
 *
 * We can race here, if multiple processes want to be the first to
 * allocate a vnode for a device. Devtovp handles this by searching
 * the list under lock, in case the vnode has been added between the
 * time the inline search failed and the point at which we obtain the lock.
 *
 * This routine has no impact on performance. Its only enterred for the first
 * (or first few, if races) references to a blk device.
 *
 * The original implementation used the SUN heap manager, which had numerous
 * problems.  If a good heap manager is created, the old implemenation could
 * come back.
 *
 * Could consider hashing devnodes if the number starts to get large.
 */ 

struct vnode *
devtovp(dev)
	dev_t	dev;
{
	register struct	vnode	*newvp;
	register struct	vnode	*dvp;
	spl_t	s;

	/*
	 * Allocate a new vnode, since we will probably need one.
	 * Since devnode_max is current highest active, take next one.
	 */

	s = p_lock(&devnode_mutex, SPLFS);

	newvp = devnode_max + 1;
	ASSERT(newvp < devnode+ndevnode, "devtovp: no more devnodes");
	/*
	 *+ An insuffient number of device vnode have been configured.
	 *+ If the system paramter NDEVNODE is non-zero increas its value.
	 *+ If it is zero then the system has mis-calulated the required
	 *+ number of device vnodes.
	 */

	newvp->v_op = &dev_vnode_ops;
	newvp->v_rdev = dev;
	newvp->v_flag = VNBACCT;		/* let bio avoid VN_HOLD/RELE */
	/*
	 * If its B_TAPE, then let bdwrite() know to do a bawrite().
	 */
	if (bdevsw[major(dev)].d_flags & B_TAPE)
		newvp->v_flag |= VBTAPE;
	init_lock(&newvp->v_mutex, G_NFS);

	/*
	 * Can race with another call to devtovp() creating the entry
	 * we want to create.  Thus, search table to insure no duplicates.
	 */

	for (dvp = devnode; dvp <= devnode_max; dvp++) {
		if (dvp->v_rdev == dev) {
			/*
			 * Lost a race: return found one.
			 */
			v_lock(&devnode_mutex, s);
			return (dvp);
		}
	}

	/*
	 * Didn't find it ==> "newvp" is it.  Update devnode_max to
	 * reflect now larger list.  Note that devnode_max only changes
	 * under devnode_mutex lock.
	 */

	devnode_max = newvp;
	v_lock(&devnode_mutex, s);

	return (newvp);
}
