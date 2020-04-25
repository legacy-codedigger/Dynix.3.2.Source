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
static	char	rcsid[] = "$Header: ufs_alloc.c 2.15 1991/07/09 00:20:09 $";
#endif

/*
 * ufs_alloc.c
 *	File-System allocation/deallocation and other management of space
 *	(inode and data).
 *
 * Note that space (block,frags), inode allocation/deallocation, and
 * cylinder group fuss in general is implicitly mutexed by virtue of
 * using the buf-cache (eg, bread(); fuss; bdwrite(); ).
 */

/* $Log: ufs_alloc.c,v $
 *
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../h/kernel.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

#include "../machine/gate.h"
#include "../machine/intctl.h"

u_long		hashalloc();
ino_t		ialloccg();
daddr_t		alloccg();
daddr_t		alloccgblk();
daddr_t		fragextend();
daddr_t		blkpref();
daddr_t		mapsearch();

extern int		inside[], around[];
extern unsigned char	*fragtbl[];

/*
 * alloc()
 *	Allocate a block in the file system.
 * 
 * The size of the requested block is given, which must be some
 * multiple of fs_fsize and <= fs_bsize.
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate a block:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate a block in the same cylinder group.
 *   4) quadratically rehash into other cylinder groups, until an
 *      available block is located.
 * If no block preference is given the following heirarchy is used
 * to allocate a block:
 *   1) allocate a block in the cylinder group that contains the
 *      inode for the file.
 *   2) quadratically rehash into other cylinder groups, until an
 *      available block is located.
 *
 * Noclr argument ==> don't clear buffer and return file-sys blkno, not
 * buffer.  Used by upper levels (bmap and indirectly vinifod, rwip) to
 * avoid clrbuf and buf-cache overheads if will be over-writing entire buffer.
 *
 * Called with locked inode.
 */

struct buf *
alloc(ip, bpref, size, noclr)
	register struct inode *ip;
	daddr_t	bpref;
	int	size;
	bool_t	noclr;				/* avoid clearing buffer */
{
	register struct fs *fs;
	register struct buf *bp;
	daddr_t	bno;
	int	cg;
	
	fs = ip->i_fs;
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("alloc: bad size");
		/*
		 *+ A request was made to allocate a disk block in a
		 *+ filesystem.  However, the request specified an invalid
		 *+ block size.
		 */
	}

	/*
	 * Quick check for space; no locking, since this is a quick read
	 * and if space is this low, "nospace" will happen soon anyhow.
	 */

	if (size == fs->fs_bsize && fs->fs_cstotal.cs_nbfree == 0)
		goto nospace;
	if (u.u_uid != 0 && freespace(fs, fs->fs_minfree) <= 0)
		goto nospace;
#ifdef QUOTA
	if (ip->i_dquot != NULL) {
		u.u_error = chkdq(ip, (long)btodb(size), 0);
		if (u.u_error)
			return (NULL);
	}
#endif
	if (bpref >= fs->fs_size)
		bpref = 0;
	if (bpref == 0)
		cg = itog(fs, ip->i_number);
	else
		cg = dtog(fs, bpref);

	bno = (daddr_t)hashalloc(ip, cg, (long)bpref, size,
		(u_long (*)())alloccg);
	if (bno <= 0)
		goto nospace;

	ip->i_blocks += btodb(size);
	IMARK( ip, IUPD|ICHG );
	if (noclr)
		return((struct buf *)bno);			/* NOTE: FSB! */
	bp = getblk(ip->i_devvp, fsbtodb(fs, bno), size, 0);
	clrbuf(bp);
	return (bp);
nospace:
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	/*
	 *+ There is no more room on the file system.
	 *+ Corrective action: remove some files.
	 */
	p_sema(&lbolt, PZERO);		/* delay processes ignoring failure */
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * realloccg()
 *	Reallocate a fragment to a bigger size
 *
 * The number and size of the old block is given, and a preference
 * and new size is also specified. The allocator attempts to extend
 * the original block. Failing that, the regular block allocator is
 * invoked to get an appropriate block.
 *
 * Called with locked inode.
 */

struct buf *
realloccg(ip, bprev, bpref, osize, nsize)
	register struct inode *ip;
	daddr_t	bprev;
	daddr_t	bpref;
	int	osize;
	int	nsize;
{
	register struct fs *fs;
	register struct buf *bp, *obp;
	int cg, request;
	daddr_t bno, bn;
	
	fs = ip->i_fs;
	if ((unsigned)osize > fs->fs_bsize || fragoff(fs, osize) != 0 ||
	    (unsigned)nsize > fs->fs_bsize || fragoff(fs, nsize) != 0) {
		printf("dev = 0x%x, bsize = %d, osize = %d, nsize = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, osize, nsize, fs->fs_fsmnt);
		panic("realloccg: bad size");
		/*
		 *+ A request was made to resize a filesystem block
		 *+ fragment to a larger size.  However, either the old
		 *+ fragment size or the new fragment size is
		 *+ invalid.
		 */
	}
	if (u.u_uid != 0 && freespace(fs, fs->fs_minfree) <= 0)
		goto nospace;
	if (bprev == 0) {
		printf("dev = 0x%x, bsize = %d, bprev = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, bprev, fs->fs_fsmnt);
		panic("realloccg: bad bprev");
		/*
		 *+ An attempt was made to extend a 
		 *+ fragment in an invalid filesystem block number.
		 */
	}
#ifdef QUOTA
	if (ip->i_dquot != NULL) {
		u.u_error = chkdq(ip, (long)btodb(nsize - osize), 0);
		if (u.u_error)
			return (NULL);
	}
#endif

	/*
	 * See about extending the fragment.
	 */

	cg = dtog(fs, bprev);
	bno = fragextend(ip, cg, (long)bprev, osize, nsize);
	if (bno != 0) {
		do {
			bp = bread(ip->i_devvp, fsbtodb(fs, bno), osize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return (NULL);
			}
		} while (brealloc(bp, nsize) == 0);
		sema_count(&bp->b_iowait) = 1;			/* IO "done" */
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
		ip->i_blocks += btodb(nsize - osize);
		IMARK( ip, IUPD|ICHG );
		return (bp);
	}

	/*
	 * Can't extend fragment.  Allocate new one and copy.
	 */

	if (bpref >= fs->fs_size)
		bpref = 0;
	switch ((int)fs->fs_optim) {
	case FS_OPTSPACE:
		/*
		 * Allocate an exact sized fragment. Although this makes 
		 * best use of space, we will waste time relocating it if 
		 * the file continues to grow. If the fragmentation is
		 * less than half of the minimum free reserve, we choose
		 * to begin optimizing for time.
		 */
		request = nsize;
		if (fs->fs_minfree < 5 ||
		    fs->fs_cstotal.cs_nffree >
		    fs->fs_dsize * fs->fs_minfree / (2 * 100))
			break;
		fs->fs_optim = FS_OPTTIME;
		break;
	case FS_OPTTIME:
		/*
		 * At this point we have discovered a file that is trying
		 * to grow a small fragment to a larger fragment. To save
		 * time, we allocate a full sized block, then free the 
		 * unused portion. If the file continues to grow, the 
		 * `fragextend' call above will be able to grow it in place
		 * without further copying. If aberrant programs cause
		 * disk fragmentation to grow within 2% of the free reserve,
		 * we choose to begin optimizing for space.
		 */
		request = fs->fs_bsize;
		if (fs->fs_cstotal.cs_nffree <
		    fs->fs_dsize * (fs->fs_minfree - 2) / 100)
			break;
		fs->fs_optim = FS_OPTSPACE;
		break;
	default:
		printf("dev = 0x%x, optim = %d, fs = %s\n",
		    ip->i_dev, fs->fs_optim, fs->fs_fsmnt);
		panic("realloccg: bad optim");
		/*
		 *+ The copy of the optimisation policy datum from a
		 *+ file system super block has either become
		 *+ corrupted or is not supported.
		 */
		/* NOTREACHED */
	}
	bno = (daddr_t)hashalloc(ip, cg, (long)bpref, request,
		(u_long (*)())alloccg);
	if (bno > 0) {
		obp = bread(ip->i_devvp, fsbtodb(fs, bprev), osize);
		if (obp->b_flags & B_ERROR) {
			brelse(obp);
			return (NULL);
		}
		bn = fsbtodb(fs, bno);
		bp = getblk(ip->i_devvp, bn, nsize, 0);
		bcopy(obp->b_un.b_addr, bp->b_un.b_addr, (u_int)osize);
		bzero(bp->b_un.b_addr + osize, (unsigned)nsize - osize);
 		if (obp->b_flags & B_DELWRI) {
 			obp->b_flags &= ~B_DELWRI;

			/*
			 * Delete charge.  Note that this code assumes that
			 * this is the same process as the one which allocated
			 * the block in the first place.  Thus, "deleting the
			 * charge" is only a rough approximation of reality.
			 * By bounding at zero, we are allowing the code to
			 * generally get it right (usually the same process
			 * is responsible for adjacent frag extensions),
			 * while avoiding evil negative numbers, which confuse
			 * other utilities (especially accounting)
			 */
			if (u.u_ru.ru_oublock > 0)
				u.u_ru.ru_oublock--;
 		}
 		obp->b_flags |= B_AGE;
		brelse(obp);
		free(ip, bprev, (off_t)osize);
		if (nsize < request)
			free(ip, bno + numfrags(fs, nsize),
				(off_t)(request - nsize));
		ip->i_blocks += btodb(nsize - osize);
		IMARK( ip, IUPD|ICHG );
		return (bp);
	}
nospace:
	/*
	 * no space available
	 */
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	/*
	 *+ There is no more room on the file system.
	 *+ Corrective action: remove some files.
	 */
	p_sema(&lbolt, PZERO);		/* delay processes ignoring failure */
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * ialloc()
 *	Allocate an inode in the file system.
 * 
 * A preference may be optionally specified. If a preference is given
 * the following hierarchy is used to allocate an inode:
 *   1) allocate the requested inode.
 *   2) allocate an inode in the same cylinder group.
 *   3) quadratically rehash into other cylinder groups, until an
 *      available inode is located.
 * If no inode preference is given the following heirarchy is used
 * to allocate an inode:
 *   1) allocate an inode in cylinder group 0.
 *   2) quadratically rehash into other cylinder groups, until an
 *      available inode is located.
 *
 * Argument inode need not be locked, but caller must hold a reference.
 */

struct inode *
ialloc(pip, ipref, mode)
	register struct inode *pip;
	ino_t	ipref;
	int	mode;
{
	register struct fs *fs;
	register struct inode *ip;
	ino_t	ino;
	int	cg;
#ifdef QUOTA
	struct mount *mp;
#endif	

	fs = pip->i_fs;
	if (fs->fs_cstotal.cs_nifree == 0)
		goto noinodes;
#ifdef QUOTA
	mp = VFSTOM(ITOV(pip)->v_vfsp);
	if (mp->m_qinod != NULL) {
		u.u_error = chkiq(mp, (struct inode *)NULL, u.u_uid, 0);
		if (u.u_error)
			return (NULL);
	}
#endif
	if (ipref >= fs->fs_ncg * fs->fs_ipg)
		ipref = 0;
	cg = itog(fs, ipref);
	ino = (ino_t)hashalloc(pip, cg, (long)ipref, mode, ialloccg);
	if (ino == 0)
		goto noinodes;
	ip = iget(pip->i_dev, pip->i_fs, ino);
	if (ip == NULL) {
		ifree(pip, ino, 0);
		return (NULL);
	}
	if (ip->i_mode) {
		/* in case the dup ialloc is on root, panic */
		if (ip->i_dev == rootdev) {
			printf("mode = 0%o, inum = %d, fs = %s\n",
		    	ip->i_mode, ip->i_number, fs->fs_fsmnt);
			panic("ialloc: dup alloc");
			/*
			 *+ A "free" inode on the filesystem was found to have
			 *+ been already allocated.  This indicates
			 *+ an inconsistency between 
			 *+ an inode's state in the inode bit map and
			 *+ its image in the inode free list.
			 */
		} 
 		/* Mark the FS as READ-ONLY */
 	 	((ITOV(ip))->v_vfsp)->vfs_flag |= VFS_RDONLY;
 		printf("ialloc: dup inode mode = 0%o, inum = %d, fs = %s\n",
		    ip->i_mode, ip->i_number, fs->fs_fsmnt);
 		printf("ialloc: turning filesystem read-only, to recover: umount, fsck, and re-mount\n");
		/*
		 *+ A "free" inode on the filesystem was found to have
		 *+ been already allocated.  This indicates
		 *+ an inconsistency between 
		 *+ an inode's state in the inode bit map and
		 *+ its image in the inode free list.
		 */
		/* we must reverse the iget */
		iunget(ip);
 		u.u_error = EROFS;
 		return(NULL);
	}
	if (ip->i_blocks) {				/* XXX */
		printf("free inode %s/%d had %d blocks\n",
		    fs->fs_fsmnt, ino, ip->i_blocks);
		/*
		 *+ While the kernel was allocating an inode, 
		 *+ it found a "free" inode 
		 *+ with blocks allocated to it.
		 *+ This is an inconsistent state.  
		 */
		ip->i_blocks = 0;
	}
	return (ip);
noinodes:
	fserr(fs, "out of inodes");
	uprintf("\n%s: create/symlink failed, no inodes free\n", fs->fs_fsmnt);
	/*
	 *+ There is no more room on the file system for new inodes.
	 *+ Corrective action: remove some files.
	 */
	p_sema(&lbolt, PZERO);		/* delay processes ignoring failure */
	u.u_error = ENOSPC;
	return (NULL);
}

/*
 * dirpref()
 *	Find a cylinder to place a directory.
 *
 * The policy implemented by this algorithm is to select from
 * among those cylinder groups with above the average number of
 * free inodes, the one with the smallest number of directories.
 *
 * Caller guarantees validity of "fs".
 *
 * We don't lock the fs structure, due to the heuristic & read-only
 * nature of this algorithm.
 */

ino_t
dirpref(fs)
	register struct fs *fs;
{
	int cg, minndir, mincg, avgifree;

	avgifree = fs->fs_cstotal.cs_nifree / fs->fs_ncg;
	minndir = fs->fs_ipg;
	mincg = 0;
	for (cg = 0; cg < fs->fs_ncg; cg++)
		if ((fs->fs_cs(fs, cg).cs_ndir < minndir) &&
		    (fs->fs_cs(fs, cg).cs_nifree >= avgifree)) {
			mincg = cg;
			minndir = fs->fs_cs(fs, cg).cs_ndir;
		}
	return ((ino_t)(fs->fs_ipg * mincg));
}

/*
 * blkpref()
 *	Select the desired position for the next block in a file.
 *
 * The file is logically divided into sections. The first section is
 * composed of the direct blocks. Each additional section contains
 * fs_maxbpg blocks.
 * 
 * If no blocks have been allocated in the first section, the policy is to
 * request a block in the same cylinder group as the inode that describes
 * the file. If no blocks have been allocated in any other section, the
 * policy is to place the section in a cylinder group with a greater than
 * average number of free blocks.  An appropriate cylinder group is found
 * by using a rotor that sweeps the cylinder groups. When a new group of
 * blocks is needed, the sweep begins in the cylinder group following the
 * cylinder group from which the previous allocation was made. The sweep
 * continues until a cylinder group with greater than the average number
 * of free blocks is found. If the allocation is for the first block in an
 * indirect block, the information on the previous allocation is unavailable;
 * here a best guess is made based upon the logical block number being
 * allocated.
 * 
 * If a section is already partially allocated, the policy is to
 * contiguously allocate fs_maxcontig blocks.  The end of one of these
 * contiguous blocks and the beginning of the next is physically separated
 * so that the disk head will be in transit between them for at least
 * fs_rotdelay milliseconds.  This is to allow time for the processor to
 * schedule another I/O transfer.
 */

daddr_t
blkpref(ip, lbn, indx, bap)
	struct inode *ip;
	daddr_t	lbn;
	int	indx;
	daddr_t	*bap;
{
	register struct fs *fs;
	register int cg;
	int avgbfree, startcg;
	daddr_t nextblk;

	fs = ip->i_fs;
	if (indx % fs->fs_maxbpg == 0 || bap[indx - 1] == 0) {
		if (lbn < NDADDR) {
			cg = itog(fs, ip->i_number);
			return (fs->fs_fpg * cg + fs->fs_frag);
		}
		/*
		 * Find a cylinder with greater than average number of
		 * unused data blocks.  Similar comments on locking.
		 */
		if (indx == 0 || bap[indx - 1] == 0)
			startcg = itog(fs, ip->i_number) + lbn / fs->fs_maxbpg;
		else
			startcg = dtog(fs, bap[indx - 1]) + 1;
		startcg %= fs->fs_ncg;
		avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		for (cg = startcg; cg < fs->fs_ncg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		for (cg = 0; cg <= startcg; cg++)
			if (fs->fs_cs(fs, cg).cs_nbfree >= avgbfree) {
				fs->fs_cgrotor = cg;
				return (fs->fs_fpg * cg + fs->fs_frag);
			}
		return (NULL);
	}
	/*
	 * One or more previous blocks have been laid out. If less
	 * than fs_maxcontig previous blocks are contiguous, the
	 * next block is requested contiguously, otherwise it is
	 * requested rotationally delayed by fs_rotdelay milliseconds.
	 *
	 * Note: should (indx < maxcontig) ==> pick next block?  Ie,
	 * this appears to ignore contiguity in 1st maxcontig blocks.
	 */
	nextblk = bap[indx - 1] + fs->fs_frag;
	if (indx > fs->fs_maxcontig &&
	    bap[indx - fs->fs_maxcontig] + blkstofrags(fs, fs->fs_maxcontig)
	    != nextblk)
		return (nextblk);
	if (fs->fs_rotdelay != 0)
		/*
		 * Here we convert ms of delay to frags as:
		 * (frags) = (ms) * (rev/sec) * (sect/rev) /
		 *	((sect/frag) * (ms/sec))
		 * then round up to the next block.
		 */
		nextblk += roundup(fs->fs_rotdelay * fs->fs_rps * fs->fs_nsect /
		    (NSPF(fs) * 1000), fs->fs_frag);
	return (nextblk);
}

/*
 * hashalloc()
 *	Implement the cylinder overflow algorithm.
 *
 * The policy implemented by this algorithm is:
 *   1) allocate the block in its requested cylinder group.
 *   2) quadratically rehash on the cylinder group number.
 *   3) brute force search for a free block.
 */
/*VARARGS5*/
u_long
hashalloc(ip, cg, pref, size, allocator)
	struct inode *ip;
	int	cg;
	long	pref;
	int	size;	/* size for data blocks, mode for inodes */
	u_long	(*allocator)();
{
	register struct fs *fs;
	long	result;
	int	i;
	int	icg = cg;

	fs = ip->i_fs;
	/*
	 * 1: preferred cylinder group
	 */
	result = (*allocator)(ip, cg, pref, size);
	if (result)
		return (result);
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		result = (*allocator)(ip, cg, 0, size);
		if (result)
			return (result);
	}
	/*
	 * 3: brute force search
	 * Note that we start at i == 2, since 0 was checked initially,
	 * and 1 is always checked in the quadratic rehash.
	 */
	cg = (icg + 2) % fs->fs_ncg;
	for (i = 2; i < fs->fs_ncg; i++) {
		result = (*allocator)(ip, cg, 0, size);
		if (result)
			return (result);
		cg++;
		if (cg == fs->fs_ncg)
			cg = 0;
	}
	return (NULL);
}

/*
 * fragextend()
 *	Determine whether a fragment can be extended.
 *
 * Check to see if the necessary fragments are available, and 
 * if they are, allocate them.
 */



daddr_t
fragextend(ip, cg, bprev, osize, nsize)
	struct inode *ip;
	int	cg;
	long	bprev;
	int	osize;
	int	nsize;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	long	bno;
	int	frags;
	int	ofrags;
	int	bbase;
	int	i;
	int	delta;
	GATESPL(s);

	fs = ip->i_fs;
	/*
	 * no locking
	 */
 	if (fs->fs_cs(fs, cg).cs_nffree < numfrags(fs, nsize - osize))	
		return (NULL);
	frags = numfrags(fs, nsize);
	ofrags = numfrags(fs, osize);
	ASSERT(frags >= ofrags, "fragextend: nsize");
	/*
	 *+ When checking whether a fragment could be extended, the kernel
	 *+ discovered that the "extended" fragment would be smaller than
	 *+ the old one.
	 */

 	bbase = fragnum(fs, bprev);
 	if (bbase > fragnum(fs, (bprev + frags - 1))) {
		/* cannot extend across a block boundry */
		return (NULL);
	}
	bp = bread(ip->i_devvp, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || !cg_chkmagic(cgp)) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	bno = dtogd(fs, bprev);
	for (i = ofrags; i < frags; i++)
		if (isclr(cg_blksfree(cgp), bno + i)) {
			brelse(bp);
			return (NULL);
		}
	/*
	 * the current fragment can be extended
	 * deduct the count on fragment being extended into
	 * increase the count on the remaining fragment (if any)
	 * allocate the extended piece
	 */
	for (i = frags; i < fs->fs_frag - bbase; i++)
		if (isclr(cg_blksfree(cgp), bno + i))
			break;
	cgp->cg_frsum[i - ofrags]--;
	if (i != frags)
		cgp->cg_frsum[i - frags]++;
	for (i = ofrags; i < frags; i++) {
		clrbit(cg_blksfree(cgp), bno + i);
		cgp->cg_cs.cs_nffree--;
	}
	delta = frags - ofrags;
	FSLOCK(fs, s);
	fs->fs_cstotal.cs_nffree -= delta;
	fs->fs_cs(fs, cg).cs_nffree -= delta;
	fs->fs_fmod++;
	FSUNLOCK(fs, s);
	bdwrite(bp);
	return (bprev);
}

/*
 * alloccg()
 *	Determine whether a block can be allocated.
 *
 * Check to see if a block of the apprpriate size is available,
 * and if it is, allocate it.
 */

daddr_t
alloccg(ip, cg, bpref, size)
	struct inode *ip;
	int	cg;
	daddr_t	bpref;
	int	size;
{
	register struct fs *fs;
	register struct buf *bp;
	register struct cg *cgp;
	register int i;
	int	bno;
	int	frags;
	int	allocsiz;
	GATESPL(s);

	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nbfree == 0 && size == fs->fs_bsize)
		return (NULL);
	bp = bread(ip->i_devvp, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || !cg_chkmagic(cgp) ||
	    (cgp->cg_cs.cs_nbfree == 0 && size == fs->fs_bsize)) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;		/* no lock; just seconds */
	if (size == fs->fs_bsize) {
		bno = alloccgblk(fs, cgp, bpref);
		bdwrite(bp);
		return (bno);
	}
	/*
	 * check to see if any fragments are already available
	 * allocsiz is the size which will be allocated, hacking
	 * it down to a smaller size if necessary
	 */
	frags = numfrags(fs, size);
	for (allocsiz = frags; allocsiz < fs->fs_frag; allocsiz++)
		if (cgp->cg_frsum[allocsiz] != 0)
			break;
	if (allocsiz == fs->fs_frag) {
		/*
		 * no fragments were available, so a block will be 
		 * allocated, and hacked up
		 */
		if (cgp->cg_cs.cs_nbfree == 0) {
			brelse(bp);
			return (NULL);
		}
		bno = alloccgblk(fs, cgp, bpref);
		bpref = dtogd(fs, bno);
		for (i = frags; i < fs->fs_frag; i++)
			setbit(cg_blksfree(cgp), bpref + i);
		i = fs->fs_frag - frags;
		cgp->cg_frsum[i]++;
		cgp->cg_cs.cs_nffree += i;
		FSLOCK(fs, s);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		fs->fs_fmod++;
		FSUNLOCK(fs, s);
		bdwrite(bp);
		return (bno);
	}
	bno = mapsearch(fs, cgp, bpref, allocsiz);
	if (bno < 0) {
		brelse(bp);
		return (NULL);
	}
	for (i = 0; i < frags; i++)
		clrbit(cg_blksfree(cgp), bno + i);
	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz)
		cgp->cg_frsum[allocsiz - frags]++;
	cgp->cg_cs.cs_nffree -= frags;
	FSLOCK(fs, s);
	fs->fs_cstotal.cs_nffree -= frags;
	fs->fs_cs(fs, cg).cs_nffree -= frags;
	fs->fs_fmod++;
	FSUNLOCK(fs, s);
	bdwrite(bp);
	return (cg * fs->fs_fpg + bno);
}

/*
 * alloccgblk()
 *	Allocate a block in a cylinder group.
 *
 * This algorithm implements the following policy:
 *   1) allocate the requested block.
 *   2) allocate a rotationally optimal block in the same cylinder.
 *   3) allocate the next available block on the block rotor for the
 *      specified cylinder group.
 * Note that this routine only allocates fs_bsize blocks; these
 * blocks may be fragmented by the routine that allocates them.
 *
 * Caller arranged mutex of "cgp".
 */

daddr_t
alloccgblk(fs, cgp, bpref)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t	bpref;
{
	register short	*cylbp;
	register int i;
	daddr_t	bno;
	int	cylno, pos, delta;
	GATESPL(s);

	if (bpref == 0) {
		bpref = cgp->cg_rotor;
		goto norot;
	}
	bpref = blknum(fs, bpref);
	bpref = dtogd(fs, bpref);
	/*
	 * if the requested block is available, use it
	 */
	if (isblock(fs, cg_blksfree(cgp), fragstoblks(fs, bpref))) {
		bno = bpref;
		goto gotit;
	}
	/*
	 * check for a block available on the same cylinder
	 */
	cylno = cbtocylno(fs, bpref);
	if (cg_blktot(cgp)[cylno] == 0)
		goto norot;
	if (fs->fs_cpc == 0) {
		/*
		 * block layout info is not available, so just have
		 * to take any block in this cylinder.
		 */
		bpref = howmany(fs->fs_spc * cylno, NSPF(fs));
		goto norot;
	}
	/*
	 * check the summary information to see if a block is 
	 * available in the requested cylinder starting at the
	 * requested rotational position and proceeding around.
	 */
	cylbp = cg_blks(fs, cgp, cylno);
	pos = cbtorpos(fs, bpref);
	for (i = pos; i < fs->fs_nrpos; i++)
		if (cylbp[i] > 0)
			break;
	if (i == fs->fs_nrpos)
		for (i = 0; i < pos; i++)
			if (cylbp[i] > 0)
				break;
	if (cylbp[i] > 0) {
		/*
		 * found a rotational position, now find the actual
		 * block. A panic if none is actually there.
		 * Note: fs_postbl and fs_rotbl are read-only.
		 */
		pos = cylno % fs->fs_cpc;
		bno = (cylno - pos) * fs->fs_spc / NSPB(fs);
		if (fs_postbl(fs, pos)[i] == -1) {
			printf("pos = %d, i = %d, fs = %s\n",
			    pos, i, fs->fs_fsmnt);
			panic("alloccgblk: cyl groups corrupted");
			/*
			 *+ When allocating a filesystem block 
			 *+ within a cylinder group, the kernel discovered
			 *+ that the cylinder group data structures
			 *+ contained inconsistent data.
			 */
		}
		for (i = fs_postbl(fs, pos)[i];; ) {
			if (isblock(fs, cg_blksfree(cgp), bno + i)) {
				bno = blkstofrags(fs, (bno + i));
				goto gotit;
			}
			delta = fs_rotbl(fs)[i];
			if (delta <= 0 ||
			    delta + i > fragstoblks(fs, fs->fs_fpg))
				break;
			i += delta;
		}
		printf("pos = %d, i = %d, fs = %s\n", pos, i, fs->fs_fsmnt);
		panic("alloccgblk: can't find blk in cyl");
		/*
		 *+ When allocating a filesystem block 
		 *+ within a cylinder group, the kernel discovered
		 *+ that the cylinder group data structures
		 *+ contained inconsistent data.
		 */
	}
norot:
	/*
	 * no blocks in the requested cylinder, so take next
	 * available one in this cylinder group.
	 */
	bno = mapsearch(fs, cgp, bpref, (int)fs->fs_frag);
	if (bno < 0)
		return (NULL);
	cgp->cg_rotor = bno;
gotit:
	clrblock(fs, cg_blksfree(cgp), (long)fragstoblks(fs, bno));
	cgp->cg_cs.cs_nbfree--;
	FSLOCK(fs, s);
	fs->fs_cstotal.cs_nbfree--;
	fs->fs_cs(fs, cgp->cg_cgx).cs_nbfree--;
	fs->fs_fmod++;
	FSUNLOCK(fs, s);
	cylno = cbtocylno(fs, bno);
	cg_blks(fs, cgp, cylno)[cbtorpos(fs, bno)]--;
	cg_blktot(cgp)[cylno]--;
	fs->fs_fmod++;
	return (cgp->cg_cgx * fs->fs_fpg + bno);
}

/*
 * ialloccg()
 *	Determine whether an inode can be allocated.
 *
 * Check to see if an inode is available, and if it is,
 * allocate it using the following policy:
 *   1) allocate the requested inode.
 *   2) allocate the next available inode after the requested
 *      inode in the specified cylinder group.
 *
 * Argument "ip" need not be locked, but caller holds a reference.
 */

ino_t
ialloccg(ip, cg, ipref, mode)
	struct inode *ip;
	int	cg;
	daddr_t	ipref;
	int	mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	struct buf *bp;
	int start, len, loc, map, i;

	GATESPL(s);
	fs = ip->i_fs;
	if (fs->fs_cs(fs, cg).cs_nifree == 0)
		return (NULL);
	bp = bread(ip->i_devvp, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || !cg_chkmagic(cgp) ||
	    cgp->cg_cs.cs_nifree == 0) {
		brelse(bp);
		return (NULL);
	}
	cgp->cg_time = time.tv_sec;
	if (ipref) {
		ipref %= fs->fs_ipg;
		if (isclr(cg_inosused(cgp), ipref))
			goto gotit;
	}
	start = cgp->cg_irotor / NBBY;
	len = howmany(fs->fs_ipg - cgp->cg_irotor, NBBY);
	loc = skpc((u_char)0xff, (u_int)len,
		(u_char *)&cg_inosused(cgp)[start]);
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = skpc((u_char)0xff, (u_int)len,
			(u_char *)&cg_inosused(cgp)[0]);
		if (loc == 0) {
			printf("cg = %s, irotor = %d, fs = %s\n",
			    cg, cgp->cg_irotor, fs->fs_fsmnt);
			panic("ialloccg: map corrupted");
			/*
			 *+ The internal inode cylinder group table
			 *+ has become corrupted.
			 */
			/* NOTREACHED */
		}
	}
	i = start + len - loc;
	map = cg_inosused(cgp)[i];
	ipref = i * NBBY;
	for (i = 1; i < (1 << NBBY); i <<= 1, ipref++) {
		if ((map & i) == 0) {
			cgp->cg_irotor = ipref;
			goto gotit;
		}
	}
	printf("fs = %s\n", fs->fs_fsmnt);
	panic("ialloccg: block not in map");
	/*
	 *+ A cylinder group was expected to contain a free block, but
	 *+ no free blocks were found during a search of the cylinder group
	 *+ free list.  This implies inconsistent data structures.
	 */

	/* NOTREACHED */
gotit:
	setbit(cg_inosused(cgp), ipref);
	cgp->cg_cs.cs_nifree--;
	FSLOCK(fs, s);
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	fs->fs_fmod++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir++;
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++;
	}
	FSUNLOCK(fs, s);
	bdwrite(bp);
	return (cg * fs->fs_ipg + ipref);
}

/*
 * free()
 *	Free a block or fragment.
 *
 * The specified block or fragment is placed back in the
 * free map. If a fragment is deallocated, a possible 
 * block reassembly is checked.
 *
 * Caller must hold reference to inode.
 */
free(ip, bno, size)
	register struct inode *ip;
	daddr_t	bno;
	off_t	size;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	register int i;
	int	cg, blk, frags, bbase;
	static	char dup_free[] = "dev = 0x%x, block = %d (%d,%d), fs = %s\n";
	GATESPL(s);

	fs = ip->i_fs;
	if ((unsigned)size > fs->fs_bsize || fragoff(fs, size) != 0) {
		printf("dev = 0x%x, bsize = %d, size = %d, fs = %s\n",
		    ip->i_dev, fs->fs_bsize, size, fs->fs_fsmnt);
		panic("free: bad size");
		/*
		 *+ The kernel freed a block having an incorrect size.
		 *+ This indicates either a filesystem software bug
		 *+ or corrupted data structures on disk in the filesystem.
		 */
	}
	cg = dtog(fs, bno);
	if (badblock(fs, bno)) {
		printf("bad block %d, ino %d\n", bno, ip->i_number);
		/*
		 *+ The kernel freed a block having an invalid block
		 *+ number.  This indicates possible corruption of
		 *+ data structures on disk in the filesystem.
		 */
		return;
	}
	bp = bread(ip->i_devvp, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || !cg_chkmagic(cgp)) {
		brelse(bp);
		return;
	}
	cgp->cg_time = time.tv_sec;
	bno = dtogd(fs, bno);
	if (size == fs->fs_bsize) {
		if (isblock(fs, cg_blksfree(cgp), fragstoblks(fs, bno))) {
			printf(dup_free, ip->i_dev,
				cg * fs->fs_fpg + bno, cg, bno, fs->fs_fsmnt);
			panic("free: freeing free block");
			/*
			 *+ The kernel freed a filesystem block that was
			 *+ already on the free list.
			 */
		}
		setblock(fs, cg_blksfree(cgp), fragstoblks(fs, bno));
		cgp->cg_cs.cs_nbfree++;
		i = cbtocylno(fs, bno);
		cg_blks(fs, cgp, i)[cbtorpos(fs, bno)]++;
		cg_blktot(cgp)[i]++;
		FSLOCK(fs, s);
		fs->fs_cstotal.cs_nbfree++;
		fs->fs_cs(fs, cg).cs_nbfree++;
		fs->fs_fmod++;
		FSUNLOCK(fs, s);
	} else {
		bbase = bno - fragnum(fs, bno);
		/*
		 * decrement the counts associated with the old frags
		 */
		blk = blkmap(fs, cg_blksfree(cgp), bbase);
		fragacct(fs, blk, cgp->cg_frsum, -1);
		/*
		 * deallocate the fragment
		 */
		frags = numfrags(fs, size);
		for (i = 0; i < frags; i++) {
			if (isset(cg_blksfree(cgp), bno + i)) {
				printf(dup_free, ip->i_dev,
					cg * fs->fs_fpg + bno + i, cg, bno + i,
					fs->fs_fsmnt);
				panic("free: freeing free frag");
				/*
				 *+ The kernel freed a filesystem fragment 
				 *+ that overlapped with a fragment
				 *+ already on the free list.
				 */
			}
			setbit(cg_blksfree(cgp), bno + i);
		}
		cgp->cg_cs.cs_nffree += i;
		FSLOCK(fs, s);
		fs->fs_cstotal.cs_nffree += i;
		fs->fs_cs(fs, cg).cs_nffree += i;
		fs->fs_fmod++;
		FSUNLOCK(fs, s);
		/*
		 * add back in counts associated with the new frags
		 */
		blk = blkmap(fs, cg_blksfree(cgp), bbase);
		fragacct(fs, blk, cgp->cg_frsum, 1);
		/*
		 * if a complete block has been reassembled, account for it
		 */
		if (isblock(fs, cg_blksfree(cgp),
		    (daddr_t)fragstoblks(fs, bbase))) {
			cgp->cg_cs.cs_nffree -= fs->fs_frag;
			cgp->cg_cs.cs_nbfree++;
			FSLOCK(fs, s);
			fs->fs_cstotal.cs_nffree -= fs->fs_frag;
			fs->fs_cs(fs, cg).cs_nffree -= fs->fs_frag;
			fs->fs_cstotal.cs_nbfree++;
			fs->fs_cs(fs, cg).cs_nbfree++;
			FSUNLOCK(fs, s);
			i = cbtocylno(fs, bbase);
			cg_blks(fs, cgp, i)[cbtorpos(fs, bbase)]++;
			cg_blktot(cgp)[i]++;
		}
	}
	bdwrite(bp);
}

/*
 * ifree()
 *	Free an inode.
 *
 * The specified inode is placed back in the free map.
 *
 * Caller has locked the inode.
 */

ifree(ip, ino, mode)
	struct inode *ip;
	ino_t	ino;
	int	mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int	cg;
	GATESPL(s);

	fs = ip->i_fs;
	if ((unsigned)ino >= fs->fs_ipg*fs->fs_ncg) {
		printf("dev = 0x%x, ino = %d, fs = %s\n",
		    ip->i_dev, ino, fs->fs_fsmnt);
		panic("ifree: range");
		/*
		 *+ The kernel attempted to free an inode number that
		 *+ is outside the range of inode numbers in that
		 *+ filesystem.
		 */
	}
	cg = itog(fs, ino);
	bp = bread(ip->i_devvp, fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize);
	cgp = bp->b_un.b_cg;
	if (bp->b_flags & B_ERROR || !cg_chkmagic(cgp)) {
		brelse(bp);
		return;
	}
	cgp->cg_time = time.tv_sec;
	ino %= fs->fs_ipg;
	if (isclr(cg_inosused(cgp), ino)) {
		printf("dev = 0x%x, ino = %d (%d,%d), fs = %s\n", ip->i_dev,
				cg * fs->fs_ipg + ino, cg, ino, fs->fs_fsmnt);
		panic("ifree: freeing free inode");
		/*
		 *+ The kernel freed an inode that was already on the list
		 *+ of free inodes for that filesystem.
		 */
	}
	clrbit(cg_inosused(cgp), ino);
	if (ino < cgp->cg_irotor)
		cgp->cg_irotor = ino;
	cgp->cg_cs.cs_nifree++;
	FSLOCK(fs, s);
	fs->fs_cstotal.cs_nifree++;
	fs->fs_cs(fs, cg).cs_nifree++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_cs.cs_ndir--;
		fs->fs_cstotal.cs_ndir--;
		fs->fs_cs(fs, cg).cs_ndir--;
	}
	fs->fs_fmod++;
	FSUNLOCK(fs, s);
	bdwrite(bp);
}

/*
 * mapsearch()
 *	Find a block of the specified size in the specified cylinder group.
 *
 * It is a panic if a request is made to find a block if none are
 * available.
 */

daddr_t
mapsearch(fs, cgp, bpref, allocsiz)
	register struct fs *fs;
	register struct cg *cgp;
	daddr_t	bpref;
	int	allocsiz;
{
	daddr_t	bno;
	int	start, len, loc, i;
	int	blk, field, subfield, pos;

	/*
	 * find the fragment by searching through the free block
	 * map for an appropriate bit pattern
	 */
	if (bpref)
		start = dtogd(fs, bpref) / NBBY;
	else
		start = cgp->cg_frotor / NBBY;
	len = howmany(fs->fs_fpg, NBBY) - start;
	loc = scanc((unsigned)len, (char *)&cg_blksfree(cgp)[start],
		(char *)fragtbl[fs->fs_frag],
		(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));
	if (loc == 0) {
		len = start + 1;
		start = 0;
		loc = scanc((unsigned)len, (caddr_t)&cg_blksfree(cgp)[0],
			(caddr_t)fragtbl[fs->fs_frag],
			(int)(1 << (allocsiz - 1 + (fs->fs_frag % NBBY))));

		if (loc == 0) {
#ifdef DEBUG
			printf("start = %d, len = %d, fs = %s\n",
			    start, len, fs->fs_fsmnt);
			panic("mapsearch: map corrupted");
#endif DEBUG
			return (-1);
			/* NOTREACHED */
		}
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * found the byte in the map
	 * sift through the bits to find the selected frag
	 */
	for (i = bno + NBBY; bno < i; bno += fs->fs_frag) {
		blk = blkmap(fs, cg_blksfree(cgp), bno);
		blk <<= 1;
		field = around[allocsiz];
		subfield = inside[allocsiz];
		for (pos = 0; pos <= fs->fs_frag - allocsiz; pos++) {
			if ((blk & field) == subfield)
				return (bno + pos);
			field <<= 1;
			subfield <<= 1;
		}
	}
	printf("bno = %d (%d,%d), fs = %s\n",
		cgp->cg_cgx * fs->fs_fpg + bno, cgp->cg_cgx, bno, fs->fs_fsmnt);
	panic("mapsearch: block not in map");
	/*
	 *+ A cylinder group was expected to contain a free block, but
	 *+ no free blocks were found during a search of the cylinder group
	 *+ free list.  This implies inconsistent data structures.
	 */
	return (-1);
}

/*
 * fserr()
 *	Print the name of a file system with an error diagnostic.
 * 
 * The form of the error message is:
 *	fs: error message
 */

fserr(fs, cp)
	struct fs *fs;
	char	*cp;
{
	printf("%s: %s\n", fs->fs_fsmnt, cp);
	/*
	 *+ The specified file system error occured on the the specified
	 *+ file system.
	 *+ The errors are one of "file system full" "out of inode" or
	 *+ "bad block".
	 */
}

extern struct inode *ifreeh, **ifreet;
/*
 * iunget()
 *	Reverse iget for ialloc ONLY
 *
 * Replace the inode on the free list and clear up the flags
 * We expect the inode to have come form the free list and therefore we
 * do not release Bamp or Quota structures
 *
 */
iunget(ip)
	struct inode *ip;
{
	spl_t s;

	/* lock inode list */
	s = p_lock(&ino_list, SPLFS);

	/* put back on the free list */
	if (ifreeh) {
		*ifreet = ip;
		ip->i_freeb = ifreet;
	} else {
		ifreeh = ip;
		ip->i_freeb = &ifreeh;
	}
	ip->i_freef = NULL;
	ifreet = &ip->i_freef;
	
	/* clear up the flag */
	ip->i_flag = IFREE;
	ip->i_mode = 0;

	IUNLOCK(ip);
	v_lock(&ino_list, s);
}
