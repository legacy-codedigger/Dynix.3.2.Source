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
static	char	rcsid[] = "$Header: ufs_directIO.c 1.6 91/03/31 $";
#endif

/*
 * ufs_directIO.c
 *	Manage IO directly between user address space and UFS file, bypassing
 *	buffer-cache.
 *
 * TODO:
 *	Generalize to other file-systems (eg, NFS).  Probably cause errors,
 *	but handle in the file-sys rather than explicit test for UFS.
 *	Probably add VOP_DIRECTIO().
 *
 *	If dynamic growth of file using bsize blocks is frequent, should do
 *	B_NOCLR to avoid redundant clear and write of data.  Current
 *	implementation supports file growth, but not optimally (assumption is
 *	growth is rare, and prefer to keep code simpler).
 *
 *	Sense contiguous blocks of file and support multi-block transfers.
 *
 *	Figure out what direct IO on a mapped file means, and possibly
 *	support.
 *
 *	Should a baddr() hit in coherent_directIO just supply this data?
 *
 *	How important is keeping IUPD|ICHG updated?  Can this be removed?
 *	See direct_rwip().
 *
 *	Resolve possible memory deadlock: physio() does ++p_noswap then
 *	vslock(); p_noswap nails entire Rset in memory.  Better to ++p_noswap
 *	after call to vslock() if this is ok (comments say so, need verify).
 *	Ideal (?) solution is to attach list of "nailed" address ranges in
 *	process and give swapper ability to do all but these.  Need finer
 *	grain "p_noswap" in other cases also.
 *
 *	Asynch direct IO: once have translated it (bmap) and transfer will do
 *	all remaining bytes, could let it "go".  Need some form of
 *	notification, queue outstanding requests to process, allow multiple
 *	outstanding RAW IO's, etc.  May be important to have formal RW sema
 *	so biodone() w/B_CALL can release request.  Ideally this should
 *	generalize to other vnode types (eg, asynch RAW tape IO) ==> physio()
 *	integration.
 *
 *	Expand physio() in-line in direct_rwip().  Maybe optimize more and/or
 *	makes asynch IO integration easier.
 *
 *	Use VN_SHARENODE() for misc read only operations in normal use of
 *	files.  Eg, read()'s should be ok (resolve setting IACC); thus could
 *	have concurrent reads of file.  In addition, could allow multiple
 *	readers of buf-cache block (might be more difficult), thus allow true
 *	concurrent reading of files, directory parsing, etc.
 *
 * NOTES:
 *	No assist to align transfers in memory or on device boundaries, nor to
 *	fuss with count.  Passes requests to driver.  If device has
 *	restrictions, caller must obey them.  Eg, ZDC insists on transfers
 *	in multiples of 512 bytes, on device block (512 byte) boundaries.
 *	If you want transparency, use the file system normally ;-)
 *
 *	Doesn't set IACC when direct read inode.  Don't think this is an issue.
 *
 *	Code that sets IUPD|ICHG and clears SUIG|SGID on direct writes is
 *	allowed to race.  See direct_rwip().
 */

/* $Log:	ufs_directIO.c,v $
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
#include "../ufs/fs.h"
#include "../ufs/inode.h"

#include "../machine/intctl.h"	/* For SPLs */

/*
 * Module local data.
 */

int	coherent_directIO = 1;			/* keep buf-cache coherent? */

/*
 * DirectIO()
 *	RAW read/write from file.
 *
 * Insist that file-descriptor references VREG vnode.
 *
 * Returns zero for success, else error number.
 */

int
DirectIO(fd, cbuf, count, offset, rw)
	int	fd;
	char	*cbuf;
	int	count;
	long	offset;
	enum	uio_rw rw;
{
	register struct vnode *vp;
	struct	file	*fp;
	int	resid;
	int	error;

	/*
	 * Basic sanity check on arguments.
	 */

	if (offset < 0 || count < 0 || (offset + count) < 0)
		return EINVAL;
	if (count == 0)
		return 0;

	/*
	 * Get object referenced by fd.  Insist on VREG vnode and readable
	 * or writable as appropriate.
	 */

	if (error = getvnodefp(fd, &fp))
		return error;
	if ((fp->f_flag & (rw == UIO_READ ? FREAD : FWRITE)) == 0)
		return EBADF;
	vp = (struct vnode *) fp->f_data;
	if (vp->v_type != VREG)
		return EINVAL;

	/*
	 * If write, make sure filesystem is writable.
	 */

	if (rw == UIO_WRITE && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return EROFS;

	/*
	 * Kludge for now: insist on UFS vnode.  Should generalize this into
	 * a VOP().
	 */

	if (vp->v_op != &ufs_vnodeops)
		return EINVAL;

	/*
	 * Lock the vnode for "shared" access -- allow multiple directIO's
	 * but no concurrent "normal" read/write/mmap/truncate (any buf-cache
	 * or shape changing operations).  Should consider making read() and
	 * other "read-only" operations use shared locks.
	 *
	 * For now, direct IO on mapped files is illegal.
	 */

	VN_SHARENODE(vp);

	if (VN_MAPPED(vp)) {
		VN_UNSHARENODE(vp);
		return EBUSY;
	}

	error = direct_rwip(VTOI(vp), cbuf, count, offset, rw, &resid);

	VN_UNSHARENODE(vp);

	u.u_r.r_val1 = count - resid;
	return error;
}

/*
 * direct_rwip()
 *	Work-horse for Direct IO.  Streamlined clone of rwip().
 *
 * For now, don't allow growth of a file -- if space isn't there, don't
 * create it.  Simplifying assumption, can probably be extended later.
 *
 * Calls physio directly (a minor cheat on the drivers).  No partition
 * check (physck()) needed since doing IO into allocated space in files.
 *
 * Caller insures IFREG file.
 * Called with share-locked vnode (eg, p_reader()).
 *
 * Returns zero for success, else error number.
 */

static int
direct_rwip(ip, cbuf, count, offset, rw, residp)
	register struct	inode	*ip;
	char		*cbuf;
	int		count;
	long		offset;
	enum	uio_rw	rw;
	int		*residp;
{
	int		n;
	int		on;
	daddr_t		lbn;
	daddr_t		bn;
	register struct	fs *fs = ip->i_fs;
	struct	bdevsw	*driver = &bdevsw[major((dev_t)ip->i_dev)];
	int		diff; 
	int		error = 0;
	u_long		osize;
	struct	uio	auio;
	struct	iovec	aiov;

	do {
		/*
		 * Remember old size to truncate in case of a failure
		 */
		osize = ip->i_size;
		/*
		 * Determine logical block number in file, offset in block,
		 * and bytes remaining in block to transfer.
		 */

		lbn = offset / fs->fs_bsize;
		on = offset % fs->fs_bsize;
		n = MIN((unsigned)(fs->fs_bsize - on), count);

		/*
		 * If read, map file if space exists.
		 *
		 * Otherwise (write), assume space exists (keep share lock)
		 * and handle special if not (loop attempting to create
		 * space; requires transition on lock state, which allows
		 * other operations to get in: hence, loop).
		 */

		diff = ip->i_size - offset;
		if (rw == UIO_READ) {
			if (diff <= 0)
				break;
			if (diff < n)
				n = diff;
			bn = fsbtodb(fs, BMAP(ip, lbn, B_READ, on+n));
		} else while (diff < n ||
			      (bn = fsbtodb(fs,BMAP(ip,lbn,B_READ,on+n))) < 0) {
			if (!direct_grow(ip, offset, lbn, on, n)) {
				bn = -1;
				break;
			}
			diff = ip->i_size - offset;
		}

		/*
		 * If mapped badly (error or space doesn't exist), done.
		 */

		if ((long) bn <= 0)
			return (u.u_error ? u.u_error : EINVAL);

		/*
		 * Optionally, insure buf-cache is coherent with this by
		 * invalidating matching buffer.  When read() and others use
		 * share-locks, can concurrently instantiate buffers.  This is
		 * ok, just maybe a bit wasteful.
		 *
		 * NOTE: if not doing this, user assumes responsiblity.
		 * Unclear what user can do, other than simply avoid mixing
		 * flavors of IO (there is no "flush read-only buffers"
		 * function -- but one could be created: a stronger fsync()).
		 * This is here to understand performance impact of keeping
		 * buf-cache coherent.
		 */

		if (coherent_directIO) {
			struct buf *bp;
			int size = blksize(fs, ip, lbn);
			/*
			 * If hit and buffer dirty, must write if doing read or
			 * write that doesn't totally overlap.  Otherwise, just
			 * invalidate.
			 *
			 * In some cases (eg, read and buffer not dirty), could
			 * keep buffer but invalidate anyhow to reduce baddr()
			 * hits for later directIO's (ie, less overhead).
			 * Assume that if directIO is being used, it is
			 * predominant access mechanism.
			 *
			 * Setting B_INVAL before bwrite() invalidates buffer
			 * when IO completes.
			 */
			if (bp = baddr(ip->i_devvp, bn, size)) {
				bp->b_flags |= B_INVAL;
				if ((bp->b_flags & B_DELWRI) &&
				    (rw == UIO_READ || n < size))
					bwrite(bp);
				else
					brelse(bp);
			}
		}

		/*
		 * Do the physical IO.  Call physio() directly, bypassing
		 * driver read/write procedure (hard to know character device
		 * number; only have block device number).  This is legit for
		 * all current Sequent disk drivers, and should be ok for
		 * others.  This should be cleaner in the NOBDEVSW kernel.
		 */

		aiov.iov_base = cbuf;
		auio.uio_resid = aiov.iov_len = n;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = dbtob(bn) + on;
		auio.uio_segflg = UIOSEG_USER;

		error = physio(	driver->d_strategy,
				(struct buf *) 0,
				(dev_t) ip->i_dev,
				(rw == UIO_READ) ? B_READ : B_WRITE,
				driver->d_minphys,
				&auio
			);

		count -= n;
		cbuf += n;
		offset += n;
		if (rw == UIO_WRITE) {
			/*
			 * Mark the file.  This is "safe" since writing flag
			 * bits and time stamps, and only "problem" flag bit is
			 * IFREE which can't be fiddled with since caller holds
			 * reference to inode.  Note that this implementation
			 * keeps the update+change time(s) with coarser
			 * resolution than normal writes.
			 *
			 * Ignore this whole issue for reads (IACC).
			 *
			 * Low probability of munged timestamp due to
			 * concurrent updates, but this isn't really worse
			 * than sampling time directly (see IMARK()).  Also,
			 * all updaters will use same time.  IUPD "unique"
			 * time may be non-NFS "coherent" (particularly due
			 * to coarser resolution), but don't expect this to be
			 * a problem in practice.
			 */
			if ((ip->i_flag & (IUPD|ICHG)) != (IUPD|ICHG)) {
				IMARK(ip, IUPD|ICHG);
			}
			/*
			 * Also zap ISUID|ISGID bits (security precaution).
			 * Again "safe" since no "hard write" operations can
			 * occur concurrently which will modify this field.
			 */
			if ((ip->i_mode & (ISUID|ISGID)) && u.u_ruid != 0)
				ip->i_mode &= ~(ISUID|ISGID);
		}
	} while (error == 0 && count != 0);

	if (error != 0) {
		/*
		 * Write failed, truncate inode to old size to avoid having
		 * inode pointing to uninitialized disk blocks
		 * Since itrunc can right we need to promote the lock.
		 * If we lose the race to the inode then too bad.
		 * we just trunc the file back anyway. This will leave
		 * the data short but consistant.
		 */
		VN_UNSHARENODE(ITOV(ip));	/* NOTE: vnode now UNLOCKED! */
		VN_LOCKNODE(ITOV(ip));		/* now locked exclusive */
		itrunc(ip,osize);
		VN_UNLOCKNODE(ITOV(ip));	/* NOTE: vnode now UNLOCKED! */
		VN_SHARENODE(ITOV(ip));		/* now shared-locked */
	}
	*residp = count;
	return error;
}

/*
 * direct_grow()
 *	Grow a file in a Direct IO case (write).
 *
 * Implementation isn't optimal: could use B_NOCLR and avoid extra buf-header
 * fussing, at a cost in additional complexity in direct_rwip() and
 * interesting error unwind cases.
 *
 * Relies on buf-cache coherence to insure block is written properly (see
 * direct_rwip()).  If !coherent_directIO, don't write-extend the file.
 * This can be relaxed by duplicating the "baddr" code from direct_rwip(),
 * but this is redundant if coherent_directIO, and expect !coherent_directIO
 * to typically be on.
 *
 * Called with share-locked inode.  Returns with share-locked inode, but
 * promotes this to exclusive lock temporarily while creating the extended
 * space in the file.  This creates race when transitioning flavor of lock.
 * Eg, file could get truncated in the middle.  Caller is assumed to tollerate
 * this (eg, call in a loop).
 *
 * Returns true for success, else false.
 */

static bool_t
direct_grow(ip, offset, lbn, on, n)
	register struct	inode	*ip;
	long	offset;
	daddr_t	lbn;
	int	on;
	int	n;
{
	daddr_t	bn;

	if (!coherent_directIO)			/* make it "safe" */
		return 0;

	/*
	 * Change lock on inode to exclusive lock.  This opens hole where can
	 * race with other manipulation of the file.
	 */

	VN_UNSHARENODE(ITOV(ip));		/* NOTE: vnode now UNLOCKED! */
	VN_LOCKNODE(ITOV(ip));			/* now locked exclusive */

	/*
	 * Bmap for writing.  Ok if raced with other callers, have it
	 * exclusive now.  It's possible another caller created the space
	 * we're about to bmap, but this is ok.
	 */

	/*
	 * Use the the B_SYNC option to bmap and call iupdat to
	 * guarantee that the new file blocks are actually
	 * on disk when the DirectWrite call returns
	 */

	bn = fsbtodb(ip->i_fs, BMAP(ip, lbn, B_WRITE|B_SYNC, on+n));
	if (bn > 0) {
		if ((offset + n) > ip->i_size)
			ip->i_size = offset + n;
		IMARK(ip, IUPD|ICHG);
		if (u.u_ruid != 0)
			ip->i_mode &= ~(ISUID|ISGID);
		iupdat(ip, 1);
		/* "baddr" code here when/if necessary */
	}

	/*
	 * Change back to shared lock.  This allows another race; caller must
	 * deal with this.
	 */

	VN_UNLOCKNODE(ITOV(ip));		/* NOTE: vnode now UNLOCKED! */
	VN_SHARENODE(ITOV(ip));			/* now shared-locked */

	return (bn > 0);
}
