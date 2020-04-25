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
static	char	rcsid[] = "$Header: ufs_subr.c 2.19 90/06/09 $";
#endif

/*
 * ufs_subr.c
 *	Various file-system related subroutines.  Some shared with utilities.
 */

/* $Log:	ufs_subr.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"
#include "../h/kernel.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

/*
 * update()
 *	Update inodes, super-blocks to disks.
 *
 * Update is the internal name of 'sync'. It goes through the 
 * mount table to initiate the writing of the modified super blocks
 * and it goes through the inodes to write modified nodes.
 * The upper level code is responsible for flushing the disk buffer cache.
 */

update()
{
	register struct inode *ip;
	register struct mount *mp;
	register struct fs *fs;
	GATESPL(s);

	/*
	 * If vfs_mutex available, Write back modified superblocks.
	 * Note that we turn off "fmod", then unlock the file-sys;
	 * this allows mods to occur whilst we're updating. This is
	 * ok, since unmount does an explicit sbupdate().
	 */
	if (cp_sema(&vfs_mutex)) {
		for (mp = mounttab; mp < mountNMOUNT; mp++) {
			if (mp->m_bufp == NULL || mp->m_dev == NODEV)
				continue;
			fs = mp->m_bufp->b_un.b_fs;
			if (fs->fs_fmod == 0)		/* ok no lock */
				continue;
			if (fs->fs_ronly) {
				printf("fs = %s\n", fs->fs_fsmnt);
				panic("update: rofs mod");
				/*
				 *+ When synching the disks, the kernel found
				 *+ that a filesystem mounted read-only had
				 *+ been modified.
				 */
			}
			FSLOCK(fs, s);
			fs->fs_fmod = 0;
			fs->fs_time = time.tv_sec;
			FSUNLOCK(fs, s);
			sbupdate(mp->m_dev, fs);
		}
		v_sema(&vfs_mutex);
	}

	/*
	 * Write back each (modified) inode.  Ignore busy inodes.
	 *
	 * For each inode, if it's modified and available (not locked),
	 * *then* lock inode hash+free list and update the inode if it's
	 * still available.  This avoids list locking for uninteresting
	 * inodes.  Loosing a race is ok (we don't modify anything),
	 * since this is heuristic update.
	 */
	for (ip = inode; ip < inodeNINODE; ip++) {
		if ((ip->i_flag&(IUPD|IACC|ICHG)) == 0		/* not mod'd, */
		||  !RWSEMA_IDLE(&ip->i_mutex))			/* or locked */
			continue;				/* ==> ignore */
		if (igrab(ip)) {				/* try for it */
			iupdat(ip, 0);				/* update it */
			IPUT(ip);				/* done */
		}
	}
}

/*
 * syncip()
 *	Flush all the blocks associated with an inode.
 *
 * Note that we make a more stringent check of
 * writing out any block in the buffer pool that may
 * overlap the inode. This brings the inode up to
 * date with recent mods to the cooked device.
 *
 * This can be nasty on a huge sparse file.
 *
 * Called with locked inode.
 */

syncip(ip)
	register struct inode *ip;
{
	register struct fs *fs;
	register long	lbn;
	register long	lastlbn;
	register daddr_t blkno;

	ASSERT_DEBUG(RWSEMA_WRBUSY(&ip->i_mutex), "syncip: unlocked");
	fs = ip->i_fs;
	lastlbn = howmany(ip->i_size, fs->fs_bsize);
	for (lbn = 0; lbn < lastlbn; lbn++) {
		blkno = bmap(ip, lbn, B_READ|B_SYNC);
		if (blkno != (daddr_t)-1)
			blkflush(ip->i_devvp, fsbtodb(fs, blkno),
				blksize(fs, ip, lbn));
	}
	ip->i_pflags &= ~IP_EXECMOD;		/* file now sync'd */
	IMARK(ip, ICHG);
	iupdat(ip, 1);
}

/*
 * fragacct()
 *	Update the frsum fields to reflect addition or deletion 
 *	of some frags.
 *
 * fraglist is part of a cylinder-group; caller locked this.
 */

extern	int around[];
extern	int inside[];
extern	u_char *fragtbl[];

fragacct(fs, fragmap, fraglist, cnt)
	register struct fs *fs;
	int	fragmap;
	long	fraglist[];
	int	cnt;
{
	register int field;
	register int subfield;
	register int siz;
	register int pos;
	int	inblk;
	int	mask;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	mask = 1 << (1 + (fs->fs_frag % NBBY));
	for (siz = 1; siz < fs->fs_frag; siz++, mask <<= 1) {
		if ((inblk & mask) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

/*
 * badblock()
 *	Check that a specified block number is in range.
 */

badblock(fs, bn)
	struct fs *fs;
	daddr_t bn;
{
	if ((unsigned)bn >= fs->fs_size) {
		printf("bad block %d, ", bn);
		/*
		 *+ A bad block number was encountered while the kernel was 
		 *+ doing a filesystem block allocation or
		 *+ freeing a filesystem block.  This
		 *+ probably indicates a corrupted filesystem.
		 *+ Corrective action:  run fsck on the filesystem.
		 */
		fserr(fs, "bad block");
		return (1);
	}
	return (0);
}

/*
 * isblock()
 *	check if a block is available
 */

isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		panic("isblock");
		/*
		 *+ On attempting to allocate a fileystem block, the kernel
		 *+ discovered that the superblock contained an illegal
		 *+ value for the number of fragments in a block.
		 */
		return (NULL);
	}
}

/*
 * clrblock()
 *	take a block out of the map
 */

clrblock(fs, cp, h)
	struct fs *fs;
	u_char *cp;
	daddr_t h;
{

	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		panic("clrblock");
		/*
		 *+ On attempting to allocate a fileystem block, the kernel
		 *+ discovered that the superblock contained an illegal
		 *+ value for the number of fragments in a block.
		 */
	}
}

/*
 * setblock()
 *	put a block into the map
 */

setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{

	switch (fs->fs_frag) {

	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		panic("setblock");
		/*
		 *+ On attempting to free a fileystem block, the kernel
		 *+ discovered that the superblock contained an illegal
		 *+ value for the number of fragments in a block.
		 */
	}
}

/*
 * trygetfs()
 *	get fs associated with dev (if there is one)
 *
 * Called with vfs_mutex sema locked to prevent concurrent unmount().
 * Returns pointer to fs if found, else NULL.
 */
struct fs *
trygetfs(dev)
	dev_t dev;
{
	register struct mount *mp;
	struct mount *getmp();

	mp = getmp(dev);
	if (mp == NULL) {
		return (NULL);
	}
	return (mp->m_bufp->b_un.b_fs);
}

struct mount *
getmp(dev)
	dev_t dev;
{
	register struct mount *mp;
	register struct fs *fs;

	for (mp = &mounttab[0]; mp < mountNMOUNT; mp++) {
		if (mp->m_bufp == NULL || mp->m_dev != dev)
			continue;
		fs = mp->m_bufp->b_un.b_fs;
		if (fs->fs_magic != FS_MAGIC) {
			printf("dev = 0x%x, fs = %s\n", dev, fs->fs_fsmnt);
			panic("getmp: bad magic");
			/*
			 *+ The kernel found an in-core superblock with an 
			 *+ invalid magic number.
			 */
		}

		return (mp);
	}
	return (NULL);
}

#ifdef	NFS
/*
 * nfsgetmp()
 *	Identical to getmp() but does handshake with ufs_unmount().
 *
 * Used by NFS's fhtovp().
 * ***Warning: this is EXCEEDINGLY UGLY and will be replaced.*** It is
 * only exceptable because fhtovp() currently assumes ufs is the only
 * type of local file system. This will be fixed ala NFS3.2.
 */

struct mount *
nfsgetmp(dev)
	dev_t dev;
{
	register struct mount *mp;
	register struct fs *fs;
	spl_t	s_ipl;

	for (mp = &mounttab[0]; mp < mountNMOUNT; mp++) {
		if (mp->m_bufp == NULL || mp->m_dev != dev)
			continue;

		s_ipl = p_lock(&mp->m_lock, SPLFS);
		/*
		 * Now check again in case we have lost a race with unmount().
		 */
		if (mp->m_bufp == NULL || mp->m_dev != dev) {
			/*
			 * lost the race.
			 */
			v_lock(&mp->m_lock, s_ipl);
			return (NULL);
		}

		fs = mp->m_bufp->b_un.b_fs;
		if (fs->fs_magic != FS_MAGIC) {
			v_lock(&mp->m_lock, s_ipl);
			printf("dev = 0x%x, fs = %s\n", dev, fs->fs_fsmnt);
			panic("nfsgetmp: bad magic");
			/*
			 *+ A filesystem with a bad filesystem magic number
			 *+ was found in the mounted filesystem table.
			 *+ This indicates a kernel software problem.
			 */
		}

		/*
		 * Add a reference so a concurrent ufs_unmount() 
		 * will return EBUSY.
		 */
		mp->m_count++;
		v_lock(&mp->m_lock, s_ipl);
		return (mp);
	}
	return (NULL);
}
#endif	NFS

#if !defined(vax) && !defined(tahoe)
/*
 * Emulate the VAX skpc instruction in C
 */
skpc(mask, size, cp)
	register u_char mask;
	u_int size;
	register u_char *cp;
{
	register u_char *end = &cp[size];

	while (cp < end && *cp == mask)
		cp++;
	return (end - cp);
}
#endif
