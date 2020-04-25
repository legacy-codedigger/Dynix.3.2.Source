/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
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
static	char	rcsid[] = "$Header: nfs_vnodeops.c 1.19 90/09/02 $";
#endif	lint

/*
 * nfs_vnodeops.c
 *
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* $Log:	nfs_vnodeops.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../netinet/in.h"
#include "../h/proc.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"

#define	RN_LOCKCRED(rp)		p_lock(&(rp)->r_credlock, SPLFS)
#define	RN_UNLOCKCRED(rp, ipl)	v_lock(&(rp)->r_credlock, ipl)

/*
 * Purge dnlc if stale file handle. Locked and unlocked versions.
 */
#define CHECK_STALE_FH(errno, vp) \
	if ((errno) == ESTALE && (vp)->v_type == VDIR) { \
		VN_LOCKNODE(vp); \
		dnlc_purge_vp(vp); \
		VN_UNLOCKNODE(vp); \
	}
#define L_CHECK_STALE_FH(errno, vp) \
	 if ((errno) == ESTALE && (vp)->v_type == VDIR) { dnlc_purge_vp(vp); }

/*
 * Invalidate cached attributes.
 */
#define NFSATTR_INVAL(vp)	(vtor(vp)->r_nfsattrtime.tv_sec = 0)

#ifdef	NFSDEBUG
extern int nfsdebug;
#endif	NFSDEBUG

struct vnode *makenfsnode();

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system.  These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 *
 * Note on attribute and directory name lookup cacheing: it is desired that
 * all operations on a given client machine come out the same with or without
 * the cache. This is the same property we have with the disk buffer cache.
 * In order to guarantee this, we serialize all operations on a given
 * directory, by using rlock and runlock around rfscalls to the server.
 * This way, we cannot get into races with ourself that would cause invalid
 * information in the cache. Other clients (or the server itself) can cause
 * our cached information to become invalid, the same as with data buffers.
 *
 * Also, if we do detect a stale fhandle, we purge the directory cache
 * relative to that vnode.  This way, the user won't get burned by the
 * cache repeatedly.
 */

static int nfs_open();
static int nfs_close();
static int nfs_rdwr();
static int nfs_ioctl();
static int nfs_select();
static int nfs_getattr();
static int nfs_setattr();
static int nfs_access();
static int nfs_lookup();
static int nfs_create();
static int nfs_remove();
static int nfs_link();
static int nfs_rename();
static int nfs_mkdir();
static int nfs_rmdir();
static int nfs_readdir();
static int nfs_symlink();
static int nfs_csymlink();
static int nfs_readlink();
static int nfs_readclink();
static int nfs_fsync();
static int nfs_inactive();
static int nfs_bmap();
static int nfs_strategy();
static int nfs_badop();
static int nfs_minphys();

struct vnodeops nfs_vnodeops = {
	nfs_open,
	nfs_close,
	nfs_rdwr,
	nfs_ioctl,
	nfs_select,
	nfs_getattr,
	nfs_setattr,
	nfs_access,
	nfs_lookup,
	nfs_create,
	nfs_remove,
	nfs_link,
	nfs_rename,
	nfs_mkdir,
	nfs_rmdir,
	nfs_readdir,
	nfs_symlink,
	nfs_csymlink,
	nfs_readlink,
	nfs_readclink,
	nfs_fsync,
	nfs_inactive,
	nfs_bmap,
	nfs_strategy,
	nfs_badop,
	nfs_badop,
	nfs_minphys
};

/*
 * nfs_open()
 *	Open a nfs vnode (rnode).
 */

/*ARGSUSED*/
static int
nfs_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	int error;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_open %s %o %d flag %d\n",
		vtomi(*vpp)->mi_hostname,
		vtofh(*vpp)->fh_fsid, vtofh(*vpp)->fh_fno, flag);
#endif	NFSDEBUG

	VN_LOCKNODE(*vpp);
	/*
	 * Validate cached data by getting the attributes from the server.
	 */
	RLOCK_ATTR(vtor(*vpp));
	NFSATTR_INVAL(*vpp);
	RUNLOCK_ATTR(vtor(*vpp));
	error = nfs_getattr(*vpp, &va, cred);
	if (!error)
		vtor(*vpp)->r_nopen++;
	VN_UNLOCKNODE(*vpp);
	return (error);
}

/*
 * nfs_close()
 *	Close a NFS vnode (rnode).
 */

/*ARGSUSED*/
static int
nfs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct rnode *rp;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_close %s %o %d flag %d\n",
		vtomi(vp)->mi_hostname,
		vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno, flag);
#endif	NFSDEBUG

	rp = vtor(vp);
	VN_LOCKNODE(vp);
	--rp->r_nopen;

	/*
	 * If this is a close of a file open for writing or an unlinked
	 * open file or a file that has had an asynchronous write error,
	 * flush synchronously. This allows us to invalidate the file's
	 * buffers if there was a write error or the file was unlinked.
	 * Invalidating the buffers kills their references to the vnode
	 * so that it will free up quickly.
	 */
	if (flag & FWRITE || rp->r_unldvp != NULL || rp->r_error) {
		sync_vp(vp);
	}
	if (rp->r_nopen == 0 && (rp->r_unldvp != NULL || rp->r_error)) {
		binvalfree(vp);
	}
	VN_UNLOCKNODE(vp);
	return ((flag & FWRITE) ? rp->r_error : 0);
}

/*
 * nfs_rdwr()
 *	Read or write a vnode.
 *
 * Only allow I/O for regular files.
 *
 * Called with locked vnode (vp).
 */

static int
nfs_rdwr(vp, uiop, rw, ioflag, cred)
	register struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	int error;
	spl_t	s_ipl;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rdwr: %s %o %d %s %x %d\n",
		vtomi(vp)->mi_hostname,
		vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
		rw == UIO_READ ? "READ" : "WRITE",
		uiop->uio_iov->iov_base, uiop->uio_iov->iov_len);
#endif	NFSDEBUG

	if (vp->v_type != VREG) {
		return (EISDIR);
	}

	if (rw == UIO_WRITE || (rw == UIO_READ && vtor(vp)->r_cred == NULL)) {
		s_ipl = RN_LOCKCRED(vtor(vp));
		if (rw == UIO_WRITE || vtor(vp)->r_cred == NULL) {
			/*
			 * atomically update r_cred so that do_bio() does not
			 * pick up stale credential.
			 */
			crhold(cred);
			if (vtor(vp)->r_cred) {
				crfree(vtor(vp)->r_cred);
			}
			vtor(vp)->r_cred = cred;
		}
		RN_UNLOCKCRED(vtor(vp), s_ipl);
	}

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		error = nfs_getattr(vp, &va, cred);
		if (error)
			return (error);
		uiop->uio_offset = vtor(vp)->r_size;
	}

	error = rwvp(vp, uiop, rw);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rdwr returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * rwvp
 *	called from nfs_rdwr() with locked vnode (vp).
 */

static int
rwvp(vp, uio, rw)
	register struct vnode *vp;
	register struct uio *uio;
	enum uio_rw rw;
{
	register int n, on;
	struct buf *bp;
	struct rnode *rp;
	daddr_t bn;
	int size;
	int error = 0;
	struct vnode *mapped_vp;
	daddr_t mapped_bn, mapped_rabn;
	int eof = 0;

	if (uio->uio_resid == 0) {
		return (0);
	}
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0) {
		return (EINVAL);
	}
	if (rw == UIO_WRITE && vp->v_type == VREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
	size = vtoblksz(vp);
	size &= ~(DEV_BSIZE - 1);
	ASSERT(size > 0, "rwvp: zero size");

	rp = vtor(vp);
	do {
		bn = uio->uio_offset / size;
		on = uio->uio_offset % size;
		n = MIN((unsigned)(size - on), uio->uio_resid);
		if ((vp->v_flag & VMAPPED) && mmreg_rw(vp, bn, rw, on, n, uio))
			continue;
		VOP_BMAP(vp, bn, &mapped_vp, &mapped_bn,
				(rw == UIO_READ) ? B_READ : B_WRITE, 0);
		if (rw == UIO_READ) {
			int diff;

			if ((long)mapped_bn < 0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else {
				if (incore(mapped_vp, mapped_bn)) {
					struct vattr va;

					/*
					 * get attributes to check whether in
					 * core data is stale. mapped_vp is
					 * the same as vp. See nfs_bmap().
					 */
					(void) do_getattr(mapped_vp, &va,
								u.u_cred);
				}
				if (rp->r_lastr + 1 == bn) {
					VOP_BMAP(vp, bn + 1, &mapped_vp,
						&mapped_rabn, B_READ, 0);
					if (mapped_rabn < 0)
						bp = bread(mapped_vp,
							mapped_bn, size);
					else
						bp = breada(mapped_vp,
							mapped_bn, size,
							mapped_rabn, size);
				} else {
					bp = bread(mapped_vp, mapped_bn, size);
				}
			}
			if (bp->b_flags & B_ERROR) {
				error = geterror(bp);
				brelse(bp);
				return (error);
			}
			rp->r_lastr = bn;
			diff = rp->r_size - uio->uio_offset;
			if (diff <= 0) {
				brelse(bp);
				return (0);
			}
			if (diff < n) {
				n = diff;
				eof = 1;
			}
		} else {
			if (rp->r_error) {
				return (rp->r_error);
			}
			if (n == size) {
				bp = getblk(mapped_vp, mapped_bn, size, 0);
			} else {
				if (incore(mapped_vp, mapped_bn)) {
					struct vattr va;

					/*
					 * get attributes to check whether in
					 * core data is stale
					 */
					(void) do_getattr(mapped_vp, &va,
								u.u_cred);
				}
				bp = bread(mapped_vp, mapped_bn, size);
			}
		}
		if (bp->b_flags & B_ERROR) {
			error = geterror(bp);
			brelse(bp);
			return (error);
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
			/*
			 * r_size is the maximum number of bytes known
			 * to be in the file.
			 * Make sure it is at least as high as the last
			 * byte we just wrote into the buffer.
			 */
			if (rp->r_size < uio->uio_offset) {
				RLOCK_ATTR(rp);
				if (rp->r_size < uio->uio_offset)
					rp->r_size = uio->uio_offset;
				RUNLOCK_ATTR(rp);
			}
			rp->r_flags |= RDIRTY;
			if (n + on == size) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else {
				bdwrite(bp);
			}
			vp->v_flag |= VMAPSYNC;		/* fsync before map */
		}
	} while (!u.u_error && uio->uio_resid > 0 && !eof);
	if (rw == UIO_WRITE && uio->uio_resid && u.u_error == 0) {
		printf("rwvp: short write. resid %d vp %x bn %d\n",
			uio->uio_resid, vp, bn);
	}
	if (!error)				/* XXX */
		error = u.u_error;		/* XXX */
	return (error);
}

/*
 * Write to file.
 * Writes to remote server in largest size chunks that the server can
 * handle.  Write is synchronous.
 *
 * Called with rnode's attributes locked (vtor(vp)->r_attrsema).
 * Return with attributes unlocked.
 */
static
nfswrite(vp, base, offset, count, cred)
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	struct ucred *cred;
{
	register int tsize;
	int error;
	struct nfswriteargs wa;
	struct nfsattrstat ns;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfswrite %s %o %d offset = %d, count = %d\n",
		vtomi(vp)->mi_hostname,
		vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno, offset, count);
#endif	NFSDEBUG

	do {
		tsize = MIN(vtomi(vp)->mi_stsize, count);
		wa.wa_data = base;
		wa.wa_fhandle = *vtofh(vp);
		wa.wa_begoff = offset;
		wa.wa_totcount = tsize;
		wa.wa_count = tsize;
		wa.wa_offset = offset;
		error = rfscall(vtomi(vp), RFS_WRITE, xdr_writeargs,
				(caddr_t)&wa, xdr_attrstat, (caddr_t)&ns, cred);
		if (!error) {
			error = geterrno(ns.ns_status);
		}

#ifdef	NFSDEBUG
		dprint(nfsdebug, 3, "nfswrite: sent %d of %d, error %d\n",
			tsize, count, error);
#endif	NFSDEBUG

		count -= tsize;
		base += tsize;
		offset += tsize;
	} while (!error && count);

	if (!error) {
		/*
		 * nfs_attrcache will return with r_attrsema unlocked.
		 */
		nfs_attrcache(vp, &ns.ns_attr, NOFLUSH);
	} else {
		RUNLOCK_ATTR(vtor(vp));
	}

	switch (error) {
	case 0:
	case EDQUOT:
		break;

	case ENOSPC:
		printf("NFS write error: on host %s remote file system full\n",
			vtomi(vp)->mi_hostname );
		break;

	default:
		printf("NFS write error %d on host %s fs %o file %d\n",
			error, vtomi(vp)->mi_hostname,
			vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
		break;
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfswrite: returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * Read from a file.
 * Reads data in largest chunks our interface can handle
 *
 * Called with rnode's attributes locked (vtor(vp)->r_attrsema).
 * Return with attributes unlocked.
 */
static
nfsread(vp, base, offset, count, residp, cred)
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	int *residp;
	struct ucred *cred;
{
	register int tsize;
	int error;
	struct nfsreadargs ra;
	struct nfsrdresult rr;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfsread %s %o %d offset = %d, totcount = %d\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
		offset, count);
#endif	NFSDEBUG

	do {
		tsize = MIN(vtomi(vp)->mi_tsize, count);
		rr.rr_data = base;
		ra.ra_fhandle = *vtofh(vp);
		ra.ra_offset = offset;
		ra.ra_totcount = tsize;
		ra.ra_count = tsize;
		error = rfscall(vtomi(vp), RFS_READ, xdr_readargs, (caddr_t)&ra,
				xdr_rdresult, (caddr_t)&rr, cred);
		if (!error) {
			error = geterrno(rr.rr_status);
		}

#ifdef	NFSDEBUG
		dprint(nfsdebug, 3, "nfsread: got %d of %d, error %d\n",
			tsize, count, error);
#endif	NFSDEBUG

		if (!error) {
			count -= rr.rr_count;
			base += rr.rr_count;
			offset += rr.rr_count;
		}
	} while (!error && count && rr.rr_count == tsize);

	*residp = count;

	if (!error) {
		if (vtor(vp)->r_size < rr.rr_attr.na_size) {
			vtor(vp)->r_size = rr.rr_attr.na_size;
		}
		/*
		 * nfs_attrcache will return with r_attrsema unlocked.
		 */
		nfs_attrcache(vp, &rr.rr_attr, SFLUSH);
	} else
		RUNLOCK_ATTR(vtor(vp));

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfsread: returning %d, resid %d\n",
		error, *residp);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_ioctl
 *	not supported
 */

/*ARGSUSED*/
static int
nfs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{

	return (EOPNOTSUPP);
}

/*
 * nfs_select
 *	not supported
 */

/*ARGSUSED*/
static int
nfs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{

	return (EOPNOTSUPP);
}

/*
 * Client attribute cache
 *
 * Maintain attributes on remote files. Contents are invalidated on timeout.
 *
 * nfs_attrcache() is:
 * called with the rnode's r_attrsema locked.
 * returns with the rnode's r_attrsema unlocked.
 */

nfs_attrcache(vp, na, fflag)
	struct vnode *vp;
	struct nfsfattr *na;
	enum staleflush fflag;
{
	register struct rnode *rp;
	int flush = 0;
	extern int nfsac_regtimeo;	/* regular file timeout */
	extern int nfsac_dirtimeo;	/* directory timeout */

	rp = vtor(vp);
	/*
	 * check the new modify time against the old modify time
	 * to see if cached data is stale.
	 */
	if (na->na_mtime.tv_sec != rp->r_nfsattr.na_mtime.tv_sec ||
	    na->na_mtime.tv_usec != rp->r_nfsattr.na_mtime.tv_usec) {
		/*
		 * The file has changed.
		 *
		 * If this was unexpected (fflag == SFLUSH),
		 * flush the delayed write blocks associated with this vnode
		 * from the buffer cache and mark the cached blocks on the
		 * free list as invalid. If this is currently being done, then
		 * this process doesn't need to do so. Also flush the page
		 * cache.
		 * If this is a text mark it invalid so that the next pagein
		 * from the file will fail.
		 * If the vnode is a directory, purge the directory name
		 * lookup cache.
		 */
		if (fflag == SFLUSH && !rp->r_flushing) {
			flush = 1;
			rp->r_flushing = 1;
		}
		if (vp->v_flag & VTEXT) {
			vp->v_badmap |= MM_STALETEXT;
		}
		if (vp->v_type == VDIR) {
			/*
			 * Should be called with vnode locked.
			 */
			ASSERT(VN_LOCKEDNODE(vp),
				"nfs_attrcache: vp not locked");
			dnlc_purge_vp(vp);
		}
	}
	rp->r_nfsattr = *na;
	rp->r_nfsattrtime = time;
	if (vp->v_type == VDIR) {
		rp->r_nfsattrtime.tv_sec += nfsac_dirtimeo;
	} else {
		rp->r_nfsattrtime.tv_sec += nfsac_regtimeo;
	}
	RUNLOCK_ATTR(rp);
	if (flush) {
		binvalfree(vp);
		RLOCK_ATTR(rp);
		rp->r_flushing = 0;
		RUNLOCK_ATTR(rp);
	}
}

/*
 * do_getattr()
 *	Called by nfs_getattr() and rwvp().
 *	Return attributes of NFS vnode (rnode).
 *
 * Called with locked vnode (vp).
 */
static int
do_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	struct rnode *rp;
	int error;
	struct nfsattrstat ns;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "do_getattr %s %d %o\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif	NFSDEBUG

	rp = vtor(vp);
	RLOCK_ATTR(rp);
	if (timercmp(&time, &rp->r_nfsattrtime, <)) {
		/*
		 * Use cached attributes.
		 */
		nattr_to_vattr(&rp->r_nfsattr, vap);
		vap->va_fsid = makedev(255, vtomi(vp)->mi_mntno);
		if (rp->r_size >= vap->va_size &&
		    ((rp->r_flags & RDIRTY) || VN_MAPPED(vp))) {
			vap->va_size = rp->r_size;
		} else {
			rp->r_size = vap->va_size;
		}
		RUNLOCK_ATTR(rp);
		return (0);
	}
	error = rfscall(vtomi(vp), RFS_GETATTR, xdr_fhandle, (caddr_t)vtofh(vp),
			xdr_attrstat, (caddr_t)&ns, cred);
	if (!error) {
		error = geterrno(ns.ns_status);
		if (!error) {
			nattr_to_vattr(&ns.ns_attr, vap);
			/*
			 * this is a kludge to make programs that use dev from
			 * stat to tell file systems apart happy.  we kludge up
			 * a dev from the mount number and an arbitrary major
			 * number 255.
			*/
			vap->va_fsid = makedev(255, vtomi(vp)->mi_mntno);
			if (rp->r_size >= vap->va_size &&
			    ((rp->r_flags & RDIRTY) || VN_MAPPED(vp))){
				vap->va_size = rp->r_size;
			} else {
				rp->r_size = vap->va_size;
			}
			nfs_attrcache(vp, &ns.ns_attr, SFLUSH);
			return (0);
		} else {
			L_CHECK_STALE_FH(error, vp);
		}
	}
	RUNLOCK_ATTR(rp);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "do_getattr: returns %d\n", error);
#endif	NFSDEBUG

	return (error);
}

static int
nfs_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	sync_vp(vp);    /* sync blocks so mod time is right */
	return (do_getattr(vp, vap, cred));
}

/*
 * nfs_setattr()
 *	Set attributes of NFS vnode (rnode).
 *
 * Called with locked vnode (vp).
 */

static int
nfs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int error;
	struct nfssaargs args;
	struct nfsattrstat ns;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_setattr %s %o %d\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif	NFSDEBUG

	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_ctime.tv_sec != -1) || (vap->va_ctime.tv_usec != -1)) {
		error = EINVAL;
	} else {
		sync_vp(vp);
		/*
		 * Don't allow truncation of mapped files, directories, fifos
		 * or files you don't have write permission for.
		 */
		if (vap->va_size != -1) {
			if (vp->v_type == VFIFO) {
				error = EINVAL;
				goto out;
			}
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			error = nfs_access(vp, VWRITE, cred);
			if (error) {
				goto out;
			}
			if (VN_MAPPED(vp)) {	/* VMAPPED can be stale */
				error = EBUSY;
				goto out;
			}
		}
		vattr_to_sattr(vap, &args.saa_sa);
		args.saa_fh = *vtofh(vp);
		RLOCK_ATTR(vtor(vp));
		error = rfscall(vtomi(vp), RFS_SETATTR, xdr_saargs,
				(caddr_t)&args, xdr_attrstat, (caddr_t)&ns,
				cred);
		if (!error) {
			error = geterrno(ns.ns_status);
			if (!error) {
				if (vap->va_size != -1) {
					(vtor(vp))->r_size = vap->va_size;
				}
				nfs_attrcache(vp, &ns.ns_attr, SFLUSH);
				if (vap->va_mode != (u_short)0xffff) {
					vp->v_flag &= ~VSVTX;
					vp->v_flag |= (ns.ns_attr.na_mode & VSVTX);
				}
				return (0);
			} else {
				L_CHECK_STALE_FH(error, vp);
			}
		}
		RUNLOCK_ATTR(vtor(vp));
	}

out:

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_setattr: returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_access()
 *	See if given access mode is allowed.
 *
 * Called with locked vnode (vp).
 * Return 0 for access allowed, or error number.
 */

static int
nfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	int *gp;
	struct vattr va;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_access %s %o %d mode %d uid %d\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
		mode, cred->cr_uid);
#endif	NFSDEBUG

	u.u_error = nfs_getattr(vp, &va, cred);
	if (u.u_error) {
		return (u.u_error);
	}

	/*
	 * Cannot write active texts.
	 */
	if (mode & VWRITE) {
		/*
		 * If there's shared text associated with the vnode,
		 * try to free it up once.  If we fail, we can't allow writing.
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
	if (cred->cr_uid == 0)
		return (0);

	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then check public access.
	 */
	if (cred->cr_uid != va.va_uid) {
		mode >>= 3;
		if (cred->cr_gid == va.va_gid)
			goto found;
		gp = cred->cr_groups;
		for (; gp < &cred->cr_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (va.va_gid == *gp)
				goto found;
		mode >>= 3;
	}
found:
	if ((va.va_mode & mode) == mode) {
		return (0);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_access: returning %d\n", u.u_error);
#endif	NFSDEBUG

	u.u_error = EACCES;
	return (EACCES);
}

/*
 * nfs_readlink()
 *	Read contents of a symbolic-link.
 *
 * Called with locked vnode (vp).
 */

static int
nfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio   *uiop;
	struct ucred *cred;
{
	int error;
	struct nfsrdlnres rl;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_readlink %s %o %d\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif	NFSDEBUG

	if(vp->v_type != VLNK)
		return (ENXIO);
	rl.rl_data = (char *)kmem_alloc((u_int)NFS_MAXPATHLEN);
	error = rfscall(vtomi(vp), RFS_READLINK, xdr_fhandle,
			(caddr_t)vtofh(vp), xdr_rdlnres, (caddr_t)&rl, cred);
	if (!error) {
		error = geterrno(rl.rl_status);
		if (!error) {
			error = uiomove(rl.rl_data, (int)rl.rl_count,
					UIO_READ, uiop);
		}
	}
	kmem_free((caddr_t)rl.rl_data, (u_int)NFS_MAXPATHLEN);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_readlink: returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_readclink()
 *	Read the branch of the conditional link requested by flag
 *	NOT SUPPORTED
 */
/*ARGSUSED*/
static
nfs_readclink(vp, uiop, flag, cred)
	struct vnode *vp;
	struct uio *uiop;
	int flag;
	struct ucred *cred;
{
	return (EOPNOTSUPP);
}

/*
 * flush_vp()
 *	Flush the vnode.
 *
 * Called with locked vnode (vp).
 */
static
flush_vp(vp)
	struct vnode *vp;
{
	register struct rnode *rp;
	register int offset, blksize;

	rp = vtor(vp);
	rp->r_flags &= ~RDIRTY;
	bflush(vp);		/* start delayed writes */
	blksize = vtoblksz(vp);
	for (offset = 0; offset < rp->r_size; offset += blksize) {
		blkflush(vp, (daddr_t)(offset >> DEV_BSHIFT), (long)blksize);
	}
}

/*
 * sync_vp
 *	Synch-out an rnode -- insure all it's dirty blocks are written.
 *
 * Called with locked vnode (vp).
 */
static
sync_vp(vp)
	struct vnode *vp;
{
	if (vtor(vp)->r_flags & RDIRTY)
		flush_vp(vp);
	vp->v_flag &= ~VMAPSYNC;
}

/*
 * nfs_fsync()
 *	Synch-out an rnode -- insure all it's dirty blocks are written.
 *
 * Called with locked vnode (vp).
 */
/*ARGSUSED*/
static int
nfs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_fsync %s %o %d\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif	NFSDEBUG

	flush_vp(vp);
	vp->v_flag &= ~VMAPSYNC;
	return (vtor(vp)->r_error);
}

/*
 * Weirdness: if the file was removed while it was open it got
 * renamed (by nfs_remove) instead.  Here we remove the renamed
 * file.
 */
static int
nfs_inactive(vp, flag)
	struct vnode *vp;
	int flag;
{
	register struct rnode *rp;
	int error;
	enum nfsstat status;
	spl_t	s_ipl;
	struct nfsdiropargs da;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_inactive %s %o %d\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif	NFSDEBUG

	/*
	 * We have the vnode locked if we are called from VN_PUT(). As
	 * soon as we know we have the vnode, unlock the vnode. Holding
	 * the vnode locked until this point ensures that we will notice
	 * if a racing makenfsnode()/rfind() finds this rnode in the
	 * rnode() table before we can throw the rnode away. This works
	 * because holding the vnode locked ensures that rfind() will spin
	 * in the VN_HOLD() until the VN_TRYLOCKNODE below is executed.
	 * We cannot keep the vnode locked if we need to sleep, hence
	 * we release it as soon as possible.
	 */
	if (flag == NODEISLOCKED || VN_TRYLOCKNODE(vp)) {
		VN_UNLOCK(vp);
		s_ipl = LOCK_RTAB;
		/*
		 * Free the rnode unless there is another process waiting
		 * for it. This is sufficient (no windows) because another
		 * process could be waiting IFF it was in the psvl in rfind().
		 * In this case it had the rnode table locked until it
		 * blocked.
		 */
		if (VN_WAITERS(vp)) {
			VN_UNLOCKNODE(vp);
			UNLOCK_RTAB(s_ipl);
			return;
		}
		/*
		 * Now throw away the rnode and decrement the mount
		 * reference count. Note that mi_refct is actually
		 * mutex'd by the rnode table lock!
		 */
		rp = vtor(vp);
		runsave(rp);
		if (rp->r_unlname != NULL) {
			UNLOCK_RTAB(s_ipl);
			setdiropargs(&da, rp->r_unlname, rp->r_unldvp);
			/*
			 * This can race with nfs_lookup() since the
			 * r_unldvp vnode is not locked. In this case
			 * nfs_lookup will create a new rnode for the
			 * file that is being removed. The new rnode will
			 * be encounter stale fh when used.
			 */
			RLOCK_ATTR(vtor(rp->r_unldvp));
			error = rfscall(vtomi(rp->r_unldvp), RFS_REMOVE,
					xdr_diropargs, (caddr_t)&da, xdr_enum,
					(caddr_t)&status, rp->r_unlcred);
			NFSATTR_INVAL(rp->r_unldvp);	/* mod time changed */
			RUNLOCK_ATTR(vtor(rp->r_unldvp));
			if (!error) {
				error = geterrno(status);
			}
			VN_RELE(rp->r_unldvp);
			kmem_free((caddr_t)rp->r_unlname, (u_int)NFS_MAXNAMLEN);
			crfree(rp->r_unlcred);
			s_ipl = LOCK_RTAB;
		}

		/*
		 * Decrement mount table referecence count.
		 */
		((struct mntinfo *)vp->v_vfsp->vfs_data)->mi_refct--;
		UNLOCK_RTAB(s_ipl);

		/*
		 * No need to lock as no references to rnode.
		 */
		if (rp->r_cred) {
			crfree(rp->r_cred);
		}
		kmem_free((caddr_t)rp, (u_int)sizeof(struct rnode));
	} else {
		/*
		 * VN_TRYLOCKNODE() failed (couldn't get the rnode), which
		 * means that a concurrent makenfsnode()/rfind() did in
		 * between this process' lock of the vnode and the
		 * VN_TRYLOCKNODE(). Having the vnode locked until this
		 * point ensures that we WILL notice this and avoid
		 * the destruction of the rnode. If the vnode wasn't locked
		 * on entry to nfs_inactive(), then a concurrent process
		 * could have found the rnode, locked it, referenced it, and
		 * unlocked it (still holding the reference) as this process
		 * proceeded to destroy the rnode. This would cause problems. 
		 */
		VN_UNLOCK(vp);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_inactive done\n");
#endif	NFSDEBUG
}

/*
 *
 * Remote file system operations having to do with directory manipulation.
 *
 */


/*
 * nfs_lookup()
 *	Look up a component of a path-name in a directory.
 *
 * Caller passes locked vnode (dvp).
 * On return: dvp is unlocked, *vpp is locked.
 * If error, dvp is still unlocked, *vpp is unchanged.
 */

static
nfs_lookup(dvp, nm, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
{
	return (do_lookup(dvp, nm, vpp, cred, 1));
}

/*
 * do_lookup()
 *	The dirty work of nfs_lookup() is done here.
 *
 * Called from nfs_lookup(), nfs_remove, and nfs_create().
 * Both nfs_lookup() and nfs_create() call with unlockdir flag set.
 * Caller passes locked vnode (dvp).
 *
 * Normal return:
 *	If unlockdir is set, dvp is unlocked, *vpp is locked.
 *		Except in the case of "." where dvp is locked since dvp == *vpp.
 *	If unlockdir is not set, dvp is locked, *vpp is locked.
 *		Except in the case of "..", then dvp is unlocked and
 *		*vpp is locked.
 * Error return: dvp is always unlocked, *vpp is unchanged.
 */

static int
do_lookup(dvp, nm, vpp, cred, unlockdir)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	int unlockdir;
{
	int namelen;		/* strlen(nm) */
	int error;
	struct nfsdiropargs da;
	struct nfsdiropres  dr;
	struct vattr va;
	struct vnode *dnlc_lookup();

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "do_lookup %s %o %d '%s'\n",
		vtomi(dvp)->mi_hostname,
		vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm);
#endif	NFSDEBUG

	/*
	 * Before checking dnlc, call getattr to be
	 * sure directory hasn't changed.  getattr
	 * will purge dnlc if a change has occurred.
	 */
	if (error = nfs_getattr(dvp, &va, cred)) {
		VN_UNLOCKNODE(dvp);
		return (error);
	}

	/*
	 * Short circuit '.'
	 */
	namelen = strlen(nm);
	if (namelen == 1 && nm[0] == '.') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	/*
	 * Assumes that ".." is not in dnlc cache.
	 */
	*vpp = (struct vnode *)dnlc_lookup(dvp, nm, cred);
	if (*vpp) {
		VN_HOLD(*vpp);
		if (unlockdir)
			VN_UNLOCKNODE(dvp);
		VN_LOCKNODE(*vpp);
		return (0);
	}

	setdiropargs(&da, nm, dvp);
	error = rfscall(vtomi(dvp), RFS_LOOKUP, xdr_diropargs, (caddr_t)&da,
			xdr_diropres, (caddr_t)&dr, cred);
	if (!error) {
		error = geterrno(dr.dr_status);
		L_CHECK_STALE_FH(error, dvp);
	}
	if (!error) {
		if (namelen == 2 && nm[0] == '.' && nm[1] == '.') {
			VN_UNLOCKNODE(dvp);	/* race with makenfsnode */
			*vpp = makenfsnode(&dr.dr_fhandle, &dr.dr_attr,
						dvp->v_vfsp);
		} else {
			*vpp = makenfsnode(&dr.dr_fhandle, &dr.dr_attr,
						dvp->v_vfsp);
			if ((enum vtype)dr.dr_attr.na_type == VDIR) {
				dnlc_enter(dvp, nm, *vpp, cred);
			}
			if (unlockdir)
				VN_UNLOCKNODE(dvp);
		}
	} else {
		/* error */
		VN_UNLOCKNODE(dvp);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "do_lookup returning %d vp = %x\n", error, *vpp);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_create()
 *	Create a new NFS file.
 *
 * Sucess: *vpp is returned locked.
 * Error:  *vpp is unchanged.
 */

/*ARGSUSED*/
static
nfs_create(dvp, nm, va, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct nfsdiropres  dr;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_create %s %o %d '%s' excl=%d, mode=%o\n",
		vtomi(dvp)->mi_hostname,
		vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm, exclusive, mode);
#endif	NFSDEBUG

	/*
	 * This is buggy: there is a race between the lookup and the
	 * create.  We should send the exclusive flag over the wire.
	 */
	VN_LOCKNODE(dvp);
	error = do_lookup(dvp, nm, vpp, cred, 1);

	if (!error) {
		if (exclusive == EXCL) {
			VN_PUT(*vpp);
			return (EEXIST);
		}
		/*
		 * Don't allow truncation of a mapped file.... this can cause
		 * unpredictable results.
		 */
		if ((*vpp)->v_type == VREG && va->va_size == 0 &&
		    VN_MAPPED(*vpp)) {
			VN_PUT(*vpp);
			return (EBUSY);
		}
		VN_PUT(*vpp);
	}
	*vpp = NULLVP;

	/*
	 * Until the protocol change, disallow mknod on remote filesystems.
	 */
	if (va->va_type != VREG)
		return (EINVAL);

	setdiropargs(&args.ca_da, nm, dvp);
	vattr_to_sattr(va, &args.ca_sa);
	RLOCK_ATTR(vtor(dvp));
	error = rfscall(vtomi(dvp), RFS_CREATE, xdr_creatargs, (caddr_t)&args,
			xdr_diropres, (caddr_t)&dr, cred);
	NFSATTR_INVAL(dvp);		/* mod time changed */
	RUNLOCK_ATTR(vtor(dvp));
	if (!error) {
		error = geterrno(dr.dr_status);
		if (!error) {
			*vpp = makenfsnode(&dr.dr_fhandle, &dr.dr_attr,
						dvp->v_vfsp);
			/*
			 * trunc if told to do so.
			 */
			if (va->va_size == 0) {
				/*
				 * Flush buffer cache references before
				 * clearing rnode size. This avoids problems
				 * with do_bio().
				 */
				sync_vp(*vpp);
				RLOCK_ATTR(vtor(*vpp));
				(vtor(*vpp))->r_size = 0;
				RUNLOCK_ATTR(vtor(*vpp));
			}
			if (va != (struct vattr *)0) {
				nattr_to_vattr(&dr.dr_attr, va);
			}
		}
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_create returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_remove()
 *
 * Weirdness: if the vnode to be removed is open
 * we rename it instead of removing it and nfs_inactive
 * will remove the new name.
 */
static
nfs_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	register struct rnode *rp;
	int error;
	struct vnode *vp;
	enum nfsstat status;
	char *tmpname;
	struct nfsdiropargs da;
	char *newname();

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_remove %s %o %d '%s'\n",
		vtomi(dvp)->mi_hostname,
		vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm);
#endif	NFSDEBUG

	status = NFS_OK;
	/*
	 * If no error, do_lookup() returns locked vnode (vp) and locked dvp.
	 */
	VN_LOCKNODE(dvp);
	error = do_lookup(dvp, nm, &vp, cred, 0);
	if (!error) {
		/*
		 * Both dvp and vp are locked.
		 */
		rp = vtor(vp);
		/*
		 * The following check for VN_MAPPED(vp) is to insure that
		 * we don't remove a file out from underneath a mapping.
		 * Note that this cannot happen in Dynix 3 since mapping
		 * a file will effectively dup the fd being mapped.  Thus,
		 * r_nopen will not be 0 for a mapped file.  This check is
		 * for safety since the mmap behaviour could change in the
		 * future.
		 */
		if (((rp->r_nopen > 0) || (VN_MAPPED(vp))) &&
		    rp->r_unlname == NULL) {
			tmpname = newname();
			error = do_rename(dvp, nm, dvp, tmpname, cred);
			if (error) {
				kmem_free((caddr_t)tmpname,
						(u_int)NFS_MAXNAMLEN);
			} else {
				VN_HOLD(dvp);
				rp->r_unldvp = dvp;
				rp->r_unlname = tmpname;
				if (rp->r_unlcred != NULL) {
					crfree(rp->r_unlcred);
				}
				crhold(cred);
				rp->r_unlcred = cred;
			}
		} else {
			/*
			 * Make sure that there are no dirty buffers
			 * that haven't been written to the file. This
			 * avoids attempting async write errors to
			 * non-existent file (ESTALE).
			 */
			sync_vp(vp);
			setdiropargs(&da, nm, dvp);
			RLOCK_ATTR(vtor(dvp));
			RLOCK_ATTR(rp);
			error = rfscall(vtomi(dvp), RFS_REMOVE, xdr_diropargs,
					(caddr_t)&da, xdr_enum,
					(caddr_t)&status, cred);
			NFSATTR_INVAL(dvp);	/* mod time changed */
			NFSATTR_INVAL(vp);	/* link count changed */
			RUNLOCK_ATTR(rp);
			RUNLOCK_ATTR(vtor(dvp));
		}
		VN_UNLOCKNODE(dvp);
		VN_PUT(vp);
	}
	if (!error) {
		error = geterrno(status);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_remove: returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_link()
 *	Link a file or a directory.
 */

static
nfs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfslinkargs args;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_link from %s %o %d to %s %o %d '%s'\n",
		vtomi(vp)->mi_hostname, vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
		vtomi(tdvp)->mi_hostname,
		vtofh(tdvp)->fh_fsid, vtofh(tdvp)->fh_fno, tnm);
#endif	NFSDEBUG

	args.la_from = *vtofh(vp);
	setdiropargs(&args.la_to, tnm, tdvp);
	/*
	 * Must hold both the directory and vnode locked as both will
	 * have changed attributes after the rfscall().
	 */
	for (;;) {
		RLOCK_ATTR(vtor(tdvp));
		if ((vp == tdvp) || TRY_RLOCK_ATTR(vtor(vp)))
			break;
		/*
		 * Avoid deadlock/livelock by releasing resources, sleep
		 * on lbolt and start over.
		 */
		RUNLOCK_ATTR(vtor(tdvp));
		p_sema(&lbolt, PVNOD);
	}
	error = rfscall(vtomi(vp), RFS_LINK, xdr_linkargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(tdvp);		/* mod time changed */
	if (vp != tdvp) {
		NFSATTR_INVAL(vp);		/* link count changed */
		RUNLOCK_ATTR(vtor(vp));
	}
	RUNLOCK_ATTR(vtor(tdvp));
	if (!error) {
		error = geterrno(status);
		CHECK_STALE_FH(error, vp);
		CHECK_STALE_FH(error, tdvp);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_link returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_rename()
 *	Rename a file or directory
 */

static
nfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	int error;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rename from %s %o %d '%s' to %s %o %d '%s'\n",
		vtomi(odvp)->mi_hostname, vtofh(odvp)->fh_fsid,
		vtofh(odvp)->fh_fno, onm, vtomi(ndvp)->mi_hostname,
		vtofh(ndvp)->fh_fsid, vtofh(ndvp)->fh_fno, nnm);
#endif	NFSDEBUG

	if (!strcmp(onm, ".") || !strcmp(onm, "..") || !strcmp(nnm, ".") ||
	    !strcmp (nnm, "..")) {
		error = EINVAL;
	} else {

		/*
		 * Lock the appropriate directory vnode(s) being
		 * careful to avoid deadlock and livelock.
		 */
		for (;;) {
			VN_LOCKNODE(odvp);
			if (ndvp == odvp || VN_TRYLOCKNODE(ndvp))
				break;
			VN_UNLOCKNODE(odvp);
			p_sema(&lbolt, PVNOD);
		}

		error = do_rename(odvp, onm, ndvp, nnm, cred);

		/*
		 * Release locked directory vnode(s).
		 */
		VN_UNLOCKNODE(odvp);
		if (ndvp != odvp)
			VN_UNLOCKNODE(ndvp);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rename returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * do_rename()
 *	The dirty work of nfs_rename() is done here.
 *
 * Called from nfs_rename() and nfs_remove.
 * Caller passes locked vnodes (dvp).
 * Return with vnodes still locked.
 */

static int
do_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsrnmargs args;

	/*
	 * first remove names from dnlc. Hold directory vnode(s) locked
	 * until rename() completed to keep nfs_lookup() from
	 * replacing them into the dnlc.
	 */
	dnlc_remove(odvp, onm);
	dnlc_remove(ndvp, nnm);

	setdiropargs(&args.rna_from, onm, odvp);
	setdiropargs(&args.rna_to, nnm, ndvp);

	for (;;) {
		RLOCK_ATTR(vtor(odvp));
		if (ndvp == odvp || TRY_RLOCK_ATTR(vtor(ndvp)))
			break;
		RUNLOCK_ATTR(vtor(odvp));
		p_sema(&lbolt, PVNOD);
	}
	error = rfscall(vtomi(odvp), RFS_RENAME,
			xdr_rnmargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(odvp);	/* mod time changed */
	NFSATTR_INVAL(ndvp);	/* mod time changed */
	RUNLOCK_ATTR(vtor(odvp));
	if (ndvp != odvp)
		RUNLOCK_ATTR(vtor(ndvp));
	if (!error) {
		error = geterrno(status);
		L_CHECK_STALE_FH(error, odvp);
		L_CHECK_STALE_FH(error, ndvp);
	}
	return (error);
}

/*
 * nfs_mkdir()
 *	Create a new UNIX file-system directory.
 *
 * Success: return locked vnode (*vpp)
 * Failure: return NULLVP (*vpp) and error code.
 */

static
nfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct nfsdiropres dr;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_mkdir %s %o %d '%s'\n",
		vtomi(dvp)->mi_hostname,
		vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm);
#endif	NFSDEBUG

	setdiropargs(&args.ca_da, nm, dvp);
	vattr_to_sattr(va, &args.ca_sa);
	RLOCK_ATTR(vtor(dvp));
	error = rfscall(vtomi(dvp), RFS_MKDIR, xdr_creatargs, (caddr_t)&args,
			xdr_diropres, (caddr_t)&dr, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	RUNLOCK_ATTR(vtor(dvp));
	if (!error) {
		error = geterrno(dr.dr_status);
		CHECK_STALE_FH(error, dvp);
	}
	if (!error) {
		*vpp = makenfsnode(&dr.dr_fhandle, &dr.dr_attr, dvp->v_vfsp);
	} else {
		*vpp = NULLVP;
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_mkdir returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_rmdir()
 *	Remove a directory, given parent directory vnode and name to remove.
 *
 * Called with unlocked vnode (dvp).
 */

static
nfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsdiropargs da;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rmdir %s %o %d '%s'\n",
		vtomi(dvp)->mi_hostname,
		vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm);
#endif	NFSDEBUG

	setdiropargs(&da, nm, dvp);
	/*
	 * remove name from dnlc.
	 */
	VN_LOCKNODE(dvp);
	dnlc_remove(dvp, nm);

	/*
	 * Hold attributes while zapping directory.
	 */
	RLOCK_ATTR(vtor(dvp));
	error = rfscall(vtomi(dvp), RFS_RMDIR, xdr_diropargs, (caddr_t)&da,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	RUNLOCK_ATTR(vtor(dvp));
	if (!error) {
		error = geterrno(status);
		L_CHECK_STALE_FH(error, dvp);
	}
	VN_UNLOCKNODE(dvp);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rmdir returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_symlink()
 *	Create a symbolic link.
 *
 * Called with unlocked vnode (dvp).
 */

static
nfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsslargs args;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_symlink %s %o %d '%s' to '%s'\n",
		vtomi(dvp)->mi_hostname,
		vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, lnm, tnm);
#endif	NFSDEBUG

	setdiropargs(&args.sla_from, lnm, dvp);
	vattr_to_sattr(tva, &args.sla_sa);
	args.sla_tnm = tnm;
	RLOCK_ATTR(vtor(dvp));
	error = rfscall(vtomi(dvp), RFS_SYMLINK, xdr_slargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	RUNLOCK_ATTR(vtor(dvp));
	if (!error) {
		error = geterrno(status);
		CHECK_STALE_FH(error, dvp);
	}

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_sysmlink: returning %d\n", error);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_csymlink()
 *	create a conditional symobic link
 *	NOT SUPPORTED
 */
/*ARGSUSED*/
static int
nfs_csymlink(dvp, lnm, vap, ucbnm, attnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *vap;
	char *ucbnm;
	char *attnm;
	struct ucred *cred;
{
	return (EOPNOTSUPP);
}

/*
 * nfs_readdir
 *	Read directory entries.
 *
 * There are some weird things to look out for here.  The uio_offset
 * field is either 0 or it is the offset returned from a previous
 * readdir.  It is an opaque value used by the server to find the
 * correct directory block to read.  The byte count must be at least
 * vtoblksz(vp) bytes.  The count field is the number of blocks to
 * read on the server.  This is advisory only, the server may return
 * only one block's worth of entries.  Entries may be compressed on
 * the server.
 *
 * Called with locked vnode (vp).
 */
static
nfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	int error = 0;
	struct iovec *iovp;
	unsigned count;
	struct rnode *rp;
	struct nfsrddirargs rda;
	struct nfsrddirres  rd;

	rp = vtor(vp);
	if ((rp->r_flags & REOF) && (rp->r_size == (u_long)uiop->uio_offset)) {
		return (0);
	}
	iovp = uiop->uio_iov;
	count = iovp->iov_len;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_readdir %s %o %d count %d offset %d\n",
		vtomi(vp)->mi_hostname,
		vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno, count, uiop->uio_offset);
#endif	NFSDEBUG

	/*
	 * XXX We should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}
	count = MIN(count, vtomi(vp)->mi_tsize);
	rda.rda_count = count;
	rda.rda_offset = uiop->uio_offset;
	rda.rda_fh = *vtofh(vp);
	rd.rd_size = count;
	rd.rd_entries = (struct direct *)kmem_alloc((u_int)count);

	error = rfscall(vtomi(vp), RFS_READDIR, xdr_rddirargs, (caddr_t)&rda,
			xdr_getrddirres, (caddr_t)&rd, cred);
	if (!error) {
		error = geterrno(rd.rd_status);
		L_CHECK_STALE_FH(error, vp);
	}
	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (rd.rd_size) {
			error = uiomove((caddr_t)rd.rd_entries, (int)rd.rd_size,
					UIO_READ, uiop);
			rda.rda_offset = rd.rd_offset;
			uiop->uio_offset = rd.rd_offset;
		}
		if (rd.rd_eof) {
			rp->r_flags |= REOF;
			/*
	 		 * Unnecessary to lock to update r_size since
			 * already hold vnode locked.
			 */
			rp->r_size = uiop->uio_offset;
		}
	}
	kmem_free((caddr_t)rd.rd_entries, (u_int)count);

#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_readdir: returning %d resid %d, offset %d\n",
		error, uiop->uio_resid, uiop->uio_offset);
#endif	NFSDEBUG

	return (error);
}

/*
 * nfs_bmap()
 *	Return "physical" block number of an rnode for a given logical
 *	block number.
 *
 * Called with locked vnode (vp).
 */
/*ARGSUSED*/
static int
nfs_bmap(vp, bn, vpp, bnp, rwflg, size)
	struct vnode *vp;	/* file's vnode */
	daddr_t bn;		/* fs block number */
	struct vnode **vpp;	/* RETURN vp same as files vnode */
	daddr_t *bnp;		/* RETURN block number */
	int rwflg;
	int size;
{
#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_bmap %s %o %d blk %d\n",
		vtomi(vp)->mi_hostname,
		vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno, bn);
#endif	NFSDEBUG

	if (vpp)
		*vpp = vp;
	if (bnp) {
		*bnp = bn * (vtoblksz(vp) >> DEV_BSHIFT);
		if ((rwflg & B_READ) && (*bnp * DEV_BSIZE) >= vtor(vp)->r_size)
			*bnp = (daddr_t)-1;
	}
	return (0);
}

/*
 * nfs_minphys
 *	nfs will not limit request size. Thus nfs_minphys() is nop.
 */

/*ARGSUSED*/
static int
nfs_minphys(bp)
	struct buf *bp;
{
	return (0);
}

struct buf *async_bufhead;
lock_t async_buf_lock;
sema_t async_daemon_sema;

#ifdef	PERFSTAT
long	nstrat_calls;
long	nstrat_bphys;
long	nstrat_asyncself;
long	nstrat_asyncd;
#endif	PERFSTAT

#include "../h/vmmac.h"

static int
nfs_strategy(bp)
	register struct buf *bp;
{
	spl_t	s_ipl;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_strategy bp %x\n", bp);
#endif	NFSDEBUG

#ifdef	PERFSTAT
	nstrat_calls++;
#endif	PERFSTAT
	if (bp->b_flags & (B_UAREA|B_PAGET))
		panic("nfs_strategy: swapping to nfs");
	/*
	 * Note that mmap support will set B_DIRTY bit. So we could
	 * strengthen the above test by also testing
	 * if ((bp->b_flags & B_DIRTY) && !(bp->vp->v_flag & VMAPPED))
	 * 	panic("nfs_strategy: swapping to nfs");
	 */

	/*
	 * If there was an asynchronous write error on this rnode
	 * then we just return the old error code. This continues
	 * until the rnode goes away (zero ref count). We do this because
	 * there can be many procs writing this rnode.
	 */
	if (vtor(bp->b_vp)->r_error && ((bp->b_flags & B_READ) != B_READ)) {
		bp->b_error = vtor(bp->b_vp)->r_error;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	if (bp->b_flags & B_PHYS) {
		register struct pte *pte, *kpte;
		register int n;
		register int npte;
		register long a;
		caddr_t va;
		long	npages;
		caddr_t saddr;

		if (bp->b_vp->v_flag & VMAPPED) {
			struct rnode *rp;
			struct ucred *tmpcr;

			/*
			 * Set up the credentials, if there currently
			 * are none.
			 */
			rp = vtor(bp->b_vp);
			if (rp->r_cred == NULL) {
				RLOCK_ATTR(rp);
				s_ipl = RN_LOCKCRED(rp);
				/*
				 * Check again in case race with nfs_rdwr().
				 * The following is a hideous KLUDGE!!! It is 
				 * used since the pageout() process may write
				 * dirty pages in an mmap'd file where
				 * no cred has been placed in the rnode
				 * by a normal process doing I/O. See
				 * nfs_rdwr(). If pageout() attempts to use
				 * its credentials and super-user access
				 * is denied by the server, the process
				 * for whom the I/O is being done will be
				 * killed do to a swapio error.
				 * Hence, the kludge! This must be better
				 * thought out in a future release.
				 */
				if (rp->r_cred == NULL) {
					/*
					 * If not a system process then go
					 * with current process's cred.
					 */
					if ((u.u_procp->p_flag & SSYS) == 0) {
						crhold(u.u_cred);
						rp->r_cred = u.u_cred;
					} else {
						/*
						 * If a system process 
						 * (eg pageout, swapper), then
						 * construct a cred that
						 * will pass as rnode owner.
						 * Ie construct credentials
						 * from the rnode's attributes.
						 * This assumes the rnode's
						 * attributes are valid.
						 */
						tmpcr = crget();
						tmpcr->cr_ref = 1;
						init_lock(&tmpcr->cr_lock,
							  G_CRED);
						tmpcr->cr_uid =
							rp->r_nfsattr.na_uid;
						tmpcr->cr_gid =
							rp->r_nfsattr.na_gid;
						tmpcr->cr_ruid = tmpcr->cr_uid;
						tmpcr->cr_rgid = tmpcr->cr_gid;
						for (n = NGROUPS-1; n; n--)
							tmpcr->cr_groups[n]
								= NOGROUP;
						tmpcr->cr_groups[0] =
							tmpcr->cr_gid;
						rp->r_cred = tmpcr;
					}
				}
				RN_UNLOCKCRED(rp, s_ipl);
				RUNLOCK_ATTR(rp);
			}
			/*
			 * rsize is the maximum number of bytes known.
			 * Make sure it is at least as high as the last
			 * byte about to be transferred.
			 */
			n = (bp->b_blkno << DEV_BSHIFT) + bp->b_bcount;
			if (rp->r_size < n) {
				RLOCK_ATTR(rp);
				if (rp->r_size < n)
					rp->r_size = n;
				RUNLOCK_ATTR(rp);
			}
		}
		/*
		 * Buffer's data is in userland, or in some other
		 * currently inaccessable place. We get a hunk of
		 * kernel address space and map it in.
		 *
		 * B_PHYS implies B_PTEIO.
		 */
		npte = btoc(bp->b_bcount);
		npages = clrnd(npte);
		a = uptalloc(npages, 1);

		flush_tlb();

		kpte = &Usrptmap[a];
		pte = bp->b_un.b_pte;
		for (n = npte; n--; kpte++, pte++)
			*(int *)kpte = *(int *)pte & PG_PFNUM;
		va = (caddr_t)kmxtob(a);
		vmaccess(&Usrptmap[a], va, npte);
		saddr = bp->b_un.b_addr;
		bp->b_un.b_addr = va;
		/*
		 * do the io
		 */
#ifdef	PERFSTAT
		nstrat_bphys++;
#endif	PERFSTAT
		do_bio(bp);
		/*
		 * Free resources.
		 */
		bp->b_un.b_addr = saddr;
		iodone(bp);
		kpte = &Usrptmap[a];
		for (n = npte; n-- ; kpte++)
			*(int *)kpte = 0;
		uptfree(npages, a);
		return;
	}

	/*
	 * Not physical I/O.
	 */
	if (blocked_sema(&async_daemon_sema) && (bp->b_flags & B_ASYNC)) {
		s_ipl = p_lock(&async_buf_lock, SPLBUF);
		if (!blocked_sema(&async_daemon_sema)) {
			/*
			 * Either all daemon processes are busy or have
			 * been killed.
			 */
			v_lock(&async_buf_lock, s_ipl);
#ifdef	PERFSTAT
			nstrat_asyncself++;
#endif	PERFSTAT
			do_bio(bp);
			return;
		}

		/*
		 * Queue request for an async_daemon process.
		 */
#ifdef	PERFSTAT
		nstrat_asyncd++;
#endif	PERFSTAT
		if (async_bufhead) {
			register struct buf *bp1;

			/*
			 * Attach to the end.
			 */
			bp1 = async_bufhead;
			while (bp1->b_actf) {
				bp1 = bp1->b_actf;
			}
			bp1->b_actf = bp;
		} else {
			async_bufhead = bp;
		}
		bp->b_actf = NULL;
		v_sema(&async_daemon_sema);
		v_lock(&async_buf_lock, s_ipl);
	} else {
#ifdef	PERFSTAT
		if (bp->b_flags & B_ASYNC)
			nstrat_asyncself++;
#endif	PERFSTAT
		do_bio(bp);
	}
}

/*
 * async_daemon() system call.
 *
 * Called by /etc/biod to handle B_ASYNC I/O.
 */
async_daemon()
{
	register struct buf *bp;
	register spl_t	s_ipl;

	/*
	 * First release resources.
	 */
	if ((u.u_procp->p_flag & SVFORK) == 0) {
		++u.u_procp->p_noswap;
		vrelvm();
		--u.u_procp->p_noswap;
	}

	if (setjmp(&u.u_qsave)) {
		/*
		 * Must lock list and drain any I/Os in case this is
		 * the last async_daemon process. Should only be a max
		 * of 1 queued request per async_daemon process. So each
		 * process need only worry about performing 1 request.
		 * do_bio must not block on a semaphore with a signallable
		 * priority.
		 */
		s_ipl = p_lock(&async_buf_lock, SPLBUF);
		if (async_bufhead != NULL) {
			bp = async_bufhead;
			async_bufhead = bp->b_actf;
			v_lock(&async_buf_lock, s_ipl);
			do_bio(bp);
			exit(0);
		}
		v_lock(&async_buf_lock, s_ipl);
		exit(0);
	}

	/*
	 * Forever service B_ASYNC I/O. Only a signal
	 * will terminate loop.
	 */
	for (;;) {
		s_ipl = p_lock(&async_buf_lock, SPLBUF);
		if (async_bufhead != NULL) {
			bp = async_bufhead;
			async_bufhead = bp->b_actf;
			v_lock(&async_buf_lock, s_ipl);
			do_bio(bp);
		} else
			v_lock(&async_buf_lock, s_ipl);
		p_sema(&async_daemon_sema, PZERO + 1);
	}
}

static
do_bio(bp)
	register struct buf *bp;
{
	register struct rnode *rp = vtor(bp->b_vp);
	int count, resid;
	struct ucred *cred;
	spl_t	s_ipl;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4,
		"do_bio: addr %x, blk %d, offset %d, size %d, B_READ %d\n",
		bp->b_un.b_addr, bp->b_blkno, bp->b_blkno * DEV_BSIZE,
		bp->b_bcount, bp->b_flags & B_READ);
#endif	NFSDEBUG

	/*
	 * Nail down a cred.
	 */
	s_ipl = RN_LOCKCRED(rp);
	cred = rp->r_cred;
	crhold(cred);
	RN_UNLOCKCRED(rp, s_ipl);

	/*
	 * Lock attributes here. nfsread() or nfswrite() will unlock.
	 */
	RLOCK_ATTR(rp);
	if ((bp->b_flags & B_READ) == B_READ) {
		bp->b_error = nfsread(bp->b_vp, bp->b_un.b_addr,
					(int)bp->b_blkno * DEV_BSIZE,
					(int)bp->b_bcount, &resid, cred);
		if (bp->b_vp->v_badmap) {
			/*
			 * Fail I/O because of stale text.
			 */
			bp->b_flags |= B_ERROR;
			
		}
		if (resid) {
			bzero(bp->b_un.b_addr + bp->b_bcount - resid,
			      (u_int)resid);
		}
	} else {
		/*
		 * If the write fails and it was asynchronous
		 * all future writes will get an error.
		 */
		if (rp->r_error == 0) {
			count = rp->r_size - (bp->b_blkno * DEV_BSIZE);
			count = MIN(bp->b_bcount, count);
			ASSERT(count >= 0, "do_bio: write count < 0");
			bp->b_error = nfswrite(bp->b_vp, bp->b_un.b_addr,
						(int)bp->b_blkno * DEV_BSIZE,
						count, cred);
			if (bp->b_error && (bp->b_flags & B_ASYNC)) {
				rp->r_error = bp->b_error;
			}
		} else {
			RUNLOCK_ATTR(rp);
			bp->b_error = rp->r_error;
		}
	}

	if (bp->b_error) {
		bp->b_flags |= B_ERROR;
		/*
		 * Stale filehandle the mapped file is bad.
		 */
		if (bp->b_error == ESTALE && (bp->b_flags & B_PHYS) &&
		    (bp->b_vp->v_flag & VMAPPED))
			bp->b_vp->v_badmap |= MM_IOERR;
	}

	crfree(cred);
	if ((bp->b_flags & B_PHYS) == 0)
		iodone(bp);
}

/*
 * Bad news!
 */
static int
nfs_badop()
{
	panic("nfs_badop");
}
#endif	NFS
