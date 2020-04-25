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

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * #ifndef lint
 * static char sccsid[] = "@(#)inode.c	5.6 (Berkeley) 5/7/88";
 * #endif not lint
 */

#ident "$Header: inode.c 1.1 90/01/23 $"

/* $Log:	inode.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/dir.h>
#include <pwd.h>
#include "fsck.h"

BUFAREA *pbp = 0;
#ifndef STANDALONE
extern char *sprintf();
#endif

/*
 * input:
 *	idesc: initialized with
 *		id_type	(ADDR or DATA)
 *		id_func	address of sanity-checkinf function.
 *
 *	dp: a pointer to a disk inode to be checked.
 *
 *
 * output:
 *	id_filesize: is initialized from inode's size.
 *
 *	before calling block-check function, initialize
 *		id_numfrags with size of file in frags, so far.
 *		id_blkno with block number of current direct block.
 *
 * algorithm: ckinode(dp, idesc)
 *
 *	Do more idesc initialization.
 *	If inode is for a special file, return(KEEPON)
 *	For each direct block pointer
 *		add up number of frags in file (include sparse-file space).
 *		if block is a sparse one (i.e. zero)
 *			continue;
 *
 *		set id_blkno to current block no.
 *		pass-dependent Sanity-check id_blkno (call either dirscan or
 *			id_func).
 *		if sanity-check was bad
 *			return sanity-check status;
 * 	end direct block
 *
 *	for each non-zero indirect block pointer
 *		set id_blkno to the indirect block number
 *		call iblock() to sanity check the indirect block.
 *		if iblock() indicates a fatal problem
 *			return iblock status.
 * 	end each non-zero indirect-block
 *
 * 	return KEEPON
 *
 *	Note, the sanity check function is id_func for ADDR type idesc's.
 *		for other type idesc's (DATA), use dirscan().
 *
 * differences from DYNIX 3.0:
 *	The id_fix, other fields are initialized even for SPECIAL files.
 *	The iblock check includes the third indirect level block,
 *		(even though this won't get hit with a 32-bit
 *		signed file size on either a 4k or 8k block file system.)
 *	id_filesize is set directly from di_isize.
 *	id_entryno is used instead of id_filesize to mantain real file
 *		size.
 */
ckinode(dp, idesc)
	DINODE *dp;
	register struct inodesc *idesc;
{
	register daddr_t *ap;
	int ret, n, ndb, offset;
	DINODE dino;

	idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = dp->di_size;
	if (SPECIAL(dp))
		return (KEEPON);
	dino = *dp;
	ndb = howmany(dino.di_size, sblock.fs_bsize);
	/*
	 * for each direct block entry,
	 *	sanity check the block number.
	 */
	for (ap = &dino.di_db[0]; ap < &dino.di_db[NDADDR]; ap++) {
		/* 
		 * If this is the last block, and it's not a whole
		 * block, calculate number of frags in the last block.
		 * otherwise, use number of frags in a block.
		 */
		if (--ndb == 0 && (offset = blkoff(&sblock, dino.di_size)) != 0)
			idesc->id_numfrags =
				numfrags(&sblock, fragroundup(&sblock, offset));
		else
			idesc->id_numfrags = sblock.fs_frag;
		/*
		 * don't check zero block numbers.
		 *	file is sparse in direct blocks.
		 */
		if (*ap == 0)
			continue;

		/*
		 * sanity check the block number.
		 *	exact sanity check performed depends on
		 *	idesc type and which pass we're in.
		 */
		idesc->id_blkno = *ap;
		if (idesc->id_type == ADDR)
			ret = (*idesc->id_func)(idesc);
		else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	/*
	 * sanity check the indirect blocks.
	 */
	idesc->id_numfrags = sblock.fs_frag;
	for (ap = &dino.di_ib[0], n = 1; n <= NIADDR; ap++, n++) {
		if (*ap) {
			idesc->id_blkno = *ap;
			ret = iblock(idesc, n,
				dino.di_size - sblock.fs_bsize * NDADDR);
			if (ret & STOP)
				return (ret);
		}
	}
	return (KEEPON);
}

/*
 * input:
 *	idesc has been initialized with:
 *		id_func pointer to pass-dependent sanity-check function
 *		id_type (ADDR or DATA)
 *		id_blkno being the block number of the indirect block to check
 *		id_numfrags being number of frags in block being checked.
 *			will alwyas be fs_frag.
 *		
 *	ilevel is the level of indirection remaining for this block.
 *
 *	isize is the number of blocks pointed to by indirect blocks.
 *
 * output:
 *
 * algorithm:
 *	pass-dependent sanity check the indirect block passed in.
 *	check that the indirect block is in range.
 *	read indirect block from disk
 *	calculate number of entries in this indirect block.
 *	if pass1 and are there non-zero idb entires beyond end of file?
 *		"PARTIALLY TRUNCATED INODE"
 *	sanity check each entry in this block
 *		if it's another indirect block recurse in iblock
 *		else do pass-dependent check on block.
 *	
 *	
 * differences with DYNIX 3.0
 *	This has been modified to deal with the third level of
 *	indirect blocks.
 *
 *	A new check for "PARTIALLY TRUNCATED" inodes has been added.
 *		This is equivalent to rbk's "Blocks after eof" checks.
 *
 *	There are modifications for dealing with fsck's internal disk
 *	block cache.
 */
static
iblock(idesc, ilevel, isize)
	struct inodesc *idesc;
	register ilevel;
	long isize;
{
	register daddr_t *ap;
	register daddr_t *aplim;
	int i, n, (*func)(), nif, sizepb;
	register BUFAREA *bp;
	char buf[BUFSIZ];
	extern int dirscan(), pass1check();

	if (idesc->id_type == ADDR) {
		func = idesc->id_func;
		if (((n = (*func)(idesc)) & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (outrange(idesc->id_blkno, idesc->id_numfrags)) /* protect thyself */
		return (SKIP);
	bp = getdatablk(idesc->id_blkno, sblock.fs_bsize);
	ilevel--;
	for (sizepb = sblock.fs_bsize, i = 0; i < ilevel; i++)
		sizepb *= NINDIR(&sblock);
	nif = isize / sizepb + 1;
	if (nif > NINDIR(&sblock))
		nif = NINDIR(&sblock);
	if (idesc->id_func == pass1check && nif < NINDIR(&sblock)) {
		aplim = &bp->b_un.b_indir[NINDIR(&sblock)];
		for (ap = &bp->b_un.b_indir[nif]; ap < aplim; ap++) {
			if (*ap == 0)
				continue;
#ifdef STANDALONE
			if (dofix(idesc, "PARTIALLY TRUNCATED INODE")) {
				*ap = 0;
				dirty(bp);
			}
#else
			(void)sprintf(buf, "PARTIALLY TRUNCATED INODE I=%d",
				idesc->id_number);
			if (dofix(idesc, buf)) {
				*ap = 0;
				dirty(bp);
			}
#endif
		}
		flush(&dfile, bp);
	}
	aplim = &bp->b_un.b_indir[nif];
	for (ap = bp->b_un.b_indir, i = 1; ap < aplim; ap++, i++) {
		if (*ap) {
			idesc->id_blkno = *ap;
			if (ilevel > 0)
				n = iblock(idesc, ilevel, isize - i * sizepb);
			else
				n = (*func)(idesc);
			if (n & STOP) {
				bp->b_flags &= ~B_INUSE;
				return (n);
			}
		}
	}
	bp->b_flags &= ~B_INUSE;
	return (KEEPON);
}

/*
 * input:
 *   fmax: number of blocks in volume (includes cg structure space, etc).
 *   blk: disk block number (in frags) of block being checked.
 *   cnt: number of frag-sized blocks in this block.
 *
 * output:
 *
 * algorithm:
 *	make sure the block (specified as frag number, and number of frags)
 *		is within the range of frags on this volume.
 *		doesn't overlap any cylinder-group structures on the volume.
 *
 *	The alghorithm recognizes that there are really three regions
 *	in a cylinder group:   The data blocks BEFORE the cylinder
 *	group data structures, the cylinder group data structures
 *	themselves (super-block, cg structure, inode and data block
 *	bit maps, and inodes), and the data blocks  between the end
 *	of the cylinder group structures and the base of the next
 *	cylinder group.
 *
 * Differences with DYNIX/3.0:
 *	none
 */
outrange(blk, cnt)
	daddr_t blk;
	int cnt;
{
	register int c;

	/*
	 * check that the block doesn't go beyond end of volume
	 */
	if ((unsigned)(blk+cnt) > fmax)
		return (1);
	/*
	 * determine which cylinder group the block resides in.
	 */
	c = dtog(&sblock, blk);
	if (blk < cgdmin(&sblock, c)) {
		/*
		 * if the block begins BEFORE in the first part of the
		 * cylinder group (i.e., before the cg structures, etc)
		 * make sure it doesn't overlap this cylinder group's
		 * structures.
		 */
		if ((blk+cnt) > cgsblock(&sblock, c)) {
			if (debug) {
				printf("blk %d < cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > cgsbase %d\n",
				    blk+cnt, cgsblock(&sblock, c));
			}
			return (1);
		}
	} else {
		/*
		 * if the block begins AFTER the cylinder group
		 * data structures, make sure it doesn't run into
		 * the NEXT cylinder group.
		 */
		if ((blk+cnt) > cgbase(&sblock, c+1)) {
			if (debug)  {
				printf("blk %d >= cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > sblock.fs_fpg %d\n",
				    blk+cnt, sblock.fs_fpg);
			}
			return (1);
		}
	}
	return (0);
}

DINODE *
ginode(inumber)
	ino_t inumber;
{
	daddr_t iblk;
	static ino_t startinum = 0;	/* blk num of first in raw area */

	if (inumber < ROOTINO || inumber > imax)
		errexit("bad inode number %d to ginode\n", inumber);
	if (startinum == 0 ||
	    inumber < startinum || inumber >= startinum + INOPB(&sblock)) {
		iblk = itod(&sblock, inumber);
		if (pbp != 0)
			pbp->b_flags &= ~B_INUSE;
		pbp = getdatablk(iblk, sblock.fs_bsize);
		startinum = (inumber / INOPB(&sblock)) * INOPB(&sblock);
	}
	return (&pbp->b_un.b_dinode[inumber % INOPB(&sblock)]);
}

inodirty()
{
	
	dirty(pbp);
}

clri(idesc, s, flg)
	register struct inodesc *idesc;
	char *s;
	int flg;
{
	register DINODE *dp;

	dp = ginode(idesc->id_number);
	if (flg == 1) {
		pwarn("%s %s", s, DIRCT(dp) ? "DIR" : "FILE");
		pinode(idesc->id_number);
	}
	if (preen || reply("CLEAR") == 1) {
		if (preen)
			printf(" (CLEARED)\n");
		n_files--;
		(void)ckinode(dp, idesc);
		zapino(dp);
		statemap[idesc->id_number] = USTATE;
		inodirty();
	}
}

findname(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (dirp->d_ino != idesc->id_parent)
		return (KEEPON);
	bcopy(dirp->d_name, idesc->id_name, (unsigned)dirp->d_namlen + 1);
	return (STOP|FOUND);
}

findino(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (dirp->d_ino == 0)
		return (KEEPON);
	if (strcmp(dirp->d_name, idesc->id_name) == 0 &&
	    dirp->d_ino >= ROOTINO && dirp->d_ino <= imax) {
		idesc->id_parent = dirp->d_ino;
		return (STOP|FOUND);
	}
	return (KEEPON);
}

pinode(ino)
	ino_t ino;
{
	register DINODE *dp;
	register char *p;
	struct passwd *pw;
#ifndef STANDALONE
	char *ctime();
#endif

	printf(" I=%u ", ino);
	if (ino < ROOTINO || ino > imax)
		return;
	dp = ginode(ino);
	printf(" OWNER=");
#ifndef STANDALONE
	if (!hotroot && (pw = getpwuid(dp->di_uid)) != 0)
		printf("%s ", pw->pw_name);
	else
		printf("%d ", dp->di_uid);
#else
		printf("%d ", dp->di_uid);
#endif
	printf("MODE=%o\n", dp->di_mode);
	if (preen)
		printf("%s: ", devname);
	printf("SIZE=%ld ", dp->di_size);
#ifndef STANDALONE
	p = ctime(&dp->di_mtime);
	printf("MTIME=%12.12s %4.4s ", p+4, p+20);
#endif
}

/*
 * algorthm:
 *	print error message, and change state if inode.
 *
 * Differences with 4.2:
 *	tahoe has more states.  It differentiates between directory and
 *	non-directory files.
 *
 *	also, checks validity of incoming state, rather than just
 *	changing state without looking.
 */
blkerr(ino, s, blk)
	ino_t ino;
	char *s;
	daddr_t blk;
{

	pfatal("%ld %s I=%u", blk, s, ino);
	printf("\n");
	switch (statemap[ino]) {

	case FSTATE:
		statemap[ino] = FCLEAR;
		return;

	case DSTATE:
		statemap[ino] = DCLEAR;
		return;

	case FCLEAR:
	case DCLEAR:
		return;

	default:
		errexit("BAD STATE %d TO BLKERR", statemap[ino]);
		/* NOTREACHED */
	}
}

/*
 * allocate an unused inode
 */
ino_t
allocino(request, type)
	ino_t request;
	int type;
{
	register ino_t ino;
	register DINODE *dp;

	if (request == 0)
		request = ROOTINO;
	else if (statemap[request] != USTATE)
		return (0);
	for (ino = request; ino < imax; ino++)
		if (statemap[ino] == USTATE)
			break;
	if (ino == imax)
		return (0);
	switch (type & IFMT) {
	case IFDIR:
		statemap[ino] = DSTATE;
		break;
	case IFREG:
	case IFLNK:
		statemap[ino] = FSTATE;
		break;
	default:
		return (0);
	}
	dp = ginode(ino);
	dp->di_db[0] = allocblk(1L);
	if (dp->di_db[0] == 0) {
		statemap[ino] = USTATE;
		return (0);
	}
	dp->di_mode = type;
	(void)time(&dp->di_atime);
	dp->di_mtime = dp->di_ctime = dp->di_atime;
	dp->di_size = sblock.fs_fsize;
	dp->di_blocks = btodb(sblock.fs_fsize);
	n_files++;
	inodirty();
	return (ino);
}

/*
 * deallocate an inode
 */
freeino(ino)
	ino_t ino;
{
	struct inodesc idesc;
	extern int pass4check();
	DINODE *dp;

	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	idesc.id_number = ino;
	dp = ginode(ino);
	(void)ckinode(dp, &idesc);
	zapino(dp);
	inodirty();
	statemap[ino] = USTATE;
	n_files--;
}
