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
static	char	rcsid[] = "$Header: vfs_vnode.c 2.15 1991/07/09 22:20:16 $";
#endif

/*
 * vfs_vnode.c
 *	Various virtual file-system vnode manipulation procedures.
 */

/* $Log: vfs_vnode.c,v $
 *
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/mutex.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/pathname.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

/*
 * read or write a vnode
 * Caller locks vp depending on type.  See vno_rw().
 */

int
vn_rdwr(rw, vp, base, len, offset, seg, ioflag, aresid)
	enum uio_rw rw;
	struct vnode *vp;
	caddr_t base;
	size_t len;
	int offset;
	int seg;
	int ioflag;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	int error;
/*printf("vn_rdwr: rw=%x vp=%x base=%x len=%x offset=%x seg=%x ioflag=%x areasid=%x\n",
rw, vp, base, len, offset, seg, ioflag, aresid);*/

	if (rw == UIO_WRITE && (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		return (EROFS);
	}

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = seg;
	auio.uio_resid = len;
	error = VOP_RDWR(vp, &auio, rw, ioflag, u.u_cred);
/*printf("vn_rdwr: VOP_RDWR ret error=%d\n", error);*/
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid)
			error = EIO;
	return (error);
}

/*
 * Open/create a vnode.
 * This may be callable by the kernel, the only known side effect being that
 * the current user uid and gid are used for permissions.
 * *vpp is returned referenced, with a locked node.
 */

int
vn_open(pnamep, seg, filemodep, createmode, vpp)
	char *pnamep;
	int *filemodep;
	int createmode;
	struct vnode **vpp;
{
	struct vnode *vp;		/* ptr to file vnode */
	register int mode;
	register int error;
	register int filemode = *filemodep;

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE | FTRUNC))
		mode |= VWRITE;
 
	if (filemode & FCREAT) {
		struct vattr vattr;
		enum vcexcl excl;

		/*
		 * Wish to create a file.
		 */
		vattr_null(&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = createmode;
		if (filemode & FTRUNC)
			vattr.va_size = 0;
		if (filemode & FEXCL)
			excl = EXCL;
		else
			excl = NONEXCL;
		filemode &= ~(FCREAT | FTRUNC | FEXCL);

		error = vn_create(pnamep, seg, &vattr, excl, mode, &vp);
		if (error)
			return (error);
	} else {
		/*
		 * Wish to open a file.
		 * Just look it up.
		 */
		error =
		    lookupname(pnamep, seg, FOLLOW_LINK,
			(struct vnode **)0, &vp);
		if (error)
			return (error);
		/*
		 * cannnot write directories, active texts or
		 * read only filesystems
		 */
		if (filemode & (FWRITE | FTRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
				error = EROFS;
				goto out;
			}
			/*
			 * If there's shared text associated with
			 * the vnode, try to free it up once.
			 * If we fail, we can't allow writing.
			 */
			if (vp->v_flag & VTEXT) {
				if (!xrele(vp)) {
					error = ETXTBSY;
					goto out;
				}
			}
		}
		/*
		 * check permissions
		 */
		error = VOP_ACCESS(vp, mode, u.u_cred);
		if (error)
			goto out;
		/*
		 * Sockets in filesystem name space are not supported (yet?)
		 */
		if (vp->v_type == VSOCK) {
			error = EOPNOTSUPP;
			goto out;
		}
	}
out:
	if (error) {
		VN_PUT(vp);
	} else {
		*vpp = vp;
		*filemodep = filemode;
	}
	return (error);
}

/*
 * create a vnode (makenode)
 *	Returns a referenced node-locked vnode.
 */

int
vn_create(pnamep, seg, vap, excl, mode, vpp)
	char *pnamep;
	int seg;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
{
	struct vnode *dvp;	/* ptr to parent dir vnode */
	struct pathname pn;
	register int error;

	/*
	 * Lookup directory.
	 * If new object is a file, call lower level to create it.
	 * Note that it is up to the lower level to enforce exclusive
	 * creation, if the file is already there.
	 * This allows the lower level to do whatever
	 * locking or protocol that is needed to prevent races.
	 * If the new object is directory call lower level to make
	 * the new directory, with "." and "..".
	 */
	*vpp = NULLVP;
	dvp = NULLVP;
	error = pn_get(pnamep, seg, &pn);
	if (error)
		return (error);
	/*
	 * lookup will find the parent directory for the vnode.
	 * When it is done the pn hold the name of the entry
	 * in the directory.
	 * If this is a non-exclusive create we also find the node itself.
	 */
	error = lookuppn(&pn, FOLLOW_LINK, &dvp,
				(excl == NONEXCL? vpp: (struct vnode **)0));
	if (error) {
		pn_free(&pn);
		return (error);
	}
	/*
	 * Make sure filesystem is writeable
	 */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		if (*vpp) {
			VN_PUT(*vpp);
		}
		error = EROFS;
	} else if (excl == NONEXCL && *vpp != NULLVP) {
		/*
		 * The file is already there.
		 * If we are writing, and there's a shared text
		 * associated with the vnode, try to free it up once.
		 * If we fail, we can't allow writing.
		 */
		if ((mode & VWRITE) && ((*vpp)->v_flag & VTEXT)) {
			if (!xrele(*vpp))
				error = ETXTBSY;
		}
		/*
		 * we throw the vnode away to let VOP_CREATE truncate the
		 * file in a non-racy manner.
		 */
		VN_PUT(*vpp);
	}
	if (error == 0) {
		/*
		 * call mkdir if directory or create if other
		 */
		if (vap->va_type == VDIR) {
			error = VOP_MKDIR(dvp, pn.pn_path, vap, vpp, u.u_cred);
		} else {
			error = VOP_CREATE(
			    dvp, pn.pn_path, vap, excl, mode, vpp, u.u_cred);
		}
	}
	pn_free(&pn);
	VN_RELE(dvp);
	return (error);
}

/*
 * close a vnode
 */

int
vn_close(vp, flag)
register struct vnode *vp;
int flag;
{
	return (VOP_CLOSE(vp, flag, u.u_cred));
}

/*
 * Link.
 */

int
vn_link(from_p, to_p, seg)
	char *from_p;
	char *to_p;
	int seg;
{
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct pathname pn;
	register int error;

	fvp = tdvp = NULLVP;
	error = pn_get(to_p, seg, &pn);
	if (error)
		return (error);

	/*
	 * After the following lookups, fvp is node-locked, 
	 * tdvp is node-unlocked. Both are referenced.
	 * Unlock fvp, as the code below VOP_LINK expects.
	 * This could possibly be optimized in the future.
	 */

	error = lookupname(from_p, seg, FOLLOW_LINK, (struct vnode **)0, &fvp);
	if (error)
		goto out;
	VN_UNLOCKNODE(fvp);

	error = lookuppn(&pn, FOLLOW_LINK, &tdvp, (struct vnode **)0);
	if (error)
		goto out;

	/*
	 * Make sure both source vnode and target directory vnode are
	 * in the same vfs and that it is writeable.
	 */

	if (fvp->v_vfsp != tdvp->v_vfsp) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * Do the link.
	 */

	error = VOP_LINK(fvp, tdvp, pn.pn_path, u.u_cred);
out:
	pn_free(&pn);
	if (fvp)
		VN_RELE(fvp);
	if (tdvp)
		VN_RELE(tdvp);
	return (error);
}

/*
 * vn_rename()
 *	Rename a vnode.
 */

int
vn_rename(from_p, to_p, seg)
	char *from_p;
	char *to_p;
	int seg;
{
	struct vnode *fdvp;		/* from directory vnode ptr */
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct pathname fpn;		/* from pathname */
	struct pathname tpn;		/* to pathname */
	register int error;

	fvp = fdvp = tdvp = NULLVP;

	/*
	 * Get to and from pathnames.
	 */

	error = pn_get(from_p, seg, &fpn);
	if (error)
		return (error);

	error = pn_get(to_p, seg, &tpn);
	if (error) {
		pn_free(&fpn);
		return (error);
	}

	/*
	 * Lookup to and from directories, and make sure "from" file exists.
	 */

	error = lookuppn(&fpn, NO_FOLLOW, &fdvp, &fvp);
	if (error)
		goto out;
	if (fvp == NULLVP) {
		error = ENOENT;
		goto out;
	}
	VN_UNLOCKNODE(fvp);		/* unlock but still referenced */

	error = lookuppn(&tpn, NO_FOLLOW, &tdvp, (struct vnode **)0);
	if (error)
		goto out;

	/*
	 * Make sure both the from vnode and the to directory are
	 * in the same vfs and that it is writeable.
	 */

	if (fvp->v_vfsp != tdvp->v_vfsp) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * Do the rename.
	 */

	error = VOP_RENAME(fdvp, fpn.pn_path, tdvp, tpn.pn_path, u.u_cred);
out:
	pn_free(&fpn);
	pn_free(&tpn);
	if (fvp)
		VN_RELE(fvp);
	if (fdvp)
		VN_RELE(fdvp);
	if (tdvp)
		VN_RELE(tdvp);
	return (error);
}

/*
 * vn_remove()
 *	Remove a file or directory.
 */

int
vn_remove(fnamep, seg, dirflag)
	char *fnamep;
	int seg;
	enum rm dirflag;
{
	struct vnode *vp;		/* entry vnode */
	struct vnode *dvp;		/* ptr to parent dir vnode */
	struct pathname pn;		/* name of entry */
	enum vtype vtype;
	register int error;

	error = pn_get(fnamep, seg, &pn);
	if (error)
		return (error);
	vp = NULLVP;
	error = lookuppn(&pn, NO_FOLLOW, &dvp, &vp);
	if (error) {
		pn_free(&pn);
		return (error);
	}
	if (dvp == vp) {	/* disallow if child is alias of parent */
		error = EINVAL;
		goto out;
	}
	/*
	 * make sure there is an entry
	 */
	if (vp == NULLVP) {
		error = ENOENT;
		goto out;
	}
	/*
	 * make sure filesystem is writeable
	 */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * don't unlink the root of a mounted filesystem or where a mount
	 * is concurrently being attempted.
	 */
	if (vp->v_flag & (VROOT|VMOUNTING)) {
		error = EBUSY;
		goto out;
	}
	/*
	 * Release vnode before removing.
	 * Also try once to delete text ref (if any); this can race with
	 * concurrent create, execute, release of file by same name in this
	 * directory with very low probability (doesn't hurt, might just
	 * leave a tacky text with no file-system name; see xfree()).
	 */
	vtype = vp->v_type;
	if (vp->v_flag & VTEXT)
		(void) xrele(vp);
	VN_PUT(vp);
	vp = NULLVP;
	if (vtype == VDIR) {
		/*
		 * if caller thought it was removing a directory, go ahead
		 */
		if (dirflag == DIRECTORY || suser())
			error = VOP_RMDIR(dvp, pn.pn_path, u.u_cred);
		else
			error = EPERM;
	} else {
		/*
		 * if caller thought it was removing a directory, barf.
		 */
		if (dirflag == FILE)
			error = VOP_REMOVE(dvp, pn.pn_path, u.u_cred);
		else
			error = ENOTDIR;
	}
out:
	pn_free(&pn);
	if (vp != NULLVP)
		VN_PUT(vp);
	VN_RELE(dvp);
	return (error);
}

/*
 * nullvattr_init()
 *	Initialize the nullvattr structure.
 */

struct	vattr	nullvattr;

nullvattr_init()
{
	register int n;
	register char *cp;

	n = sizeof(struct vattr);
	cp = (char *) &nullvattr;
	while (n--) {
		*cp++ = -1;
	}
}
