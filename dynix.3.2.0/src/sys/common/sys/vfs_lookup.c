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
static	char	rcsid[] = "$Header: vfs_lookup.c 2.12 91/01/10 $";
#endif

/*
 * vfs_lookup.c
 *	Virtual File-System path-name lookup/etc.
 */

/* $Log:	vfs_lookup.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/dir.h"
#include "../h/buf.h"
#include "../h/pathname.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

/*
 * lookupname()
 *	lookup the user file name.
 *
 * Handles allocation and freeing of pathname buffer, return error.
 *
 * On return, **compvpp (if given) is (soft) locked, **dirvp (if given)
 * is not locked.  Both have new references.
 */

lookupname(fnamep, seg, followlink, dirvpp, compvpp)
	char *fnamep;			/* user pathname */
	int seg;			/* addr space that name is in */
	enum symfollow followlink;	/* follow sym links */
	struct vnode **dirvpp;		/* ret for ptr to parent dir vnode */
	struct vnode **compvpp;		/* ret for ptr to component vnode */
{
	struct pathname lookpn;
	register int error;

	error = pn_get(fnamep, seg, &lookpn);
	if (error) {
		return (error);
	}
	error = lookuppn(&lookpn, followlink, dirvpp, compvpp);
	pn_free(&lookpn);
	ASSERT_DEBUG( error != 0 
		|| compvpp == (struct vnode **)0 
		|| VN_LOCKEDNODE(*compvpp), "lookupname return unlocked");
	ASSERT_DEBUG( error != 0 
		|| compvpp == (struct vnode **)0 
		|| ((*compvpp)->v_count != 0), "lookupname return unrefd");
	return (error);
}

/*
 * lookuppn()
 *	Starting at current directory, translate pathname pnp to end.
 *
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 *
 * On return: *dirvpp is unlocked (but ref'd), *compvpp is locked (if != 0).
 * If error, nothing stored in *dirvpp or *compvpp.
 */

#define	SZLCOMP	23			/* size of stack local component */

lookuppn(pnp, followlink, dirvpp, compvpp)
	register struct pathname *pnp;		/* pathaname to lookup */
	enum symfollow followlink;		/* (don't) follow sym links */
	struct vnode **dirvpp;			/* ptr for parent vnode */
	struct vnode **compvpp;			/* ptr for entry vnode */
{
	register struct vnode *vp;		/* current directory vp */
	register struct vnode *cvp = NULLVP;	/* current component vp */
	register int	error;
	struct	vnode	*tvp;			/* non-reg temp ptr */
	struct	buf	*comp_bp = NULL;	/* holds longer components */
	char	comp[SZLCOMP+1];		/* buffer for component */
	register char	*component = comp;	/* current component string */
	int	szcomp = SZLCOMP;		/* size of component array */
	int	nlink = 0;			/* sym-link counter */
	extern	int	maxsymlinks;		/* max sym-link hops */

	/*
	 * start at current directory.
	 */
	vp = u.u_cdir;
	VN_HOLD(vp);

	/*
	 * Each time we begin a new name interpretation (e.g.
	 * when first called and after each symbolic link is
	 * substituted), we allow the search to start at the
	 * root directory if the name starts with a '/', otherwise
	 * continuing from the current directory.
	 */
begin:
	component[0] = 0;
	if (pn_peekchar(pnp) == '/') {
		VN_RELE(vp);
		pn_skipslash(pnp);
		if (u.u_rdir)
			vp = u.u_rdir;
		else
			vp = rootdir;
		VN_HOLD(vp);
	}
	VN_LOCKNODE(vp);

	/*
	 * Make sure we have a directory.
	 *
	 * At this point, vp is ref'd and locked, cvp is null.
	 */
next:
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}

	/*
	 * Process the next component of the pathname.
	 * Complicated "comp_bp" fuss avoids the need for a 256 byte
	 * component[] on the stack.
	 */
	while (error = pn_getcomponent(pnp, component, szcomp)) {
		if (error == ENAMETOOLONG && comp_bp == NULL) {
			comp_bp = geteblk(MAXNAMLEN+1);
			component = comp_bp->b_un.b_addr;
			szcomp = MAXNAMLEN;
			continue;
		}
		goto bad;
	}

	/*
	 * Check for degenerate name (e.g. / or "") which is a way of talking
	 * about a directory, e.g. "/." or ".".
	 */
	if (component[0] == 0) {
		/*
		 * If the caller was interested in the parent then
		 * return an error since we don't have the real parent
		 */
		if (dirvpp != (struct vnode **)0) {
			error = EISDIR;		/*SVVS*/
			goto bad;
		}
		if (error = pn_set(pnp, "."))
			goto bad;
		if (compvpp != (struct vnode **)0)
			*compvpp = vp;			/* already locked */
		else {
			VN_PUT(vp);
		}
		if (comp_bp)
			brelse(comp_bp);
		return (0);
	}

	/*
	 * Handle "..": two special cases.
	 * 1. If at root directory (e.g. after chroot)
	 *    then ignore it so can't get out.
	 * 2. If this vnode is the root of a mounted
	 *    file system, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    .. in the other file system.  No races with
	 *    unmount since hold a ref to the root vnode
	 *    until after hold ref to covered vnode.
	 */
	if (component[0] == '.' && component[1] == '.' && component[2] == '\0'){
checkforroot:
		if (vp == u.u_rdir || vp == rootdir) {
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
		}
		if (vp->v_flag & VROOT) {
			cvp = vp;			/* currently locked */
			vp = vp->v_vfsp->vfs_vnodecovered;
			VN_HOLD(vp);
			VN_PUT(cvp);
			VN_LOCKNODE(vp);
			cvp = NULLVP;
			goto checkforroot;
		}
	}

	/*
	 * Perform a lookup in the current directory.
	 *
	 * "vp" is locked at entry to VOP_LOOKUP().
	 * VOP_LOOKUP() returns "tvp" node locked, vp unlocked (still ref'd).
	 */
	error = VOP_LOOKUP(vp, component, &tvp, u.u_cred);
	if (error) {
		/*
		 * On error, if more pathname or if caller was not interested
		 * in the parent directory then hard error.
		 */
		if (pn_pathleft(pnp) || dirvpp == (struct vnode **)0 ||
		    error == EACCES)
			goto bad1;
		if (error = pn_set(pnp, component))
			goto bad1;
		*dirvpp = vp;
		if (compvpp != (struct vnode **)0)
			*compvpp = NULLVP;
		if (comp_bp)
			brelse(comp_bp);
		return (0);
	}
	cvp = tvp;

	/*
	 * At this point, vp is ref'd and unlocked, cvp is ref'd and locked.
	 */

	/*
	 * If we hit a symbolic link and there is more path to be
	 * translated or this operation does not wish to apply
	 * to a link, then place the contents of the link at the
	 * front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK &&
	    (followlink == FOLLOW_LINK || pn_pathleft(pnp))) {
		struct pathname linkpath;

		if (++nlink > maxsymlinks) {
			error = ELOOP;
			goto bad;
		}
		error = getsymlink(cvp, &linkpath);
		if (error)
			goto bad;
		if (pn_pathleft(&linkpath) == 0)
			(void) pn_set(&linkpath, ".");
		error = pn_combine(pnp, &linkpath);	/* linkpath before pn */
		pn_free(&linkpath);
		if (error)
			goto bad;
		VN_PUT(cvp);
		cvp = NULLVP;
		goto begin;
	}

	/*
	 * If this vnode is mounted on, then we
	 * transparently indirect to the vnode which 
	 * is the root of the mounted file system.
	 *
	 * No race with unmount since hold reference to mounted on
	 * vnode until after hold ref to new root.
	 */
	while (cvp->v_vfsmountedhere) {
		VN_UNLOCKNODE(cvp);
		error = VFS_ROOT(cvp->v_vfsmountedhere, &tvp);
		if (error) {
			VN_RELE(cvp);
			cvp = NULLVP;
			goto bad1;
		}
		VN_RELE(cvp);
		cvp = tvp;
	}

	/*
	 * Skip to next component of the pathname.
	 * If no more components, return last directory (if wanted)  and
	 * last component (if wanted).
	 *
	 * vp is ref'd unlocked, cvp is ref'd locked.
	 */
skip:
	if (pn_pathleft(pnp) == 0) {
		if (error = pn_set(pnp, component))
			goto bad;
		if (dirvpp != (struct vnode **)0) {
			*dirvpp = vp;
		} else {
			VN_RELE(vp);
		}
		if (compvpp != (struct vnode **)0)
			*compvpp = cvp;			/* already locked */
		else {
			VN_PUT(cvp);
		}
		if (comp_bp)
			brelse(comp_bp);
		return (0);
	}

	/*
	 * skip over slashes from end of last component
	 */
	pn_skipslash(pnp);

	/*
	 * Searched through another level of directory:
	 * release previous directory handle and save new (result
	 * of lookup) as current directory.
	 */
	VN_RELE(vp);
	vp = cvp;
	cvp = NULLVP;
	goto next;

	/*
	 * Error.  Release vnodes and return.
	 */
bad:
	if (cvp) {
		VN_PUT(cvp);
	} else {
		VN_UNLOCKNODE(vp);
	}
bad1:
	VN_RELE(vp);
	if (comp_bp)
		brelse(comp_bp);
	return (error);
}

/*
 * getsymlink()
 *	Gets symbolic link into pathname.
 */

static int
getsymlink(vp, pnp)
	struct vnode *vp;			/* locked by caller */
	register struct pathname *pnp;
{
	struct iovec aiov;
	struct uio auio;
	register int error;

	aiov.iov_base = pnp->pn_path = pnp->pn_buf = pnp->pn_data;
	aiov.iov_len = auio.uio_resid = PN_BUFSIZE;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIOSEG_KERNEL;
	error = VOP_READLINK(vp, &auio, u.u_cred);
	if (error)
		return (error);
	/*
	 * If it fit in local buffer, done.
	 * Else must allocate full-size path-name and re-read.
	 */
	if (auio.uio_resid) {					/* it fit! */
		pnp->pn_pathlen = PN_BUFSIZE - auio.uio_resid;
		return (0);
	}
	pn_alloc(pnp);
	aiov.iov_base = pnp->pn_path;
	aiov.iov_len = auio.uio_resid = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIOSEG_KERNEL;
	error = VOP_READLINK(vp, &auio, u.u_cred);
	pnp->pn_pathlen = MAXPATHLEN - auio.uio_resid;
	return (error);
}
