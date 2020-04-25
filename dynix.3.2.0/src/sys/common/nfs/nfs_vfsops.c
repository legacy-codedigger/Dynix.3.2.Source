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
static	char	rcsid[] = "$Header: nfs_vfsops.c 1.5 89/09/25 $";
#endif	lint

/*
 * nfs_vfsops.c
 *	nfs vfs operations.
 *
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* $Log:	nfs_vfsops.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../h/mount.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"


#ifdef	NFSDEBUG
extern int nfsdebug;
#endif	NFSDEBUG

static int nfsmntno;		/* mutex'd with vfs_mutex sema */

/*
 * nfs vfs operations.
 */
static int nfs_mount();
static int nfs_unmount();
static int nfs_root();
static int nfs_statfs();
static int nfs_sync();

struct vfsops nfs_vfsops = {
	nfs_mount,
	nfs_unmount,
	nfs_root,
	nfs_statfs,
	nfs_sync,
};

/*
 * nfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 *
 * Called with vfsp->vfs_vnodecovered locked.
 * vfsp points to copy. nfs_mount() must vfs_add real vfs.
 */
/*ARGSUSED*/
static int
nfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;		/* unused */
	caddr_t data;
{
	register struct mntinfo *mi = NULL; /* mount info, pointed at by vfs */
	register int error;
	struct vnode *rootvp = NULL;	/* the server's root */
	struct vattr va;		/* root vnode attributes */
	struct nfsfattr na;		/* root vnode attributes in nfs form */
	struct statfs sb;		/* server's file system stats */
	fhandle_t fh;			/* root fhandle */
	struct nfs_args args;		/* nfs mount arguments */
	struct vnode *makenfsnode();

	/*
	 * get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof(args));
	if (error) {
		return (error);
	}

	/*
	 * create a mount record and link it to the vfs struct
	 */
	mi = (struct mntinfo *)kmem_alloc((u_int)sizeof(*mi));
	mi->mi_refct = 0;
	mi->mi_stsize = 0;
	mi->mi_hard = ((args.flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((args.flags & NFSMNT_INT) == NFSMNT_INT);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errout;
		}
	} else {
		mi->mi_retrans = NFS_RETRIES;
	}
	if (args.flags & NFSMNT_TIMEO) {
		mi->mi_timeo = args.timeo;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errout;
		}
	} else {
		mi->mi_timeo = NFS_TIMEO;
	}
	mi->mi_printed = 0;
	error = copyin((caddr_t)args.addr, (caddr_t)&mi->mi_addr,
			sizeof(mi->mi_addr));
	if (error) {
		goto errout;
	}
	/*
	 * For now we just support AF_INET
	 */
	if (mi->mi_addr.sin_family != AF_INET) {
		error = EPFNOSUPPORT;
		goto errout;
	}
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyin((caddr_t)args.hostname, (caddr_t)mi->mi_hostname,
				HOSTNAMESZ);
		if (error) {
			goto errout;
		}
	} else {
		addr_to_str(&(mi->mi_addr), mi->mi_hostname);
	}

	vfsp->vfs_data = (caddr_t)mi;
	mi->mi_vfs = *vfsp;		/* copy vfs into mount table */

	/*
	 * Make the root vnode
	 */
	error = copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof(fh));
	if (error) {
		goto errout;
	}
	/*
	 * Must lock vfs_mutex as nfsmntno is mutex'd via vfs_mutex sema.
	 */
	p_sema(&vfs_mutex, PVFS);
	mi->mi_mntno = nfsmntno++;
	v_sema(&vfs_mutex);

	rootvp = makenfsnode(&fh, (struct nfsfattr *)0, &mi->mi_vfs);

	/*
	 * get attributes of the root vnode then remake it to include 
	 * the attributes.
	 */
	error = VOP_GETATTR(rootvp, &va, u.u_cred);
	VN_PUT(rootvp);
	if (error)
		goto errout;
	vattr_to_nattr(&va, &na);
	rootvp = makenfsnode(&fh, &na, &mi->mi_vfs);
	rootvp->v_flag |= VROOT;
	mi->mi_rootvp = rootvp;
	VN_UNLOCKNODE(rootvp);		/* still referenced */

	/*
	 * Get server's filesystem stats.  Use these to set transfer
	 * sizes, filesystem block size, and read-only.
	 */
	error = VFS_STATFS(&mi->mi_vfs, &sb);
	if (error)
		goto errout2;
	mi->mi_tsize = NFS_MAXDATA;
	if (args.flags & NFSMNT_RSIZE) {
		if (args.rsize <= 0) {
			error = EINVAL;
			goto errout2;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, args.rsize);
	}
	if (args.flags & NFSMNT_WSIZE) {
		if (args.wsize <= 0) {
			error = EINVAL;
			goto errout2;
		}
		mi->mi_stsize = MIN(mi->mi_stsize, args.wsize);
	}
#ifdef	NFSDEBUG
	dprint(nfsdebug, 1,
		"nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
		mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
		mi->mi_tsize);
#endif	NFSDEBUG
	/*
	 * Should set read only here!
	 */

	/*
	 * Set filesystem block size to at least CLBYTES and at most MAXBSIZE
	 */
	mi->mi_bsize = MAX(va.va_blocksize, CLBYTES);
	mi->mi_bsize = MIN(mi->mi_bsize, MAXBSIZE);
	mi->mi_vfs.vfs_bsize = mi->mi_bsize;
	/*
	 * Add to vfs list.
	 */
	p_sema(&vfs_mutex, PVFS);
	vfs_add(&mi->mi_vfs, 0);
	v_sema(&vfs_mutex);
	return (0);

errout2:
	/*
	 * bad news. Drop rootvp and free mount entry memory.
	 */
	VN_RELE(rootvp);
errout:
	kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
	return (error);
}

/*
 * unmount vfs op
 *
 * Called with unmount_mutex held. This excludes any concurrent unmount.
 */
static int
nfs_unmount(vfsp)
	struct vfs *vfsp;
{
	struct mntinfo *mi = (struct mntinfo *)vfsp->vfs_data;
	register struct vnode *vp;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_unmount(%x) mi = %x\n", vfsp, mi);
#endif	NFSDEBUG

	/*
	 * Holding the covered vnode locked is sufficient to ensure that
	 * no other processes could venture into the fs being unmounted.
	 */
	vp = vfsp->vfs_vnodecovered;
	if ((vp == NULLVP) || !VN_TRYLOCKNODE(vp))
		return (EBUSY);

	rflush();

	/*
	 * free vnodes held in buffer cache. Note that there is a VERY small
	 * probability that rinval() can miss invalidating all non-busy rnodes
	 * if it races with some concurrent processes traversing around within
	 * the vfs. However, the filesystem will not be unmounted because the
	 * mi_refct will be greater than 1. Thus, no data will be lost.
	 */
	if (mi->mi_refct != 1) {
		rinval(vfsp);
	}

	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1 ||
	    vp->v_count > 1) {
		VN_UNLOCKNODE(vp);
		return (EBUSY);
	}
	/*
	 * vfs_remove() is called with the covered vnode still locked, to
	 * ensure that no one enters the fs being unmounted until
	 * vp->vfsmountedhere is cleared, after which the filesystem is
	 * inaccessible.
	 *
	 * This relies on the lookup routine holding a ref to an vnode until
	 * it holds a ref on vnode being looked up, and vnode allocation
	 * holding a ref to the parent directory until the vnode is allocated
	 * and held. These rules plus the fact that mi_refct == 1 and the
	 * root vnode is only held once (by the mount), then the file-system
	 * may be safely unmounted (there are no in-use entries and none
	 * being concurrently created).
	 */
	VN_RELE(mi->mi_rootvp);
	p_sema(&vfs_mutex, PVFS);
	vfs_remove(vfsp);
	v_sema(&vfs_mutex);
	VN_UNLOCKNODE(vp);
	kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
	return (0);
}

/*
 * find root of nfs
 *
 * Return with locked vnode (*vpp).
 */
static int
nfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{

	*vpp = (struct vnode *)((struct mntinfo *)vfsp->vfs_data)->mi_rootvp;
	VN_LOCKNODE(*vpp);
	VN_HOLD(*vpp);
#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_root(0x%x) = %x\n", vfsp, *vpp);
#endif	NFSDEBUG
	return (0);
}

/*
 * Get file system statistics.
 *
 * called via nfs_mount or [f]statfs system calls. A vnode in the filesystem
 * is held (either locked or unlocked) by the caller, thus no unmount
 * can happen.
 */
static int
nfs_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{
	struct mntinfo *mi;
	fhandle_t *fh;
	int error = 0;
	struct nfsstatfs fs;

	mi = vftomi(vfsp);
	fh = vtofh(mi->mi_rootvp);
#ifdef	NFSDEBUG
	dprint(nfsdebug, 4, "nfs_statfs fh %o %d\n", fh->fh_fsid, fh->fh_fno);
#endif	NFSDEBUG
	error = rfscall(mi, RFS_STATFS, xdr_fhandle,
	    (caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		if (mi->mi_stsize == 0) {
			mi->mi_stsize = fs.fs_tsize;
		}
		sbp->f_bsize = fs.fs_bsize;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		sbp->f_files = -1;
		sbp->f_ffree = -1;
		/*
		 * XXX This is wrong - should be a real fsid
		 */
		bcopy((caddr_t)&fh->fh_fsid, (caddr_t)sbp->f_fsid,
		    sizeof(fsid_t));
	}
#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_statfs returning %d\n", error);
#endif	NFSDEBUG
	return (error);
}

/*
 * Flush any pending I/O.
 * Nothing to do here as upper level code will flush the buffer cache.
 */
static int
nfs_sync()
{
#ifdef	NFSDEBUG
	dprint(nfsdebug, 5, "nfs_sync\n");
#endif	NFSDEBUG
	return (0);
}

static char *
itoa(n, str)
	u_short n;
	char *str;
{
	char prbuf[11];
	register char *cp;

	cp = prbuf;
	do {
		*cp++ = "0123456789"[n%10];
		n /= 10;
	} while (n);
	do {
		*str++ = *--cp;
	} while (cp > prbuf);
	return (str);
}

/*
 * Convert a INET address into a string for printing
 */
static
addr_to_str(addr, str)
	struct sockaddr_in *addr;
	char *str;
{
	str = itoa(addr->sin_addr.s_net, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_host, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_lh, str);
	*str++ = '.';
	str = itoa(addr->sin_addr.s_impno, str);
	*str = '\0';
}
#endif	NFS
