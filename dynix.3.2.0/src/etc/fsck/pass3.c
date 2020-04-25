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
 * static char sccsid[] = "@(#)pass3.c	5.2 (Berkeley) 1/7/87";
 * #endif not lint
 */

#ident "$Header: pass3.c 1.1 90/01/23 $"

/* $Log:	pass3.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#define KERNEL
#include <sys/dir.h>
#undef KERNEL
#include "fsck.h"

int	pass2check();

pass3()
{
	register DINODE *dp;
	struct inodesc idesc;
	ino_t inumber, orphan;
	int loopcnt;

	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = DATA;
	for (inumber = ROOTINO; inumber <= lastino; inumber++) {
		if (statemap[inumber] == DSTATE) {
			pathp = pathname;
			*pathp++ = '?';
			*pathp = '\0';
			idesc.id_func = findino;
			idesc.id_name = "..";
			idesc.id_parent = inumber;
			loopcnt = 0;
			do {
				orphan = idesc.id_parent;
				if (orphan < ROOTINO || orphan > imax)
					break;
				dp = ginode(orphan);
				idesc.id_parent = 0;
				idesc.id_number = orphan;
				if ((ckinode(dp, &idesc) & FOUND) == 0)
					break;
				if (loopcnt >= sblock.fs_cstotal.cs_ndir)
					break;
				loopcnt++;
			} while (statemap[idesc.id_parent] == DSTATE);
			if (linkup(orphan, idesc.id_parent) == 1) {
				idesc.id_func = pass2check;
				idesc.id_number = lfdir;
				descend(&idesc, orphan);
			}
		}
	}
}
