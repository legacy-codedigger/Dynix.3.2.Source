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
static	char	rcsid[] = "$Header: vfs_syscalls.c 2.22 1991/06/14 22:13:31 $";
#endif

/*
 * vfs_syscalls.c
 *	Various virtual file-system system calls.
 */

/* $Log: vfs_syscalls.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/vfs.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../h/vmmeter.h"
#include "../ufs/inode.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

extern	struct fileops vnodefops;

/*
 * System call routines for operations on files other
 * than read, write and ioctl.  These calls manipulate
 * the per-process file table which references the
 * networkable version of normal UNIX inodes, called vnodes.
 *
 * Many operations take a pathname, which is read
 * into a kernel buffer by pn_get (see vfs_pathname.c).
 * After preparing arguments for an operation, a simple
 * operation proceeds:
 *
 *	error = lookupname(pname, seg, followlink, &dvp, &vp)
 *
 * where pname is the pathname operated on, seg is the segment that the
 * pathname is in (UIOSEG_USER or UIOSEG_KERNEL), followlink specifies
 * whether to follow symbolic links, dvp is a pointer to the vnode that
 * represents the parent directory of vp, the pointer to the vnode
 * referenced by the pathname. The lookupname routine fetches the
 * pathname string into an internal buffer using pn_get (vfs_pathname.c),
 * and iteratively running down each component of the path until the
 * the final vnode and/or it's parent are found. If either of the addresses
 * for dvp or vp are NULL, then it assumes that the caller is not interested
 * in that vnode.  Once the vnode or its parent is found, then a vnode
 * operation (e.g. VOP_OPEN) may be applied to it.
 *
 * One major change from SUN VFS for Dynix (multiprocessor) is that vnodes
 * are held locked at this level.  Eg, lookuppn() returns a locked vnode.
 * This is *much* simpler and more efficient in a multiprocessor.
 */

/*
 * Change current working directory (".").
 */
chdir()
{
	register struct a {
		char *dirnamep;
	} *uap;
	struct vnode *vp;

	uap = (struct a *)u.u_ap;
	u.u_error = chdirec(uap->dirnamep, &vp);
	if (u.u_error == 0) {
		VN_RELE(u.u_cdir);
		u.u_cdir = vp;
	}
}

/*
 * Change notion of root ("/") directory.
 */
chroot()
{
	register struct a {
		char *dirnamep;
	} *uap;
	struct vnode *vp;

	uap = (struct a *)u.u_ap;
	if (!suser())
		return;

	u.u_error = chdirec(uap->dirnamep, &vp);
	if (u.u_error == 0) {
		if (u.u_rdir != (struct vnode *)0)
			VN_RELE(u.u_rdir);
		u.u_rdir = vp;
	}
}

/*
 * Common code for chdir and chroot.
 * Translate the pathname and insist that it
 * is a directory to which we have execute access.
 * If it is replace u.u_[cr]dir with new vnode.
 */
chdirec(dirnamep, vpp)
	char *dirnamep;
	struct vnode **vpp;
{
	struct vnode *vp;		/* new directory vnode */
	register int error;

        error =
	    lookupname(dirnamep, UIOSEG_USER, FOLLOW_LINK,
			(struct vnode **)0, &vp);
	if (error)
		return (error);
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
	} else {
		error = VOP_ACCESS(vp, VEXEC, u.u_cred);
	}
	if (error) {
		VN_PUT(vp);
	} else {
		VN_UNLOCKNODE(vp);
		*vpp = vp;
	}
	return (error);
}

/*
 * Open system call.
 */
open()
{
	register struct a {
		char *fnamep;
		int fmode;
		int cmode;
	} *uap;

	uap = (struct a *)u.u_ap;
	copen(uap->fnamep, (int)(uap->fmode - FOPEN), uap->cmode);
}

/*
 * Creat system call.
 */
creat()
{
	register struct a {
		char *fnamep;
		int cmode;
	} *uap;

	uap = (struct a *)u.u_ap;
	copen(uap->fnamep, FWRITE|FCREAT|FTRUNC, uap->cmode);
}

/*
 * copen_trunc()
 *	This routine was added to save the space required by the vattr
 *	from being on copen's stack info.
 *
 */
static
copen_trunc(vp)
	struct vnode *vp;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_size = 0;
	return (VOP_SETATTR(vp, &vattr, u.u_cred));
}

/*
 * copen()
 *	Common code for open, creat.
 */

copen(pnamep, filemode, createmode)
	char	*pnamep;
	int	filemode;
	int	createmode;
{
	register struct file *fp;
	struct	vnode	*vp;
	int	i;

	/*
	 * Allocate a user file descriptor and file table entry.
	 * Fill in the file table entry to point to the vnode.
	 */

	fp = falloc();
	if (fp == NULL) {
		return;
	}
	i = u.u_r.r_val1;	/* Save in case need to unwind */

	/*
	 * Open the vnode.
	 * We pass &filemode to allow vn_open to change these flags.
	 */

	u.u_error = vn_open(pnamep, UIOSEG_USER, &filemode,
			((createmode & 07777) & ~u.u_cmask), &vp);
	if (u.u_error) {
		ofile_install(u.u_ofile_tab, i, (struct file *) NULL);
		ffree(fp);
		return;
	}

	/*
	 * FMASK only include FREAD, FWRITE, FAPPEND, FASYNC, and FSYNC,
	 * so we arent effected by filemode changes in vn_open
	 * (which may turn off FCREAT, FTRUNC, and FEXCL.
	 */ 

	fp->f_flag = filemode & FMASK;
	fp->f_type = DTYPE_VNODE;
	fp->f_data = (caddr_t)vp;
	fp->f_ops = &vnodefops;

	/*
	 * Truncate if required.
	 */

	if (filemode & FTRUNC) {
		filemode &= ~FTRUNC;
		u.u_error = copen_trunc(vp);	/* to save stack space */
	}

	VN_UNLOCKNODE(vp);

	/*
	 * VOP_OPEN(), which calls routines for special files,
	 * must be done after node is unlocked to prevent deadlock
	 * if we hang in a driver routine.
	 */

	if (u.u_error == 0)
		u.u_error = VOP_OPEN(&vp, filemode|FNORM, u.u_cred);
	if (u.u_error == 0) {
		ofile_install(u.u_ofile_tab, i, fp);
		return;
	}

	/*
	 * Unwind if anything failed above.
	 */

	ofile_install(u.u_ofile_tab, i, (struct file *) NULL);
	ffree(fp);
	VN_RELE(vp);
	return;
}

/*
 * Create a special (or regular) file.
 */
mknod()
{
	register struct a {
		char		*pnamep;
		int		fmode;
		int		dev;
	} *uap;

	struct vnode *vp;
	struct vattr vattr;
	enum vtype vtype;

	uap = (struct a *)u.u_ap;
	vtype = IFTOVT(uap->fmode);

	/*
	 * Must be super user, unless creating a FIFO
	 */
	 
	if (vtype != VFIFO && !suser())
		return;

	/*
	 * Allow super user to mknod a regular file
	 */

	if (vtype == VNON)
		vtype = VREG;

	/*
	 * Can't mknod directories in UCB tuniverse. Must use mkdir.
	 */
	if (vtype == VDIR && u.u_tuniverse == U_UCB) {
		u.u_error = EISDIR;
		return;	
	}

	/*
	 * Setup desired attributes and vn_create the file.
	 */
	vattr_null(&vattr);
	vattr.va_type = vtype;
	if (vtype == VDIR)
		vattr.va_mode = (uap->fmode & 00777) & ~u.u_cmask;
	else
		vattr.va_mode = (uap->fmode & 07777) & ~u.u_cmask;

	switch (vattr.va_type) {

	case VBAD:
	case VCHR:
	case VBLK:
		vattr.va_rdev = uap->dev;
		break;
	default:
		break;
	}

	u.u_error = vn_create(uap->pnamep, UIOSEG_USER, &vattr, EXCL, 0, &vp);
	if (u.u_error == 0)
		VN_PUT(vp);
}

/*
 * Make a directory.
 */
mkdir()
{
	struct a {
		char	*dirnamep;
		int	dmode;
	} *uap;

	struct vnode *vp;
	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	vattr_null(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & 0777) & ~u.u_cmask;

	u.u_error = vn_create(uap->dirnamep, UIOSEG_USER, &vattr, EXCL, 0, &vp);
	if (u.u_error == 0)
		VN_PUT(vp);
}

/*
 * make a hard link
 */
link()
{
	register struct a {
		char	*from;
		char	*to;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_error = vn_link(uap->from, uap->to, UIOSEG_USER);
}

/*
 * rename or move an existing file
 */
rename()
{
	register struct a {
		char	*from;
		char	*to;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_error = vn_rename(uap->from, uap->to, UIOSEG_USER);
}

/*
 * Create a symbolic link.
 * Similar to link or rename except target
 * name is passed as string argument, not
 * converted to vnode reference.
 */
symlink()
{
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;

	struct vnode *dvp;
	struct vattr vattr;
	struct pathname tpn;
	struct pathname lpn;

	uap = (struct a *)u.u_ap;
	u.u_error = pn_get(uap->linkname, UIOSEG_USER, &lpn);
	if (u.u_error)
		return;
	u.u_error = lookuppn(&lpn, NO_FOLLOW, &dvp, (struct vnode **)0);
	if (u.u_error) {
		pn_free(&lpn);
		return;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
		goto out;
	}
	u.u_error = pn_get(uap->target, UIOSEG_USER, &tpn);
	vattr_null(&vattr);
	vattr.va_mode = 0777 & ~u.u_cmask;
	if (u.u_error == 0) {
		u.u_error =
		   VOP_SYMLINK(dvp, lpn.pn_path, &vattr, tpn.pn_path, u.u_cred);
		pn_free(&tpn);
	}
out:
	pn_free(&lpn);
	VN_RELE(dvp);
}

/*
 * csymlink -- make a conditional symbolic link
 */
csymlink()
{
	register struct a {
		char	*ucb_target;
		char	*att_target;
		char	*linkname;
	} *uap;

	struct vnode *dvp;
	struct vattr vattr;
	struct pathname ucbpn;
	struct pathname attpn;
	struct pathname lpn;

	uap = (struct a *)u.u_ap;
	u.u_error = pn_get(uap->linkname, UIOSEG_USER, &lpn);
	if (u.u_error)
		return;
	u.u_error = lookuppn(&lpn, NO_FOLLOW, &dvp, (struct vnode **)0);
	if (u.u_error) {
		pn_free(&lpn);
		return;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
		goto out;
	}

	u.u_error = pn_get(uap->ucb_target, UIOSEG_USER, &ucbpn);
	if (ucbpn.pn_pathlen > MAXCSYMLEN)
		u.u_error = ELOOP;
	if (u.u_error != 0)
		goto out;

	u.u_error = pn_get(uap->att_target, UIOSEG_USER, &attpn);
	if (attpn.pn_pathlen > MAXCSYMLEN)
		u.u_error = ELOOP;
	if (u.u_error != 0)
		goto out1;

	vattr_null(&vattr);
	vattr.va_mode = 0777 & ~u.u_cmask;
	u.u_error = VOP_CSYMLINK(dvp, lpn.pn_path, &vattr, 
				 ucbpn.pn_path, attpn.pn_path,  u.u_cred);
	pn_free(&attpn);
out1:
	pn_free(&ucbpn);
out:
	pn_free(&lpn);
	VN_RELE(dvp);
}

/*
 * Unlink (i.e. delete) a file.
 */
unlink()
{
	struct a {
		char	*pnamep;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_error = vn_remove(uap->pnamep, UIOSEG_USER, FILE);
}

/*
 * Remove a directory.
 */
rmdir()
{
	struct a {
		char	*dnamep;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_error = vn_remove(uap->dnamep, UIOSEG_USER, DIRECTORY);
}

/*
 * get directory entries in a file system independent format
 */
getdirentries()
{
	register struct a {
		int	fd;
		char	*buf;
		unsigned count;
		long	*basep;
	} *uap;

			struct file *fp;
	register	struct vnode *vp;
			struct uio auio;
			struct iovec aiov;

	uap = (struct a *)u.u_ap;
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	if ((fp->f_flag & FREAD) == 0) {
		u.u_error = EBADF;
		return;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	vp = (struct vnode *)fp->f_data;
	VN_LOCKNODE(vp);
	u.u_error = VOP_READDIR(vp, &auio, fp->f_cred);
	VN_UNLOCKNODE(vp);
	if (u.u_error)
		return;
	u.u_error =
	    copyout((caddr_t)&fp->f_offset, (caddr_t)uap->basep, sizeof(long));
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	u.u_ioch += u.u_r.r_val1;
	fp->f_offset = auio.uio_offset;
}

/*
 * Seek on file.  Only hard operation
 * is seek relative to end which must
 * apply to vnode for current file size.
 * 
 * Note: lseek(0, 0, L_XTND) costs much more than it did before.
 */
lseek()
{
	register struct a {
		int	fd;
		off_t	off;
		int	sbase;
	} *uap;
	struct file *fp;

	uap = (struct a *)u.u_ap;
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error) {
		if (u.u_error == EINVAL)
			u.u_error = ESPIPE;
		return;
	}
	if (((struct vnode *)fp->f_data)->v_type == VFIFO) {
		u.u_error = ESPIPE;
		return;
	}

	fp->f_uoffset = 0;

	switch (uap->sbase) {

	case L_INCR:
		fp->f_offset += uap->off;
		break;

	case L_XTND: {
		struct vattr vattr;

		VN_LOCKNODE((struct vnode *)fp->f_data);
		u.u_error =
		    VOP_GETATTR((struct vnode *)fp->f_data, &vattr, u.u_cred);
		VN_UNLOCKNODE((struct vnode *)fp->f_data);
		if (u.u_error)
			return;
		fp->f_offset = uap->off + vattr.va_size;
		break;
	}

	case L_SET:
		fp->f_offset = uap->off;
		break;

	default:
		u.u_error = EINVAL;
	}
	u.u_r.r_off = fp->f_offset;
}

/*
 * Determine accessibility of file, by
 * reading its attributes and then checking
 * against our protection policy.
 */
saccess()
{
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;

	struct vnode *vp;
	register u_short mode;
	struct ucred *savcred = (struct ucred *)NULL;

	uap = (struct a *)u.u_ap;

	/*
	 * If necessary, use the real uid and gid to check access.
	 */
	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid) {
		savcred = u.u_cred;
		u.u_cred = crdup(u.u_cred);
		u.u_uid = u.u_ruid;
		u.u_gid = u.u_rgid;
	}

	/*
	 * Lookup file
	 */
	u.u_error = lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
				(struct vnode **)0, &vp);
	if (u.u_error) {
		if (savcred) {
			crfree(u.u_cred);
			u.u_cred = savcred;
		}
		return;
	}

	mode = 0;
	/*
	 * fmode == 0 means only check for exist
	 */
	if (uap->fmode) {
		if (uap->fmode & R_OK)
			mode |= VREAD;
		if (uap->fmode & W_OK) {
			if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
				u.u_error = EROFS;
				goto out;
			}
			mode |= VWRITE;
		}
		if (uap->fmode & X_OK)
			mode |= VEXEC;
		u.u_error = VOP_ACCESS(vp, mode, u.u_cred);
	}

	/*
	 * release the vnode and restore the uid and gid.
	 */
out:
	VN_PUT(vp);
	if (savcred) {
		crfree(u.u_cred);
		u.u_cred = savcred;
	}
}

/*
 * Get attributes from file or file descriptor.
 * Argument says whether to follow links, and is
 * passed through in flags.
 */
stat()
{
	struct a {
		char	*fname;
		struct	stat *ub;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_error = stat1(uap, FOLLOW_LINK);
}

lstat()
{
	struct a {
		char	*fname;
		struct	stat *ub;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_error = stat1(uap, NO_FOLLOW);
}

stat1(uap, follow)
	register struct a {
		char	*fname;
		struct	stat *ub;
	} *uap;
	enum symfollow follow;
{
	struct vnode *vp;
	struct stat sb;
	register int error;

	error =
	    lookupname(uap->fname, UIOSEG_USER, follow,
			(struct vnode **)0, &vp);
	if (error)
		return (error);
	error = vno_stat(vp, &sb);
	VN_PUT(vp);
	if (error)
		return (error);
	return (copyout((caddr_t)&sb, (caddr_t)uap->ub, sizeof(sb)));
}

/*
 * Read contents of symbolic link.
 */
readlink()
{
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap;

	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;

	uap = (struct a *)u.u_ap;
	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, NO_FOLLOW,
			(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if (vp->v_type != VLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READLINK(vp, &auio, u.u_cred);
out:
	VN_PUT(vp);
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	u.u_ioch += u.u_r.r_val1;
}

/*
 * Return target names of a conditional symbolic link
 */
readclink()
{
	register struct a {
		char	*name;
		char	*buf;
		int	count;
		int	u_flag;
	} *uap = (struct a *)u.u_ap;

	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;

	uap = (struct a *)u.u_ap;
	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, NO_FOLLOW,
			(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if (vp->v_type != VLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READCLINK(vp, &auio, uap->u_flag, u.u_cred);
out:
	VN_PUT(vp);
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	u.u_ioch += u.u_r.r_val1;
}

/*
 * Change mode of file given path name.
 */
chmod()
{
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;

	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr);
}

/*
 * Change mode of file given file descriptor.
 */
fchmod()
{
	register struct a {
		int	fd;
		int	fmode;
	} *uap;

	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = fdsetattr(uap->fd, &vattr);
}

/*
 * Change ownership of file given file name.
 */
chown()
{
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;

	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = namesetattr(uap->fname, NO_FOLLOW,  &vattr);
}

/*
 * Change ownership of file given file descriptor.
 */
fchown()
{
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap;

	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = fdsetattr(uap->fd, &vattr);
}

/*
 * Set access/modify times on named file.
 */
utimes()
{
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap;

	struct timeval tv[2];
	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	vattr_null(&vattr);
	if (uap->tptr != NULL) {
		u.u_error = copyin((caddr_t)uap->tptr,(caddr_t)tv, sizeof(tv));
		if (u.u_error)
			return;
		vattr.va_atime = tv[0];
		vattr.va_mtime = tv[1];
	} else {
		/*
		 * You can change the mod/acc times if
	 	 * (1) you are su or owner,   or
	 	 * (2) you have write access &&
	 	 *     uap->tptr is NULL.
		 * We need to let the fs-specific setattr routine
		 * know that uap->tptr was NULL. We do it by setting
		 * tv_sec to 0 and tv_usec to -1.
		 */
		vattr.va_atime.tv_sec = 0;
		vattr.va_mtime.tv_sec = 0;
	}
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr);
}

/*
 * Truncate a file given its path name.
 */
truncate()
{
	register struct a {
		char	*fname;
		int	length;
	} *uap;

	struct vattr vattr;

	uap = (struct a *)u.u_ap;
	if (uap->length < 0) {
		u.u_error = EINVAL;
		return;
	}
	vattr_null(&vattr);
	vattr.va_size = uap->length;
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr);
}

/*
 * Truncate a file given a file descriptor.
 */
ftruncate()
{
	register struct a {
		int	fd;
		int	length;
	} *uap;

	register struct vnode *vp;
	struct file *fp;

	uap = (struct a *)u.u_ap;
	if (uap->length < 0) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	vp = (struct vnode *)fp->f_data;
	if ((fp->f_flag & FWRITE) == 0) {
		u.u_error = EINVAL;
	} else if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
	} else {
		struct vattr vattr;

		vattr_null(&vattr);
		vattr.va_size = uap->length;
		VN_LOCKNODE(vp);
		u.u_error = VOP_SETATTR(vp, &vattr, fp->f_cred);
		VN_UNLOCKNODE(vp);
	}
}

/*
 * Common routine for modifying attributes
 * of named files.
 */
namesetattr(fnamep, followlink, vap)
	char *fnamep;
	enum symfollow followlink;
	struct vattr *vap;
{
	struct vnode *vp;
	register int error;

	error =
	    lookupname(fnamep, UIOSEG_USER, followlink,
			(struct vnode **)0, &vp);
	if (error)
		return (error);	
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else
		error = VOP_SETATTR(vp, vap, u.u_cred);
	VN_PUT(vp);
	return (error);
}

/*
 * Common routine for modifying attributes
 * of file referenced by descriptor.
 */
fdsetattr(fd, vap)
	int fd;
	struct vattr *vap;
{
	struct file *fp;
	register struct vnode *vp;
	register int error;

	error = getvnodefp(fd, &fp);
	if (error == 0) {
		vp = (struct vnode *)fp->f_data;
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return (EROFS);
		VN_LOCKNODE(vp);
		error = VOP_SETATTR(vp, vap, fp->f_cred);
		VN_UNLOCKNODE(vp);
	}
	return (error);
}

/*
 * Flush output pending for file.
 */
fsync()
{
	struct a {
		int	fd;
	} *uap;

		 struct file *fp;
	register struct vnode *vp;

	uap = (struct a *)u.u_ap;
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0) {
		vp = (struct vnode *)fp->f_data;
		VN_LOCKNODE(vp);
		u.u_error = VOP_FSYNC(vp, fp->f_cred);
		VN_UNLOCKNODE(vp);
	}
}

/*
 * Set file creation mask.
 */
umask()
{
	register struct a {
		int mask;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_r.r_val1 = u.u_cmask;
	u.u_cmask = uap->mask & 07777;
}

/*
 * vhangup()
 *	Revoke access the current tty by all processes.
 *
 * Used by the super-user in init to give ``clean'' terminals at login.
 * Also used by (eg) rlogind, window managers, etc, to render old file
 * descriptors on pseudo-ttys useless when session ends.
 *
 * This is a poor interface; MUCH better would be to insist on open
 * file-descriptor (eg, fhangup(fd)), rather than using side-effects of
 * the tty sub-system.  An "fhangup" interface would also be more efficient
 * (since the underlying object can be known) and more useful (it can work
 * on *any* type of file, not just tty's).
 */

vhangup()
{
	spl_t s_ipl;

	if (!suser())
		return;
	if (u.u_ttyp == NULL)
		return;
	forceclose(u.u_ttyd);
	s_ipl = p_lock(&u.u_ttyp->t_ttylock, SPLTTY);
	if ((u.u_ttyp->t_state) & TS_ISOPEN)
		gsignal(u.u_ttyp->t_pgrp, SIGHUP);
	v_lock(&u.u_ttyp->t_ttylock, s_ipl);
}

/*
 * forceclose()
 *	Search for references to a particular dev in the file table.
 *	For each one found, make it useless for read and write.
 *
 * Note that on a multiprocessor, this can race with a concurrent open
 * and not clear the flags in the new open.  This is a racey issue anyhow,
 * even on a mono-processor (if you consider cases), so don't worry.
 *
 * An "fhangup" interface could look into the underlying vnode since it
 * would be *known* the vnode wouldn't go away.  Thus could match on the
 * vnode itself, avoiding the file-table list lock round-trip per
 * DTYPE_VNODE entry, only locking it per matching entry.
 */

static
forceclose(dev)
	dev_t	dev;
{
	register struct file *fp;
	register struct vnode *vp;
	register spl_t	s;

	/*
	 * For each file-table entry that's in use and references a vnode,
	 * zap flags if it matches the dev of interest.
	 *
	 * If the overhead is too high, can store v_rdev in the file-table
	 * entry for VCHR vnodes, thus only lock tables on matching entries.
	 */

	for (fp = file; fp < fileNFILE; fp++) {

		if (fp->f_count == 0 || fp->f_type != DTYPE_VNODE)
			continue;

		/*
		 * Lock file-table list.  Is now ok to check count and
		 * type again since if count != 0 it can't be actually
		 * getting freed.  Careful of f_data -- this is zero
		 * until filled out in copen(), even if count != 0.
		 *
		 * Locking file-table free-list insures can't race with
		 * process freeing the file-table entry (since want to
		 * lock the entry, must insure it can't concurrently get
		 * free'd and re-allocated).  Also can check the entry
		 * without locking it since vno_close() does ffree()
		 * *before* calling vn_close(), and error unwind does
		 * ffree() *before* VN_RELE().
		 */

		s = p_lock(&file_list, SPLFS);

		if (fp->f_count != 0
		&&  fp->f_type == DTYPE_VNODE
		&&  (vp = (struct vnode *)fp->f_data) != NULL
		&&  vp->v_type == VCHR
		&&  vp->v_rdev == dev) {
			(void) p_lock(&fp->f_mutex, SPLFS);
			fp->f_flag &= ~(FREAD|FWRITE);
			v_lock(&fp->f_mutex, SPLFS);
		}

		v_lock(&file_list, s);
	}
}

/*
 * getvnodefp()
 *	Get the file structure entry for the file descrpitor, but make sure
 *	its a vnode.
 */

int
getvnodefp(fd, fpp)
	int fd;
	struct file **fpp;
{
	register struct file *fp;

	GETFP(fp, fd);
	if (fp == NULL)
		return EBADF;
	if (fp->f_type != DTYPE_VNODE)
		return (EINVAL);
	*fpp = fp;
	return (0);
}

/*
 * check_rw_fd()
 *	Check if a writable file-descriptor exists for a given vnode.
 *
 * Used to insure don't cache a text (or allow exec) if the file can change.
 * Returns 0 for no such file-descriptor, else error (ETXTBSY).
 *
 * Caller holds locked vnode.
 */

check_rw_fd(vp)
	register struct vnode *vp;
{
	register struct file *fp;

	/*
	 * Scan file-table to check for writing use of same vnode.  This
	 * needs no locking of file-table or entries, since we hold
	 * locked vnode, and copen() arranges that file-table entry is
	 * filled out before unlocking vnode.  This can race and miss a
	 * concurrent close, but this is ok.  Can also race with other
	 * use of same text going away, but locked inode here also avoids
	 * problems.
	 *
	 * This check assumes if v_count==1 (caller has only ref) the
	 * file-system doesn't increment v_count until after holding vnode
	 * (soft) locked.  See ufs_inode.c:iget() for example.
	 */

	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_type == DTYPE_VNODE
		&&  fp->f_count > 0
		&&  (struct vnode *)fp->f_data == vp
		&&  (fp->f_flag & FWRITE))
			return (ETXTBSY);
	}

	return (0);				/* no RW fd's */
}

/*
 * universe()
 *	Universe system call -- set flag for UCB or AT&T universe.
 */

universe()
{
	register struct a {
		int u_flag;
	} *uap = (struct a *) u.u_ap;

	u.u_r.r_val1 = u.u_universe;
	if (uap->u_flag == U_GET)
		return;

	if (uap->u_flag != U_ATT && uap->u_flag != U_UCB) {
		u.u_error =  EINVAL;
		return;
	}
	u.u_universe = uap->u_flag;
}
