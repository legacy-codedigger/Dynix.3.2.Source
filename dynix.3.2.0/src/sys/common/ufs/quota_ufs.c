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
static	char	rcsid[] = "$Header: quota_ufs.c 1.7 91/02/28 $";
#endif

/*
 * quota_ufs.c
 *	Routines used in checking limits on file system usage.
 */

/* @(#)quota_ufs.c	1.4 87/09/10 3.2/4.3NFSSRC */
/* @(#)quota_ufs.c	1.5 86/12/18 NFSSRC */
/* @(#)quota_ufs.c 1.1 86/09/25 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* $Log:	quota_ufs.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"

#include "../balance/slic.h"

#include "../machine/intctl.h"
#include "../machine/hwparam.h"
#include "../machine/vmparam.h"

/*
 * Find the dquot structure that should
 * be used in checking i/o on inode ip.
 * Returns the quota stucture unlocked.
 */
struct dquot *
getinoquota(ip)
	register struct inode *ip;
{
	register struct dquot *dqp;

	if (ndquot == 0)	/* are quota's configured ? */
		return (NULL);
	dqp = getdiskquota(ip->i_uid, VFSTOM(ITOV(ip)->v_vfsp));
	if (dqp != NULL &&
	    dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		UNLOCK_DQ(dqp);
		dqrele(dqp);
		dqp = NULL;
	}
	if (dqp != NULL) {
		UNLOCK_DQ(dqp);
	}
	return (dqp);
}

/*
 * Update disk usage, and take corrective action.
 * Assumes the inode is locked.
 */
int
chkdq(ip, change, force)
	struct inode *ip;
	long change;
	int force;
{
	register struct dquot *dqp;
	register u_long ncurblocks;
	char *warn_string = NULL;
	int error = 0;

	if (change == 0)
		return (0);
	dqp = ip->i_dquot;
	if (dqp == NULL)
		return (0);
	LOCK_DQ(dqp);
	dqp->dq_flags |= DQ_MOD;
	if (change < 0) {
		if ((int)dqp->dq_curblocks + change >= 0)
			dqp->dq_curblocks += change;
		else
			dqp->dq_curblocks = 0;
		dqp->dq_flags &= ~DQ_BLKS;
		UNLOCK_DQ(dqp);
		return (0);
	}

	ncurblocks = dqp->dq_curblocks + change;
	/*
	 * Allocation. Check hard and soft limits.
	 * Skip checks for super user.
	 */
	if (u.u_uid == 0)
		goto out;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	if (ncurblocks >= dqp->dq_bhardlimit && dqp->dq_bhardlimit && !force) {
		if ((dqp->dq_flags & DQ_BLKS) == 0 && ip->i_uid == u.u_ruid) {
			warn_string =
			    "\nDISK LIMIT REACHED (%s) - WRITE FAILED\n";
			dqp->dq_flags |= DQ_BLKS;
		}
		error = EDQUOT;
	}
	if (ncurblocks >= dqp->dq_bsoftlimit && dqp->dq_bsoftlimit) {
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit ||
		    dqp->dq_btimelimit == 0) {
			dqp->dq_btimelimit =
			    time.tv_sec +
			    VFSTOM(ITOV(ip)->v_vfsp)->m_btimelimit;
			if (ip->i_uid == u.u_ruid)
				warn_string =
				    "\nWARNING: disk quota (%s) exceeded\n";
		} else if (time.tv_sec > dqp->dq_btimelimit && !force) {
			if ((dqp->dq_flags & DQ_BLKS) == 0 &&
			    ip->i_uid == u.u_ruid) {
				warn_string =
				 "\nOVER DISK QUOTA: (%s) NO MORE DISK SPACE\n";
				dqp->dq_flags |= DQ_BLKS;
			}
			error = EDQUOT;
		}
	}
out:
	if (error == 0)
		dqp->dq_curblocks = ncurblocks;
	UNLOCK_DQ(dqp);
	if (warn_string) {
		uprintf(warn_string, ip->i_fs->fs_fsmnt);
		/*
		 *+ The user exceded the specified disk quota limit
		 *+ on the specified filesystem.
		 *+ Corrective action:  remove some files from the
		 *+ specified filesystem or have your limit increased.
		 */
	}
	return (error);
}

/*
 * Check the inode limit, applying corrective action.
 * Assumes the inode if provided is locked.
 */
int
chkiq(mp, ip, uid, force)
	struct mount *mp;
	struct inode *ip;
	short uid;
	int force;
{
	register struct dquot *dqp;
	register u_long ncurfiles;
	char *warn_string = NULL;
	int error = 0;

	if (ip != NULL) {		/* free */
		dqp = ip->i_dquot;
		if (dqp == NULL)
			return (0);
		LOCK_DQ(dqp);
		dqp->dq_flags |= DQ_MOD;
		if (dqp->dq_curfiles)
			dqp->dq_curfiles--;
		dqp->dq_flags &= ~DQ_FILES;
		UNLOCK_DQ(dqp);
		return (0);
	}

	/*
	 * Allocation. Get dquot for for uid, fs.
	 */
	dqp = getdiskquota(uid, mp);
	if (dqp == NULL) {
		return (0);
	} else if (dqp->dq_fsoftlimit == 0 && dqp->dq_fhardlimit == 0) {
		UNLOCK_DQ(dqp);
		dqrele(dqp);
		return (0);
	}
	dqp->dq_flags |= DQ_MOD;
	/*
	 * Skip checks for super user.
	 */
	if (u.u_uid == 0)
		goto out;
	ncurfiles = dqp->dq_curfiles + 1;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	if (ncurfiles >= dqp->dq_fhardlimit && dqp->dq_fhardlimit && !force) {
		if ((dqp->dq_flags & DQ_FILES) == 0 && uid == u.u_ruid) {
			warn_string =
			    "\nFILE LIMIT REACHED - CREATE FAILED (%s)\n";
			dqp->dq_flags |= DQ_FILES;
		}
		error = EDQUOT;
	} else if (ncurfiles >= dqp->dq_fsoftlimit && dqp->dq_fsoftlimit) {
		if (ncurfiles == dqp->dq_fsoftlimit || dqp->dq_ftimelimit==0) {
			dqp->dq_ftimelimit = time.tv_sec + mp->m_ftimelimit;
			if (uid == u.u_ruid)
				warn_string =
				    "\nWARNING - too many files (%s)\n";
		} else if (time.tv_sec > dqp->dq_ftimelimit && !force) {
			if ((dqp->dq_flags&DQ_FILES) == 0 && uid == u.u_ruid) {
				warn_string =
				    "\nOVER FILE QUOTA - NO MORE FILES (%s)\n";
				dqp->dq_flags |= DQ_FILES;
			}
			error = EDQUOT;
		}
	}
out:
	if (error == 0)
		dqp->dq_curfiles++;
	UNLOCK_DQ(dqp);
	dqrele(dqp);
	if (warn_string) {
		uprintf(warn_string, mp->m_bufp->b_un.b_fs->fs_fsmnt);
		/*
		 *+ The user exceded the specified disk quota limit
		 *+ on the specified filesystem.
		 *+ Corrective action:  remove some files from the
		 *+ specified filesystem or have your limit increased.
		 */
	}
	return (error);
}
#endif /* QUOTA */
