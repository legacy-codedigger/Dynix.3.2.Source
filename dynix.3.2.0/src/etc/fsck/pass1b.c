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
 * static char sccsid[] = "@(#)pass1b.c	5.2 (Berkeley) 5/7/88";
 * #endif not lint
 */

#ident "$Header: pass1b.c 1.1 90/01/23 $"

/* $Log:	pass1b.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#define KERNEL
#include <sys/dir.h>
#undef KERNEL
#include "fsck.h"

int	pass1bcheck();
static  struct dups *duphead;

/*
 * purpose:
 *	re-scan the non-free inodes, looking for duplicate block
 *	allocations.
 *
 * input:
 *	duplist, muldup, duphead: linked list of duplicate blocks
 *	statemap: state of inodes.
 *	
 * output:
 *	inodes with duplicate blocks are marked for clearing
 *
 * algorithm:
 *	initialize idesc
 *		id_type = ADDR
 *		id_func = pass1bcheck
 *	for each cylinder group
 *		for each inode in cg
 *			get inode
 *			initialize id_number
 *			if statemap for inode is not USTATE
 *				if (ckinode() & STOP)
 *					goto out
 *		end each inode
 *	end each cg
 *	out:
 *
 * differences with 4.2:
 *	4.2 must flush block with inode in it.
 */
pass1b()
{
	register int c, i;
	register DINODE *dp;
	struct inodesc idesc;
	ino_t inumber;

	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1bcheck;
	duphead = duplist;
	inumber = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0; i < sblock.fs_ipg; i++, inumber++) {
			if (inumber < ROOTINO)
				continue;
			dp = ginode(inumber);
			if (dp == NULL)
				continue;
			idesc.id_number = inumber;
			if (statemap[inumber] != USTATE &&
			    (ckinode(dp, &idesc) & STOP))
				goto out1b;
		}
	}
out1b:;
}

/*
 * algorithm:
 *	for each frag in block
 *		if frag is out of range
 *			set up a SKIP return status.
 *		for each entry on dup list
 *			
 */
pass1bcheck(idesc)
	register struct inodesc *idesc;
{
	register struct dups *dlp;
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (outrange(blkno, 1))
			res = SKIP;
		for (dlp = duphead; dlp; dlp = dlp->next) {
			if (dlp->dup == blkno) {
				blkerr(idesc->id_number, "DUP", blkno);
				dlp->dup = duphead->dup;
				duphead->dup = blkno;
				duphead = duphead->next;
			}
			if (dlp == muldup)
				break;
		}
		if (muldup == 0 || duphead == muldup->next)
			return (STOP);
	}
	return (res);
}
