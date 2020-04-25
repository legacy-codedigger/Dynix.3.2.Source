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
static	char	rcsid[] = "$Header: vfs.c 2.13 1991/06/14 00:21:48 $";
#endif

/*
 * vfs.c
 *	Virtual File-System system calls.
 */

/* $Log: vfs.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/vfs.h"
#undef	NFS
#include "../h/mount.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../h/systm.h"		/* for unmount_mutex */
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"

/*
 * vfs global data.
 */
struct	vnode	*rootdir;		/* pointer to root vnode */

struct	vfs	*rootvfs;		/* pointer to root vfs. This is */
					/* also the head of the vfs list */

sema_t	sync_sema;			/* Sync if not already sync'ing */

/*
 * System calls
 */

/*
 * mount system call.
 */
mount()
{
	register struct a {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap = (struct a *)u.u_ap;

	cmount(uap->type, uap->dir, uap->flags, uap->data);
}

/*
 * Common mount.
 */
cmount(type, dir, flags, data)
	int	type;
	char	*dir;
	int	flags;
	caddr_t	data;
{
	struct pathname	pn;
	struct vnode	*vp;
	struct vfs	lvfs;

	/*
	 * Must be super user
	 */
	if (!suser())
		return;

	/*
	 * Get the vnode to be covered.  After looking it up, purge
	 * directory-name cache of any entries referring to the
	 * desired vnode (e.g., need to remove any unnecessary ref's
	 * so that v_count can be == 1).
	 */
	u.u_error = lookupname(dir, UIOSEG_USER, FOLLOW_LINK,
					(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	dnlc_purge_vp(vp);
	if (vp->v_count != 1) {
		VN_PUT(vp);
		u.u_error = EBUSY;
		return;
	}
	if (vp->v_type != VDIR) {
		VN_PUT(vp);
		u.u_error = ENOTDIR;
		return;
	}
	u.u_error = pn_get(dir, UIOSEG_USER, &pn);
	if (u.u_error) {
		VN_PUT(vp);
		return;
	}
	if ((type < 0) || (type >= mount_ntypes) ||
	    vfssw[type] == (struct vfsops *)0) {
		u.u_error = ENODEV;
		pn_free(&pn);		/* release pathname */
		VN_PUT(vp);
		return;
	}

	/*
	 * Initialize local vfs structure. The VFS_MOUNT() routine
	 * will enter the vfs into the vfs list.
	 *
	 * This differs from the SUN implementation.
	 */
	VFS_INIT(&lvfs, vfssw[type], (caddr_t)0);
	u.u_error = vfs_filldata(vp, &lvfs, flags);
	if (!u.u_error) {
		u.u_error = VFS_MOUNT(&lvfs, pn.pn_path, data);
	}
	pn_free(&pn);
	if (u.u_error) {
		VN_PUT(vp);
	} else {
		VN_UNLOCKNODE(vp);
	}
}

/*
 * Sync system call.
 *	Sync each vfs for fs specific items, then flush the buffer cache.
 */
sync()
{
	register struct vfsops **vfstyp;

	/*
	 * If this fails, do nothing (sync() is heuristic update).
	 */
	if (!cp_sema(&sync_sema))
		return;

	/*
	 * Call the VFS_SYNC routine for each filesystem TYPE rather than for
	 * each filesystem mounted.
	 */
	for (vfstyp = vfssw; vfstyp < &vfssw[mount_ntypes]; ++vfstyp) {
		if (*vfstyp != (struct vfsops *)0) {
			(*(*vfstyp)->vfs_sync)();
		}
	}

	/*
	 * For all devices and filesystems, force stale buffer cache
	 * information to be flushed.
	 */
	bflush(NULLVP);

	v_sema(&sync_sema);
}

/*
 * get filesystem statistics
 */
statfs()
{
	struct a {
		char *path;
		struct statfs *buf;
	} *uap = (struct a *)u.u_ap;

	struct vnode *vp;

	u.u_error =
	    lookupname(uap->path, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	cstatfs(vp->v_vfsp, uap->buf);
	VN_PUT(vp);
}

fstatfs()
{
	struct a {
		int fd;
		struct statfs *buf;
	} *uap = (struct a *)u.u_ap;

	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0)
		cstatfs(((struct vnode *)fp->f_data)->v_vfsp, uap->buf);
}

cstatfs(vfsp, ubuf)
	struct vfs *vfsp;
	struct statfs *ubuf;
{
	struct statfs sb;

	u.u_error = VFS_STATFS(vfsp, &sb);
	if (u.u_error)
		return;
	u.u_error = copyout((caddr_t)&sb, (caddr_t)ubuf, sizeof(sb));
}

/*
 * Unmount system call.
 *
 * Note: unmount takes a path to the vnode mounted on as argument,
 * not special file (as before).
 */
unmount()
{
	struct a {
		char	*pathp;
	} *uap = (struct a *)u.u_ap;

	if (!suser())
		return;

	/*
	 * Exclude any concurrent activity on vfs chain.
	 * This locks out system 5 umount().
	 */
	p_sema(&unmount_mutex, PVFS);
	cunmount(uap->pathp, UIOSEG_USER);
	v_sema(&unmount_mutex);		/* u.u_error set by cunmount */
}

/*
 * Common unmount. Assumes caller holds/releases unmount_mutex. The
 * mutex granularity has been increased to cover this entire
 * function. This is ok since there is rarely any concurrent
 * unmount activity.
 */
cunmount(pathp, segflg)
	char *pathp;
{
	extern void dnlc_purge_vfsp();
	struct vnode *fsrootvp;
	register struct vnode *coveredvp;
	register struct vfs *vfsp;

	/*
	 * lookup root of fs
	 */
	u.u_error = lookupname(pathp, segflg, FOLLOW_LINK,
			(struct vnode **)0, &fsrootvp);
	if (u.u_error)
		return;

	/*
	 * make sure this is a root
	 */
	if ((fsrootvp->v_flag & VROOT) == 0) {
		u.u_error = EINVAL;
		VN_PUT(fsrootvp);
		return;
	}

	/*
	 * get vfs and covered vnode
	 */
	vfsp = fsrootvp->v_vfsp;
	VN_PUT(fsrootvp);
	coveredvp = vfsp->vfs_vnodecovered;

	xumount(vfsp);	        /* remove unused sticky files from text table */
	dnlc_purge_vfsp(vfsp);	/* purge dnlc entries for this file sys */
	VFS_SYNC(vfsp);

	u.u_error = VFS_UNMOUNT(vfsp);
	if (!u.u_error)
		VN_RELE(coveredvp);
}

/*
 * External routines
 */

/*
 * vfs_mountroot is called by main (init_main.c) to
 * mount the root filesystem.
 */

void
vfs_mountroot()
{
	register int error;
	extern int (*rootfsmount)();	/* pointer to root mounting routine */
					/* set by (auto)configuration */

	/*
	 * Rootfsmount is a pointer to the routine which will mount a specific
	 * filesystem as the root. It is setup by autoconfiguration.
	 * If error panic.
	 */
	error = (*rootfsmount)();
	if (error) {
		printf("vfs_mountroot: mount error = %d\n", error);
		panic("vfs_mountroot cannot mount root");
		/*
		 *+ When booting the system,
		 *+ the kernel couldn't mount the root filesystem.
		 */
	}
	/*
	 * Get vnode for '/'.
	 * Setup rootdir, u.u_rdir and u.u_cdir to point to it.
	 * These are used by lookuppn so that it knows where
	 * to start from '/' or '.'.
	 */
	error = VFS_ROOT(rootvfs, &rootdir);
	VN_UNLOCKNODE(rootdir);
	ASSERT(!error, "rootmount: cannot find root vnode");
	/*
	 *+ When booting the system,
	 *+ the kernel couldn't find the root vnode of the root
	 *+ filesystem.
	 */
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	u.u_rdir = NULL;
}

/*
 * vfs_filldata
 * Fill out vfs_flag and vfs_vnodecovered fields.
 * coveredvp is NULLVP if this is the root.
 */
int
vfs_filldata(coveredvp, vfsp, mflag)
	struct vnode *coveredvp;
	register struct vfs *vfsp;
	int mflag;
{
	if (coveredvp != NULLVP) {
		/*
		 * Return EBUSY if the covered vp is already mounted on.
		 */
		if (coveredvp->v_vfsmountedhere != (struct vfs *)0)
			return (EBUSY);
	}
	vfsp->vfs_vnodecovered = coveredvp;
	if (mflag & M_RDONLY) {
		vfsp->vfs_flag |= VFS_RDONLY;
	} else {
		vfsp->vfs_flag &= ~VFS_RDONLY;
	}
	if (mflag & M_NOSUID) {
		vfsp->vfs_flag |= VFS_NOSUID;
	} else {
		vfsp->vfs_flag &= ~VFS_NOSUID;
	}
	return (0);
}

/*
 * vfs_add is called by a specific filesystem's mount routine to add
 * the new vfs into the vfs list and to cover the mounted on vnode.
 * The vfs is also locked so that lookuppn will not venture into the
 * covered vnodes subtree.
 *
 * Caller holds vfs_mutex sema to insure one at a time in here.
 */
void
vfs_add(vfsp, root)
	register struct vfs *vfsp;
	bool_t root;
{

	if (root) {
		/*
		 * This is the root of the whole world.
		 */
		rootvfs = vfsp;
		vfsp->vfs_next = (struct vfs *)0;
	} else {
		/*
		 * Put the new vfs on the vfs list after root.
		 * Point the covered vnode at the new vfs so lookuppn
		 * (vfs_lookup.c) can work its way into the new file system.
		 */
		vfsp->vfs_next = rootvfs->vfs_next;
		rootvfs->vfs_next = vfsp;
		vfsp->vfs_vnodecovered->v_vfsmountedhere = vfsp;
	}
}

/*
 * Remove a vfs from the vfs list, and destory pointers to it.
 * Should be called by filesystem implementation after it determines
 * that an unmount is legal but before it destroys the vfs.
 *
 * Caller holds vfs_mutex and mounted-on vnode locked.
 */
void
vfs_remove(vfsp)
register struct vfs *vfsp;
{
	register struct vfs *tvfsp;
	register struct vnode *vp;

	/*
	 * can't unmount root. Should never happen, because fs will be busy.
	 */
	ASSERT(vfsp != rootvfs, "vfs_remove: unmounting root");
	/*
	 *+ A call to unmount succeeded in unmounting the root
	 *+ filesystem.  This should normally fail because the
	 *+ root filesystem is always busy.
	 */

	tvfsp = rootvfs;
	for (; tvfsp != (struct vfs *)0; tvfsp = tvfsp->vfs_next) {
		if (tvfsp->vfs_next == vfsp) {
			/*
			 * remove vfs from list, unmount covered vp.
			 */
			tvfsp->vfs_next = vfsp->vfs_next;
			vp = vfsp->vfs_vnodecovered;
			vp->v_vfsmountedhere = (struct vfs *)0;
			return;
		}
	}
	/*
	 * can't find vfs to remove
	 */
	panic("vfs_remove: vfs not found");
	/*
	 *+ While the kernel was unmounting a filesystem, 
	 *+ it could not find the structure describing
	 *+ that mounted filesystem in its
	 *+ mount table.  This indicates a kernel software bug.
	 */
}
