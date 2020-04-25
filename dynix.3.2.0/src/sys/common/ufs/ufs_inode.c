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
static	char	rcsid[] = "$Header: ufs_inode.c 2.36 1991/11/06 19:05:15 $";
#endif

/*
 * ufs_inode.c
 *	Inode management.
 */

/* $Log: ufs_inode.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/vmmeter.h"
#include "../ufs/mount.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

#define	INOHSZ	64
#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev,ino)	(((dev)+(ino))&(INOHSZ-1))
#else
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%INOHSZ)
#endif

union ihead {				/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[INOHSZ];

struct	inode *ifreeh, **ifreet;

lock_t	ino_list;			/* hash+free list-locker */

/*
 * Convert inode formats to vnode types
 */
enum vtype iftovt_tab[] = {
	VNON, VFIFO, VCHR, VBAD, VDIR, VBAD, VBLK, VBAD,
	VREG, VBAD, VLNK, VBAD, VSOCK, VBAD
};

int vttoif_tab[] = {
	0, IFREG, IFDIR, IFBLK, IFCHR, IFLNK, IFSOCK, IFIFO, IFMT
};

/*
 * ihinit()
 *	Initialize hash links for inodes and build inode free list.
 *
 * i_mutex sema's are set up to allow swapping.
 */

ihinit()
{
	register struct inode *ip = inode;
	register union  ihead *ih = ihead;
	register int i;
	register int gateno;
	register int curgate;

	init_lock(&ino_list, G_INODE);
	gateno = 0;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = ip;
	ifreet = &ip->i_freef;
	ip->i_freeb = &ifreeh;
	ip->i_forw = ip;
	ip->i_back = ip;
	ip->i_vnode.v_data = (caddr_t)ip;
	ip->i_vnode.v_op = &ufs_vnodeops;
	ip->i_vnode.v_mapx = VMAPX_NULL;
	/*
	 * v_exsema, and v_shsema and v_fllock all use the same gate as
	 * v_nodemutex because this is easy to do, and it also makes a
	 * certain amount of sense.  The number of concurrent references
	 * to that gate are bounded by the number of references to that inode.
	 *
	 * NOTE: could init i_flag=IFREE, but isn't really necessary, since
	 * iget() will do a "psvl" on first fetch.
	 */
	curgate = (G_INOMIN + gateno++ % (G_INOMAX-G_INOMIN+1));
	init_rwsema(&ITOV(ip)->v_nodemutex, RWSEMA_SWP, (gate_t)curgate); 
	init_lock(&ITOV(ip)->v_mutex, (gate_t)curgate);
	init_sema(&ITOV(ip)->v_exsema, 0, 0, (gate_t)curgate);
	init_sema(&ITOV(ip)->v_shsema, 0, 0, (gate_t)curgate);
	init_lock(&ITOV(ip)->v_fllock, (gate_t)curgate);
	for (i = ninode; --i > 0;) {
		++ip;
		ip->i_forw = ip;
		ip->i_back = ip;
		ip->i_vnode.v_data = (caddr_t)ip;
		ip->i_vnode.v_op = &ufs_vnodeops;
		ip->i_vnode.v_mapx = VMAPX_NULL;
		*ifreet = ip;
		ip->i_freeb = ifreet;
		ifreet = &ip->i_freef;
		curgate = (G_INOMIN + gateno++ % (G_INOMAX-G_INOMIN+1));
		init_rwsema(&ITOV(ip)->v_nodemutex, RWSEMA_SWP, (gate_t)curgate); 
		init_lock(&ITOV(ip)->v_mutex, (gate_t)curgate);
		init_sema(&ITOV(ip)->v_exsema, 0, 0, (gate_t)curgate);
		init_sema(&ITOV(ip)->v_shsema, 0, 0, (gate_t)curgate);
		init_lock(&ITOV(ip)->v_fllock, (gate_t)curgate);
	}
	ip->i_freef = NULL;
#ifdef	lint
	lint_ref_int(curgate);
#endif	lint
}

/*
 * iget()
 *	Look up an inode by device,inumber.
 *
 * If it is in core (in the inode structure), honor the locking protocol.
 * If it is not in core, read it in from the specified device.
 *
 * In all cases, a pointer to a locked inode structure is returned.
 */

struct inode *
iget(dev, fs, ino)
	dev_t	dev;
	struct fs *fs;
	ino_t	ino;
{
	register struct inode *ip;
	register union  ihead *ih;
	register struct buf *bp;
	register struct inode *iq;
	register struct dinode *dp;
	register struct mount *mp;
	spl_t	s;
#ifdef QUOTA
	struct dquot *old_dquot;
#endif

	ih = &ihead[INOHASH(dev, ino)];
loop:
	s = p_lock(&ino_list, SPLFS);			/* lock lists */

	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw) {
		if (ino == ip->i_number && dev == ip->i_dev) {
			if (ip->i_flag & IFREE) {	/* ino on free list */
				/*
				 * If on free-list, count == 0, and we
				 * can just "take" it (nobody else is trying
				 * for it right now).
				 *
				 * Take it off the free-list.
				 */
				if (iq = ip->i_freef)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
				ip->i_flag = 0;			/* zap IFREE */
				/*
				 * "take" it by declaring it locked, and
				 * set ref count = 1.  Is ok to not VN_LOCK(vp)
				 * to bump count, since vnode just came off
				 * free list ==> only race is with another
				 * iget(), which will block on the sema before
				 * trying to bump count.
				 *
				 * Must set v_count *after* "take" sema to
				 * avoid race with igrab().
				 */
				ASSERT_DEBUG(RWSEMA_IDLE(&ip->i_mutex),
						"iget: mutex");
				RWSEMA_SETWRBUSY(&ip->i_mutex);	/* locked */
				ASSERT_DEBUG(ITOV(ip)->v_count==0,
						"iget: free count");
				v_lock(&ino_list, s);
				ITOV(ip)->v_count = 1;		/* 1st ref */
				return (ip);
			}
			/*
			 * Not free ==> we must wait for it.  When the
			 * "p_sema" returns, we have the locked inode.
			 * This can occur with count == 0, since iinactive()
			 * won't put inode on free-list if somebody is
			 * waiting for the inode.
			 */
			p_writer_v_lock(&ip->i_mutex, PINOD, &ino_list);
			ASSERT_DEBUG(RWSEMA_WRBUSY(&ip->i_mutex),
					"iget: bad psvl");
			ASSERT_DEBUG(!(ip->i_flag & IFREE),
					"iget: unex free inode");
			VN_HOLD(ITOV(ip));
			if (ip->i_number != ino) {	/* error in prev iget */
				IPUT(ip);
				goto loop;
			}
			return (ip);
		}
	}

	/*
	 * Not found ==> get a free-one and reassign to our inode.
	 * Hash+free list is still locked.
	 */

	if ((ip = ifreeh) == NULL) {
		v_lock(&ino_list, s);
		/*
		 * If can get back some inodes, try again; else complain.
		 */
		if (xflush(0) || dnlc_purge())
			goto loop;
		tablefull("inode");
		u.u_error = ENFILE;
		return (NULL);
	}
	if (iq = ip->i_freef)
		iq->i_freeb = &ifreeh;
	ifreeh = iq;
	ip->i_freef = NULL;
	ip->i_freeb = NULL;

	ASSERT_DEBUG(RWSEMA_IDLE(&ip->i_mutex), "iget: free not avail2");

	/*
	 * Now to take inode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (ino, dev) pair
	 *
	 * Must set v_count *after* "take" sema to avoid race with igrab().
	 */

	remque(ip);
	insque(ip, ih);
#ifdef QUOTA
	/*
	 * Save old quota pointer so it can be released latter, after
	 * ino_list has been released.  The call to dqrele can block for
	 * I/O to the quota file
	 */

	old_dquot = ip->i_dquot;
	ip->i_dquot = NULL;
#endif
	ip->i_dev = dev;
	ip->i_number = ino;
	ip->i_flag = 0;				/* zap IFREE */
	RWSEMA_SETWRBUSY(&ip->i_mutex);		/* locked */
	ASSERT_DEBUG(ip->i_bmcache == NULL, "iget: i_bmcache !NULL");
	v_lock(&ino_list, s);			/* unlock lists */
#ifdef QUOTA
	if (old_dquot != NULL) {
		dqrele(old_dquot);
	}
#endif

	/*
	 * Inode is ours now, and locked.  Fill it out.
	 * Is ok to initialize fields outside ino_list lock since only
	 * hash-chain, i_dev, i_number, and IFREE flag are important in
	 * looking thru the cache.
	 */

	DEVTOVP(dev, ip->i_devvp);
	ip->i_diroff = 0;
	ip->i_fs = fs;
	ip->i_lastr = 0;

	bp = bread(ip->i_devvp, fsbtodb(fs, itod(fs, ino)), (int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		/*
		 * The inode doesn't contain anything useful, so it would
		 * be nice to remove it from the hash-chain.  This requires
		 * us to re-enter ino_list lock; since it doesn't hurt to,
		 * we'll leave it on the hash-list.  However, we zap the
		 * i_number field, so no-one will find it (and above
		 * waiting code will drop it).  It will eventually be
		 * reallocated and then rehashed.  Races on i_number
		 * are resolved by the looser dropping the inode.
		 */
		ip->i_number = 0;
		ip->i_nlink = 1;		/* avoid truncate */
		VN_LOCK(ITOV(ip));		/* iinactive needs locked */
		iinactive(ITOV(ip), NODEISLOCKED);
		return (NULL);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;			/* copy the i_common part */

	mp = getmp(dev);
	ASSERT(mp != NULL, "iget: bad dev");
	/*
	 *+ On trying to read an inode from disk, the superblock for that
	 *+ filesystem did not appear to be in memory.
	 */
	ASSERT(mp->m_bufp->b_un.b_fs == fs, "iget: bad fs");
	/*
	 *+ The pointer to the superblock passed in as an argument to the
	 *+ iget() function did not match the superblock pointer obtained
	 *+ by getting the superblock for the filesystem containing the inode.
	 */

	VN_INIT(ITOV(ip), &mp->m_vfs, IFTOVT(ip->i_mode), ip->i_rdev);
#if	ISVTX != VSVTX
	ERROR -- implementation assumes ISVTX==VSVTX
#endif
	ip->i_vnode.v_flag = (ip->i_mode & ISVTX);	/* assume ISVTX==VSVTX*/
	if (ino == (ino_t)ROOTINO)
		ip->i_vnode.v_flag |= VROOT;
	if (ip->i_pflags & IP_EXECMOD)
		ip->i_vnode.v_flag |= VMAPSYNC;	/* copy to independent part */

	brelse(bp);
#ifdef QUOTA
	if (ip->i_mode != 0 && 	VFSTOM(ITOV(ip)->v_vfsp)->m_qinod != NULL) {
		ASSERT_DEBUG(ip->i_dquot == NULL, "iget: i_dquot non-NULL");
		ip->i_dquot = getinoquota(ip);
	}
#endif
	return (ip);
}

#ifdef QUOTA
/*
 * iref()
 *	Lock and increment reference count for an unreferenced inode.
 *
 * This is modeled after the iget routine.  The locking is performed the
 * same way, so that no new race conditions will be introduced.
 *
 * Only called fron closedq() in ../ufs/quota_syscalls.c
 */

void
iref(ip)
	register struct inode *ip;
{
	struct inode *iq;
	spl_t	s;

	s = p_lock(&ino_list, SPLFS);			/* lock lists */

	if (ip->i_flag & IFREE) {	/* ino on free list */
		/*
		 * If on free-list, count == 0, and we
		 * can just "take" it (nobody else is trying
		 * for it right now).
		 *
		 * Take it off the free-list.
		 */
		if (iq = ip->i_freef)
			iq->i_freeb = ip->i_freeb;
		else
			ifreet = ip->i_freeb;
		*ip->i_freeb = iq;
		ip->i_freef = NULL;
		ip->i_freeb = NULL;
		ip->i_flag = 0;			/* zap IFREE */
		/*
		 * "take" it by declaring it locked, and
		 * set ref count = 1.  Is ok to not VN_LOCK(vp)
		 * to bump count, since vnode just came off
		 * free list ==> only race is with another
		 * iget(), which will block on the sema before
		 * trying to bump count.
		 *
		 * Must set v_count *after* "take" sema to
		 * avoid race with igrab().
		 */
		ASSERT_DEBUG(RWSEMA_IDLE(&ip->i_mutex), "iref: mutex");
		ASSERT_DEBUG(ITOV(ip)->v_count==0, "iref: free count");
		RWSEMA_SETWRBUSY(&ip->i_mutex);	/* locked */
		v_lock(&ino_list, s);
		ITOV(ip)->v_count = 1;		/* 1st ref */
		return;
	}
	/*
	 * Not free ==> we must wait for it.  When the
	 * "p_sema" returns, we have the locked inode.
	 * This can occur with count == 0, since iinactive()
	 * won't put inode on free-list if somebody is
	 * waiting for the inode.
	 */
	p_writer_v_lock(&ip->i_mutex, PINOD, &ino_list);
	ASSERT_DEBUG(RWSEMA_WRBUSY(&ip->i_mutex), "iref: bad psvl");
	ASSERT_DEBUG(!(ip->i_flag & IFREE), "iref: unex free inode");
	VN_HOLD(ITOV(ip));
}
#endif /* QUOTA */

/*
 * igrab()
 *	Lock and hold an inode that caller thinks is currently 
 *	referenced but unlocked.
 *
 * If these conditions no longer hold, then return (0),
 * else return locked inode with an added reference.
 *
 * iget() perverts semaphore protocol for efficiency.  We pay here for it.
 *
 * Called only by update().
 */

igrab(ip)
	register struct inode *ip;
{
	register struct vnode *vp;

	/*
	 * Race analysis:
	 *
	 * We know the inode is NOT on the free list because we check its
	 * count while holding the vnode lock.
	 *
	 * If ref count is 0 we are no longer interested, as the inode is
	 * (most likely) heading to the freelist, before which it will be
	 * iupdat()ed, else (less likely) it got picked up by an iget().
	 *
	 * As in iinactive(), holding the vnode lock until we see if we
	 * can lock the inode (TRY_ILOCK()) ensures that we dont race
	 * with iget.  If an iget() was in the process of getting this
	 * inode, then IT will have the inode locked at the point it trys
	 * to get the vnode lock in VN_HOLD.  In this case TRY_ILOCK()
	 * will fail, and freeing the vnode lock gives the inode to iget().
	 *
	 * Code in iget() is careful when pulling inode off free list to
	 * "lock" the sema and init v_count in that order, to avoid
	 * race with the test below.  Also, VN_INIT() doesn't re-init
	 * sema's or locks to avoid races.
	 *
	 * If we increment ref count, then no one releasing the inode
	 * will try to put it on the free list. If someone does release
	 * the inode, then we MAY be the one who calls iinactive() to
	 * free the inode().
	 *
	 * Alternative is to extract i_number and i_dev, lock ino_list,
	 * and see if can essentially "iget" the same inode.  Allow race,
	 * but back out if i_number or i_dev changed.  A similar technique
	 * is necessary for parallel iget() hash-bucket searches (maybe also
	 * for atomic inc/dec on v_count).
	 */

	vp = ITOV(ip);

	VN_LOCK(vp);
	if (vp->v_count != 0 && TRY_ILOCK(ip)) {
		++vp->v_count;
		VN_UNLOCK(vp);
		return (1);
	} else {
		VN_UNLOCK(vp);
		return (0);
	}
}

/*
 * iinactive()
 *	Vnode is no longer referenced, write the inode out and if necessary,
 *	truncate and deallocate the file.
 *
 * We are called with the vnode locked.
 * The inode itself MAY be locked, as indicated by the flag==NODEISLOCKED.
 */

iinactive(vp, flag)
	register struct vnode *vp;
	register flag;
{
	int	mode;
	spl_t	s;
	register struct inode *ip = VTOI(vp);

	/*
	 * We have the inode locked if we are called from VN_PUT() or IPUT().
	 * As soon as we know we have the inode, unlock the vnode.
	 * Holding the vnode locked until this point ensures that we will notice
	 * if a racing iget() finds this inode in the cache before we can put
	 * it on the free list. This works because holding the vnode locked
	 * ensures that iget() will spin in VN_HOLD until the TRY_ILOCK below
	 * is executed.
	 * We cant keep the vnode locked if we need to sleep in the code below
	 * (if for example we need to call itrunc), so we release it as soon
	 * as possible.
	 */

	if ((flag==NODEISLOCKED) || TRY_ILOCK(ip)) {
		ASSERT_DEBUG(ILOCKED(ip), "iinactive: unlocked");
		VN_UNLOCK(vp);

		/*
		 * Update inode if filesystem is writeable.
		 */

		if (ip->i_fs->fs_ronly == 0) {
			/*
			 * Truncate & free if all links went away.
			 */
			if (ip->i_nlink <= 0) {
				ip->i_gen++;
				itrunc(ip, (u_long)0);
				mode = ip->i_mode;
				ip->i_mode = 0;
				ip->i_rdev = 0;
				ip->i_pflags &= ~IP_CSYMLN;
				ip->i_ucbln = 0;
				ip->i_attln = 0;
				IMARK(ip, IUPD|ICHG);
				ifree(ip, ip->i_number, mode);
#ifdef QUOTA
				if (ip->i_dquot != NULL) {
					(void)chkiq(VFSTOM(ITOV(ip)->v_vfsp),
						    ip, ip->i_uid, 0);
					dqrele(ip->i_dquot);
					ip->i_dquot = NULL;
				}
#endif
			}
			/*
			 * In any case, update.
			 */
			IUPDAT(ip, 0);
		}

		/*
		 * If inode has a bmap cache, toss it.  At this point, hold
		 * inode locked and not "free".  Thus, racing iget() will
		 * block on the semaphore, and BmapPurge() will ignore it.
		 *
		 * Ideally would keep cache and toss in iget(), but hard to
		 * coordinate with iget()'s optimized locking protocol.  See
		 * ufs_bmapcache.c for discussion on this.
		 */
		if (ip->i_bmcache)
			BmapDisable(ip);

		s = p_lock(&ino_list, SPLFS);
	
		/*
		 * Free the inode unless someone is waiting for it.
		 * This is sufficient(windowless) because another process 
		 * could be waiting iff it was in the psvl in iget, in
		 * which case it had the lists locked until it blocked.
		 * This is necessary to avoid marking the inode IFREE
		 * if the inode was going to be passed to a blocked waiter
		 * upon the vlock below.
		 */

		if (!VN_WAITERS(ITOV(ip))) {
			if (ifreeh) {
				*ifreet = ip;
				ip->i_freeb = ifreet;
			} else {
				ifreeh = ip;
				ip->i_freeb = &ifreeh;
			}
			ip->i_freef = NULL;
			ifreet = &ip->i_freef;
			ip->i_flag = IFREE;
		}

		/*
		 * Careful: iunlock before v_lock lists, so
		 * we dont step on i_mutex as someone takes the inode
		 * off of the free list.
		 */

		IUNLOCK(ip);
		v_lock(&ino_list, s);
	} else {	
		/*
		 * TRY_ILOCK(ip) failed  (we couldnt get the inode), 
		 * which means that a concurrent iget() did, in between 
		 * our lock of the vnode and our TRY_ILOCK.
		 *
		 * Having the vnode locked until this point ensures that
		 * we we WILL notice this and avoid freeing the inode above.
		 * If the vnode wasnt locked on entry to iinactive(), then
		 * a concurrent process could have gotten the inode off the
		 * free list, locked it, reffed it, and unlocked it 
		 * (still holding a reference to it) as we proceed to 
		 * move it to the free list. That would cause damage.
		 *
		 * OLD NOTE: (may no longer hold)
		 * Think a concurrent update() could race here too. also ok.
		 */
		VN_UNLOCK(vp);
	}
}

#ifdef	NFS
/*
 * Drop inode without going through the normal chain of unlocking
 * and releasing. This is closely modelled after iinactive(). See
 * comments there for locking strategy.
 *
 * Called with locked ip. Returns with ip unlocked.
 *
 * This is only used by NFS code (fhtovp()).
 */
idrop(ip)
	register struct inode *ip;
{
	register struct vnode *vp = ITOV(ip);
	register spl_t s;

	ASSERT_DEBUG(RWSEMA_WRBUSY(&ip->i_mutex), "idrop: sema avail");

	VN_LOCK(vp);
	if (--vp->v_count == 0) {
		VN_UNLOCK(vp);
		s = p_lock(&ino_list, SPLFS);

		if (!VN_WAITERS(vp)) {
			/*
			 * Put the inode back on the end of the free list.
			 */
			if (ifreeh) {
				*ifreet = ip;
				ip->i_freeb = ifreet;
			} else {
				ifreeh = ip;
				ip->i_freeb = &ifreeh;
			}
			ip->i_freef = NULL;
			ifreet = &ip->i_freef;
			ip->i_flag = IFREE;
		}
		IUNLOCK(ip);
		v_lock(&ino_list, s);
	} else {
		VN_UNLOCK(vp);
		IUNLOCK(ip);
	}
}
#endif	NFS

/*
 * iupdat()
 *	Check accessed and update flags on an inode structure.
 *	If any is on, update the inode with the current time.
 *
 * If waitfor is given, then must insure
 * i/o order so wait for write to complete.
 *
 * Called with locked inode.
 *
 * Since we only look at seconds (for time resolution), no locking on
 * time values.
 *
 * Note: should be sufficient for iinactive() to "copy" VMAPSYNC -> IP_EXECMOD.
 */

iupdat(ip, waitfor)
	register struct inode *ip;
	int waitfor;
{
	register struct buf *bp;
	struct dinode *dp;
	register struct fs *fp;
	register daddr_t *ibp;
	register int i;

	ASSERT_DEBUG(RWSEMA_WRBUSY(&ip->i_mutex), "iupdat: unlocked inode");
	fp = ip->i_fs;
	if (ip->i_flag & (IUPD|IACC|ICHG)) {
		if (fp->fs_ronly)
			return;
		bp = bread(ip->i_devvp, fsbtodb(fp, itod(fp, ip->i_number)),
			(int)fp->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		ip->i_flag &= ~(IUPD|IACC|ICHG);
		if (ITOV(ip)->v_flag & VMAPSYNC)
			ip->i_pflags |= IP_EXECMOD;	/* copy to perm flags */
		dp = bp->b_un.b_dino + itoo(fp, ip->i_number);
		dp->di_ic = ip->i_ic;
		if ((ip->i_mode&IFMT) == IFIFO) {
			ibp = dp->di_ib;
			for (i = 0; i < NIADDR; i++)
				*ibp++ = 0;
		}
		if (waitfor)
			bwrite(bp);
		else
			bdwrite(bp);
	}
}

/*
 * itrunc()
 *	Truncate the inode ip to at most length size.
 *
 * Free affected disk blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * Called with locked inode.
 *
 * NB: triple indirect blocks are untested.
 *     Very dependent on NIADDR == 3.
 */

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */

itrunc(oip, length)
	struct inode *oip;
	u_long length;
{
	register struct fs *fs;
	register struct inode *ip;
	register daddr_t lastblock;
	register i;
	struct	inode tip;
	daddr_t	lastiblock[NIADDR];
	daddr_t	bn;
	long	blocksreleased = 0;
	long	nblocks;
	int	level;
	long	indirtrunc();
	int	offset;
	daddr_t	lbn;


	ASSERT_DEBUG(RWSEMA_WRBUSY(&oip->i_mutex), "itrunc: unlocked inode");
	/*
	 * If file size not changing, mark it as changed; since
	 * no space change, no need to write inode synchronously.
	 */
	if (oip->i_size == length) {
		IMARK(oip, ICHG|IUPD);
		iupdat(oip, 0);
		return;
	}
	/*
	 * If inode has bmap cache, keep it coherent with loss of space.
	 */
	if (oip->i_bmcache)
		BmapTrunc(oip, length);

	fs = oip->i_fs;
	if (length > oip->i_size) {
		/*
		 * Trunc up case.  bmap will insure that the right blocks
		 * are allocated.  This includes extending the old frag to
		 * a full block (if needed) in addition to doing any work
		 * needed for allocating the last block.
		 */

		offset = blkoff(fs, length);
		lbn = lblkno(fs, length - 1);

		if (offset == 0) {
			bn = bmap(oip, lbn, B_WRITE, (int)fs->fs_bsize);
		} else {
			bn = bmap(oip, lbn, B_WRITE, offset);
		}
		/*
		 * If there are no errors or bmap allocated a block
		 * but there was an I/O error, lets update the i-node.
		 */
		if (u.u_error == 0 || bn >= (daddr_t)0) {
			oip->i_size = length;
			IMARK(oip, ICHG|IUPD);
			ITOV(oip)->v_flag |= VMAPSYNC;	/* "wrote" on file */
			iupdat(oip, 1);
		}
		return;
	}
	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Update size of file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *oip;
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;
	oip->i_size = length;
	IMARK(oip, ICHG|IUPD);
	iupdat(oip, 1);
	ip = &tip;

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			blocksreleased +=
			    indirtrunc(ip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				free(ip, bn, (off_t)fs->fs_bsize);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		register int size;

		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		size = (off_t)blksize(fs, ip, i);
		free(ip, bn, size);
		blocksreleased += btodb(size);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
	if (bn != 0) {
		int oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip, lastblock);
		ASSERT(newspace != 0, "itrunc: newspace");
		/*
		 *+ After truncating a file and freeing the extra blocks,
		 *+ the file size in the inode did not correspond to the
		 *+ number of blocks in the inode.
		 */
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			free(ip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
	oip->i_blocks -= blocksreleased;
	if (oip->i_blocks < 0)			/* sanity */
		oip->i_blocks = 0;
#ifdef	DEBUG
	for (level = SINGLE; level <= TRIPLE; level++)
		ASSERT(ip->i_ib[level] == oip->i_ib[level], "itrunc1");
	for (i = 0; i < NDADDR; i++)
		ASSERT(ip->i_db[i] == oip->i_db[i], "itrunc2");
	if (length == 0 && oip->i_blocks) {
		printf("itrunc: %s:%d, ip=%x, has %d blocks.\n",
				oip->i_fs->fs_fsmnt, oip->i_number,
				oip, oip->i_blocks);
	}
#endif	DEBUG
	IMARK(oip, ICHG);
#ifdef QUOTA
	if (oip->i_dquot)
		(void) chkdq(oip, -blocksreleased, 0);
#endif
	/*
	 * If we truncated to the middle of a frag/block, zero the
	 * remainder.  This allows a truncated file to behave
	 * as if the space was never allocated in the first place.
	 */
	izaptail(oip);
}

/*
 * indirtrunc()
 *	Release blocks associated with the inode ip and
 *	stored in the indirect block bn.
 *
 * Blocks are free'd in LIFO order up to (but not including) lastbn.
 * If level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */

long
indirtrunc(ip, bn, lastbn, level)
	register struct inode *ip;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = ip->i_fs;
	daddr_t nb, last;
	long factor;
	int blocksreleased = 0, nblocks;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those 
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = geteblk((int)fs->fs_bsize);
	bp = bread(ip->i_devvp, fsbtodb(fs, bn), (int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	  (u_int)(NINDIR(fs) - (last + 1)) * sizeof(daddr_t));
	bwrite(bp);
	bp = copy, bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    indirtrunc(ip, nb, (daddr_t)-1, level - 1);
		free(ip, nb, (int)fs->fs_bsize);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += indirtrunc(ip, nb, last, level - 1);
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * izaptail()
 *	Insure tail of inode data is zeroed.
 *
 * Called with locked inode from itrunc() to insure truncate to middle
 * of block/frag zeroes remainder of allocated block/frag.
 */

izaptail(ip)
	register struct inode *ip;
{
	register struct fs *fs;
	register struct buf *bp;
	register int	boff;
	register int	size;
	daddr_t		lbn;
	daddr_t		bn;

	fs = ip->i_fs;
	size = 0;
	boff = blkoff(fs, ip->i_size);
	lbn = lblkno(fs, ip->i_size);

	/*
	 * If into indirect blocks and not on block boundary, or
	 * in direct blocks and not on a frag boundary, then need
	 * to zap some.  Set `size' to size of block/frag to read.
	 */

	if (lbn >= NDADDR) {
		if (boff)
			size = fs->fs_bsize;
	} else if (fragoff(fs, boff))
		size = fragroundup(fs, boff);

	/*
	 * Non-zero `size' ==> need to zap.  Do read-flavor bmap(),
	 * in case sparse file (bmap() returns ((daddr_t)-1)).
	 */

	if (size) {
		bn = bmap(ip, lbn, B_READ);
		if ((long)bn <= 0 || u.u_error)
			return;
		bp = bread(ip->i_devvp, fsbtodb(fs, bn), size);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		bzero(bp->b_un.b_addr+boff, (u_int)(size-boff));
		bdwrite(bp);
		ITOV(ip)->v_flag |= VMAPSYNC;		/* "wrote" on file */
	}
}

/*
 * iflush()
 *	Remove any inodes in the inode cache belonging to dev.
 *
 * There should not be any active ones, return error if any are found
 * (nb: this is a user error, not a system err)
 *
 * This is called from unmount1()/ufs_vfsops.c when dev is being unmounted.
 *
 * Called with ino_list locked, for consistency of the data.  This is
 * expensive in terms of latency to those lists.  Unmounts happen rarely,
 * so this is acceptable.
 */

iflush(dev)
	dev_t dev;
{
	register struct inode *ip;
#ifdef QUOTA
	register struct inode *iq;
	struct mount *mp;

	mp = getmp(dev);
	/*
	 * iq contains a pointer to the inode for the quota file for
	 * this dev, if quotas are enabled.  If quotas are not enabled
	 * it contains NULL.  The inode for the quota file can not
	 * be flushed while quotas are still on.
	 */
	iq = mp->m_qinod;
#endif

	for (ip = inode; ip < inodeNINODE; ip++) {
#ifdef QUOTA
		if (ip != iq && ip->i_dev == dev)
#else
		if (ip->i_dev == dev)
#endif
		{
			if ((ip->i_flag & IFREE) == 0)
				return (0);		/* bad news */
			else {
				remque(ip);
				ip->i_forw = ip;
				ip->i_back = ip;
				/*
				 * As IFREE == 1, the inode was on the free
				 * list already, just leave it there; it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
			}
		}
	}

	/*
	 * If got here, didn't find any still active.
	 */

	return (1);
}

/*
 * iaccess()
 *	Check mode permission on inode.
 *
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the read-only status of the file system is checked.
 * Also in WRITE, prototype text segments cannot be written.
 * The mode is shifted to select the owner/group/other fields.
 * The super user is granted all permissions.
 *
 * Some calls pass unlocked IFDIR inode; this is ok, since there can never
 * be VTEXT in a directory.  All other calls pass locked inode.
 */

iaccess(ip, m)
	register struct inode *ip;
	register int m;
{
	register int *gp;

	if (m & IWRITE) {
		register struct vnode *vp;

		vp = ITOV(ip);
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK) {
				u.u_error = EROFS;
				return (EROFS);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		if (vp->v_flag & VTEXT) {
			if (!xrele(vp)) {
				u.u_error = ETXTBSY;
				return (ETXTBSY);
			}
		}
	}
	/*
	 * If you're the super-user, you always get access.
	 */
	if (u.u_uid == 0)
		return (0);
	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then check public access.
	 */
	if (u.u_uid != ip->i_uid) {
		m >>= 3;
		if (u.u_gid == ip->i_gid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
	}
found:
	if ((ip->i_mode & m) == m)
		return (0);
	u.u_error = EACCES;
	return (EACCES);
}
