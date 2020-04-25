/* $Copyright:	$
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
#ifdef QUOTA
#ifndef	lint
static	char	rcsid[] = "$Header: quota_syscalls.c 1.10 91/02/28 $";
#endif

/*
 * quota_syscalls.c
 *	Quota system calls.
 */

/* @(#)quota_syscalls.c	1.5 87/09/10 3.2/4.3NFSSRC */
/* @(#)quota_syscalls.c	1.3 86/12/18 NFSSRC */
/* @(#)quota_syscalls.c 1.1 86/09/25 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* $Log:	quota_syscalls.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/vmmeter.h"

#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"

#include "../balance/slic.h"

#include "../machine/hwparam.h"
#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

/*
 * Sys call to allow users to find out
 * their current position wrt quota's
 * and to allow super users to alter it.
 */
quotactl()
{
	register struct a {
		int	cmd;
		caddr_t	fdev;
		int	uid;
		caddr_t	addr;
	} *uap = (struct a *)u.u_ap;
	struct mount *mp;

	if (ndquot == 0) {
		nosys();		/* report error */
		return;
	}
	if (uap->uid < 0)
		uap->uid = (int)u.u_ruid;
	if (uap->uid != (int)u.u_ruid && !suser())
		return;
	if (uap->cmd == Q_SYNC && uap->fdev == NULL) {
		mp = 0;
	} else {
		/*
		 * fdevtomp() returns with vfs_mutex held.
		 */
		u.u_error = fdevtomp(uap->fdev, &mp);
		if (u.u_error)
			return;
	}
	/*
	 * We are now holding the vfs_mutex semaphore.  This keeps
	 * the mounted filesystem from being unmounted under us.
	 */
	switch (uap->cmd) {

	case Q_QUOTAON:
		u.u_error = opendq(mp, uap->addr);
		v_sema(&vfs_mutex);
		break;

	case Q_QUOTAOFF:
		u.u_error = closedq(mp);
		v_sema(&vfs_mutex);
		break;

	case Q_SETQUOTA:
	case Q_SETQLIM:
		u.u_error = setquota(uap->cmd, (short)uap->uid, mp, uap->addr);
		break;

	case Q_GETQUOTA:
		u.u_error = getquota((short)uap->uid, mp, uap->addr);
		break;

	case Q_SYNC:
		v_sema(&vfs_mutex);	/* ok to drop: qsync() is heuristic */
		u.u_error = qsync(mp);
		break;

	default:
		v_sema(&vfs_mutex);
		u.u_error = EINVAL;
		break;
	}
}

/*
 * Set the quota file up for a particular file system.
 * Called as the result of a quotactl system call.
 */
static int
opendq(mp, addr)
	register struct mount *mp;
	caddr_t addr;			/* quota file */
{
	struct vnode *vp;
	struct dquot *dqp;
	int error;

	if (!suser())
		return (u.u_error);
	error =
	    lookupname(addr, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (error) {
		return (error);
	}
	if (VFSTOM(vp->v_vfsp) != mp || vp->v_type != VREG) {
		VN_PUT(vp);
		return (EACCES);
	}
	/*
	 * There can be no quotas on the quota file.  If a quota
	 * structure was assigned to the quota file remove it.
	 */
	dqp = VTOI(vp)->i_dquot;
	if (dqp != NULL) {
		dqrele(dqp);
		VTOI(vp)->i_dquot = NULL;
	}
	if (mp->m_qinod != NULL) {
		/*
		 * Unlock the vnode for the quota file.  It is  the
		 * same as is currently in use and closedq will lock it.
		 * Relock it after the call to closedq.
		 */
		VN_UNLOCKNODE(vp);
		(void)closedq(mp);
		VN_LOCKNODE(vp);
	}
	mp->m_qinod = VTOI(vp);
	VN_UNLOCKNODE(vp);
	mp->m_qflags = 0;
	/*
	 * Timelimits for the super user set the relative time
	 * the other users can be over quota for this file system.
	 * If it is zero a default is used (see param.c).
	 */
	dqp = getdiskquota(0, mp);
	if (dqp != NULL) {
		mp->m_btimelimit =
		    (dqp->dq_btimelimit? dqp->dq_btimelimit: dq_btimedefault);
		mp->m_ftimelimit =
		    (dqp->dq_ftimelimit? dqp->dq_ftimelimit: dq_ftimedefault);
		UNLOCK_DQ(dqp);
		dqrele(dqp);
	} else {
		mp->m_btimelimit = dq_btimedefault;
		mp->m_ftimelimit = dq_ftimedefault;
	}
	return (0);
}

/*
 * Close off disk quotas for a file system.
 */
int
closedq(mp)
	register struct mount *mp;
{
	register struct dquot *dqp;
	register struct inode *ip;
	spl_t s;

	if (!suser())
		return (u.u_error);
	if (mp->m_qinod == NULL)
		return (0);
	/*
	 * Prevent new inodes in this filesystem from referencing dquots.
	 */
	mp->m_qflags |= Q_CLOSING;
	/*
	 * Run down the inode table and release all dquots assciated with
	 * inodes on this filesystem.  We have to lock the ino_list, so
	 * that iget will be locked out and will not be able to claim 
	 * inodes from the free list.
	 *
	 * We lock the quota_list so that dquot structures cannot be
	 * allocated or deallocated.  The dqrele may do a write, but
	 * since the filesystem is marked as quota's closing new inode's
	 * will have NULL i_dquot fields.  Therefore, we can continue
	 * scanning from where we left off.
	 */
	s = p_lock(&quota_list, SPLFS);
	(void) p_lock(&ino_list, SPLFS);
	for (ip = inode; ip < inodeNINODE; ip++) {
		dqp = ip->i_dquot;
		if (dqp != NULL && dqp->dq_mp == mp) {
			v_lock(&ino_list, SPLFS);
			v_lock(&quota_list, s);
			/*
			 * Get a reference to the inode and lock it.
			 * Since we unlocked the quota_list and ino_list,
			 * we must verify we are still interested in him.
			 * Then we can clear the pointer to the quota
			 * structure and unlock the inode.
			 */
			iref(ip);
			dqp = ip->i_dquot;
			if (dqp != NULL && dqp->dq_mp == mp) {
				ip->i_dquot = NULL;
			}
			dqrele(dqp);
			IPUT(ip);
			(void) p_lock(&quota_list, SPLFS);
			(void) p_lock(&ino_list, SPLFS);
		}
	}
	v_lock(&ino_list, SPLFS);
	/*
	 * Run down the dquot table and check whether dquots for this
	 * filesystem are still referenced. If not take them off their
	 * hash list and put them on a private, unfindable hash list.
	 */
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		if (dqp->dq_mp == mp) {
			if (dqp->dq_cnt == 0) {
				remque(dqp);
				dqp->dq_forw = dqp;
				dqp->dq_back = dqp;
				dqp->dq_mp = NULL;
			} else
				 ASSERT(mp->m_qcnt != 0,"closedq: stray dquot");
				/*
				 *+ After releasing all of the disk quota
				 *+ structures for a filesystem, the kernel
				 *+ found a disk quota structure
				 *+ with a nonzero reference
				 *+ count for that filesystem.
				 */
		}
	}
	v_lock(&quota_list, s);

	/*
	 * If there are still disk quota structures active, wait
	 * for them to become inactive.
	 */
	while (mp->m_qcnt != 0) {
		p_sema(&quota_sema, PZERO);
	}

	/*
	 * Release the quota file inode.
	 */
	IRELE(mp->m_qinod);
	mp->m_qinod = NULL;
	return (0);
}

/*
 * Set various feilds of the dqblk according to the command.
 * Q_SETQUOTA - assign an entire dqblk structure.
 * Q_SETQLIM - assign a dqblk structure except for the usage.
 */
static int
setquota(cmd, uid, mp, addr)
	int cmd;
	short uid;
	struct mount *mp;
	caddr_t addr;
{
	register struct dquot *dqp;
	struct dqblk newlim;
	int error;

	if (!suser()) {
		v_sema(&vfs_mutex);
		return (u.u_error);
	}
	dqp = getdiskquota(uid, mp);
	/*
	 * We now have the disk quota structure locked so we can release
	 * Semaphore on the mount structures.
	 */
	v_sema(&vfs_mutex);
	if (dqp == NULL)
		return (ESRCH);
	error = copyin(addr, (caddr_t)&newlim, sizeof (struct dqblk));
	if (error)
		return (error);
	/*
	 * Don't change disk usage on Q_SETQLIM
	 */
	if (cmd == Q_SETQLIM) {
		newlim.dqb_curblocks = dqp->dq_curblocks;
		newlim.dqb_curfiles = dqp->dq_curfiles;
	}
	if (uid == 0) {
		/*
		 * Timelimits for the super user set the relative time
		 * the other users can be over quota for this file system.
		 * If it is zero a default is used (see quota.h).
		 */
		mp->m_btimelimit =
		    newlim.dqb_btimelimit? newlim.dqb_btimelimit: dq_btimedefault;
		mp->m_ftimelimit =
		    newlim.dqb_ftimelimit? newlim.dqb_ftimelimit: dq_ftimedefault;
	} else {
		/*
		 * If the user was under quota before, set timelimit to zero.
		 * If the (l)user is now over quota, the timelimit will start
		 * the next time he does an allocation.
		 */
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit)
			newlim.dqb_btimelimit = 0;
		if (dqp->dq_curfiles < dqp->dq_fsoftlimit)
			newlim.dqb_ftimelimit = 0;
	}
	dqp->dq_dqb = newlim;
	dqp->dq_flags |= DQ_MOD;
	UNLOCK_DQ(dqp);
	dqupdate(dqp);
	dqrele(dqp);
	return (0);
}

/*
 * Q_GETDLIM - return current values in a dqblk structure.
 */
static int
getquota(uid, mp, addr)
	short uid;
	struct mount *mp;
	caddr_t addr;
{
	register struct dquot *dqp;
	int error;

	dqp = getdiskquota(uid, mp);
	/*
	 * We now have the disk quota structure locked so we can release
	 * Semaphore on the mount structures.
	 */
	v_sema(&vfs_mutex);
	if (dqp == NULL)
		return (ESRCH);
	SET_BUSY(dqp);
	error = copyout((caddr_t)&dqp->dq_dqb, addr, sizeof (struct dqblk));
	CLEAR_BUSY(dqp);
	UNLOCK_DQ(dqp);
	dqrele(dqp);
	return (error);
}

/*
 * Q_SYNC - sync quota files to disk.
 */
static int
qsync(mp)
	register struct mount *mp;
{
	register struct dquot *dqp;
	spl_t s;

	if (!suser())
		return (u.u_error);			/* XXX */
	/*
	 * Force any quota structures for this device out to disk. The
	 * quota list is not locked, since any new entries that were added
	 * while the syncing is going on could have happened immediately
	 * after the sync.
	 */
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		if ((dqp->dq_flags & DQ_MOD) == 0 || (mp && dqp->dq_mp != mp))
			continue;
		LOCK_DQ(dqp);
		s = p_lock(&quota_list, SPLFS);
		if ((dqp->dq_flags & DQ_MOD) == 0
		     || (mp && dqp->dq_mp != mp)) {
			v_lock(&quota_list, s);
			UNLOCK_DQ(dqp);
			continue;
		     }
		ASSERT_DEBUG(dqp->dq_cnt != 0, "qsync: dq_cnt == 0");
		++dqp->dq_cnt;
		v_lock(&quota_list, s);
		UNLOCK_DQ(dqp);
		dqupdate(dqp);
		dqrele(dqp);
	}
	return (0);
}

/*
 * Convert the name pointed at by fdev to a pointer into the mount table.
 * If successfull it updates *mpp to point to the correct mount structure,
 * hold the vfs_mutex semaphore and retuns a 0.  Otherwise it returns an
 * error code.
 */
static int
fdevtomp(fdev, mpp)
	char *fdev;
	struct mount **mpp;
{
	struct vnode *vp;
	dev_t dev;
	int error;

	error =
	    lookupname(fdev, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	if (vp->v_type != VBLK) {
		VN_PUT(vp);
		return (ENOTBLK);
	}
	dev = vp->v_rdev;

	VN_PUT(vp);
	p_sema(&vfs_mutex, PVFS);			/* stablize "getmp" */
	*mpp = getmp(dev);

	if (*mpp == NULL) {
		v_sema(&vfs_mutex);
		return (ENODEV);
	} else
		return (0);
}
#endif /* QUOTA */
