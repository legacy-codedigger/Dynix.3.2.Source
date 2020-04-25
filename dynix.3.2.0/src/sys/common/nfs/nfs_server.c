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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: nfs_server.c 1.14 91/03/13 $";
#endif

/*
 * nfs_server.c
 *	NFS server side routines.
 *
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* $Log:	nfs_server.c,v $
 *
 * NFSSRC porting nfssrc2 @(#)nfs_server.c	2.1 86/04/14
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../ufs/fsdir.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/auth_unix.h"
#include "../rpc/svc.h"
#include "../rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../h/mbuf.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"
 
#include "../h/dirent.h"	/* for support of nfs_readdir with odd sizes */

/*
 * rpc service program version range supported
 */
#define	VERSIONMIN	2
#define	VERSIONMAX	2

struct vnode	*fhtovp();
struct file	*getsock();
void		svcerr_progvers();
void		rfs_dispatch();

#ifdef	NFSDEBUG
extern int nfsdebug;
#endif	NFSDEBUG

struct {
	int	ncalls;		/* number of calls received */
	int	nbadcalls;	/* calls that failed */
	int	reqs[32];	/* count for each request */
} svstat;

/*
 * NFS Server system call.
 * Does all of the work of running a NFS server.
 * sock is the fd of an open UDP socket.
 */
nfs_svc()
{
	struct a {
		int     sock;
	} *uap = (struct a *)u.u_ap;
	struct vnode	*rdir;
	struct vnode	*cdir;
	struct socket   *so;
	struct file	*fp;
	SVCXPRT *xprt;
	u_long vers;
 
	fp = getsock(uap->sock);
	if (fp == 0) {
		u.u_error = EBADF;
		return;
	}
	so = (struct socket *)fp->f_data;
	 
	/*
	 * Be sure that rdir (the server's root vnode) is set.
	 * Save the current directory and restore it again when
	 * the call terminates.  rfs_lookup uses u.u_cdir for lookupname.
	 *
	 * Be sure that server lives in UCB universe so that CSYMLINKs
	 * are interpreted as normal SYMLINKs.
	 */
	rdir = u.u_rdir;
	cdir = u.u_cdir;
	if (u.u_rdir == NULLVP) {
		u.u_rdir = u.u_cdir;
	}
	u.u_universe = U_UCB;

	xprt = svckudp_create(so, NFS_PORT);
	for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
		(void) svc_register(xprt, NFS_PROGRAM, vers, rfs_dispatch,
				    FALSE);
	}
	if (setjmp(&u.u_qsave)) {
		for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
			svc_unregister(NFS_PROGRAM, vers);
		}
		SVC_DESTROY(xprt);
		u.u_error = EINTR;
	} else {
		svc_run(xprt);  /* never returns */
	}
	u.u_rdir = rdir;
	u.u_cdir = cdir;
}


/*
 * Get file handle system call.
 * Takes open file descriptor and returns a file handle for it.
 */
nfs_getfh()
{
	register struct a {
		int	fdes;
		fhandle_t	*fhp;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct vnode *vp;
	fhandle_t fh;

	if (!suser()) {
		return;
	}
	fp = getf(uap->fdes);
	if (fp == NULL) {
		return;
	}
	vp = (struct vnode *)fp->f_data;
	u.u_error = makefh(&fh, vp);
	if (!u.u_error) {
		u.u_error =
		    copyout((caddr_t)&fh, (caddr_t)uap->fhp, sizeof(fh));
	}
}

	
/*
 * These are the interface routines for the server side of the
 * Networked File System.  See the NFS protocol specification
 * for a description of this interface.
 */


/*
 * Get file attributes.
 * Returns the current attributes of the file with the given fhandle.
 */
static int
rfs_getattr(fhp, ns)
	fhandle_t *fhp;
	register struct nfsattrstat *ns;
{
	register struct vnode *vp;
	int error;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_getattr fh %o %d\n",
		fhp->fh_fsid, fhp->fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(fhp);
	if (vp == NULLVP) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	error = VOP_GETATTR(vp, &va, u.u_cred);
	VN_PUT(vp);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
	}
	ns->ns_status = puterrno(error);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_getattr: returning %d\n", error);
#endif	NFSDEBUG
}

/*
 * Set file attributes.
 * Sets the attributes of the file with the given fhandle.  Returns
 * the new attributes.
 */
static int
rfs_setattr(args, ns)
	struct nfssaargs *args;
	register struct nfsattrstat *ns;
{
	register struct vnode *vp;
	int error;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_setattr fh %o %d\n",
		args->saa_fh.fh_fsid, args->saa_fh.fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(&args->saa_fh);
	if (vp == NULLVP) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	sattr_to_vattr(&args->saa_sa, &va);
	error = VOP_SETATTR(vp, &va, u.u_cred);
	if (!error) {
		error = VOP_GETATTR(vp, &va, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &ns->ns_attr);
		}
	}
	VN_PUT(vp);
	ns->ns_status = puterrno(error);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_setattr: returning %d\n", error);
#endif	NFSDEBUG
}

/*
 * Directory lookup.
 * Returns an fhandle and file attributes for file name in a directory.
 */
static int
rfs_lookup(da, dr)
	struct nfsdiropargs *da;
	register struct  nfsdiropres *dr;
{
	register struct vnode *dvp;
	int error;
	struct vnode *vp;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_lookup %s fh %o %d\n",
		da->da_name, da->da_fhandle.fh_fsid, da->da_fhandle.fh_fno);
#endif	NFSDEBUG

	dvp = fhtovp(&da->da_fhandle);
	if (dvp == NULLVP) {
		dr->dr_status = NFSERR_STALE;
		return;
	}

	/*
	 * do lookup.
	 */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, u.u_cred);
	if (error) {
		vp = NULLVP;
	} else {
		error = VOP_GETATTR(vp, &va, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp);
		}
	}
	if (vp) {
		VN_PUT(vp);
	}
	VN_RELE(dvp);
	dr->dr_status = puterrno(error);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_lookup: returning %d\n", error);
#endif	NFSDEBUG
}

/*
 * Read symbolic link.
 * Returns the string in the symbolic link at the given fhandle.
 */
static int
rfs_readlink(fhp, rl)
	fhandle_t *fhp;
	register struct nfsrdlnres *rl;
{
	struct vnode *vp;
	int error;
	struct iovec iov;
	struct uio uio;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_readlink fh %o %d\n",
		fhp->fh_fsid, fhp->fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(fhp);
	if (vp == NULLVP) {
		rl->rl_status = NFSERR_STALE;
		return;
	}

	/*
	 * Allocate data for pathname.  This will be freed by rfs_rlfree.
	 */
	rl->rl_data = kmem_alloc((u_int)MAXPATHLEN);

	/*
	 * Set up io vector to read sym link data
	 */
	iov.iov_base = rl->rl_data;
	iov.iov_len = NFS_MAXPATHLEN;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIOSEG_KERNEL;
	uio.uio_offset = 0;
	uio.uio_resid = NFS_MAXPATHLEN;

	/*
	 * read link
	 */
	error = VOP_READLINK(vp, &uio, u.u_cred);
	VN_PUT(vp);

	/*
	 * Clean up
	 */
	if (error) {	
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
		rl->rl_count = 0;
		rl->rl_data = NULL;
	} else {
		rl->rl_count = NFS_MAXPATHLEN - uio.uio_resid;
	}
	rl->rl_status = puterrno(error);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_readlink: returning '%s' %d\n",
		rl->rl_data, error);
#endif	NFSDEBUG
}

/*
 * Free data allocated by rfs_readlink
 */
/*ARGSUSED*/
static
rfs_rlfree(rl, senderr)
	struct nfsrdlnres *rl;
	int senderr;
{
	if (rl->rl_data) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN); 
	}
}

/*
 * Read data.
 * Returns some data read from the file at the given fhandle.
 */
static int
rfs_read(ra, rr)
	struct nfsreadargs *ra;
	register struct nfsrdresult *rr;
{
	struct vnode *vp;
	int error;
	int offset, fsbsize;
	struct buf *bp;
	struct vattr va;
	struct iovec iov;
	struct uio uio;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_read %d from fh %o %d\n",
		ra->ra_count, ra->ra_fhandle.fh_fsid, ra->ra_fhandle.fh_fno);
#endif	NFSDEBUG

	rr->rr_data = NULL;
	rr->rr_count = 0;
	vp = fhtovp(&ra->ra_fhandle);
	if (vp == NULLVP) {
		rr->rr_status = NFSERR_STALE;
		return;
	}
	if (vp->v_type != VREG) {
		error = EISDIR;
		goto bad;
	}
	error = VOP_GETATTR(vp, &va, u.u_cred);
	if (error) {
		goto bad;
	}
	/*
	 * This is a kludge to allow reading of files created
	 * with no read permission.  The owner of the file
	 * is always allowed to read it.
	 */
	if (u.u_uid != va.va_uid) {
		error = VOP_ACCESS(vp, VREAD, u.u_cred);
		if (error) {
			/*
			 * Exec is the same as read over the net because
			 * of demand loading.
			 */
			error = VOP_ACCESS(vp, VEXEC, u.u_cred);
		}
		if (error) {
			goto bad;
		}
	}

	/*
	 * Check whether we can do this with bread, which would
	 * save the copy through the uio.
	 */
	fsbsize = vp->v_vfsp->vfs_bsize;
	offset = ra->ra_offset % fsbsize;
	if (offset + ra->ra_count <= fsbsize) {
		if (ra->ra_offset >= va.va_size) {
			rr->rr_count = 0;
			vattr_to_nattr(&va, &rr->rr_attr);
			goto done;
		}
		error = VOP_BREAD(vp, ra->ra_offset / fsbsize, &bp);
		if (error == 0) {
			rr->rr_data = bp->b_un.b_addr + offset;
			rr->rr_count = min((u_int)(va.va_size - ra->ra_offset),
					    (u_int)ra->ra_count);
			rr->rr_bp = bp;
			rr->rr_vp = vp;
			/*
			 * Must hold so that eventual VOP_BRELSE() in
			 * rrokfree() has correct vp.
			 */
			VN_HOLD(vp);
			vattr_to_nattr(&va, &rr->rr_attr);
			goto done;
		} else {
			printf("nfs read: failed, errno %d\n", error);
		}
	}
	rr->rr_bp = (struct buf *)0;
			
	/*
	 * Allocate space for data.  This will be freed by xdr_rdresult
	 * when it is called with x_op = XDR_FREE.
	 */
	rr->rr_data = kmem_alloc((u_int)ra->ra_count);

	/*
	 * Set up io vector
	 */
	iov.iov_base = rr->rr_data;
	iov.iov_len = ra->ra_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIOSEG_KERNEL;
	uio.uio_offset = ra->ra_offset;
	uio.uio_resid = ra->ra_count;
	/*
	 * for now we assume no append mode and ignore
	 * totcount (read ahead)
	 */
	error = VOP_RDWR(vp, &uio, UIO_READ, IO_SYNC, u.u_cred);
	if (error) {
		goto bad;
	}
	vattr_to_nattr(&va, &rr->rr_attr);
	rr->rr_count = ra->ra_count - uio.uio_resid;
	/*
	 * free the unused part of the data allocated
	 */
	if (uio.uio_resid) {
		kmem_free((caddr_t)(rr->rr_data + rr->rr_count),
				(u_int)uio.uio_resid);
	}
bad:
	if (error && rr->rr_data != NULL) {
		kmem_free((caddr_t)rr->rr_data, (u_int)ra->ra_count);
		rr->rr_data = NULL;
		rr->rr_count = 0;
	}
done:
	VN_PUT(vp);
	rr->rr_status = puterrno(error);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_read returning %d, count = %d\n",
		error, rr->rr_count);
#endif	NFSDEBUG
}

/*
 * Free data allocated by rfs_read.
 * If error on send, release buffer and vnode if necessary.
 */
static
rfs_rdfree(rr, senderr)
	struct nfsrdresult *rr;
	int senderr;
{
	/*
	 * If senderr and rr_bp is set then release resources.
	 */
	if (senderr && rr->rr_bp) {
		VOP_BRELSE(rr->rr_vp, rr->rr_bp);
		VN_RELE(rr->rr_vp);
		return;
	}
	if (rr->rr_bp == 0 && rr->rr_data) {
		kmem_free((caddr_t)rr->rr_data, (u_int)rr->rr_count);
	}
}

/*
 * Write data to file.
 * Returns attributes of a file after writing some data to it.
 */
static int
rfs_write(wa, ns)
	struct nfswriteargs *wa;
	struct nfsattrstat *ns;
{
	register int error;
	register struct vnode *vp;
	struct vattr va;
	struct iovec iov;
	struct uio uio;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_write: %d bytes fh %o %d\n",
		wa->wa_count, wa->wa_fhandle.fh_fsid, wa->wa_fhandle.fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(&wa->wa_fhandle);
	if (vp == NULLVP) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	if (vp->v_type != VREG) {
		error = EISDIR;
	} else {
		error = VOP_GETATTR(vp, &va, u.u_cred);
	}
	if (!error) {
		if (u.u_uid != va.va_uid) {
			/*
			 * This is a kludge to allow writes of files created
			 * with read only permission.  The owner of the file
			 * is always allowed to write it.
			 */
			error = VOP_ACCESS(vp, VWRITE, u.u_cred);
		}
		if (!error) {
			iov.iov_base = wa->wa_data;
			iov.iov_len = wa->wa_count;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_segflg = UIOSEG_KERNEL;
			uio.uio_offset = wa->wa_offset;
			uio.uio_resid = wa->wa_count;
			/*
			 * for now we assume no append mode
			 */
			error = VOP_RDWR(vp, &uio, UIO_WRITE, IO_SYNC, u.u_cred);
		}
	}
	if (!error) {
		/*
		 * Get attributes again so we send the latest mod
		 * time to the client side for his cache.
		 */
		error = VOP_GETATTR(vp, &va, u.u_cred);
	} else {
		printf("nfs write: failed, errno %d fh 0x%x %d\n",
			error, wa->wa_fhandle.fh_fsid, wa->wa_fhandle.fh_fno);
	}
	VN_PUT(vp);
	ns->ns_status = puterrno(error);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_write: returning %d\n", error);
#endif	NFSDEBUG
}

/*
 * Create a file.
 * Creates a file with given attributes and returns those attributes
 * and an fhandle for the new file.
 */
static int
rfs_create(args, dr, req)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct svc_req *req;
{
	register struct vnode *dvp;
	register int error;
	struct vnode *vp;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_create: %s dfh %o %d\n",
		args->ca_da.da_name, args->ca_da.da_fhandle.fh_fsid,
		args->ca_da.da_fhandle.fh_fno);
#endif	NFSDEBUG

startover:
	sattr_to_vattr(&args->ca_sa, &va);
	va.va_type = VREG;
	/*
	 * XXX Should get exclusive flag and use it.
	 */
	dvp = fhtovp(&args->ca_da.da_fhandle);
	if (dvp == NULLVP) {
		dr->dr_status = NFSERR_STALE;
		return;
	}
	VN_UNLOCKNODE(dvp);	/* unlock dvp for VOP_CREATE */
	error = VOP_CREATE(dvp, args->ca_da.da_name, &va, NONEXCL, VWRITE, &vp,
				u.u_cred);
	if (error == EACCES) {
		/*
		 * check for dup request
		 */
		if (svckudp_dup(req)) { 
			VN_LOCKNODE(dvp);
			error = VOP_LOOKUP(dvp, args->ca_da.da_name, &vp,
						u.u_cred);
			if (error == ENOENT) {
				/*
				 * if the file is removed between the time the
				 * create was attempted and the lookup
				 * is done, then lets try again.
				 */
				VN_RELE(dvp);
				goto startover;
			}
		}
	}
	if (!error) {
		error = VOP_GETATTR(vp, &va, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp);
		}
		VN_PUT(vp);
	}
	VN_RELE(dvp);
	dr->dr_status = puterrno(error);
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_create: returning %d\n", error);
#endif	NFSDEBUG
}

/*
 * Remove a file.
 * Remove named file from parent directory.
 */
static int
rfs_remove(da, status, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct svc_req *req;
{
	register struct vnode *vp;
	struct vnode *tvp = NULLVP;
	int error;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_remove %s dfh %o %d\n",
		da->da_name, da->da_fhandle.fh_fsid, da->da_fhandle.fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(&da->da_fhandle);
	if (vp == NULLVP) {
		*status = NFSERR_STALE;
		return;
	}
	error = VOP_LOOKUP(vp, da->da_name, &tvp, u.u_cred);
	if (!error) {
		/*
		 * Don't allow unlink of directories by a NFS client.
		 * Some systems allow unlink of dirs, Dynix doesn't.
		 * It wants them mapped to rmdir operations.
		 */
		if (tvp->v_type == VDIR) {
			VN_PUT(tvp);
			*status = NFSERR_PERM;
			VN_RELE(vp);
			return;
		}
		VN_PUT(tvp);
	}
	error = VOP_REMOVE(vp, da->da_name, u.u_cred);
	if (error == ENOENT) {
		/*
		 * check for dup request
		 */
		if (svckudp_dup(req)) { 
			error = 0;
		}
	}
	VN_RELE(vp);
	*status = puterrno(error);
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_remove: %s returning %d\n",
		da->da_name, error);
#endif	NFSDEBUG
}

/*
 * rename a file
 * Give a file (from) a new name (to).
 */
static int
rfs_rename(args, status, req)
	struct nfsrnmargs *args;
	enum nfsstat *status; 
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_rename %s ffh %o %d -> %s tfh %o %d\n",
		args->rna_from.da_name,
		args->rna_from.da_fhandle.fh_fsid,
		args->rna_from.da_fhandle.fh_fno,
		args->rna_to.da_name,
		args->rna_to.da_fhandle.fh_fsid,
		args->rna_to.da_fhandle.fh_fno);
#endif	NFSDEBUG

	fromvp = fhtovp(&args->rna_from.da_fhandle);
	if (fromvp == NULLVP) {
		*status = NFSERR_STALE;
		return;
	}
	VN_UNLOCKNODE(fromvp);
	tovp = fhtovp(&args->rna_to.da_fhandle);
	if (tovp == NULLVP) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		return;
	}
	VN_UNLOCKNODE(tovp);
	error = VOP_RENAME(fromvp, args->rna_from.da_name, tovp,
				args->rna_to.da_name, u.u_cred);
	if (error == ENOENT) {
		if (svckudp_dup(req)) {
			error = 0;
		}
	}
	VN_RELE(tovp);
	VN_RELE(fromvp);
	*status = puterrno(error); 
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_rename: returning %d\n", error);
#endif	NFSDEBUG
} 

/*
 * Link to a file.
 * Create a file (to) which is a hard link to the given file (from).
 */
static int
rfs_link(args, status, req) 
	struct nfslinkargs *args;
	enum nfsstat *status;  
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_link ffh %o %d -> %s tfh %o %d\n",
		args->la_from.fh_fsid, args->la_from.fh_fno,
		args->la_to.da_name,
		args->la_to.da_fhandle.fh_fsid, args->la_to.da_fhandle.fh_fno);
#endif	NFSDEBUG

	fromvp = fhtovp(&args->la_from);
	if (fromvp == NULLVP) {
		*status = NFSERR_STALE;
		return;
	}
	VN_UNLOCKNODE(fromvp);
	tovp = fhtovp(&args->la_to.da_fhandle);
	if (tovp == NULLVP) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		return;
	}
	VN_UNLOCKNODE(tovp);
	error = VOP_LINK(fromvp, tovp, args->la_to.da_name, u.u_cred);
	if (error == EEXIST) {
		/*
		 * check for dup request
		 */
		if (svckudp_dup(req)) { 
			error = 0;
		}
	}
	VN_RELE(fromvp);
	VN_RELE(tovp);
	*status = puterrno(error);
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_link: returning %d\n", error);
#endif	NFSDEBUG
} 
 
/*
 * Symbolicly link to a file.
 * Create a file (to) with the given attributes which is a symbolic link
 * to the given path name (to).
 */
static int
rfs_symlink(args, status, req) 
	struct nfsslargs *args;
	enum nfsstat *status;   
	struct svc_req *req;
{		  
	register struct vnode *vp;
	int error;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_symlink %s ffh %o %d -> %s\n",
		args->sla_from.da_name,
		args->sla_from.da_fhandle.fh_fsid,
		args->sla_from.da_fhandle.fh_fno,
		args->sla_tnm);
#endif	NFSDEBUG

	sattr_to_vattr(&args->sla_sa, &va);
	va.va_type = VLNK;
	vp = fhtovp(&args->sla_from.da_fhandle);
	if (vp == NULLVP) {
		*status = NFSERR_STALE;
		return;
	}
	VN_UNLOCKNODE(vp);
	error = VOP_SYMLINK(vp, args->sla_from.da_name, &va, args->sla_tnm,
				u.u_cred);
	if (error == EEXIST) {
		/*
		 * check for dup request
		 */
		if (svckudp_dup(req)) {
			error = 0;
		}
	}
	VN_RELE(vp);
	*status = puterrno(error);
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_symlink: returning %d\n", error);
#endif	NFSDEBUG
}  
  
/*
 * Make a directory.
 * Create a directory with the given name, parent directory, and attributes.
 * Returns a file handle and attributes for the new directory.
 */
static int
rfs_mkdir(args, dr, req)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct svc_req *req;
{
	register struct vnode *vp;
	int error;
	struct vnode *dvp;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_mkdir %s fh %o %d\n",
		args->ca_da.da_name, args->ca_da.da_fhandle.fh_fsid,
		args->ca_da.da_fhandle.fh_fno);
#endif	NFSDEBUG

tryagain:
	sattr_to_vattr(&args->ca_sa, &va);
	va.va_type = VDIR;
	/*
	 * Should get exclusive flag and pass it on here
	 */
	vp = fhtovp(&args->ca_da.da_fhandle);
	if (vp == NULLVP) {
		dr->dr_status = NFSERR_STALE;
		return;
	}
	VN_UNLOCKNODE(vp);
	error = VOP_MKDIR(vp, args->ca_da.da_name, &va, &dvp, u.u_cred);
	if (error == EEXIST) {
		/*
		 * check for dup request
		 */
		if (svckudp_dup(req)) {
			VN_LOCKNODE(vp);
			error = VOP_LOOKUP(vp, args->ca_da.da_name, &dvp,
						u.u_cred);
			if (error == ENOENT) {
				/*
				 * if the directory is removed between the
				 * time the mkdir was attempted and the lookup
				 * is done, then lets try again.
				 */
				VN_RELE(vp);
				goto tryagain;
			}
			if (!error) {
				error = VOP_GETATTR(dvp, &va, u.u_cred);
				/*
				 * Should never error on VOP_GETATTR since
				 * we hold the vnode locked...
				 */
				if (error)
					VN_PUT(dvp);
			}
		}
	}
	if (!error) {
		vattr_to_nattr(&va, &dr->dr_attr);
		error = makefh(&dr->dr_fhandle, dvp);
		VN_PUT(dvp);
	}
	VN_RELE(vp);
	dr->dr_status = puterrno(error);
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_mkdir: returning %d\n", error);
#endif	NFSDEBUG
}

/*
 * Remove a directory.
 * Remove the given directory name from the given parent directory.
 */
static int
rfs_rmdir(da, status, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct svc_req *req;
{
	register struct vnode *vp;
	int error;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_rmdir %s fh %o %d\n",
		da->da_name, da->da_fhandle.fh_fsid, da->da_fhandle.fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(&da->da_fhandle);
	if (vp == NULLVP) {
		*status = NFSERR_STALE;
		return;
	}
	VN_UNLOCKNODE(vp);	/* VOP_RMDIR wants unlocked vp */
	error = VOP_RMDIR(vp, da->da_name, u.u_cred);
	if (error == ENOENT) {
		/*
		 * check for dup request
		 */
		if (svckudp_dup(req)) { 
			error = 0;
		}
	}
	VN_RELE(vp);
	*status = puterrno(error);
	if (!error) {
		svckudp_dupsave(req);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rmdir returning %d\n", error);
#endif	NFSDEBUG
}

static int
rfs_readdir(rda, rd)
	struct nfsrddirargs *rda;
	register struct nfsrddirres  *rd;
{
	register struct vnode *vp;
	struct direct *dp;
	int error;
	u_long offset;
	u_long skipped;
	u_long bufsize;
	struct iovec iov;
	struct uio uio;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_readdir fh %o %d count %d\n",
		rda->rda_fh.fh_fsid, rda->rda_fh.fh_fno, rda->rda_count);
#endif	NFSDEBUG

	vp = fhtovp(&rda->rda_fh);
	if (vp == NULLVP) {
		rd->rd_status = NFSERR_STALE;
		return;
	}
	/*
	 * check read access of dir.  we have to do this here because
	 * the opendir doesn't go over the wire.
	 */
	error = VOP_ACCESS(vp, VREAD, u.u_cred);
	if (error) {
		goto bad;
	}

nxtblk:
	bufsize = MAX(DIRBLKSIZ, rda->rda_count);
	/*
	 * Allocate data for entries.  This will be freed by rfs_rdfree.
	 */
	rd->rd_entries = (struct direct *)kmem_alloc((u_int)bufsize);
	rd->rd_bufsize = bufsize;

	rd->rd_offset = offset = rda->rda_offset & ~(DIRBLKSIZ -1);

	/*
	 * Set up io vector to read directory data
	 */
	iov.iov_base = (caddr_t)rd->rd_entries;
	iov.iov_len = bufsize;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIOSEG_KERNEL;
	uio.uio_offset = offset;
	uio.uio_resid = bufsize;

	/*
	 * read directory
	 */
	error = VOP_READDIR(vp, &uio, u.u_cred);

	/*
	 * Clean up
	 */
	if (error) {	
		kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
		rd->rd_bufsize = 0;
		rd->rd_size = 0;
		goto bad;
	}
 
	/*
	 * set size and eof
	 *
	 * One is tempted to assume directory EOF if uio_resid != 0.
	 * However, VOP_READDIR() may return less than the requested
	 * size for reasons other than EOF, such as rounding down
	 * to disk block sizes.  Therefore, we indicate EOF to the
	 * client only if VOP_READDIR() was unable to return ANY data.
	 *
	 * Note that this approach requires a client to make successive
	 * readdir requests until an empty response is generated; this
	 * is always one more readdir request than would theoretically
	 * be necessary.  A better approach might be to change VOP_READDIR()
	 * to return a separate indication of whether EOF was reached or not.
	 *
	 * Also note that clients typically mitigate this problem by not
	 * making a readdir request with an offset beyond the size of
	 * the directory.
	 */
	if (uio.uio_resid == bufsize) {
		rd->rd_size = 0;
		rd->rd_eof = TRUE;
	} else {
		rd->rd_size = bufsize - uio.uio_resid;
		rd->rd_eof = FALSE;
	}

	/*
	 * if client request was in the middle of a block
	 * or block begins with null entries skip entries
	 * til we are on a valid entry >= client's requested
	 * offset.
	 */
	dp = rd->rd_entries;
	skipped = 0;
	while ((skipped < rd->rd_size) &&
	       ((offset + dp->d_reclen <= rda->rda_offset) ||
	       (dp->d_ino == 0))) {
		skipped += dp->d_reclen;
		offset += dp->d_reclen;
		dp = (struct direct *)((int)dp + dp->d_reclen);
	}
	/*
	 * Reset entries pointer and free space we are skipping
	 */
	if (skipped) {
		rd->rd_size -= skipped;
		rd->rd_bufsize -= skipped;
		rd->rd_offset = offset;
		kmem_free((caddr_t)rd->rd_entries, (u_int)skipped);
		rd->rd_entries = (struct direct *)
					((int)rd->rd_entries + skipped);
		if (rd->rd_size == 0 && !rd->rd_eof) {
			/*
			 * we have skipped a whole block, reset offset
			 * and read another block (unless eof)
			 */
			rda->rda_offset = rd->rd_offset;
			if (rd->rd_bufsize) {
				kmem_free((caddr_t)rd->rd_entries,
					(u_int)rd->rd_bufsize);
			}
			goto nxtblk;
		}
	}

	/*
	 * Make sure we don't pass back more information than
	 * the client requested.
	 */
	if (rd->rd_size > rda->rda_count) {
		ulong   dirent_length, new_end;

		dirent_length = new_end = 0;
		dp = rd->rd_entries;
		rd->rd_eof = 0;
		while (1) {
			if ((dirent_length + DNTSIZ(dp->d_namlen))
					> rda->rda_count) {
				break;
			}
			dirent_length += DNTSIZ(dp->d_namlen);
			new_end += dp->d_reclen;
			dp = (struct direct *)((int)dp + dp->d_reclen);
		}
		if (new_end == 0) {
			error = EINVAL;
			kmem_free((caddr_t)rd->rd_entries,
			(u_int)rd->rd_bufsize);
			rd->rd_bufsize = 0;
			rd->rd_size = 0;
			rd->rd_offset = 0;
		} else {
			rd->rd_size = new_end;
		}
	}

bad:
	VN_PUT(vp);
	rd->rd_status = puterrno(error);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_readdir: returning %d\n", error);
#endif	NFSDEBUG
}

/*ARGSUSED*/
static
rfs_rddirfree(rd, senderr)
	struct nfsrddirres *rd;
	int senderr;
{

	/*
	 * Note: If called after bad rfs_readdir, then kmem_free
	 * will simply return since rd->rd_bufsize = 0.
	 */
	kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
}

static int
rfs_statfs(fh, fs)
	fhandle_t *fh;
	register struct nfsstatfs *fs;
{
	register struct vnode *vp;
	int error;
	struct statfs sb;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "rfs_statfs fh %o %d\n", fh->fh_fsid, fh->fh_fno);
#endif	NFSDEBUG

	vp = fhtovp(fh);
	if (vp == NULLVP) {
		fs->fs_status = NFSERR_STALE;
		return;
	}
	error = VFS_STATFS(vp->v_vfsp, &sb);
	VN_PUT(vp);
	fs->fs_status = puterrno(error);
	if (!error) {
		fs->fs_tsize = NFS_MAXDATA;	/* depends on ether capability*/
		fs->fs_bsize = sb.f_bsize;
		fs->fs_blocks = sb.f_blocks;
		fs->fs_bfree = sb.f_bfree;
		fs->fs_bavail = sb.f_bavail;
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "rfs_statfs returning %d\n", error);
#endif	NFSDEBUG
}

/*ARGSUSED*/
rfs_null(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* do nothing */
	return (0);
}

/*ARGSUSED*/
rfs_error(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	return (EOPNOTSUPP);
}

int
nullfree()
{
}

/*
 * rfs dispatch table
 * Indexed by version,proc
 */

struct rfsdisp {
	int	  (*dis_proc)();	/* proc to call */
	xdrproc_t dis_xdrargs;		/* xdr routine to get args */
	int	  dis_argsz;		/* sizeof args */
	xdrproc_t dis_xdrres;		/* xdr routine to put results */
	int	  dis_ressz;		/* size of results */
	int	  (*dis_resfree)();	/* frees space allocated by proc */
} rfsdisptab[][RFS_NPROC]  = {
	{
	/*
	 * VERSION 2
	 * Changed rddirres to have eof at end instead of beginning
	 */
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof(fhandle_t),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_saargs, sizeof(struct nfssaargs),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof(struct nfsdiropargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof(fhandle_t),
	    xdr_rdlnres, sizeof(struct nfsrdlnres), rfs_rlfree},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof(struct nfsreadargs),
	    xdr_rdresult, sizeof(struct nfsrdresult), rfs_rdfree},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof(struct nfswriteargs),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_creatargs, sizeof(struct nfscreatargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof(struct nfsdiropargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof(struct nfsrnmargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof(struct nfslinkargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_slargs, sizeof(struct nfsslargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_creatargs, sizeof(struct nfscreatargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof(struct nfsdiropargs), 
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof(struct nfsrddirargs),
	    xdr_putrddirres, sizeof(struct nfsrddirres), rfs_rddirfree},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof(fhandle_t),
	    xdr_statfs, sizeof(struct nfsstatfs), nullfree},
	}
};

void
rfs_dispatch(req, xprt)
	struct svc_req *req;
	register SVCXPRT *xprt;
{
	register struct rfsdisp *disp;
	register int *gp;
	register int which;
	int vers;
	caddr_t	*args = NULL;
	caddr_t	*res = NULL;
	struct authunix_parms *aup;
	struct ucred *tmpcr;
	struct ucred *newcr = NULL;
	int error = 0;
	int senderr = 0;
	union nfs_rfsargs nfs_rfsargs;
	union nfs_rfsrslts nfs_rfsrslts;
	extern int nobody;
	extern int nfs_portmon;

	svstat.ncalls++;
	which = req->rq_proc;
	if (which < 0 || which >= RFS_NPROC) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 2,
			"rfs_dispatch: bad proc %d\n", which);
#endif	NFSDEBUG
		svcerr_noproc(req->rq_xprt);
		error++;
		goto done;
	}
	vers = req->rq_vers;
	if (vers < VERSIONMIN || vers > VERSIONMAX) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 2,
			"rfs_dispatch: bad vers %d low %d high %d\n",
			vers, VERSIONMIN, VERSIONMAX);
#endif	NFSDEBUG
		svcerr_progvers(req->rq_xprt, (u_long)VERSIONMIN,
				(u_long)VERSIONMAX);
		error++;
		goto done;
	}
	vers -= VERSIONMIN;
	disp = &rfsdisptab[vers][which];

	/*
	 * Clean up as if a system call just started
	 */
	u.u_error = 0;

	/*
	 * Allocate args struct and deserialize into it.
	 */
	if (disp->dis_argsz) {
		args = (caddr_t *)&nfs_rfsargs;
		bzero((caddr_t)args, (u_int)disp->dis_argsz);
	}
	if (! SVC_GETARGS(xprt, disp->dis_xdrargs, args)) {
		svcerr_decode(xprt);
		error++;
		goto done;
	}

	/*
	 * Check for unix style credentials
	 */
	if (req->rq_cred.oa_flavor != AUTH_UNIX && which != RFS_NULL) {
		svcerr_weakauth(xprt);
		error++;
		goto done;
	}

	/*
	 * Set uid, gid, and gids to auth params
	 */
	if (which != RFS_NULL) {
		if (nfs_portmon) {
			/*
			* Check for privileged port number
			*/
			static count = 0;
			if (ntohs(xprt->xp_raddr.sin_port) >= IPPORT_RESERVED) {
				svcerr_weakauth(xprt);
				if (count == 0) {
					printf("NFS request from unprivileged port, source IP address = %u.%u.%u.%u\n",
						xprt->xp_raddr.sin_addr.s_net,
						xprt->xp_raddr.sin_addr.s_host,
						xprt->xp_raddr.sin_addr.s_lh,
						xprt->xp_raddr.sin_addr.s_impno);
				}
				count++;
				count %= 256;
				error++;
				goto done;
			}
		}
		aup = (struct authunix_parms *)req->rq_clntcred;
		newcr = crget();
		newcr->cr_ref = 1;
		init_lock(&newcr->cr_lock, G_CRED);
		if (aup->aup_uid == 0) {
			/*
			 * root over the net becomes other on the server (uid -2)
			 */
			newcr->cr_uid = nobody;
		} else {
			newcr->cr_uid = aup->aup_uid;
		}
		newcr->cr_gid = aup->aup_gid;
		bcopy((caddr_t)aup->aup_gids, (caddr_t)newcr->cr_groups,
		    aup->aup_len * sizeof(newcr->cr_groups[0]));
		for (gp = &newcr->cr_groups[aup->aup_len];
		     gp < &newcr->cr_groups[NGROUPS];
		     gp++) {
			*gp = NOGROUP;
		}
		tmpcr = u.u_cred;
		u.u_cred = newcr;
	}

	/*
	 * Allocate results struct.
	 */
	if (disp->dis_ressz) {
		res = (caddr_t *)&nfs_rfsrslts;
		bzero((caddr_t)res, (u_int)disp->dis_ressz);
	}

	svstat.reqs[which]++;

	/*
	 * Call service routine with arg struct and results struct
	 */
	(*disp->dis_proc)(args, res, req);

done:
	/*
	 * Free arguments struct
	 */
	if (!SVC_FREEARGS(xprt, disp->dis_xdrargs, args) ) {
		error++;
		if (res != NULL)
			senderr++;
	}

	/*
	 * Serialize and send results struct
	 */
	if (!error) {
		if (!svc_sendreply(xprt, disp->dis_xdrres, (caddr_t)res)) {
			error++;
			senderr++;
		}
	}

	/*
	 * Free results struct
	 */
	if (res != NULL) {
		if (disp->dis_resfree != nullfree) {
			(*disp->dis_resfree)(res, senderr);
		}
	}
	/*
	 * restore original credentials
	 */
	if (newcr) {
		u.u_cred = tmpcr;
		crfree(newcr);
	}
	svstat.nbadcalls += error;
}
#endif	NFS
