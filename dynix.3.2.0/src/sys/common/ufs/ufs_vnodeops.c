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
static	char	rcsid[] = "$Header: ufs_vnodeops.c 2.45 1991/06/14 22:11:26 $";
#endif

/*
 * ufs_vnodeops.c
 *	UNIX file-system vnode interface procedures and vnodeops structure
 *	definition.
 */

/* $Log: ufs_vnodeops.c,v $
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fsdir.h"
#include "../h/vm.h"		/* For plocal */
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

#include "../machine/pte.h"	/* For plocal */
#include "../machine/plocal.h"	/* For l struct (rablock, etc) */
#include "../machine/intctl.h"	/* For SPLs */

/*
 * ufs_open()
 *	Open a UNIX file-system vnode.
 *
 * Call device driver open procedure for character and block special files.
 *
 * Caller passes *vpp unlocked.  It remains unlocked at exit.
 */

static
ufs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	struct inode *ip;
	dev_t dev;
	register int maj;
	int error;

	/*
	 * Setjmp in case open is interrupted.
	 * If it is, close and return error.
	 */

	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		(void) ufs_close(*vpp, flag & FMASK, cred);
		return (error);
	}
	ip = VTOI(*vpp);

	/*
	 * Do open protocol for inode type.
	 */

	dev = ip->i_rdev;
	maj = major(dev);

	switch (ip->i_mode & IFMT) {

	case IFCHR:
		if ((u_int)maj >= nchrdev)
			return (ENXIO);
		error = (*cdevsw[maj].d_open)(dev, flag);
		break;

	case IFBLK:
		if ((u_int)maj >= nblkdev)
			return (ENXIO);
		error = (*bdevsw[maj].d_open)(dev, flag);
		break;

	case IFSOCK:
		error = EOPNOTSUPP;
		break;

	case IFIFO:
		ILOCK(ip);
		error = openp(ip, flag);
		IUNLOCK(ip);
		break;
	
	default:
		error = 0;
		break;
	}
	return (error);
}

/*
 * ufs_close()
 *	Close a UNIX file-system vnode.
 *
 * For IFBLK and IFCHR special-file inodes, call the close
 * procedure *unconditionally* (eg, "every open, every close").
 * This implies no special checks for other opens of the device,
 * no scan of file-table.
 *
 * Caller passes vp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_close(vp, flag, cred)
	struct	vnode	*vp;
	int	flag;
	struct	ucred	*cred;			/* not used */
{
	register struct inode *ip;
	register struct vnode *dev_vp;
	register dev_t	dev;
	register struct mount *mp;
	int	(*cfunc)();

	/*
	 * setjmp in case close is interrupted
	 */

	if (setjmp(&u.u_qsave))
		return (EINTR);

	ip = VTOI(vp);
	dev = ip->i_rdev;

	switch (ip->i_mode & IFMT) {

	case IFCHR:
		cfunc = cdevsw[major(dev)].d_close;
		break;

	case IFBLK:
		/*
		 * If device is mounted, no need to flush and inval
		 * (other use exists).  If not, need to bflush() and
		 * binval() to clean up.
		 *
		 * Note: low probability (non-harmful) race with
		 * concurrent mount or unmount (worst case is redundant
		 * flush/inval).
		 */
		for (mp = mounttab; mp < mountNMOUNT; mp++) {
			if (mp->m_bufp != NULL && dev == mp->m_dev)
				break;
		}
		if (mp == mountNMOUNT) {		/* ie, not found */
			DEVTOVP(dev, dev_vp);
			bflush(dev_vp);
			binval(dev_vp);
		}
		cfunc = bdevsw[major(dev)].d_close;
		break;

	case IFIFO:
		ILOCK(ip);
		closep(ip, flag);
		IUNLOCK(ip);
		return (0);

	default:
		return (0);
	}
	/*
	 * Close the device.
	 */
	(*cfunc)(dev, flag);
	return (0);
}

/*
 * ufs_rdwr()
 *	Read or write a vnode.
 *
 * Only do "append" mode if regular file.
 *
 * Caller locks vp depending on type.  See vno_rw().
 */

/*ARGSUSED*/
static
ufs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;			/* not used */
{
	register struct inode *ip;

	ip = VTOI(vp);
	if ((ioflag & IO_APPEND) && rw == UIO_WRITE && vp->v_type == VREG) {
		/*
		 * in append mode start at end of file.
		 */
		uiop->uio_offset = ip->i_size;
	}
	return (rwip(ip, uiop, rw, ioflag));
}

/*
 * rwip()
 *	Read/Write an inode, using a uio structure.
 *
 * Caller insures sufficient locking on argument inode.  This implies
 * locked inode for RW of a normal file, FIFO, or directory, no locking
 * necessary for IFBLK, IFCHR files.  No problem setting bits in i_flag
 * (eg, IMARK()), since only possible harmful race is with IFREE and this
 * can't happen since calling process has a reference to the inode.
 *
 * SUN kernel clears u_error here (see below).  The only place that really
 * needs this is acct(), so we clear it there.
 */

rwip(ip, uio, rw, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	register int n;
	register int on;
	register int type;
	dev_t	dev = (dev_t)ip->i_rdev;
	struct	vnode *devvp;
	struct buf *bp;
	struct fs *fs;
	daddr_t	lbn, bn;
	int	size;
	int	noclrused;
	long	bsize;
	long	osize;
	int	error = 0;
	unsigned long usave;
	extern int mem_no;
	struct	vnode	*vp = ITOV(ip);
	int syncIO = ioflag & IO_SYNC? B_SYNC : 0;

	ASSERT(rw == UIO_READ || rw == UIO_WRITE, "rwip");
	/*
	 *+ The kernel called the function that reads/writes files, but
	 *+ the operation requested was neither a read nor a write.
	 */
	if (rw == UIO_READ && uio->uio_resid == 0)
		return (0);
	if ((uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0)
	&&  ((ip->i_mode&IFMT) != IFCHR || mem_no != major(dev)))
		return (EINVAL);
	/*
	 * Kludge to not set iacc if doing a stat on dirs. Temp.
	 * This is only set by sizedir_att(vfs_att.c).
	 */
	if (rw == UIO_READ && (u.u_tuniverse & U_NOACC) == 0)
		IMARK(ip, IACC);
	type = ip->i_mode&IFMT;
	if (type == IFCHR) {
		if (rw == UIO_READ)
			error = (*cdevsw[major(dev)].d_read)(dev, uio);
		else {
			IMARK(ip, IUPD|ICHG);
			error = (*cdevsw[major(dev)].d_write)(dev, uio);
		}
		return (error);
	}
	if (uio->uio_resid == 0)
		return (0);
	if (rw == UIO_WRITE && type == IFREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}

floop:
	if (type == IFIFO) {
		spl_t s;

		if (rw == UIO_READ) {
			while (ip->i_psize == 0) {
				if (ip->i_fwcnt == 0)
					return (0);
				if (ioflag&IO_NDELAY)
					return (EWOULDBLOCK);
				s = p_lock(&ip->i_frwlock, SPLFS);
				IUNLOCK(ip);
				p_sema_v_lock(&ip->i_rsema, PPIPE,
							&ip->i_frwlock, s);
				ILOCK(ip);
			}
			uio->uio_offset = ip->i_frptr;
		} else {
			usave = 0;
			while ((uio->uio_resid+ip->i_psize) > PIPSIZ) {
				if (ip->i_frcnt == 0)
					break;
				if (uio->uio_resid > PIPSIZ && 
				    ip->i_psize < PIPSIZ) {
					usave = uio->uio_resid;
					uio->uio_resid = PIPSIZ - ip->i_psize;
					usave -= uio->uio_resid;
					break;
				}
				if (ioflag&IO_NDELAY)
					return (EWOULDBLOCK);
				s = p_lock(&ip->i_frwlock, SPLFS);
				IUNLOCK(ip);
				p_sema_v_lock(&ip->i_wsema, PPIPE,
							&ip->i_frwlock, s);
				ILOCK(ip);
			}
			if (ip->i_frcnt == 0) {
				u.u_error = EPIPE;
				psignal(u.u_procp, SIGPIPE);
				return (EPIPE);
			}
			uio->uio_offset = ip->i_fwptr;
		}
	}

	if (type != IFBLK) {
		devvp = ip->i_devvp;
		fs = ip->i_fs;
		bsize = fs->fs_bsize;
	} else {
		DEVTOVP(dev, devvp);
		bsize = BLKDEV_IOSIZE;
	}
	u.u_error = 0;
	do {
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
		if (type != IFBLK) {
			/*
			 * If inode is mapped and overlap, use "shared"
			 * pages to do the IO.  If/when do "i_lsize"
			 * might need to restrict `n' on reads.
			 */
			if ((vp->v_flag&VMAPPED)
			&&  mmreg_rw(vp, lbn, rw, on, n, uio))
				continue;
			if (rw == UIO_READ) {
				int diff; 
				if (type == IFIFO) {
					if (ip->i_frptr < ip->i_fwptr &&
						ip->i_psize < n)
						n = ip->i_psize;
				} else {
					diff = ip->i_size - uio->uio_offset;
					if (diff <= 0)
						return (0);
					if (diff < n)
						n = diff;
				}
				bn = fsbtodb(fs, bmap(ip, lbn, B_READ, on+n));
				if (u.u_error)
					return (u.u_error);
			} else {
				if (n == bsize && type == IFREG) {
					noclrused = 1;
					bn = fsbtodb(fs, bmap(ip, lbn, B_WRITE|B_NOCLR|syncIO, n));
				} else {
					noclrused = 0;
					bn = fsbtodb(fs, bmap(ip, lbn, B_WRITE|syncIO, on+n));
				}
				if (u.u_error || (long)bn<0)
					return (u.u_error);
				osize = ip->i_size;
				if (uio->uio_offset + n > osize
				&&  (type == IFDIR || type == IFREG || type == IFLNK))
					ip->i_size = uio->uio_offset + n;
			}
			size = blksize(fs, ip, lbn);
		} else {
			bn = lbn * (BLKDEV_IOSIZE/DEV_BSIZE);
			l.rablock = bn + (BLKDEV_IOSIZE/DEV_BSIZE);
			l.rasize = size = bsize;
		}
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk((int)size);
				clrbuf(bp);
			} else if (ip->i_lastr + 1 == lbn)
				bp=breada(devvp, bn, (int)size, 
					l.rablock, l.rasize);
			else
				bp = bread(devvp, bn, (int)size);
			ip->i_lastr = lbn;
		} else {
 	  		if (n == bsize) {
 		  		bp = getblk(devvp, bn, size, 0);
		 		bp->b_resid  = 0;
			} else if (type == IFIFO && on == 0 && ip->i_psize < (PIPSIZ-bsize)){
				bp = getblk(devvp, bn, size, 0);
				bp->b_resid  = 0;
			}
			else
				bp = bread(devvp, bn, (int)size);
		}
		n = MIN(n, size - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error =
		    uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if ((u.u_error != 0) && (noclrused == 1)) {
			clrbuf(bp);
		}
		if (rw == UIO_READ) {
			if (type == IFIFO) {
				ip->i_psize -= n;
				if (uio->uio_offset >= PIPSIZ)
					uio->uio_offset = 0;
				if ((on+n) == bsize && ip->i_psize < (PIPSIZ-bsize))
					bp->b_flags &= ~B_DELWRI;
			} else if (n + on == bsize || uio->uio_offset == ip->i_size)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} else {
			if (type == IFIFO) {
				bdwrite(bp);
				ip->i_psize += n;
				if (uio->uio_offset == PIPSIZ)
					uio->uio_offset = 0;
			}
			else if (syncIO || type == IFDIR)
				bwrite(bp);
			else if (n + on == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
			if (type != IFBLK)
				vp->v_flag |= VMAPSYNC;	/* fsync before map */
			IMARK(ip, IUPD|ICHG);
			if (u.u_ruid != 0)
				ip->i_mode &= ~(ISUID|ISGID);
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);

	/*
	 * For sync I/O which has modified the inode, post the
	 * inode out to disk immediately.  Inhibit this action for
	 * inodes which vno_rw() has not locked for us--in particular,
	 * device-special files; iupdat() would otherwise panic.
	 */
	if (	syncIO &&
		rw == UIO_WRITE &&
		(ip->i_flag & (IUPD|ICHG)) &&
		vno_rw_locked(vp->v_type)) {
			iupdat(ip, 1);
	}
	if (type == IFIFO) {
		spl_t s;

		if (rw == UIO_READ) {
			if (ip->i_psize)
				ip->i_frptr = uio->uio_offset;
			else
				ip->i_frptr = ip->i_fwptr = 0;
			s = p_lock(&ip->i_frwlock, SPLFS);
			vall_sema(&ip->i_wsema);
			v_lock(&ip->i_frwlock, s);
		} else {
			ip->i_fwptr = uio->uio_offset;
			s = p_lock(&ip->i_frwlock, SPLFS);
			vall_sema(&ip->i_rsema);
			v_lock(&ip->i_frwlock, s);
			if (u.u_error==0 && usave!=0) {
				uio->uio_resid = usave;
				goto floop;
			}
		}
	}
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	if (error && type == IFREG && rw == UIO_WRITE)
		itrunc(ip, (u_long)osize);	/* in case B_NOCLR extended */
	return (error);
}

/*
 * ufs_ioctl()
 *	Do an IO-control operation.
 *
 * Caller passes vp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;			/* not used */
{
	register struct inode *ip;

	ip = VTOI(vp);
	ASSERT((ip->i_mode & IFMT) == IFCHR, "ufs_ioctl");
	/*
	 *+ An attempt has been made to pass an ioctl() to an ufs
	 *+ vnode other than a character special device.
	 *+ Check the system configuration.
	 */
	return ((*cdevsw[major(ip->i_rdev)].d_ioctl)
			(ip->i_rdev, com, data, flag));
}

/*
 * ufs_select()
 *	Call character driver to do a "select".
 *
 * Caller passes vp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;			/* not used */
{
	register struct inode *ip;

	ip = VTOI(vp);
	ASSERT((ip->i_mode & IFMT) == IFCHR, "ufs_select");
	/*
	 *+ An attempt has been made to pass a select() to an ufs
	 *+ vnode other than a character special device.
	 *+ Check the system configuration.
	 */
	return ((*cdevsw[major(ip->i_rdev)].d_select)(ip->i_rdev, which));
}

/*
 * ufs_getattr()
 *	Return attributes of UNIX file-system vnode.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

/*ARGSUSED*/
static
ufs_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;			/* not used */
{
	register struct inode *ip;

	ip = VTOI(vp);
	/*
	 * Copy from inode table.
	 */
	vap->va_type = IFTOVT(ip->i_mode);
	vap->va_mode = ip->i_mode;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	vap->va_flags = 0;
	if ((ip->i_mode & IFMT) == IFLNK && (ip->i_pflags & IP_CSYMLN)) {
		vap->va_flags |= VA_CSYMLN;
		if (u.u_universe == U_UCB)
			vap->va_size = ip->i_ucbln;
		else
			vap->va_size = ip->i_attln;
	} else {
		vap->va_size = ip->i_size;
	}
	vap->va_atime = ip->i_atime;
	vap->va_mtime = ip->i_mtime;
	vap->va_ctime = ip->i_ctime;
	vap->va_rdev = ip->i_rdev;
	vap->va_blocks = ip->i_blocks;
	switch (ip->i_mode & IFMT) {

	case IFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;

	case IFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = ip->i_fs->fs_bsize;
		break;
	}
	vap->va_psize = ip->i_psize;
	return (0);
}

/*
 * ufs_setattr()
 *	Set attributes of UNIX file-system vnode.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

static
ufs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;	
	struct ucred *cred;
{
	register struct inode *ip;
	struct timeval timenow;
	int chtime = 0;
	int error = 0;
	extern int allow_chown;    /* binary configurable, set in conf/param.c*/

	/*
	 * Cannot set certain attributes.
	 */

	if (vap->va_nlink != -1 || vap->va_blocksize != -1 ||
	    vap->va_rdev != -1 || vap->va_blocks != -1 ||
	    vap->va_fsid != -1 || vap->va_nodeid != -1 ||
	    (int)vap->va_type != -1) {
		return (EINVAL);
	}

	ip = VTOI(vp);

	/*
	 * Change file access modes.  Must be owner or su.
	 */

	if (vap->va_mode != (u_short)0xffff) {
		error = OWNER(cred, ip);
		if (error)
			goto out;
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;
		if (cred->cr_uid != 0) {
			if ((ip->i_mode & IFMT) != IFDIR)
				ip->i_mode &= ~ISVTX;
			if (!groupmember(ip->i_gid))
				ip->i_mode &= ~ISGID;
		}
#if	ISVTX != VSVTX
	ERROR -- implementation assumes ISVTX==VSVTX
#endif
		vp->v_flag &= ~VSVTX;
		vp->v_flag |= (ip->i_mode & ISVTX);
		IMARK(ip, ICHG);
	}

	/*
	 * Change file ownership.  SysV chown allows the owner and su to
	 * give away ownership, 4.2 restricts it to su.  We allow both,
	 * however, the SysV version is binary configurable.  This
	 * prevents people in the ucb universe from switching to att,
	 * doing a chown, and switching back.
	 */

	if (vap->va_uid != -1) {
		if (allow_chown) {
			if (u.u_tuniverse == U_ATT) {
				error = OWNER(cred, ip);
				if (error)
					goto out;
			} else {
				if (!suser()) {
					error = EPERM;
					goto out;
				}
			}
		} else {
			if (!suser()) {
				error = EPERM;
				goto out;
			}
		}
		error = chown1(ip, vap->va_uid, -1);
		if (error)
			goto out;
	}
	/*
	 * In 4.3 it is permitted to change the group of a file if
	 * the caller is a member of that group.
	 * The command /bin/chgrp allows this regardless.
	 */
	if (vap->va_gid != -1) {
		if (!groupmember(vap->va_gid) && !suser()) {
			error = EPERM;
			goto out;
		}
		error = chown1(ip, -1, vap->va_gid);
		if (error)
			goto out;
	}

	/*
	 * Truncate file. Must have write permission and not be a directory.
	 * can't be a FIFO, either.
	 */

	if (vap->va_size != -1) {
		if ((ip->i_mode & IFMT) == IFDIR) {
			error = EISDIR;
			goto out;
		}
		if ((ip->i_mode & IFMT) != IFREG) {
			error = EINVAL;
			goto out;
		}
		if (iaccess(ip, IWRITE)) {
			error = u.u_error;
			goto out;
		}
		if (VN_MAPPED(vp)) {		/* VMAPPED can be stale */
			error = EBUSY;
			goto out;
		}
		itrunc(ip, vap->va_size);
	}

	/*
	 * Logically grow a file.  Used in mmap() to increase size of
	 * file prior to allocation zero-fill blocks.
	 * Caller guarantees write access to file, and file
	 * is regular file (eg, not directory, FIFO, etc).
	 */

	if (vap->va_extfile != -1)
		ip->i_size = vap->va_extfile;

	/*
	 * Change file access or modified times.
	 * If the pointer to the time structure passed as argument to the
	 * syscall is null, then we must have write access to the inode
	 * (if not owner).  We know that the ptr is null since tv_sec
	 * is 0 and tv_usec is -1.
	 *
	 * See the comment in utimes() - sys/vfs_syscalls.c.
	 */

	if (vap->va_mtime.tv_sec != -1) {
		error = OWNER(cred, ip);
		chtime = 1;
		timenow = time;
		if (vap->va_mtime.tv_sec == 0 && vap->va_mtime.tv_usec == -1) {
			if (error) {
				if (iaccess(ip, IWRITE)) {
					error = u.u_error;
					goto out;
				}
				error = 0;
			}
			/*
			 * Make sure mod time is unique ala IMARK().
			 */
			if (ip->i_mtime.tv_sec == timenow.tv_sec &&
			    ip->i_mtime.tv_usec >= timenow.tv_usec) {
				timenow.tv_usec = ip->i_mtime.tv_usec + 1;
				if (timenow.tv_usec >= 1000000) {
					timenow.tv_usec = 0;
					timenow.tv_sec++;
				}
			}
			ip->i_mtime = timenow;
		} else if (error) {
			error = EPERM;
			goto out;
		} else
			ip->i_mtime = vap->va_mtime;
	}
	if (vap->va_atime.tv_sec != -1) {
		error = OWNER(cred, ip);
		if (chtime++ == 0)
			timenow = time;
		if (vap->va_atime.tv_sec == 0 && vap->va_atime.tv_usec == -1) {
			if (error) {
				if (iaccess(ip, IWRITE)) {
					error = u.u_error;
					goto out;
				}
				error = 0;
			}
			ip->i_atime = timenow;
		} else if (error) {
			error = EPERM;
			goto out;
		} else
			ip->i_atime = vap->va_atime;
	}
	if (chtime) {
		ip->i_flag |= IACC|IUPD|ICHG;
		ip->i_ctime = timenow;
	}
out:
	iupdat(ip, 1);			/* XXX should be asyn for perf */
	return (error);
}

/*
 * chown1()
 *	Perform chown operation on inode ip.
 *
 * Caller passes locked inode.
 */

static

chown1(ip, uid, gid)
	register struct inode *ip;
	int uid, gid;
{
#ifdef QUOTA
	register long change;
	struct mount *mp;
#endif

	if (uid == -1)
		uid = ip->i_uid;
	if (gid == -1)
		gid = ip->i_gid;

#ifdef QUOTA
	if (ip->i_uid == uid)	/* this just speeds things a little */
		change = 0;
	else
		change = ip->i_blocks;

	if (ip->i_dquot != NULL) {
		struct dquot *dqp;

		(void) chkdq(ip, -change, 1);
		(void) chkiq(VFSTOM(ITOV(ip)->v_vfsp), ip, ip->i_uid, 1);
		dqp = ip->i_dquot;
		ip->i_dquot = NULL;
		dqrele(dqp);
	}
#endif
	ip->i_uid = uid;
	ip->i_gid = gid;
	IMARK(ip, ICHG);
	if (u.u_tuniverse == U_ATT) {
		if (u.u_uid != 0)
			ip->i_mode &= ~(ISUID|ISGID);
	} else {
		if (u.u_ruid != 0)
			ip->i_mode &= ~(ISUID|ISGID);
	}
#ifdef QUOTA
	mp = VFSTOM(ITOV(ip)->v_vfsp);
	if (mp->m_qinod != NULL) {
		ip->i_dquot = getinoquota(ip);
		(void) chkdq(ip, change, 1);
		(void) chkiq(mp, (struct inode *)NULL, uid, 1);
		return (u.u_error);		/* should == 0 ALWAYS !! */
	}
#endif
	return (0);
}

/*
 * ufs_access()
 *	See if given access mode is allowed.
 *
 * Return 0 for access allowed, or error number.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

/*ARGSUSED*/
static
ufs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;	/* not used */
{
	return (iaccess(VTOI(vp), mode));
}

/*
 * ufs_readlink()
 *	Read contents of a symbolic-link.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

/*ARGSUSED*/
static
ufs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;			/* not used */
{
	register struct inode *ip;
	register int error, tlen, maxlen;

	if (vp->v_type != VLNK)
		return (EINVAL);
	ip = VTOI(vp);
	
	/*
	 * If conditional symbolic link, read appropriate one based on
	 * callers current "universe".
	 */
	 
	maxlen = uiop->uio_resid;
	if (ip->i_pflags & IP_CSYMLN) {
		if (u.u_universe == U_UCB) {
			tlen = ip->i_ucbln;
			uiop->uio_offset = 0;
		} else {
			tlen = ip->i_attln;
			uiop->uio_offset = MAXCSYMLEN;
		}
	} else
		tlen = maxlen;

	uiop->uio_resid = uiop->uio_iov->iov_len = MIN(tlen, maxlen);

	error = rwip(ip, uiop, UIO_READ, 0);

	/*
	 * Restore residual count to what the caller would expect.
	 */

	uiop->uio_resid = maxlen - (MIN(tlen,maxlen) - uiop->uio_resid);
	return (error);
}

/*
 * ufs_readclink()
 *	Read the branch of the conditional link requested by flag.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */
	 
/*ARGSUSED*/
static
ufs_readclink(vp, uiop, flag, cred)
	struct vnode *vp;
	struct uio *uiop;
	int flag;
	struct ucred *cred;			/* not used */
{
	register struct inode *ip;
	register int error, tlen, maxlen;

	if (vp->v_type != VLNK)
		return (EINVAL);
	if (flag != U_ATT && flag != U_UCB)
		return (EINVAL);

	ip = VTOI(vp);

	if (!(ip->i_pflags & IP_CSYMLN))  {
		return (ENXIO);	
	}
	
	maxlen = uiop->uio_resid;
	if (flag == U_UCB) {
		tlen = ip->i_ucbln;
		uiop->uio_offset = 0;
	} else {
		tlen = ip->i_attln;
		uiop->uio_offset = MAXCSYMLEN;
	}
	uiop->uio_resid = uiop->uio_iov->iov_len = MIN(tlen, maxlen);

	error = rwip(ip, uiop, UIO_READ, 0);

	/*
	 * Restore residual count to what the caller would expect.
	 */

	uiop->uio_resid = maxlen - (MIN(tlen,maxlen) - uiop->uio_resid);

	return (error);
}

/*
 * ufs_fsync()
 *	Synch-out an inode -- insure all it's dirty blocks are written.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

/*ARGSUSED*/
static
ufs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;			/* not used */
{
	vp->v_flag &= ~VMAPSYNC;		/* about to sync it out */
	syncip(VTOI(vp));
	return (0);
}

/*
 * Unix file system operations having to do with directory manipulation.
 */

/*
 * ufs_lookup()
 *	Look up a component of a path-name in a directory.
 *
 * Caller passes dvp locked.
 * On return: dvp is unlocked, *vpp is locked.
 * If error, dvp is still unlocked, *vpp is unchanged.
 */

/*ARGSUSED*/
static
ufs_lookup(dvp, nm, vpp, cred)
	struct	vnode	*dvp;
	struct	vnode	**vpp;
	char	*nm;
	struct ucred *cred;	/* not used */
{
	struct	inode	*ip;
	int	error;

	error = dirlook(VTOI(dvp), nm, &ip);	/* returns dvp unlocked */
	if (error == 0)
		*vpp = ITOV(ip);
	return (error);
}

/*
 * ufs_create()
 *	Create a new UNIX file-system file.
 *
 * Caller passes dvp unlocked.  *vpp is returned locked.
 */

static
ufs_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	register int error;
	struct inode *ip;

	/*
	 * Can't create directories. use ufs_mkdir.
	 */

	if (vap->va_type == VDIR)
		return (EISDIR);

	if (cred->cr_uid != 0) 
		vap->va_mode &= ~VSVTX;
	ip = (struct inode *) 0;
	error = direnter(VTOI(dvp), nm, DE_CREATE,
				(struct inode *)0, (struct inode *)0, vap, &ip);

	/*
	 * If file exists and this is a nonexclusive create,
	 * check for not directory and access permissions.
	 */

	if (error == EEXIST) {
		switch (exclusive) {
		case NONEXCL:
			if ((ip->i_mode & IFMT) == IFDIR && (mode & IWRITE)) {
				error = EISDIR;
			} else if (mode)
				error = iaccess(ip, mode);
			else
				error = 0;
			break;
		case EXCL:
			if (vap->va_type == VSOCK) {
				int	accerr;
				if (accerr = iaccess(ip, IWRITE))
					error = accerr;
			}
			break;
		}
		if (error) {
			IPUT(ip);
		} else if ((ip->i_mode&IFMT) == IFREG && vap->va_size == 0) {
			/*
			 * Truncate regular files, if required.
			 * No need to do this if error != EEXIST, since
			 * direnter() made a new inode.
			 */
			if (VN_MAPPED(ITOV(ip))) {
				IPUT(ip);
				error = EBUSY;
			} else {
				itrunc(ip, (u_long) 0);
			}
		}
	} 
	if (error)
		return (error);

	*vpp = ITOV(ip);
	if (vap != (struct vattr *)0)
		(void) ufs_getattr(*vpp, vap, cred);

	return (error);
}

/*
 * ufs_remove()
 *	Remove a file-system name from a file (unlink), given containing
 *	directory vnode and component name to remove.
 *
 * Caller passes vp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_remove(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;			/* not used */
{
	return (dirremove(VTOI(vp), nm, (struct inode *)0, 0));
}

/*
 * ufs_link()
 *	Link a file or a directory.
 *
 * If source is a directory, must be superuser.
 *
 * Caller passes vp, tdvp unlocked.  They remain unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;			/* not used */
{
	register struct inode *sip;
	register int error;

	sip = VTOI(vp);
	if ((sip->i_mode & IFMT) == IFDIR && !suser()) {
		return (EPERM);
	}
	error = direnter(VTOI(tdvp), tnm, DE_LINK,
		(struct inode *)0, sip, (struct vattr *)0, (struct inode **)0);
	return (error);
}

/*
 * ufs_rename()
 *	Rename a file or directory
 *
 * We are given the vnode and entry string of the source and the
 * vnode and entry string of the place we want to move the source to
 * (the target). The essential operation is:
 *	unlink(target);
 *	link(source, target);
 *	unlink(source);
 * but "atomically". Can't do full commit without saving state in the inode
 * on disk, which isn't feasible at this time. Best we can do is always
 * guarantee that the TARGET exists.
 *
 * Caller passes sdvp, tdvp unlocked.  They remain unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_rename(sdvp, snm, tdvp, tnm, cred)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
	struct ucred *cred;		/* not used */
{
	struct inode *sip;		/* source inode */
	register struct inode *sdp;	/* old (source) parent inode */
	register struct inode *tdp;	/* new (target) parent inode */
	register int error;

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);

	/*
	 * Make sure we can delete the source entry.
	 */

	error = iaccess(sdp, IWRITE);
	if (error)
		return (error);

	/*
	 * Look up inode of file we're supposed to rename.
	 */

	ILOCK(sdp);				/* dirlook expects this */
	error = dirlook(sdp, snm, &sip);	/* returns sdp unlocked */
	if (error)
		return (error);

	/*
	 * Check for renaming '.' or '..' or alias of '.'
	 */

	if (strcmp(snm, ".") == 0 || strcmp(snm, "..") == 0 || sdp == sip) {
		error = EINVAL;
	        IUNLOCK(sip);		/* unlock inode, locked in dirlook */
		goto out;
	}

	/*
	 * Honor the SYSV sticky-bit-on-directory semantics.
	 * Do this here (eventhough we will do it again later in dirremove)
	 * to ATTEMPT to avoid creating the link in the target dir 
	 * and then finding that we don't have permission to remove the
	 * link in the source dir.
	 */
        if (sdp->i_mode & ISVTX && u.u_uid != 0 && sdp->i_uid != u.u_uid &&
                sip->i_uid != u.u_uid && iaccess(sip, IWRITE)) {
                error = EACCES;
	        IUNLOCK(sip);		/* unlock inode, locked in dirlook */
                goto out;
	}
        IUNLOCK(sip);		/* unlock inode, locked in dirlook */

	/*
	 * Link source to the target.
	 */

	error = direnter(tdp, tnm, DE_RENAME,
			sdp, sip, (struct vattr *)0, (struct inode **)0);
	if (error) {
		if (error != ESAME)
			goto out;
		/*
		 * ESAME ==> this is an attempt to rename one link of a file
		 * to another, or rename a file to itself.  If different names
		 * for same file, just remove source link.  If same name, then
		 * really a NOP.
		 */
		error = 0;
		if (sdp == tdp && strcmp(snm, tnm) == 0)	/* same name */
			goto out;
	}

	/*
	 * Unlink the source entry.  dirremove() checks that the entry
	 * still reflects sip, and returns an error if it doesn't.
	 * If the entry has changed just forget about it. 
	 * Release the source inode.
	 */

	error = dirremove(sdp, snm, sip, 0);
	if (error == ENOENT)
		error = 0;
out:
	IRELE(sip);
	return (error);
}

/*
 * ufs_mkdir()
 *	Create a new UNIX file-system directory.
 *
 * Caller passes dvp unlocked.  Returns *vpp locked (if !error).
 */

/*ARGSUSED*/
static
ufs_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	struct vnode **vpp;
	struct ucred *cred;		/* not used */
{
	struct inode *ip;
	register int error;

	error = direnter(VTOI(dvp), nm, DE_CREATE,
				(struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0)
		*vpp = ITOV(ip);
	else if (error == EEXIST) {
		IPUT(ip);
	}
	return (error);
}

/*
 * ufs_rmdir()
 *	Remove a directory, given parent directory vnode and name to remove.
 *
 * Caller passes vp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_rmdir(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;	/* not used */
{
	return (dirremove(VTOI(vp), nm, (struct inode *)0, 1));
}

/*
 * ufs_readdir()
 *	Read contents of a directory in a file-system independent format.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

/*ARGSUSED*/
static
ufs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;			/* not used */
{
	register struct iovec *iovp;
	register unsigned count;
	register unsigned adjresid;
	register int ret;

	iovp = uiop->uio_iov;
	count = iovp->iov_len;
	if (uiop->uio_iovcnt != 1 || count < DIRBLKSIZ ||
	    (uiop->uio_offset & (DIRBLKSIZ -1)))
		return (EINVAL);
	/*
	 * figure out what we need to cut off to round down to a DIRBLKSIZ
	 * boundary....save in adjresid so that we can add it back on before
	 * returning.
	 */
	count &= ~(DIRBLKSIZ - 1);
	adjresid = iovp->iov_len - count;
	uiop->uio_resid -= adjresid;
	iovp->iov_len = count;
	ret = rwip(VTOI(vp), uiop, UIO_READ, 0);
	uiop->uio_resid += adjresid;
	return (ret);

}

/*
 * ufs_symlink()
 *	Create a symbolic link.
 *
 * Caller passes dvp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_symlink(dvp, lnm, vap, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *vap;
	char *tnm;
	struct ucred *cred;		/* not used */
{
	struct inode *ip;
	register int error;

	ip = (struct inode *) 0;
	vap->va_type = VLNK;
	vap->va_rdev = 0;
	error = direnter(VTOI(dvp), lnm, DE_CREATE,
			(struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		/*
		 * Clear out the csymlink fields, in over-compensation
		 * for previous bug where they weren't cleared
		 * at csymlink deletion time.
		 */
		ip->i_pflags &= ~IP_CSYMLN;
		ip->i_ucbln = 0;
		ip->i_attln = 0;
		error = rdwri(UIO_WRITE, ip,
				tnm, strlen(tnm), 0, UIOSEG_KERNEL, (int *)0);
		IPUT(ip);
	} else if (error == EEXIST) {
		IPUT(ip);
	}
	return (error);
}

/*
 * ufs_csymlink()
 *	Create a conditional symbolic link (two names stored, one for
 *	each of UCB and ATT "universe").
 *
 * Caller passes dvp unlocked.  It remains unlocked at exit.
 */

/*ARGSUSED*/
static
ufs_csymlink(dvp, lnm, vap, ucbnm, attnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *vap;
	char *ucbnm;
	char *attnm;
	struct ucred *cred;	/* not used */
{
	struct inode *ip;
	register int error;

	ip = (struct inode *) 0;
	vap->va_type = VLNK;
	vap->va_rdev = 0;
	error = direnter(VTOI(dvp), lnm, DE_CREATE,
				(struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		ip->i_pflags |= IP_CSYMLN;
		ip->i_ucbln = strlen(ucbnm);
		error = rdwri(UIO_WRITE, ip, ucbnm, (int)ip->i_ucbln,
						0, UIOSEG_KERNEL, (int *)0);
		ip->i_attln = strlen(attnm);
		if (error == 0)
			error = rdwri(UIO_WRITE, ip, attnm, (int)ip->i_attln,
					MAXCSYMLEN, UIOSEG_KERNEL, (int *)0);
		IPUT(ip);
	} else if (error == EEXIST) {
		IPUT(ip);
	}
	return (error);
}

/*
 * rdwri()
 *	Read/write an inode.
 *
 * Caller passes ip locked.
 */

rdwri(rw, ip, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	register int error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = seg;
	auio.uio_resid = len;
	error = ufs_rdwr(ITOV(ip), &auio, rw, 0, u.u_cred);
	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}
	return (error);
}

/*
 * ufs_bmap()
 *	Return "physical" block number of an inode for a given logical
 *	block number.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

static
ufs_bmap(vp, lbn, vpp, bnp, rwflg, size)
	struct vnode *vp;
	daddr_t lbn;
	struct vnode **vpp;
	daddr_t *bnp;
	int rwflg;
	int size;
{
	register struct inode *ip;

	ip = VTOI(vp);
	if (vpp)
		*vpp = ip->i_devvp;
	if (bnp) {
		daddr_t bn = fsbtodb(ip->i_fs, BMAP(ip, lbn, rwflg, size));
		*bnp = bn;		/* split due to compiler botch (PCC) */
	}
	return (0);
}

/*
 * ufs_bread()
 *	Read a logical block and return it in a buffer.
 *
 * Caller passes vp locked.  It remains locked at exit.
 */

static
ufs_bread(vp, lbn, bpp)
	struct vnode *vp;
	daddr_t lbn;
	struct buf **bpp;
{
	register struct inode *ip;
	register struct buf *bp;
	register daddr_t bn;
	register int size;

	ip = VTOI(vp);
	size = blksize(ip->i_fs, ip, lbn);
	bn = fsbtodb(ip->i_fs, bmap(ip, lbn, B_READ));
	if ((long)bn < 0) {
		bp = geteblk(size);
		clrbuf(bp);
	} else if (ip->i_lastr + 1 == lbn) {
		bp = breada(ip->i_devvp, bn, size, l.rablock, l.rasize);
	} else {
		bp = bread(ip->i_devvp, bn, size);
	}
	ip->i_lastr = lbn;
	IMARK(ip, IACC);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	} else {
		*bpp = bp;
		return (0);
	}
}

/*
 * ufs_brelse()
 *	Release a block returned by ufs_bread.
 */

static
ufs_brelse(bp)
	struct buf *bp;
{
	bp->b_flags |= B_AGE;
	bp->b_resid = 0;
	brelse(bp);
}

static
ufs_badop()
{
	panic("ufs_badop");
	/*
	 *+ This filesystem type does not support the requested operation.
	 */
}

/*
 * UNIX file-system vnode-operations interface structure.
 */

extern	int	iinactive();

struct vnodeops ufs_vnodeops = {
	ufs_open,
	ufs_close,
	ufs_rdwr,
	ufs_ioctl,
	ufs_select,
	ufs_getattr,
	ufs_setattr,
	ufs_access,
	ufs_lookup,
	ufs_create,
	ufs_remove,
	ufs_link,
	ufs_rename,
	ufs_mkdir,
	ufs_rmdir,
	ufs_readdir,
	ufs_symlink,
	ufs_csymlink,
	ufs_readlink,
	ufs_readclink,
	ufs_fsync,
	iinactive,
	ufs_bmap,
	ufs_badop,
	ufs_bread,
	ufs_brelse,
	ufs_badop
};
