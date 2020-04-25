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
static	char	rcsid[] = "$Header: vfs_io.c 2.16 1991/06/14 00:22:50 $";
#endif

/*
 * vfs_io.c
 *	Virtual File-System system calls.
 */

/* $Log: vfs_io.c,v $
 *
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/mutex.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

static int vno_rw();
static int vno_ioctl();
static int vno_select();
static int vno_close();

struct	fileops	vnodefops = {
	vno_rw,
	vno_ioctl,
	vno_select,
	vno_close
};

static	bool_t	vno_dolock[] = {
	/* VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD */
	     0,    1,    1,    0,    0,    0,    0,     1,    0
};

/*
 * External interface to tell whether vno_rw has applied a lock
 * for a given type of vnode
 */
vno_rw_locked(type)
	enum vtype type;
{
	return(vno_dolock[(int)type]);
}

/*
 * vno_rw()
 *	If appropriate for vnode type, apply lock to vnode.
 *	Vector out to VOP_RDWR to do the I/O.
 *	Handle special cases of append-only, ATT file systems.
 */
static int
vno_rw(fp, rw, uiop)
	register struct file *fp;
	enum	uio_rw	rw;
	struct	uio	*uiop;
{
	register struct vnode *vp;
	int	count;
	int	error;

	vp = (struct vnode *)fp->f_data;
	if ((u.u_tuniverse == U_ATT) && (vp->v_type == VDIR)
	&&  (rw == UIO_READ)) {
		error = readdir_att(vp, uiop, fp);
		return (error);
	}
	/*
	 * If write, make sure filesystem is writable.
	 */
	if ((rw == UIO_WRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return (EROFS);
	/*
	 * If necessary, do IO with locked vnode.
	 */
	count = uiop->uio_resid;
	if (vno_dolock[(int)vp->v_type]) {
		VN_LOCKNODE(vp);
		error = VOP_RDWR(vp, uiop, rw,
				(((fp->f_flag & FAPPEND) ? IO_APPEND : 0) |
				 ((fp->f_flag & FNDELAY) ? IO_NDELAY : 0) |
				 ((fp->f_flag & FSYNC) ? IO_SYNC : 0)) ,
				fp->f_cred);
		VN_UNLOCKNODE(vp);
	} else
		error = VOP_RDWR(vp, uiop, rw,
				(((fp->f_flag & FAPPEND) ? IO_APPEND : 0) |
				 ((fp->f_flag & FNDELAY) ? IO_NDELAY : 0) |
				 ((fp->f_flag & FSYNC) ? IO_SYNC : 0)) ,
				fp->f_cred);
	if (error)
		return (error);
	if (fp->f_flag & FAPPEND) {
		/*
		 * The actual offset used for append is set by VOP_RDWR
		 * so compute actual starting location
		 */
		fp->f_offset = uiop->uio_offset - (count - uiop->uio_resid);
	}
	return (0);
}

static int
vno_ioctl(fp, com, data)
	struct	file	*fp;
	int	com;
	caddr_t	data;
{
	struct	vnode	*vp;
	int	error = 0;
	struct	vattr	vattr;

	vp = (struct vnode *)fp->f_data;
	switch(vp->v_type) {

	case VREG:
	case VDIR:
		switch (com) {

		case FIONREAD:
			VN_LOCKNODE(vp);
			error = VOP_GETATTR(vp, &vattr, u.u_cred);
			VN_UNLOCKNODE(vp);
			if (!error)
				*(off_t *)data = vattr.va_size - fp->f_offset;
			break;

		case FIONBIO:
		case FIOASYNC:
			break;

		default:
			error = ENOTTY;
			break;
		}
		break;

	case VCHR:
		u.u_r.r_val1 = 0;
		if ((u.u_procp->p_flag & SOUSIG) == 0 && setjmp(&u.u_qsave)) {
			u.u_eosys = RESTARTSYS;
		} else {
			error = VOP_IOCTL(vp, com, data, fp->f_flag,fp->f_cred);
		}
		break;

	case VFIFO:
		if (com == FIONREAD) {
			VN_LOCKNODE(vp);
			error = VOP_GETATTR(vp, &vattr, fp->f_cred);
			VN_UNLOCKNODE(vp);
			if (!error)
				*(off_t *)data = vattr.va_psize;
			break;
		}
		if (com == FIONBIO || com == FIOASYNC)
			break;

		/* fall into... */

	default:
		error = ENOTTY;
		break;
	}
	return (error);
}

static int
vno_select(fp, flag)
	struct file *fp;
	int flag;
{
	struct vnode *vp;

	vp = (struct vnode *)fp->f_data;
	switch(vp->v_type) {

	case VCHR:
		return (VOP_SELECT(vp, flag, fp->f_cred));

	default:
		/*
		 * Always selected
		 */
		return (1);
	}
}

/*
 * vno_stat
 * 	stat a vnode.
 *
 * Called with locked vnode (vp).
 */

int
vno_stat(vp, sb)
	register struct vnode *vp;
	register struct stat *sb;
{
	register int error;
	struct vattr vattr;

	error = VOP_GETATTR(vp, &vattr, u.u_cred);
	if (!error) {
		sb->st_mode = vattr.va_mode;
		sb->st_uid = vattr.va_uid;
		sb->st_gid = vattr.va_gid;
		sb->st_dev = vattr.va_fsid;
		sb->st_ino = vattr.va_nodeid;
		sb->st_nlink = vattr.va_nlink;
		switch (vp->v_type) {
		case VDIR:
			if (u.u_tuniverse == U_ATT)
				sb->st_size = sizedir_att(vp);
			else
				sb->st_size = vattr.va_size;
			break;
		case VFIFO:
			sb->st_size = vattr.va_psize;
			break;
		default:
			sb->st_size = vattr.va_size;
			break;
		}
		sb->st_blksize = vattr.va_blocksize;
		sb->st_atime = vattr.va_atime.tv_sec;
		sb->st_spare1 = 0;
		sb->st_mtime = vattr.va_mtime.tv_sec;
		sb->st_spare2 = 0;
		sb->st_ctime = vattr.va_ctime.tv_sec;
		sb->st_spare3 = 0;
		sb->st_rdev = (dev_t)vattr.va_rdev;
		sb->st_blocks = vattr.va_blocks;
		sb->st_spare4[0] = vattr.va_flags & VA_CSYMLN;
		sb->st_spare4[1] = 0;
	}
	return (error);
}

static int
vno_close(fp)
	register struct file *fp;
{
	register struct vnode *vp;
	int	flag;

	/*
	 * No lock needed on fp here, because this is the last
	 * reference to it.  So, no one else should be touching
	 * it at this time.
	 */

	vp = (struct vnode *)fp->f_data;
	if (fp->f_flag & (FSHLOCK | FEXLOCK))
		vno_unlock(fp, (FSHLOCK | FEXLOCK));
	flag = fp->f_flag;
	ffree(fp);

	u.u_error = vn_close(vp, flag);
	VN_RELE(vp);
	return (u.u_error);
}

/*
 * vno_lock()
 *	Place an advisory lock on a vnode.
 *
 * NOTE: only need one semaphore to wait on; if exclusive lock held, all
 * processes wait on v_exsema.  If shared lock held, all share-lockers 
 * get it and exclusive lockers wait on v_shsema.  Thus the two semaphores are
 * used mutually exclusively.  However, FIFO support overloads v_exsema and
 * v_shsema, thus can't collapse them in struct vnode.
 */

#define VFLOCK(vp)	p_lock(&(vp)->v_fllock, SPLFD)
#define VFUNLOCK(vp, s)	v_lock(&(vp)->v_fllock, (s))

vno_lock(fp, cmd)
	register struct file *fp;
	register int	cmd;
{
	register struct vnode *vp = (struct vnode *) fp->f_data;
	int	prio = PLOCK;
	spl_t	vs;
	spl_t	s;

	/*
	 * Incrementing the priority here causes processes wanting
	 * exclusive locks to be run after those wanting shared locks.
	 * This, along with the two level locking scheme, provide a
	 * degree of fairness among exclusive locks and shared locks.
	 * This fairness policy won't work quite as well in a TMP environment.
	 */

	if (cmd & LOCK_EX)
		prio += 4;

	/*
	 * If there's a exclusive lock currently applied
	 * to the file, wait for the lock with everyone else.
	 */

	vs = VFLOCK(vp);
again:
	while (vp->v_exlockc) {
		/*
		 * If holding an exclusive
		 * lock, then release it (avoid deadlock).
		 */
		if (fp->f_flag & FEXLOCK) {
			vp->v_exlockc = 0;
			vall_sema(&vp->v_exsema);
			s = FDLOCK(fp);
			fp->f_flag &= ~FEXLOCK;
			FDUNLOCK(fp, s);
			break;
		}
		if (cmd & LOCK_NB) {
			VFUNLOCK(vp, vs);
			return (EWOULDBLOCK);
		}
		p_sema_v_lock(&vp->v_exsema, prio, &vp->v_fllock, vs);
		vs = VFLOCK(vp);
	}
	if ((cmd & LOCK_EX) && (vp->v_shlockc)) {
		/*
		 * Wait for shared locks to finish 
		 * before we try to apply exclusive lock.
		 *
		 * If holding shared 
		 * lock, release it (avoid deadlock).
		 */
		if (fp->f_flag & FSHLOCK) {
			if (--vp->v_shlockc == 0)
				vall_sema(&vp->v_shsema);
			s = FDLOCK(fp);
			fp->f_flag &= ~FSHLOCK;
			FDUNLOCK(fp, s);
			goto again;
		}
		if (cmd & LOCK_NB) {
			VFUNLOCK(vp, vs);
			return (EWOULDBLOCK);
		}
		p_sema_v_lock(&vp->v_shsema, prio, &vp->v_fllock, vs);
		vs = VFLOCK(vp);
		goto again;		/* things may have changed */
	}

	/*
	 * Set the lock.
	 */

	ASSERT_DEBUG((fp->f_flag & FEXLOCK) == 0 &&
		!((fp->f_flag & FSHLOCK) && (cmd & LOCK_EX)), "vno_lock");

	if (cmd & LOCK_EX) {
		cmd &= ~LOCK_EX;
		vp->v_exlockc = 1;
		s = FDLOCK(fp);
		fp->f_flag |= FEXLOCK;
		FDUNLOCK(fp, s);
	}
	if ((cmd & LOCK_SH) && (fp->f_flag & FSHLOCK) == 0) {
		vp->v_shlockc++;
		s = FDLOCK(fp);
		fp->f_flag |= FSHLOCK;
		FDUNLOCK(fp, s);
	}

	VFUNLOCK(vp, vs);
	return (0);
}

/*
 * vno_unlock()
 *	Remove an advisory lock on a file.
 */

vno_unlock(fp, kind)
	register struct file *fp;
	int	kind;
{
	register struct vnode *vp = (struct vnode *)fp->f_data;
	register spl_t	s;
	spl_t	vs;

	vs = VFLOCK(vp);
	kind &= fp->f_flag;
	if (kind & FSHLOCK) {
		/*
		 * If release the last shared lock, wake up everyone waiting
		 * for there to be no more shared locks.
		 */
		ASSERT(vp->v_shlockc != 0, "vno_unlock: SHLOCK");
		/*
		 *+ When attempting to release the last shared lock
		 *+ on the file, the reference count was found to be zero.
		 */
		if (--vp->v_shlockc == 0)
			vall_sema(&vp->v_shsema);
		s = FDLOCK(fp);
		fp->f_flag &= ~FSHLOCK;
		FDUNLOCK(fp, s);
	}
	if (kind & FEXLOCK) {
		/*
		 * If release the last (only) exclusive lock, wake up everyone
		 * waiting for there to be no more exclusive locks.
		 */
		ASSERT(vp->v_exlockc == 1, "vno_unlock: EXLOCK");
		/*
		 *+ When attempting to release the last exclusive lock
		 *+ on the file, the reference count was found not to be one.
		 */
		vp->v_exlockc = 0;
		vall_sema(&vp->v_exsema);
		s = FDLOCK(fp);
		fp->f_flag &= ~FEXLOCK;
		FDUNLOCK(fp, s);
	}
	VFUNLOCK(vp, vs);
}
