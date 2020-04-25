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
 * static char sccsid[] = "@(#)pass1.c	5.4 (Berkeley) 4/9/87";
 * #endif not lint
 */

#ident "$Header: pass1.c 1.1 90/01/23 $"

/* $Log:	pass1.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#define KERNEL
#include <sys/dir.h>
#undef KERNEL
#include "fsck.h"

static daddr_t badblk;
static daddr_t dupblk;
int pass1check();
#ifndef STANDALONE
extern char *malloc();
#endif

/*
 * Assumptions:
 *	blockmap, statemap, lncntp have been malloc'd
 *	sblock contains a sane superblock.
 *	zino has been zero'd.
 *	preen has been set to represent -p flag state.
 *
 * calls:
 *	ginode(ino)  assumes won't fail if ino is not out of range.
 *	zapino(), inodirty(),
 *	ckinode(), with pass1check.  return code is ignored.
 *
 * output:
 *	lastino: inode number of last allocated inode
 *			used, by later passes to limit inode searches.
 *	statemap: updated with state of inode.
 *	blockmap: mark blocks used by cylinder group structures as
 *		  allocated.
 *	n_files:
 *	n_blks:
 *	znlhead: contains linked list of inodes with negative link counts
 *	lncntp:	saves link count for each inode.
 *		
 * algorithm:
 *	for each cg
 *		mark in blockmap blocks used by sb, cg, and inodes
 *		as allocated;
 *	end cg;
 *
 *	for each cg
 *		for each inode in cg
 *			get disk inode
 *
 *			if inode is NOT allocated
 *				pfatal test for "PARTIALLY ALLOCATED INODE"
 *					if reply, zapino and mark inode dirty;
 *				set inode statemap to USTATE;
 *				continue;
 *
 *			lastino = inumber;
 *			if invalid inode size
 *				unknown file type*
 *
 *			if a !preen && "bad block" file  && "HOLD BAD BLOCK",
 *				change into a regular file.
 *
 *			if number of non-zero db and idb entries is not
 *				consistent with file size, 
 *				unknown file type*
 *			
 *			if file is of unknown type
 *				unknown file type*
 *
 *			n_files++;
 *			save inode link count in lncnt array
 *			if inode has negative link count
 *				add entry to link count table.
 *				"LINK COUNT TABLE OVERFLOW"
 *
 *			set statemap entry to DSTATE (directory) or
 *					FSTATE (not a directory).
 *
 *			set up idesc (with pass1check()), and call ckinode()
 *			
 *			multiply id_entryno (calc'd by chkino) by frag size.
 *			if (id_entryno != number of blocks in inode)
 *				pwarn("INCORRECT BLOCK COUNT");
 *				if preen or reply("CORRECT")
 *					set inode blocks to id_entryno;
 *					inodirty();
 *		end inode
 *	end cg
 *
 *	unknown:*
 *		pfatal("UNKOWN FILE TYPE");
 *		statemap = FCLEAR;
 *		if (reply("CLEAR")
 *			statemap = USTATE
 *			zapino;
 *			inodirty;
 *			
 * differences from 4.2:
 *	removed checks on superblock and cg summaries.
 *		removed reading of cg super blocks.
 *	removed checks on inode bit map for allocated/free inodes.
 *	moved ftypeok() check, so that more checks are done on an inode
 *		BEFORE checking if it's an unknown type of file.
 *		e.g., checks on direct blocks, indirect blocks, size, etc.
 *	n_blks calculation is moved out.  
 *	n_files calculation is different.
 *	the zlnp array is now a linked list.
 *	There's a new state for "unknown" inodes, FCLEAR
 *	Check for block usage by FIFO's is removed.
 *	
 */
pass1()
{
	register int c, i, j;
	register DINODE *dp;
	struct zlncnt *zlnp;
	int ndb, cgd;
	struct inodesc idesc;
	ino_t inumber;

	/*
	 * Set file system reserved blocks in used block map.
	 *	Note, cg 0 is treated specially,  This is because
	 *	it contains the bootblock, extra superblock.  So, the space
	 *	used here really starts at the first block of the
	 *	cylinder group (cgbase), and goes to the first
	 *	data block after the cg data structures (cgdmin).
	 *
	 *	For other cylinder groups, the cg data structures
	 *	start at the begnning of the redundant superblock
	 *	(at cgsblock(), which is usually somewhere WITHIN
	 *	the cylinder group), and extends to the first data
	 *	block after (cgdmin) the dta structures.  These
	 *	data structures include redundant super block, cg
	 *	summaries, cg structure, inode and block bit maps, 
	 *	and inodes.
	 */
	for (c = 0; c < sblock.fs_ncg; c++) {
		cgd = cgdmin(&sblock, c);
		if (c == 0) {
			i = cgbase(&sblock, c);
			cgd += howmany(sblock.fs_cssize, sblock.fs_fsize);
		} else
			i = cgsblock(&sblock, c);
		for (; i < cgd; i++)
			setbmap(i);
	}
	/*
	 * Find all allocated blocks.
	 */
	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	inumber = 0;
	n_files = n_blks = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0; i < sblock.fs_ipg; i++, inumber++) {
			if (inumber < ROOTINO)
				continue;
			dp = ginode(inumber);
			/*
			 * If inode is NOT allocated, (mode&IFMT == 0)
			 *	check that direct & indirect block
			 *	pointers, mode, and size are all zero.
			 *
			 *	set state to USTATE;
			 */
			if (!ALLOC(dp)) {
				if (bcmp((char *)dp->di_db, (char *)zino.di_db,
					NDADDR * sizeof(daddr_t)) ||
				    bcmp((char *)dp->di_ib, (char *)zino.di_ib,
					NIADDR * sizeof(daddr_t)) ||
				    dp->di_mode || dp->di_size) {
					pfatal("PARTIALLY ALLOCATED INODE I=%u",
						inumber);
					if (reply("CLEAR") == 1) {
						zapino(dp);
						inodirty();
					}
				}
				statemap[inumber] = USTATE;
				continue;
			}
			lastino = inumber;
			/*
			 * Check for invalid file size.
			 */
			if (dp->di_size < 0 ||
			    dp->di_size + sblock.fs_bsize - 1 < 0) {
				if (debug)
					printf("bad size %d:", dp->di_size);
				goto unknown;
			}
			if (!preen && (dp->di_mode & IFMT) == IFMT &&
			    reply("HOLD BAD BLOCK") == 1) {
				dp->di_size = sblock.fs_fsize;
				dp->di_mode = IFREG|0600;
				inodirty();
			}
			/*
			 * Check that appropriate number of
			 * direct and indirect block pointers are non-zero, 
			 * given size of inode.
			 *
			 * special files are treated specially
			 * because the first direct block pointer is
			 * overlayed with major/minor number of special
			 * file.
			 */
			ndb = howmany(dp->di_size, sblock.fs_bsize);
			if (SPECIAL(dp))
				ndb++;
			for (j = ndb; j < NDADDR; j++)
				if (dp->di_db[j] != 0) {
					if (debug)
						printf("bad direct addr: %d\n",
							dp->di_db[j]);
					goto unknown;
				}
			for (j = 0, ndb -= NDADDR; ndb > 0; j++)
				ndb /= NINDIR(&sblock);
			for (; j < NIADDR; j++)
				if (dp->di_ib[j] != 0) {
					if (debug)
						printf("bad indirect addr: %d\n",
							dp->di_ib[j]);
					goto unknown;
				}
			/*
			 * check that mode&IFMT is one of the know
			 * file types (e.g., regular file, char special,
			 * FIFO, etc.
			 */
			if (ftypeok(dp) == 0)
				goto unknown;
			n_files++;
			/*
			 * Save the inode's link count.
			 * If negative link count, save reference
			 * to inode (inumber) away in zlnhead linked list.
			 */
			lncntp[inumber] = dp->di_nlink;
			if (dp->di_nlink <= 0) {
#ifdef STANDALONE
				zlnp = (struct zlncnt *)calloc(sizeof *zlnp);
#else
				zlnp = (struct zlncnt *)malloc(sizeof *zlnp);
#endif
				if (zlnp == NULL) {
					pfatal("LINK COUNT TABLE OVERFLOW");
					if (reply("CONTINUE") == 0)
						errexit("");
				} else {
					zlnp->zlncnt = inumber;
					zlnp->next = zlnhead;
					zlnhead = zlnp;
				}
			}
			/*
			 * keep track of inode state (directory or
			 * non-directory), for later use by directory
			 * sanity checks.
			 */
			statemap[inumber] = DIRCT(dp) ? DSTATE : FSTATE;
			badblk = dupblk = 0; maxblk = 0;
			idesc.id_number = inumber;
			/*
			 *
			 */
			(void)ckinode(dp, &idesc);
			idesc.id_entryno *= btodb(sblock.fs_fsize);
			if (dp->di_blocks != idesc.id_entryno) {
				pwarn("INCORRECT BLOCK COUNT I=%u (%ld should be %ld)",
				    inumber, dp->di_blocks, idesc.id_entryno);
				if (preen)
					printf(" (CORRECTED)\n");
				else if (reply("CORRECT") == 0)
					continue;
				dp->di_blocks = idesc.id_entryno;
				inodirty();
			}
			continue;
	unknown:
			/*
			 * handle all the "unknown file types",
			 * (which actually is quite a few different error
			 * cases... kind of poor).
			 */
			pfatal("UNKNOWN FILE TYPE I=%u", inumber);
			statemap[inumber] = FCLEAR;
			if (reply("CLEAR") == 1) {
				statemap[inumber] = USTATE;
				zapino(dp);
				inodirty();
			}
		}
	}
}

/*
 * input:
 *	idesc: has been initiailized with
 *		id_numfrags: size in frags of this block.
 *		id_blkno: block number to check (in frag units).
 *		blockmap: has been initialized
 *	badblk has been initialized.
 *
 * output:
 *	blockmap has been updated to indicated allocated frags.
 *	duphead, duplist and muldup maintained linked list of duplicate blocks.
 *	n_blks is incremented for every frag in block.
 *	badblk is incremented if there are out of range blocks.
 *
 * alghorithm:
 *	if id_blockno, id_numfrags for out of range
 *		blkerr(BAD), to mark inode for clearing.
 *		if too many out-of-range blocks (badblk)
 *			skip the rest of this pass.
 *
 *	for each frag in block
 *		if frag is out of range
 *			set return status to SKIP
 *		else if frag is not in bitmap
 *			n_blks++
 *			set frag in bitmap.
 *		else
 *			blkerr(DUP), to mark inode for clearing.
 *			put frag number into duplist and muldup 
 *		id_entryno++;
 *		
 * differences with 4.2:
 *	tahoe checks the entire block (all frags) for out of range
 *		condition in one check, rather than frag at a time.
 *
 *	tests for whether block is in/out of bit map, or is a
 *	duplicate are re-arranged, but essentially the same.
 *
 *	The duplicate list is now a linked-list rather than an array.
 *
 *	id_entryno replaces id_filesize to keep track of sum of frags
 *			file.
 */
pass1check(idesc)
	register struct inodesc *idesc;
{
	int res = KEEPON;
	int anyout, nfrags;
	daddr_t blkno = idesc->id_blkno;
	register struct dups *dlp;
	struct dups *new;

	if ((anyout = outrange(blkno, idesc->id_numfrags)) != 0) {
		blkerr(idesc->id_number, "BAD", blkno);
		if (++badblk >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%u",
				idesc->id_number);
			if (preen)
				printf(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				errexit("");
			return (STOP);
		}
	}
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (anyout && outrange(blkno, 1)) {
			res = SKIP;
		} else if (!getbmap(blkno)) {
			n_blks++;
			setbmap(blkno);
		} else {
			blkerr(idesc->id_number, "DUP", blkno);
			if (++dupblk >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%u",
					idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
#ifdef STANDALONE
			new = (struct dups *)calloc(sizeof(struct dups));
#else
			new = (struct dups *)malloc(sizeof(struct dups));
#endif
			if (new == NULL) {
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			new->dup = blkno;
			if (muldup == 0) {
				duplist = muldup = new;
				new->next = 0;
			} else {
				new->next = muldup->next;
				muldup->next = new;
			}
			for (dlp = duplist; dlp != muldup; dlp = dlp->next)
				if (dlp->dup == blkno)
					break;
			if (dlp == muldup && dlp->dup != blkno)
				muldup = new;
		}
		/*
		 * count the number of blocks found in id_entryno
		 */
		idesc->id_entryno++;
	}
	return (res);
}
