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

#ifndef	lint
static	char	rcsid[] = "$Header: ufs_vfsops.c 2.23 91/01/15 $";
#endif

/*
 * ufs_vfsops.c
 *	UNIX file-system virtual-file system operations (mount, unmount,
 *	sync, etc).
 */

/* $Log:	ufs_vfsops.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/pathname.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../ufs/fs.h"
#include "../ufs/mount.h"
#include "../ufs/inode.h"
#undef	NFS
#include "../h/mount.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"		/* for init_mounttab */
#include "../machine/plocal.h"

/*
 * For unmount-root interface from reboot()
 */
static int ufs_unmountroot();
int (*rootfsunmount)() = ufs_unmountroot;

/*
 * ufs vfs operations.
 */
extern	int	ufs_mount();
extern	int	ufs_unmount();
extern	int	ufs_root();
extern	int	ufs_statfs();
extern	int	ufs_sync();

struct	vfsops	ufs_vfsops = {
	ufs_mount,
	ufs_unmount,
	ufs_root,
	ufs_statfs,
	ufs_sync,
};

/*
 * this is the default filesystem type.
 * this should be setup by the configurator
 */

extern	int	ufs_mountroot();
int		(*rootfsmount)() = ufs_mountroot;

/*
 * Default device to mount on.
 */
extern	dev_t	rootdev;

/*
 * ufs mount vfs operation
 */
static
ufs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	dev_t dev;
	struct vnode *vp;
	struct ufs_args args;

	/*
	 * Get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof(struct ufs_args));
	if (error) {
		return (error);
	}

	/*
	 * Get the device to be mounted on. First unlock coveredvp, in case
	 * device to be mounted on has the coveredvp in its pathname. This is
	 * to avoid deadlock in lookupname().
	 */
	vfsp->vfs_vnodecovered->v_flag |= VMOUNTING;
	VN_UNLOCKNODE(vfsp->vfs_vnodecovered);
	error = lookupname(args.fspec, UIOSEG_USER, FOLLOW_LINK,
					(struct vnode **)0, &vp);
	if (error) {
		VN_LOCKNODE(vfsp->vfs_vnodecovered);
		vfsp->vfs_vnodecovered->v_flag &= ~VMOUNTING;
		return (error);
	}

	if (vp->v_type != VBLK) {
		VN_PUT(vp);
		VN_LOCKNODE(vfsp->vfs_vnodecovered);
		vfsp->vfs_vnodecovered->v_flag &= ~VMOUNTING;
		return (ENOTBLK);
	}
	dev = vp->v_rdev;
	VN_PUT(vp);

	VN_LOCKNODE(vfsp->vfs_vnodecovered);
	/*
	 * Window shut again. Remove VMOUNTING bit.
	 */
	vfsp->vfs_vnodecovered->v_flag &= ~VMOUNTING;

	if (major(dev) >= nblkdev) {
		return (ENXIO);
	}

	/*
	 * Mount the filesystem
	 */
	return (mountfs(dev, path, vfsp, (struct vfs **)0));
}

/*
 * Called by vfs_mountroot when ufs is going to be mounted as root.
 */
static
ufs_mountroot()
{
	register struct fs *fsp;
	register int error;
	struct vfs *nvfsp;		/* new vfs entry on vfs list */
	struct vfs lvfs;
	extern daddr_t end_of_root;

	init_mounttab();		/* see comment for init_mounttab() */

	VFS_INIT(&lvfs, &ufs_vfsops, (caddr_t)0);
	(void) vfs_filldata(NULLVP, &lvfs, 0);
	error = mountfs(rootdev, "/", &lvfs, &nvfsp);
	if (error) {
		return (error);
	}
	fsp = ((struct mount *)(nvfsp->vfs_data))->m_bufp->b_un.b_fs;
	inittodr(fsp->fs_time);
	end_of_root = fsbtodb(fsp, fsp->fs_size);
	return (0);
}

/*
 * Clean up things on the root file system so that we don't have to
 * fsck it after a clean shutdown.
 */
static
ufs_unmountroot()
{
	register struct mount *mp;
	register struct fs *fsp;

	mp = (struct mount *)rootvfs->vfs_data;
	fsp = mp->m_bufp->b_un.b_fs;
	if (!PTXFS(fsp))
		return;
	fsp->fs_time = time.tv_sec;
	fsp->fs_state = FS_OKAY - (long)fsp->fs_time;
	sbupdate(mp->m_dev, fsp);
}

/*
 * init_mounttab()
 *	Initialize the m_lock field in the mount table
 *
 * This is called only once. We kludge for now and call it from
 * ufs_mountroot() because we know it is called once and before any other
 * mounts are possible. This will go away when fhtovp() is reworked to
 * be independent of the local file system type.
 */

static
init_mounttab()
{
	register struct mount *mp;

	for (mp = &mounttab[0]; mp < mountNMOUNT; mp++) {
		init_lock(&mp->m_lock, G_FS);
	}
}

/*
 * mountfs()
 *	Do the work of a mount.
 *
 * The use of wmemall()/wmemfree() has potential problems with TLB flushes;
 * if the memory space was recently used by some other processor, it could
 * have stale TLB data and *then* try to do a file-sys thing.  This can
 * be avoided if each processor context switches *after* the mount and
 * *before* accessing this file-sys.  This is typical, and cases where this
 * doesn't occur are hard to force.  The law of large numbers says we
 * ignore the problem.
 */
static int
mountfs(dev, path, vfsp, vfspp)
	dev_t dev;
	char *path;
	struct vfs *vfsp;		/* vfs to enter into vfs list */
	struct vfs **vfspp;		/* new entry in vfs list */
{
	register struct fs	*fsp;
	register struct mount	*mp;
	register struct buf	*bp = 0;
	struct	buf	*tp = 0;
	struct	vnode	*dev_vp;
	int	error;
	int	blks;
	caddr_t	space;
	caddr_t	sspace = 0;
	int	i;
	int	size;
	spl_t	s_ipl;
	char	*strncpy();

	/*
	 * Open block device mounted on.
	 * When bio is fixed for vnodes this can all be vnode operations.
	 */
	error = (*bdevsw[major(dev)].d_open)(dev,
			(vfsp->vfs_flag & VFS_RDONLY) ? 
			FREAD|FMOUNT : FREAD|FWRITE|FMOUNT);
	if (error)
		return (error);

	/*
	 * Read in superblock.
	 */
	DEVTOVP(dev, dev_vp);
	tp = bread(dev_vp, SBLOCK, SBSIZE);
	if (tp->b_flags & B_ERROR) {
		error = EIO;
		goto out;
	}
	/*
	 * Verify the super block is reasonable.
	 */
	fsp = tp->b_un.b_fs;
	if (fsp->fs_magic != FS_MAGIC || fsp->fs_bsize > MAXBSIZE) {
		error = EINVAL;
		goto out;
	}
	if (vfsp->vfs_flag & VFS_RDONLY) {
		fsp->fs_fmod = 0;
		fsp->fs_ronly = 1;
	} else {
		fsp->fs_fmod = 1;
		fsp->fs_ronly = 0;

		/*
		 * PTX file system uses the System V rules for
		 * cleanliness and so forth.
		 */
		if (PTXFS(fsp)) {
			/*
			 * If it's marked closed clean, just mount.  If
			 * it's the root, mount it anyway.  Otherwise
			 * fail the mount and make'em clean the disk.
			 */
			if ((fsp->fs_state + fsp->fs_time) == FS_OKAY)
				fsp->fs_state = FS_ACTIVE;
			else if (dev_vp->v_rdev == rootdev)
				fsp->fs_state = FS_BAD;
			else {
				error = ENOSPC;
				/*
				 * Rather than "goto out" here, fall through
				 * to eventually check whether the reason this
				 * filesystem is not FS_OKAY is because it is
				 * already mounted.  If so, we want to return
				 * EBUSY instead.  
				 *
				 * After determining that the filesystem is 
				 * not already mounted, check "error" and 
				 * fail if set.
				 */
			}
		}
	}
	/*
	 * Copy the super block into a buffer in its native size.
	 */
	bp = geteblk((int)fsp->fs_sbsize);
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
		(u_int)fsp->fs_sbsize);
	if (fsp->fs_ronly)
		brelse(tp);
	else
		bwrite(tp);
	tp = 0;
	fsp = bp->b_un.b_fs;
	vfsp->vfs_bsize = fsp->fs_bsize;

	/*
	 * If we're going to run with an old Dynix 3 disk, stash away
	 * the fs_id block in its new location, then convert the superblock
	 * to the new PTX format.
	 */
	if (!PTXFS(fsp)) {
		struct fs_dsp *ofs = &fsp->fs_un.fs_dsp;

		/*
		 * Move fs_id[] stuff from its current place to the PTX-
		 * defined place.  We do this because the "spares" we were
		 * keeping fs_id[] have been used by PTX for the Tahoe
		 * parameters.
		 */
		fsp->fs_id[0] = ofs->dfs_id[0];
		fsp->fs_id[1] = ofs->dfs_id[1];

		/*
		 * construct new tahoe-style "spares".
		 */
		fsp->fs_nrpos = 8;
		fsp->fs_optim = FS_OPTSPACE;
		fsp->fs_npsect = fsp->fs_nsect;
		fsp->fs_interleave = 1;
		fsp->fs_trackskew = 0;
		fsp->fs_headswitch = 0;
		fsp->fs_trkseek = 0;
	}
	/*
	 * Sanity checks 
	 */
	fsp->fs_npsect = MAX(fsp->fs_npsect, fsp->fs_nsect);	/* XXX */
	fsp->fs_interleave = MAX(fsp->fs_interleave, 1);	/* XXX */
	/*
	 * Read in cyl group info.
	 */
	blks = howmany(fsp->fs_cssize, fsp->fs_fsize);
	sspace = space = wmemall((int)fsp->fs_cssize, 1);
	for (i = 0; i < blks; i += fsp->fs_frag) {
		size = fsp->fs_bsize;
		if (i + fsp->fs_frag > blks)
			size = (blks - i) * fsp->fs_fsize;
		tp = bread(dev_vp, fsbtodb(fsp, fsp->fs_csaddr + i), size);
		if (tp->b_flags & B_ERROR) {
			error = EIO;
			goto out;
		}
		bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
		fsp->fs_csp[i / fsp->fs_frag] = (struct csum *)space;
		space += size;
		brelse(tp);
		tp = 0;
	}
	(void) strncpy(fsp->fs_fsmnt, path, sizeof(fsp->fs_fsmnt));

	/*
	 * NOW find mounttab slot and insert into vfs list.
	 */
	p_sema(&vfs_mutex, PVFS);	/* Exclude vfs mount/unmount activity */

	/*
	 * Check for dev already mounted on.
	 */
	for (mp = &mounttab[0]; mp < mountNMOUNT; mp++) {
		if (mp->m_bufp != 0 && dev == mp->m_dev) {
			v_sema(&vfs_mutex);
			error = EBUSY;
			goto out;
		}
	}
	/*
	 * "error" may have been set to ENOSPC above because the filesystem
	 * was not FS_OKAY.  Now we've determined that the reason this
	 * filesystem was not FS_OKAY was not because it was already mounted,
	 * so we return the ENOSPC error now.
	 */
	if (error) {
		v_sema(&vfs_mutex);
		goto out;
	}
	/*
	 * Find empty mount table entry.
	 */
	for (mp = &mounttab[0]; mp < mountNMOUNT; mp++) {
		if (mp->m_bufp == 0) {
			/*
			 * Add vfs to vfs list.
			 * Copy local to mount table slot.
			 * Lock entry to avoid races with nfsgetmp().
			 */
			s_ipl = p_lock(&mp->m_lock, SPLFS);
			mp->m_dev = dev;
			mp->m_bufp = bp;
			vfsp->vfs_data = (caddr_t)mp;
			mp->m_vfs = *vfsp;
			vfs_add(&mp->m_vfs, dev == rootdev);
			if (vfspp != (struct vfs **)0)
				*vfspp = &mp->m_vfs;	/* see ufs_mountroot */
			v_lock(&mp->m_lock, s_ipl);
			v_sema(&vfs_mutex);
			return (0);
		}
	}
	v_sema(&vfs_mutex);
	error = EBUSY;
out:
	if (sspace)
		wmemfree(sspace, (int)fsp->fs_cssize);
	if (bp)
		brelse(bp);
	if (tp)
		brelse(tp);
	/*
	 * Need to close the device, since we had a successful open above.
	 * bflush() first (paranoia).
	 */
	bflush(dev_vp);
	binval(dev_vp);
	(*bdevsw[major(dev)].d_close)(dev, 
			(vfsp->vfs_flag & VFS_RDONLY) ? FMOUNT|FREAD : FMOUNT|FREAD|FWRITE);
	return (error);
}

/*
 * vfs operations
 */

/*
 * ufs_unmount()
 *	Unmount a UNIX file-system.
 *
 * Called from unmount() while holding unmount_mutex.
.
 */
static
ufs_unmount(vfsp)
	struct vfs *vfsp;
{
	return (unmount1(vfsp, 0));
}

/*
 * unmount1()
 *	Unmount a UNIX file-system.
 *
 * Currently, no calls pass forcibly == 1; this would be *very* hard
 * to do:  probably run the file-table and mark entries as "bogus",
 * somehow force users of these to wakeup and close them.  Alternatives:
 * kill all processes with open files on the device (mean!), or yank
 * those inodes we can and bogusize the rest of the file-desc's (probably
 * racey)...
 */

/*ARGSUSED*/
static
unmount1(vfsp, forcibly)
	register struct	vfs	*vfsp;
	int	forcibly;
{
	register struct	vnode	*vp;
	register struct	mount	*mp;
	register struct	fs	*fs;
	register struct	vnode	*dev_vp;
	struct	buf		*bufp;		/* pointer to superblock */
	dev_t	dev;
	spl_t	s;
	int	flag;

	mp = (struct mount *)vfsp->vfs_data;
	dev = mp->m_dev;

	vp = vfsp->vfs_vnodecovered;
	/*
	 * If NULLVP then root, disallow unmount.
	 */
	if ((vp == NULLVP) || !VN_TRYLOCKNODE(vp))
		return (EBUSY);

	/*
	 * Lock inode lists so iflush() can tell us if anything is still in use.
	 * If nothing in use and covered vnode isn't held, do the unmount.
	 * We call vfs_remove with the covered vnode still locked,
	 * to ensure that no one enters the fs being unmounted until 
	 * vp->vfsmountedhere is cleared, after which the filesystem
	 * is inaccesible. Holding the covered vnode locked and holding
	 * vfs_mutex is sufficient to ensure that no other processes could
	 * venture into the fs being unmounted.
	 *
	 * This relies on dirlook() holding a ref to an inode until
	 * it holds a ref on inode being looked up, and inode allocation
	 * holding a ref to the parent directory until the inode is allocated
	 * and held.  These rules ==> if iflush() passes and the covered
	 * vnode is only held once (by the mount), then the file-sys
	 * may be safely unmounted (there are no in-use entries and none
	 * being concurrently created).
	 */

	p_sema(&vfs_mutex, PVFS);
	s = p_lock(&mp->m_lock, SPLFS);
	(void) p_lock(&ino_list, SPLFS);
	if (!iflush(dev))
	{
		v_lock(&ino_list, SPLFS);
		v_lock(&mp->m_lock, s);
		v_sema(&vfs_mutex);
		VN_UNLOCKNODE(vp);
		return (EBUSY);
	}
	v_lock(&ino_list, SPLFS);

	/*
	 * locked vnode count not needed here: if count==1, it could only grow
	 * via an iget(), and that can't happen since we have node locked.
	 * If v_count > 1, lookuppn() could be redirecting to the mounted root
	 * as we speak.
	 */
	if (vp->v_count > 1) {
		v_lock(&mp->m_lock, s);
		v_sema(&vfs_mutex);
		VN_UNLOCKNODE(vp);
		return (EBUSY);
	}

	/*
	 * Check for race with fhtovp(). If a nfsd is about to do an
	 * iget() (or is doing iget()), then fail because of being BUSY.
	 */
	if (mp->m_count != 0) {
		v_lock(&mp->m_lock, s);
		v_sema(&vfs_mutex);
		VN_UNLOCKNODE(vp);
		return (EBUSY);
	}

	/*
	 * vfs_remove clears vp->vfsmountedhere to prevent lookuppn()
	 * from indirecting to a mounted vfs when it reaches this vnode.
	 */

	vfs_remove(vfsp);
	VN_UNLOCKNODE(vp);

#ifdef QUOTA
	if (mp->m_qinod != NULL) {
		v_lock(&mp->m_lock, s);
		(void)closedq(mp);
		/*
		 * Here we have to iflush again to get rid of the quota
		 * inode.  A drag, but it would be ugly to cheat, & this
		 * doesn't happen often.
		 */
		(void) p_lock(&mp->m_lock, SPLFS);
		(void) p_lock(&ino_list, SPLFS);
		(void)iflush(dev);
		v_lock(&ino_list, SPLFS);
	}
#endif

	/*
	 * Now vp->vfsmountedhere is cleared. Save various items into locals
	 * so that we can release the mp->m_lock. Then we can put away the
	 * filesystem at our leisure.
	 */

	fs = mp->m_bufp->b_un.b_fs;
	flag = !fs->fs_ronly;
	bufp = mp->m_bufp;
	mp->m_bufp = 0;
	mp->m_dev = 0;
	v_lock(&mp->m_lock, s);

	if (flag) {				/* insure clean update */
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		if (PTXFS(fs))
			fs->fs_state = FS_OKAY - (long)fs->fs_time;
		sbupdate(dev, fs);
	}
	wmemfree((caddr_t)fs->fs_csp[0], (int)fs->fs_cssize);
	brelse(bufp);
	v_sema(&vfs_mutex);

	/*
	 * Must close driver, since we opened at mount time.
	 * Do a bflush(),binval() and let driver worry about last
	 * close semantics.
	 */
	DEVTOVP(dev, dev_vp);
	bflush(dev_vp);
	binval(dev_vp);
	(*bdevsw[major(dev)].d_close)(dev, flag|FMOUNT);

	return (0);
}

/*
 * ufs_root()
 * 	find root of ufs
 *
 * Return with locked vnode (*vpp).
 */

static
ufs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct mount *mp;
	register struct inode *ip;

	mp = (struct mount *)vfsp->vfs_data;
	ip = iget(mp->m_dev, mp->m_bufp->b_un.b_fs, (ino_t)ROOTINO);
	if (ip == (struct inode *)0) {
		return (u.u_error);
	}
	*vpp = ITOV(ip);
	return (0);
}

/*
 * Get file system statistics.
 */

static
ufs_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{
	register struct fs *fsp;

	fsp = ((struct mount *)vfsp->vfs_data)->m_bufp->b_un.b_fs;
	ASSERT(fsp->fs_magic == FS_MAGIC, "ufs_statfs");
	/*
	 *+ When doing a statfs on a mounted filesystem, the kernel
	 *+ found that the in-core superblock had a bad magic number.
	 */
	sbp->f_bsize = fsp->fs_fsize;
	sbp->f_blocks = fsp->fs_dsize;
	sbp->f_bfree =
	    fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
		fsp->fs_cstotal.cs_nffree;
	/*
	 * avail = MAX(max_avail - used, 0)
	 */
	sbp->f_bavail = (fsp->fs_dsize * (100 - fsp->fs_minfree) / 100) -
			 (fsp->fs_dsize - sbp->f_bfree);
	/*
	 * inodes
	 */
	sbp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
	sbp->f_ffree = fsp->fs_cstotal.cs_nifree;
	bcopy((caddr_t)fsp->fs_id, (caddr_t)sbp->f_fsid, sizeof(fsid_t));
	return (0);
}

/*
 * Flush any pending I/O.
 */

static
ufs_sync()
{
	update();
	return (0);
}

/*
 * sbupdate()
 *	Update superblocks on disk.
 *
 * Called from update() and unmount1(). Note change in interface from 4.2.
 */

sbupdate(dev, fs)
	dev_t dev;
	register struct fs *fs;
{
	register struct buf *bp;
	register struct vnode *dev_vp;
	int blks;
	caddr_t space;
	int i, size;

	DEVTOVP(dev, dev_vp);
	bp = getblk(dev_vp, SBLOCK, (int)fs->fs_sbsize, 0);
	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);

	/*
	 * Restore the stashed fields for Dynix 3 file systems
	 */
	if (!PTXFS(fs)) {
		struct fs *fs2 = bp->b_un.b_fs;
		struct fs_dsp *ofs = &fs2->fs_un.fs_dsp;

		/* Restore old 4.2 "spares" */
		ofs->dfs_id[0] = fs->fs_id[0];
		ofs->dfs_id[1] = fs->fs_id[1];
		bzero((char *)ofs->dfs_sparecon, sizeof(ofs->dfs_sparecon));

		/* Restore old 4.2 fs_postbl[] */
		fs2->fs_nrpos = -1;
		fs2->fs_id[0] = -1;
		fs2->fs_id[1] = -1;
		fs2->fs_state = -1;
	}
	bwrite(bp);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(dev_vp, fsbtodb(fs, fs->fs_csaddr + i), size, 0);
		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		bwrite(bp);
	}
}
