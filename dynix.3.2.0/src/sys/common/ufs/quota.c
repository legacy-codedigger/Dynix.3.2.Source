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
static	char	rcsid[] = "$Header: quota.c 1.12 91/02/28 $";
#endif

/*
 * quota.c
 *	Code pertaining to management of the in-core data structures.
 */

/* @(#)quota.c	1.4 87/09/10 3.2/4.3NFSSRC */
/* @(#)quota.c	1.3 86/12/18 NFSSRC */
/* @(#)quota.c 1.1 86/09/25 Copyr 1985 Sun Micro */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* $Log:	quota.c,v $
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
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"

#include "../balance/slic.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/hwparam.h"
#include "../machine/vmparam.h"

/*
 * DQHASH - hash function for disk quota structures.
 */
#define	DQHASH(uid, mp) \
	(((unsigned)(mp) + (unsigned)(uid)) % ndqhash)

/*
 * Dquot free list.
 */
struct dquot dqfreelist;

typedef	struct dquot *DQptr;

/*
 * Initialize quota caches, lock for maniulating caches and freelist, put
 * each dquot structures on the free list and initialize its semaphore
 * and lock.
 */
void
qtinit()
{
	register struct dqhead *dhp;
	register struct dquot *dqp;

	init_lock(&quota_list, G_QUOTA);
	init_sema(&quota_sema, 0, 0, G_QUOTA); 

	if (ndquot == 0)
		return;
	/*
	 * Initialize the cache between the in-core structures
	 * and the per-filesystem quota files on disk.
	 */
	for (dhp = &dqhead[0]; dhp < &dqhead[ndqhash]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (DQptr)dhp;
	}

	/*
	 * Initialize the disk quota structures and put them on the free list.
	 */
	dqfreelist.dq_freef = dqfreelist.dq_freeb = &dqfreelist;
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		init_sema(&dqp->dq_sema, 0, 0, G_QUOTA); 
		init_lock(&dqp->dq_lock, G_QUOTA);
		dqp->dq_forw = dqp->dq_back = dqp;
		dqp->dq_freef = dqfreelist.dq_freef;
		dqp->dq_freeb = &dqfreelist;
		dqfreelist.dq_freef->dq_freeb = dqp;
		dqfreelist.dq_freef = dqp;
	}
}

/*
 * Obtain the user's on-disk quota limit for filesystem specified.
 *	Returns disk quota locked with count incremented,
 *	        or NULL if no quota.
 * 
 * To prevent problems with another process turning off QUOTA's or trying
 * to unmount the filesystem, we have to check for the Q_CLOSING flag in
 * the mount table entry for the filesystem after we have found a
 * disk quota entry.  We are holding the quot_list lock which prevents
 * movement of the quota structures from one list to another, but we
 * may have to do I/O to get vaild data in the quota structure and the lock
 * will be released while doing I/O.  Once we have the lock for the quota
 * structure, it is protected from unmount and closedq, until the lock is
 * released.
 */
struct dquot *
getdiskquota(uid, mp)
	short uid;
	register struct mount *mp;
{
	register struct dquot *dqp = NULL;
	register struct dqhead *dhp;
	int error;
	spl_t s;

	if (mp->m_qinod == NULL || mp->m_qflags & Q_CLOSING)
		return (NULL);
	/*
	 * Check the cache first.
	 */
	dhp = &dqhead[DQHASH(uid, mp)];
	s = p_lock(&quota_list, SPLFS);
loop:
	for (dqp = dhp->dqh_forw; dqp != (DQptr)dhp;
	     dqp = dqp->dq_forw) {
		if (dqp->dq_uid == uid && dqp->dq_mp == mp) {
			/*
			 * If the quotas are being turned off for this
			 * filesystem return NULL to indicate no quota.
			 */
			if (mp->m_qflags & Q_CLOSING) {
				v_lock(&quota_list, s);
				return (NULL);
			}
			/*
			 * Now that we have found it, lock it and make sure
			 * it is still valid.  It could have been removed
			 * due to an I/O error, while it was being read in
			 * from disk.
			 */
			v_lock(&quota_list, s);
			LOCK_DQ(dqp);
			(void) p_lock(&quota_list, SPLFS);
			if (dqp->dq_uid != uid && dqp->dq_mp != mp) {
				UNLOCK_DQ(dqp);
				goto loop;
			}
			/*
			 * Cache hit with no references.
			 * Take the structure off the free list.
			 */
			if (dqp->dq_cnt == 0) {
				dqp->dq_freeb->dq_freef = dqp->dq_freef;
				dqp->dq_freef->dq_freeb = dqp->dq_freeb;
				(void) p_lock(&mp->m_lock, SPLFS);
				mp->m_qcnt++;
				v_lock(&mp->m_lock, SPLFS);
			}
			dqp->dq_cnt++;
			v_lock(&quota_list, s);
			return (dqp);		/* returns dqp locked */
		}
	}
	/*
	 * Not in cache.
	 * Get dquot at head of free list and lock it.
	 */
	do {
		if ((dqp = dqfreelist.dq_freef) == &dqfreelist) {
			v_lock(&quota_list, s);
			tablefull("dquot");
			u.u_error = EUSERS;
			return (NULL);
		}
		/*
		 * We can not lock the quota structure while holding the
		 * quota_list lock.  So, we have to check to make sure the
		 * quota structure is still on the free list after we lock
		 * it.  If it is not, release the lock and tray again.
		 */
		v_lock(&quota_list, s);
		LOCK_DQ(dqp);
		(void) p_lock(&quota_list, SPLFS);
		if (dqp != dqfreelist.dq_freef)
			UNLOCK_DQ(dqp);
	} while (dqp != dqfreelist.dq_freef);
	/*
	 * This shouldn't happen, as we sync dquots before
	 * freeing them.
	 */
	ASSERT((dqp->dq_flags & DQ_MOD) == 0, "diskquota");
	/*
	 *+ The kernel obtained a disk quota structure from the free list that
	 *+ was marked as dirty.
	 */

	/*
	 * If the quotas are being turned off for this filesystem
	 * return NULL to indicate no quota.
	 */
	if (mp->m_qflags & Q_CLOSING) {
		v_lock(&quota_list, s);
		UNLOCK_DQ(dqp);
		return (NULL);
	}
	ASSERT_DEBUG(dqp->dq_cnt == 0, "getdiskquota: dq_cnt non-zero");
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 */
	(void) p_lock(&mp->m_lock, SPLFS);
	mp->m_qcnt++;
	v_lock(&mp->m_lock, SPLFS);
	dqp->dq_freeb->dq_freef = dqp->dq_freef;
	dqp->dq_freef->dq_freeb = dqp->dq_freeb;
	remque(dqp);

	/*
	 * Put dquot on correct hash list, so others can find it, while we
	 * are doing I/O, and wait.
	 */
	insque(dqp, dhp);
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_mp = mp;
	dqp->dq_flags = 0;
	SET_BUSY(dqp);
	v_lock(&quota_list, s);
	ILOCK(mp->m_qinod);
	/*
	 * Read quota info off disk.
	 */
	error = rdwri(UIO_READ, mp->m_qinod, (caddr_t)&dqp->dq_dqb,
		      sizeof(struct dqblk), dqoff(uid),	UIOSEG_KERNEL,
		      (int *)0);
	IUNLOCK(mp->m_qinod);
	if (error) {
		/*
		 * I/O error in reading quota file.  This was probably do to
		 * trying to read an entry that does not exist, so we will
		 * just supply a entry will all zeros.  If there really was
		 * an I/O error, this is probablly better than continually
		 * trying to read a bad spot on the disk.
		 */
		dqp->dq_bhardlimit = 0;
		dqp->dq_bsoftlimit = 0;
		dqp->dq_curblocks = 0;
		dqp->dq_fhardlimit = 0;
		dqp->dq_fsoftlimit = 0;
		dqp->dq_curfiles = 0;
		dqp->dq_btimelimit = 0;
		dqp->dq_ftimelimit = 0;
		CLEAR_BUSY(dqp);
		UNLOCK_DQ(dqp);
		return (dqp);
	}
	CLEAR_BUSY(dqp);
	return (dqp);			/* returns dqp locked */
}

/*
 * Release dquot.
 *	Called with dq_lock unlocked.
 *	Returns with dq_lock unlocked.
 */
void
dqrele(dqp)
	register struct dquot *dqp;
{
	spl_t s;
	struct mount *mp;

	if (dqp == NULL)
		return;
	s = p_lock(&quota_list, SPLFS);
	ASSERT(dqp->dq_cnt != 0, "dqrele");
	/*
	 *+ The kernel tried to release a disk quota stucture that has a zero
	 *+ reference count.
	 */
	if (dqp->dq_cnt == 1) {
		if (dqp->dq_flags & DQ_MOD) {
			v_lock(&quota_list, s);
			dqupdate(dqp);
			(void) p_lock(&quota_list, SPLFS);
		}
		/*
		 * Make sure ref count is still 1 after sleeping for i/o.
		 */
		if (dqp->dq_cnt == 1) {
			dqp->dq_freeb = dqfreelist.dq_freeb;
			dqp->dq_freef = &dqfreelist;
			dqfreelist.dq_freeb->dq_freef = dqp;
			dqfreelist.dq_freeb = dqp;
			dqp->dq_cnt--;
			mp = dqp->dq_mp;
			v_lock(&quota_list, s);
			(void) p_lock(&mp->m_lock, SPLFS);
			mp->m_qcnt--;
			if ((mp->m_qcnt == 0) && blocked_sema(&quota_sema))
				vall_sema(&quota_sema);
			v_lock(&mp->m_lock, s);
			return;
		}
	}
	dqp->dq_cnt--;
	v_lock(&quota_list, s);
}

/*
 * Update on disk quota info.
 *	Called with dq_lock unlocked.
 *	Returns with dq_lock unlocked.
 */
void
dqupdate(dqp)
	register struct dquot *dqp;
{

	LOCK_DQ(dqp);
	if (dqp->dq_flags & DQ_MOD) {
		register struct inode *qip;

		dqp->dq_flags &= ~DQ_MOD;
		qip = dqp->dq_mp->m_qinod;
		ASSERT(qip != NULL, "dqupdate");
		/*
		 *+ The kernel attempted to update the disk copy of the
		 *+ quota structure, but the inode
		 *+ pointer in the quota structure is NULL.
		 */
		SET_BUSY(dqp);
		ILOCK(qip);
		(void) rdwri(UIO_WRITE, qip,
		    (caddr_t)&dqp->dq_dqb,
		    sizeof (struct dqblk), dqoff(dqp->dq_uid),
		    UIOSEG_KERNEL, (int *)0);
		IUNLOCK(qip);
		CLEAR_BUSY(dqp);
	}
	UNLOCK_DQ(dqp);
}
#endif /* QUOTA */
