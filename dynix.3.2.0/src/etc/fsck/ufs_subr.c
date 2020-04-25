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

#ident "$Header: ufs_subr.c 2.2 90/01/23 $"

/*
 * ufs_subr.c
 *	Various file-system related subroutines.  Some shared with utilities.
 */

/* $Log:	ufs_subr.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/fs.h>

/*
 * fragacct()
 *	Update the frsum fields to reflect addition or deletion 
 *	of some frags.
 *
 * fraglist is part of a cylinder-group; caller locked this.
 */

extern	int around[];
extern	int inside[];
extern	unsigned char *fragtbl[];

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
