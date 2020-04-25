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

#ifndef	lint
static	char	rcsid[] = "$Header: ufs_bmap.c 2.3 90/05/09 $";
#endif

/*
 * ufs_bmap.c
 *	bmap() procedure -- defines structure of space allocated to
 *	an inode.
 */

/* $Log:	ufs_bmap.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/vm.h"
#include "../h/kernel.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"

/*
 * The following two macros are used to handle indirect blocks:
 *
 * BrelseIndir is used to release an indirect block that hasn't
 * been modified by bmap but may be dirty (from a previous modification).
 *
 * BwriteIndir is used to release an indirect block that has
 * been modified by bmap and must be written.
 *
 * In either case, the B_SYNC option is used to indicate that the
 * dirty indirect block should be written synchronously to disk.
 * The B_SYNC option is used by syncip(), rwip(), and direct_grow()
 * to guarantee file "structure" correctness.
 */

#define BrelseIndir(rwflg, bp) \
{  \
	if (rwflg & B_SYNC && bp->b_flags & B_DELWRI) \
		bwrite(bp); \
	else \
		brelse(bp); \
}

#define BwriteIndir(rwflg, bp) \
{ \
	if (rwflg & B_SYNC) \
		bwrite(bp); \
	else \
		bdwrite(bp); \
}

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 *
 * Caller passed locked inode.
 */
/*VARARGS3*/
daddr_t
bmap(ip, bn, rwflg, size)
	register struct inode *ip;
	register daddr_t bn;
	int	rwflg;		/* B_READ or B_WRITE, optional B_NOCLR | B_SYNC */
	int	size;		/* supplied only when rwflg == B_WRITE */
{
	register struct buf *bp;
	register struct fs *fs;
	register int	i;
	struct buf	*nbp;
	int		osize, nsize;
	int		j, sh;
	int		boff;
	daddr_t		nb, lbn, *bap, pref, blkpref();

	if (bn < 0) {
		u.u_error = EFBIG;
		return ((daddr_t)0);
	}
	fs = ip->i_fs;
	l.rablock = 0;
	l.rasize = 0;				/* conservative */

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 *
	 * Since truncate can leave us with "size" and no block(s)/frag(s)
	 * underneath, only realloc if frag exists.
	 */

	nb = lblkno(fs, ip->i_size);
	if (nb < NDADDR && nb < bn && (rwflg & B_READ) == 0) {
		osize = fragroundup(fs, blkoff(fs, ip->i_size));
		if (osize < fs->fs_bsize && ip->i_db[nb]) {
			/*
			 * Partial block needs to become a full block.
			 */
			bp = realloccg(ip, ip->i_db[nb],
				blkpref(ip, nb, (int)nb, &ip->i_db[0]),
				osize, (int)fs->fs_bsize);
			if (bp == NULL)
				return ((daddr_t)-1);
			ip->i_size = (nb + 1) * fs->fs_bsize;
			ip->i_db[nb] = dbtofsb(fs, bp->b_blkno);
			IMARK(ip,IUPD|ICHG);
			bdwrite(bp);
		} else if (osize) {
			/*
			 * Either full block with short i_size, or
			 * no data allocated but i_size in the middle
			 * of a block.  Need to round size up to block
			 * boundary.
			 */
			ip->i_size = (nb + 1) * fs->fs_bsize;
		}
	}

	/*
	 * The first NDADDR blocks are direct blocks
	 */

	if (bn < NDADDR) {
		nb = ip->i_db[bn];
		if (rwflg & B_READ) {
			if (nb == 0)
				return ((daddr_t)-1);
			goto gotit;
		}
		if (nb == 0 || ip->i_size < (bn + 1) * fs->fs_bsize) {
			boff = blkoff(fs, ip->i_size);
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, boff);
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;
				bp = realloccg(ip, nb,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
					osize, nsize);
			} else {
				/*
				 * MAX() below is for case of truncated to
				 * middle of (sparse) block, then write
				 * earlier in same block; need to allocate
				 * larger of "i_size" and "size".
				 * If i_size is in prev block, we rounded
				 * it to block boundary above; thus boff=0.
				 */
				pref = blkpref(ip, bn, (int)bn, &ip->i_db[0]);
				if (ip->i_size < (bn + 1) * fs->fs_bsize)
					nsize = fragroundup(fs, MAX(boff, size));
				else
					nsize = fs->fs_bsize;
				if (rwflg & B_NOCLR) {
					/*
					 * B_NOCLR only set for IFREG file
					 * when size == fs_bsize.
					 * alloc() returns zero if error.
					 */
					ip->i_db[bn] =
					nb = (daddr_t)alloc(ip, pref, nsize, 1);
					IMARK(ip,IUPD|ICHG);
					return(nb ? nb : (daddr_t)-1);
				}
				bp = alloc(ip, pref, nsize, 0);
			}
			if (bp == NULL)
				return ((daddr_t)-1);
			nb = dbtofsb(fs, bp->b_blkno);
			if ((ip->i_mode&IFMT) == IFDIR)
				/*
				 * Write directory blocks synchronously
				 * so they never appear with garbage in
				 * them on the disk.
				 */
				bwrite(bp);
			else
				bdwrite(bp);
			ip->i_db[bn] = nb;
			IMARK(ip,IUPD|ICHG);
		}
gotit:
		if (bn < NDADDR - 1) {
			l.rablock = fsbtodb(fs, ip->i_db[bn + 1]);
			l.rasize = blksize(fs, ip, bn + 1);
		}
		return (nb);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	lbn = bn;
	bn -= NDADDR;
	for (j = NIADDR; j>0; j--) {
		sh *= NINDIR(fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0) {
		u.u_error = EFBIG;
		return ((daddr_t)0);
	}

	/*
	 * fetch the first indirect block
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		if (rwflg & B_READ)
			return ((daddr_t)-1);
		pref = blkpref(ip, lbn, 0, (daddr_t *)0);
	        bp = alloc(ip, pref, (int)fs->fs_bsize, 0);
		if (bp == NULL)
			return ((daddr_t)-1);
		nb = dbtofsb(fs, bp->b_blkno);
		/*
		 * Write synchronously so that indirect blocks
		 * never point at garbage.
		 */
		bwrite(bp);
		ip->i_ib[NIADDR - j] = nb;
		IMARK(ip,IUPD|ICHG);
	}

	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		bp = bread(ip->i_devvp, fsbtodb(fs, nb), (int)fs->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return ((daddr_t)0);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (bn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (rwflg & B_READ) {
				BrelseIndir(rwflg, bp);
				return ((daddr_t)-1);
			}
			if (pref == 0)
				if (j < NIADDR)
					pref = blkpref(ip, lbn, 0,
						(daddr_t *)0);
				else
					pref = blkpref(ip, lbn, i, &bap[0]);
			/*
			 * Try to avoid clearing data block.
			 * B_NOCLR only set for a IFREG file.
			 */
			if (j == NIADDR && (rwflg & B_NOCLR)) {
				bap[i] = nb = (daddr_t)alloc(ip, pref,
							(int)fs->fs_bsize, 1);
				BwriteIndir(rwflg, bp);
				return(nb ? nb : (daddr_t)-1);
			}
		        nbp = alloc(ip, pref, (int)fs->fs_bsize, 0);
			if (nbp == NULL) {
				BrelseIndir(rwflg, bp);
				return ((daddr_t)-1);
			}
			nb = dbtofsb(fs, nbp->b_blkno);
			if (j < NIADDR || (ip->i_mode&IFMT) == IFDIR)
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks
				 * in directories never contain garbage.
				 */
				bwrite(nbp);
			else
				bdwrite(nbp);
			bap[i] = nb;
			/*
			 * Calculate read-ahead.  Must do before
			 * bdwrite() since `bap' points to bp's data,
			 * which is logically gone after the bdwrite()
			 * (similar for brelse(), below).  This is
			 * redundant if we loop, but doesn't hurt.
			 * TMP only concern.
			 */
			if (i < NINDIR(fs) - 1) {
				l.rablock = fsbtodb(fs, bap[i+1]);
				l.rasize = fs->fs_bsize;
			}
			BwriteIndir(rwflg, bp);
		} else {
			if (i < NINDIR(fs) - 1) {
				l.rablock = fsbtodb(fs, bap[i+1]);
				l.rasize = fs->fs_bsize;
			}
			BrelseIndir(rwflg, bp);
		}
	}
	return (nb);
}
