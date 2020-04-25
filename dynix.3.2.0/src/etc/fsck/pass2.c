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
 * static char sccsid[] = "@(#)pass2.c	5.3 (Berkeley) 3/10/87";
 * #endif not lint
 */

#ident "$Header: pass2.c 1.2 90/10/09 $"

/* $Log:	pass2.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/dir.h>
#include <strings.h>
#include "fsck.h"

int	pass2check();

pass2()
{
	register DINODE *dp;
	struct inodesc rootdesc;

	bzero((char *)&rootdesc, sizeof(struct inodesc));
	rootdesc.id_type = ADDR;
	rootdesc.id_func = pass2check;
	rootdesc.id_number = ROOTINO;
	pathp = pathname;
	switch (statemap[ROOTINO]) {

	case USTATE:
		pfatal("ROOT INODE UNALLOCATED");
		if (reply("ALLOCATE") == 0)
			errexit("");
		if (allocdir(ROOTINO, ROOTINO) != ROOTINO)
			errexit("CANNOT ALLOCATE ROOT INODE\n");
		descend(&rootdesc, ROOTINO);
		break;

	case DCLEAR:
		pfatal("DUPS/BAD IN ROOT INODE");
		if (reply("REALLOCATE")) {
			freeino(ROOTINO);
			if (allocdir(ROOTINO, ROOTINO) != ROOTINO)
				errexit("CANNOT ALLOCATE ROOT INODE\n");
			descend(&rootdesc, ROOTINO);
			break;
		}
		if (reply("CONTINUE") == 0)
			errexit("");
		statemap[ROOTINO] = DSTATE;
		descend(&rootdesc, ROOTINO);
		break;

	case FSTATE:
	case FCLEAR:
		pfatal("ROOT INODE NOT DIRECTORY");
		if (reply("REALLOCATE")) {
			freeino(ROOTINO);
			if (allocdir(ROOTINO, ROOTINO) != ROOTINO)
				errexit("CANNOT ALLOCATE ROOT INODE\n");
			descend(&rootdesc, ROOTINO);
			break;
		}
		if (reply("FIX") == 0)
			errexit("");
		dp = ginode(ROOTINO);
		dp->di_mode &= ~IFMT;
		dp->di_mode |= IFDIR;
		inodirty();
		statemap[ROOTINO] = DSTATE;
		/* fall into ... */

	case DSTATE:
		descend(&rootdesc, ROOTINO);
		break;

	default:
		errexit("BAD STATE %d FOR ROOT INODE", statemap[ROOTINO]);
	}
}

pass2check(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;
	char *curpathloc;
	int n, ret = 0;
	DINODE *dp;
	char namebuf[BUFSIZ];


	if (dirp->d_ino == 0)
		return (ret|KEEPON);

	/* 
	 * check for "." and ".."
	 */
	if (strcmp(dirp->d_name, ".") == 0) {
	  	if (idesc->id_dotflag) {
			direrr(idesc->id_number,"EXTRA '.' ENTRY");
			if (reply("FIX") == 1) {
			  	dirp->d_ino = 0;
				ret |= ALTERED;
			}
			return (KEEPON | ret);
		}
		if (dirp->d_ino != idesc->id_number) {
			direrr(idesc->id_number,"BAD INODE NUMBER FOR '.'");
			dirp->d_ino = idesc->id_number;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		idesc->id_dotflag = 1;
	} else 	if (strcmp(dirp->d_name, "..") == 0) {
		if (idesc->id_dotdotflag) {
			direrr(idesc->id_number,"EXTRA '..' ENTRY");
			if (reply("FIX") == 1) {
				dirp->d_ino = 0;
				ret |= ALTERED;
			}
			return (KEEPON | ret);
		}
		if (dirp->d_ino != idesc->id_parent) {
			direrr(idesc->id_number,"BAD INODE NUMBER FOR '..'");
			dirp->d_ino = idesc->id_parent;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		idesc->id_dotdotflag = 1;
	}
	curpathloc = pathp;
	*pathp++ = '/';
	if (pathp + dirp->d_namlen >= endpathname) {
		*pathp = '\0';
		errexit("NAME TOO LONG %s%s\n", pathname, dirp->d_name);
	}
	bcopy(dirp->d_name, pathp, (unsigned)dirp->d_namlen + 1);
	pathp += dirp->d_namlen;
	idesc->id_entryno++;
	n = 0;
	if (dirp->d_ino > imax || dirp->d_ino <= 0) {
		direrr(dirp->d_ino, "I OUT OF RANGE");
		n = reply("REMOVE");
	} else {
again:
		switch (statemap[dirp->d_ino]) {
		case USTATE:
			direrr(dirp->d_ino, "UNALLOCATED");
			n = reply("REMOVE");
			break;

		case DCLEAR:
		case FCLEAR:
			direrr(dirp->d_ino, "DUP/BAD");
			if ((n = reply("REMOVE")) == 1)
				break;
			dp = ginode(dirp->d_ino);
			statemap[dirp->d_ino] = DIRCT(dp) ? DSTATE : FSTATE;
			lncntp[dirp->d_ino] = dp->di_nlink;
			goto again;

		case DFOUND:
			if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) {
				getpathname(namebuf, dirp->d_ino, dirp->d_ino);
				pwarn("%s %s %s\n", pathname,
				    "IS AN EXTRANEOUS HARD LINK TO DIRECTORY",
				    namebuf);
				if (preen)
					printf(" (IGNORED)\n");
				else if ((n = reply("REMOVE")) == 1)
					break;
			}
			/* fall through */

		case FSTATE:
			lncntp[dirp->d_ino]--;
			break;

		case DSTATE:
			descend(idesc, dirp->d_ino);
			if (statemap[dirp->d_ino] == DFOUND) {
				lncntp[dirp->d_ino]--;
			} else if (statemap[dirp->d_ino] == DCLEAR) {
				dirp->d_ino = 0;
				ret |= ALTERED;
			} else
				errexit("BAD RETURN STATE %d FROM DESCEND",
				    statemap[dirp->d_ino]);
			break;

		default:
			errexit("BAD STATE %d FOR INODE I=%d",
			    statemap[dirp->d_ino], dirp->d_ino);
		}
	}
	pathp = curpathloc;
	*pathp = '\0';
	if (n == 0)
		return (ret|KEEPON);
	dirp->d_ino = 0;
	return (ret|KEEPON|ALTERED);
}

add_dot(idesc)
	struct inodesc *idesc;
{
	if (makeentry(idesc->id_number, idesc->id_number, ".") == 0)
	  	pfatal("SORRY. '.' NOT ADDED, INSUFFICIENT SPACE IN DIR\n");
	else {
	  	idesc->id_dotflag = 1;
		lncntp[idesc->id_number]--;
	}
}

add_dotdot(idesc)
	struct inodesc *idesc;
{
	if (makeentry(idesc->id_number, idesc->id_parent, "..") == 0)
	  	pfatal("SORRY, '..' NOT ADDED, INSUFFICIENT SPACE IN DIR\n");
	else {
	  	idesc->id_dotdotflag = 1;
		lncntp[idesc->id_parent]--;
	}
}

