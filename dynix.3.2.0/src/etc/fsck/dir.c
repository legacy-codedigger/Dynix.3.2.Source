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
 * static char sccsid[] = "@(#)dir.c	5.4 (Berkeley) 5/7/88";
 * #endif not lint
 */

#ident "$Header: dir.c 1.3 90/10/09 $"

/* $Log:	dir.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#define KERNEL
#include <sys/dir.h>
#undef KERNEL
#include "fsck.h"

char	*endpathname = &pathname[BUFSIZ - 2];
char	*lfname = "lost+found";
struct	dirtemplate emptydir = { 0, DIRBLKSIZ };
struct	dirtemplate dirhead = { 0, 12, 1, ".", 0, DIRBLKSIZ - 12, 2, ".." };

DIRECT	*fsck_readdir();
BUFAREA	*getdirblk();
extern ino_t allocino();

descend(parentino, inumber)
	struct inodesc *parentino;
	ino_t inumber;
{
	register DINODE *dp;
	struct inodesc curino;
	extern int pass2check();

	bzero((char *)&curino, sizeof(struct inodesc));
	if (statemap[inumber] != DSTATE)
		errexit("BAD INODE %d TO DESCEND", statemap[inumber]);
	statemap[inumber] = DFOUND;
	dp = ginode(inumber);
	if ((dp->di_size & (DIRBLKSIZ - 1)) != 0) {
		pwarn("DIRECTORY %s: LENGTH %d NOT MULTIPLE OF %d",
			pathname, dp->di_size, DIRBLKSIZ);
		dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
		if (preen)
			printf(" (ADJUSTED)\n");
		if (preen || reply("ADJUST") == 1)
			inodirty();
	}
	curino.id_type = DATA;
	curino.id_func = parentino->id_func;
	curino.id_parent = parentino->id_number;
	curino.id_number = inumber;
	curino.id_dotflag = 0;
	curino.id_dotdotflag = 0;
	(void)ckinode(dp, &curino);
	if (curino.id_dotflag == 0 && curino.id_func == pass2check) {
	  	direrr(curino.id_number, "MISSING '.' ENTRY");
		if (reply("FIX") == 1) {
			add_dot(&curino);
		}
	}
	if (curino.id_dotdotflag == 0 && curino.id_func == pass2check) {
		direrr(curino.id_number, "MISSING '..' ENTRY");
		if (reply("FIX") == 1) {
			add_dotdot(&curino);
		}
	}
}

dirscan(idesc)
	register struct inodesc *idesc;
{
	register DIRECT *dp;
	register BUFAREA *bp;
	int dsize, n, offset;
	long blksiz;
	char dbuf[DIRBLKSIZ];

	if (idesc->id_type != DATA)
		errexit("wrong type to dirscan %d\n", idesc->id_type);
	if (idesc->id_entryno == 0 &&
	    (idesc->id_filesize & (DIRBLKSIZ - 1)) != 0)
		idesc->id_filesize = roundup(idesc->id_filesize, DIRBLKSIZ);
	blksiz = idesc->id_numfrags * sblock.fs_fsize;
	if (outrange(idesc->id_blkno, idesc->id_numfrags)) {
		idesc->id_filesize -= blksiz;
		return (SKIP);
	}
	idesc->id_loc = 0;
	for (dp = fsck_readdir(idesc, &offset); dp != NULL;
				 dp = fsck_readdir(idesc, &offset)) {
		dsize = dp->d_reclen;
		bcopy((char *)dp, dbuf, (unsigned)dsize);
		idesc->id_dirp = (DIRECT *)dbuf;
		if ((n = (*idesc->id_func)(idesc)) & ALTERED) {
			bp = getdirblk(idesc->id_blkno, blksiz);
			dp = (DIRECT *)(bp->b_un.b_buf + offset);
			bcopy(dbuf, (char *)dp, (unsigned)dsize);
			dirty(bp);
			sbdirty();
		}
		if (n & STOP) 
			return (n);
	}
	return ( (idesc->id_filesize > 0) ? KEEPON : STOP );
}

/*
 * get next entry in a directory.
 */
DIRECT *
fsck_readdir(idesc, offset)
	register struct inodesc *idesc;
	int *offset;
{
	register DIRECT *dp, *ndp;
	register BUFAREA *bp;
	long size, blksiz;

	blksiz = idesc->id_numfrags * sblock.fs_fsize;
	bp = getdirblk(idesc->id_blkno, blksiz);
	if (idesc->id_loc % DIRBLKSIZ == 0 && idesc->id_filesize > 0 &&
	    idesc->id_loc < blksiz) {
		dp = (DIRECT *)(bp->b_un.b_buf + idesc->id_loc);
		*offset = idesc->id_loc;
		if (dircheck(idesc, dp))
			goto dpok;
		idesc->id_loc += DIRBLKSIZ;
		idesc->id_filesize -= DIRBLKSIZ;
		dp->d_reclen = DIRBLKSIZ;
		dp->d_ino = 0;
		dp->d_namlen = 0;
		dp->d_name[0] = '\0';
		if (dofix(idesc, "DIRECTORY CORRUPTED"))
			dirty(bp);
		return (dp);
	}
dpok:
	if (idesc->id_filesize <= 0 || idesc->id_loc >= blksiz)
		return NULL;
	dp = (DIRECT *)(bp->b_un.b_buf + idesc->id_loc);
	*offset = idesc->id_loc;
	idesc->id_loc += dp->d_reclen;
	idesc->id_filesize -= dp->d_reclen;
	if ((idesc->id_loc % DIRBLKSIZ) == 0)
		return (dp);
	ndp = (DIRECT *)(bp->b_un.b_buf + idesc->id_loc);
	if (idesc->id_loc < blksiz && idesc->id_filesize > 0 &&
	    dircheck(idesc, ndp) == 0) {
		size = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
		dp->d_reclen += size;
		idesc->id_loc += size;
		idesc->id_filesize -= size;
		if (dofix(idesc, "DIRECTORY CORRUPTED"))
			dirty(bp);
	}
	return (dp);
}

/*
 * Verify that a directory entry is valid.
 * This is a superset of the checks made in the kernel.
 */
static
dircheck(idesc, dp)
	struct inodesc *idesc;
	register DIRECT *dp;
{
	register int size;
	register char *cp;
	int spaceleft;

	size = DIRSIZ(dp);
	spaceleft = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
	if (dp->d_ino < imax &&
	    dp->d_reclen != 0 &&
	    dp->d_reclen <= spaceleft &&
	    (dp->d_reclen & 0x3) == 0 &&
	    dp->d_reclen >= size &&
	    idesc->id_filesize >= size &&
	    dp->d_namlen <= MAXNAMLEN) {
		if (dp->d_ino == 0)
			return (1);
		for (cp = dp->d_name, size = 0; size < dp->d_namlen; size++)
			if (*cp++ == 0)
				return (0);
		if (*cp == 0)
			return (1);
	}
	return (0);
}

direrr(ino, s)
	ino_t ino;
	char *s;
{
	register DINODE *dp;

	pwarn("%s ", s);
	pinode(ino);
	printf("\n");
	if (ino < ROOTINO || ino > imax) {
		pfatal("NAME=%s\n", pathname);
		return;
	}
	dp = ginode(ino);
	if (ftypeok(dp))
		pfatal("%s=%s\n", DIRCT(dp) ? "DIR" : "FILE", pathname);
	else
		pfatal("NAME=%s\n", pathname);
}

adjust(idesc, lcnt)
	register struct inodesc *idesc;
	short lcnt;
{
	register DINODE *dp;

	dp = ginode(idesc->id_number);
	if (dp->di_nlink == lcnt) {
		if (linkup(idesc->id_number, (ino_t)0) == 0) {
			clri(idesc, "UNREF", 0);
			return;
		}
                /*
                 * Get the link count from the lncntp array.
                 * This needs to be done, since it was changed
                 * in linkup.  Note that this assumes that if we
                 * are in this code we were called with lcnt equal
                 * to lncntp[idesc->id_number]
                 */
                lcnt = lncntp[idesc->id_number];
        }
	if (lcnt != 0) {
		pwarn("LINK COUNT %s", (lfdir == idesc->id_number) ? lfname :
			(DIRCT(dp) ? "DIR" : "FILE"));
		pinode(idesc->id_number);
		printf(" COUNT %d SHOULD BE %d",
			dp->di_nlink, dp->di_nlink-lcnt);
		if (preen) {
			if (lcnt < 0) {
				printf("\n");
				pfatal("LINK COUNT INCREASING");
			}
			printf(" (ADJUSTED)\n");
		}
		if (preen || reply("ADJUST") == 1) {
			dp->di_nlink -= lcnt;
			inodirty();
		}
	}
}

static
mkentry(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;
	DIRECT newent;
	int newlen, oldlen;

	newent.d_namlen = 11;
	newlen = DIRSIZ(&newent);
	if (dirp->d_ino != 0)
		oldlen = DIRSIZ(dirp);
	else
		oldlen = 0;
	if (dirp->d_reclen - oldlen < newlen)
		return (KEEPON);
	newent.d_reclen = dirp->d_reclen - oldlen;
	dirp->d_reclen = oldlen;
	dirp = (struct direct *)(((char *)dirp) + oldlen);
	dirp->d_ino = idesc->id_parent;	/* ino to be entered is in id_parent */
	dirp->d_reclen = newent.d_reclen;
	dirp->d_namlen = strlen(idesc->id_name);
	bcopy(idesc->id_name, dirp->d_name, (unsigned)dirp->d_namlen + 1);
	return (ALTERED|STOP);
}

static
chgino(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (bcmp(dirp->d_name, idesc->id_name, (unsigned)dirp->d_namlen + 1))
		return (KEEPON);
	dirp->d_ino = idesc->id_parent;;
	return (ALTERED|STOP);
}

linkup(orphan, pdir)
	ino_t orphan;
	ino_t pdir;
{
	register DINODE *dp;
	int lostdir, len;
	ino_t oldlfdir;
	struct inodesc idesc;
	char tempname[BUFSIZ];
	extern int pass4check();

	bzero((char *)&idesc, sizeof(struct inodesc));
	dp = ginode(orphan);
	lostdir = DIRCT(dp);
	pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
	pinode(orphan);
	if (preen && dp->di_size == 0)
		return (0);
	if (preen)
		printf(" (RECONNECTED)\n");
	else
		if (reply("RECONNECT") == 0)
			return (0);
	pathp = pathname;
	*pathp++ = '/';
	*pathp = '\0';
	if (lfdir == 0) {
		dp = ginode(ROOTINO);
		idesc.id_name = lfname;
		idesc.id_type = DATA;
		idesc.id_func = findino;
		idesc.id_number = ROOTINO;
		if ((ckinode(dp, &idesc) & FOUND) != 0) {
			lfdir = idesc.id_parent;
		} else {
			pwarn("NO lost+found DIRECTORY");
			if (preen || reply("CREATE")) {
				lfdir = allocdir(ROOTINO, (ino_t)0);
				if (lfdir != 0) {
					if (makeentry(ROOTINO, lfdir, lfname) != 0) {
						if (preen)
							printf(" (CREATED)\n");
					} else {
						freedir(lfdir, ROOTINO);
						lfdir = 0;
						if (preen)
							printf("\n");
					}
				}
			}
		}
		if (lfdir == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY");
			printf("\n\n");
			return (0);
		}
	}
	dp = ginode(lfdir);
	if (!DIRCT(dp)) {
		pfatal("lost+found IS NOT A DIRECTORY");
		if (reply("REALLOCATE") == 0)
			return (0);
		oldlfdir = lfdir;
		if ((lfdir = allocdir(ROOTINO, (ino_t)0)) == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		idesc.id_type = DATA;
		idesc.id_func = chgino;
		idesc.id_number = ROOTINO;
		idesc.id_parent = lfdir;	/* new inumber for lost+found */
		idesc.id_name = lfname;
		if ((ckinode(ginode(ROOTINO), &idesc) & ALTERED) == 0) {
			pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		inodirty();
		idesc.id_type = ADDR;
		idesc.id_func = pass4check;
		idesc.id_number = oldlfdir;
		adjust(&idesc, lncntp[oldlfdir] + 1);
		lncntp[oldlfdir] = 0;
		dp = ginode(lfdir);
	}
	if (statemap[lfdir] != DFOUND) {
		pfatal("SORRY. NO lost+found DIRECTORY\n\n");
		return (0);
	}
	len = strlen(lfname);
	bcopy(lfname, pathp, (unsigned)(len + 1));
	pathp += len;
	len = lftempname(tempname, orphan);
	if (makeentry(lfdir, orphan, tempname) == 0) {
		pfatal("SORRY. NO SPACE IN lost+found DIRECTORY");
		printf("\n\n");
		return (0);
	}
	lncntp[orphan]--;
	*pathp++ = '/';
	bcopy(tempname, pathp, (unsigned)(len + 1));
	pathp += len;
	if (lostdir) {
		dp = ginode(orphan);
		idesc.id_type = DATA;
		idesc.id_func = chgino;
		idesc.id_number = orphan;
		idesc.id_fix = DONTKNOW;
		idesc.id_name = "..";
		idesc.id_parent = lfdir;	/* new value for ".." */
		(void)ckinode(dp, &idesc);
		dp = ginode(lfdir);
		dp->di_nlink++;
		inodirty();
		lncntp[lfdir]++;
		pwarn("DIR I=%u CONNECTED. ", orphan);
		printf("PARENT WAS I=%u\n", pdir);
		if (preen == 0)
			printf("\n");
	}
	return (1);
}

/*
 * make an entry in a directory
 */
makeentry(parent, ino, name)
	ino_t parent, ino;
	char *name;
{
	DINODE *dp;
	struct inodesc idesc;
	
	if (parent < ROOTINO || parent >= imax || ino < ROOTINO || ino >= imax)
		return (0);
	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_func = mkentry;
	idesc.id_number = parent;
	idesc.id_parent = ino;	/* this is the inode to enter */
	idesc.id_fix = DONTKNOW;
	idesc.id_name = name;
	dp = ginode(parent);
	if (dp->di_size % DIRBLKSIZ) {
		dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
		inodirty();
	}
	if ((ckinode(dp, &idesc) & ALTERED) != 0)
		return (1);
	if (expanddir(dp) == 0)
		return (0);
	return (ckinode(dp, &idesc) & ALTERED);
}

/*
 * Attempt to expand the size of a directory
 *	Always expand in increments of a full block.
 *
 *	If there are no frags present, 
 *		add new block to the end of the direct block list
 *	If there are frags
 *		move the frag block to the end, and add the
 *		new block where the frag was.
 *		If the frags were in direct block zer
 *			copy first DIRBLKSIZE chunk of the frags
 *			into the new block, and clear the DIRBLKSIZE
 *			area in the frags.
 *
 */
static
expanddir(dp)
	register DINODE *dp;
{
	daddr_t lastbn, newblk, fragb;
	int fragsz;
	register BUFAREA *bp;
	char *cp, firstblk[DIRBLKSIZ];

	/*
	 * Do we want to expand?
	 */
	pwarn("NO SPACE LEFT IN %s", pathname);
	if ((!preen) && (reply("EXPAND") == 0))
		return(0);

	/*
	 * See if there are any direct block slots left
	 * for expansion.
	 */
	lastbn = lblkno(&sblock, dp->di_size);
	if (lastbn > NDADDR - 1)
		return (0);
	fragb = dp->di_db[lastbn];
	if ((lastbn == NDADDR - 1) && (fragb != 0))
		return(0);

	/*
	 * Try to allocate a new block.
	 */
	if ((newblk = allocblk(sblock.fs_frag)) == 0)
		return(0);

	/*
	 * calculate frag size (before adjusting sizes) for later use.
	 * adjust sizes and db data, to set up for recovery 
	 * of failures later.
	 */
	if (fragb != 0) {
		fragsz = dblksize(&sblock, dp, lastbn);
		dp->di_db[lastbn + 1] = dp->di_db[lastbn];
	}
	dp->di_size += sblock.fs_bsize;
	dp->di_blocks += btodb(sblock.fs_bsize);

	/*
	 * If the first block consists of frags,
	 * save away first part of data in the frags.
	 */
	if ((fragb != 0)  && (lastbn == 0)) {
		bp = getdirblk(fragb, fragsz);
		if (bp->b_errs != NULL)
			goto bad;
		bcopy(bp->b_un.b_buf, firstblk, DIRBLKSIZ);
	}

	/*
	 * Fetch the new block, and initialize it with empty
	 * entries.
	 */
	dp->di_db[lastbn] = newblk;
	bp = getdirblk(newblk, sblock.fs_bsize);
	if (bp->b_errs != NULL)
		goto bad;
	for (cp = bp->b_un.b_buf; cp < &bp->b_un.b_buf[sblock.fs_bsize];
	     cp += DIRBLKSIZ)
		bcopy((char *)&emptydir, cp, sizeof emptydir);

	/*
	 * if the first data block was frags, restore the first
	 * frag data into the new block
	 */
	if ((fragb != 0)  && (lastbn == 0))
		bcopy(firstblk, bp->b_un.b_buf, DIRBLKSIZ);
	dirty(bp);

	/*
	 * if the first data block was frags, then set the
	 * initial part of those frags to be empty directories
	 */
	if ((fragb != 0)  && (lastbn == 0)) {
		bp = getdirblk(fragb, fragsz);
		if (bp->b_errs != NULL)
			goto bad;
		bcopy((char *)&emptydir, bp->b_un.b_buf, sizeof emptydir);
		dirty(bp);
	}
	inodirty();
	if (preen)
		printf(" (EXPANDED)\n");
	return (1);

bad:
	/*
	 * failure case, after allocating a diskblock
	 */
	if (fragb != 0) {
		dp->di_db[lastbn] = dp->di_db[lastbn + 1];
		dp->di_db[lastbn + 1] = 0;
	} else {
		dp->di_db[lastbn] = 0;
	}
	dp->di_size -= sblock.fs_bsize;
	dp->di_blocks -= btodb(sblock.fs_bsize);
	freeblk(newblk, sblock.fs_frag);
	return (0);
}

/*
 * allocate a new directory
 */
allocdir(parent, request)
	ino_t parent, request;
{
	ino_t ino;
	char *cp;
	DINODE *dp;
	register BUFAREA *bp;

	ino = allocino(request, IFDIR|0755);
	dirhead.dot_ino = ino;
	dirhead.dotdot_ino = parent;
	dp = ginode(ino);
	bp = getdirblk(dp->di_db[0], sblock.fs_fsize);
	if (bp->b_errs != NULL) {
		freeino(ino);
		return (0);
	}
	bcopy((char *)&dirhead, bp->b_un.b_buf, sizeof dirhead);
	for (cp = &bp->b_un.b_buf[DIRBLKSIZ];
	     cp < &bp->b_un.b_buf[sblock.fs_fsize];
	     cp += DIRBLKSIZ)
		bcopy((char *)&emptydir, cp, sizeof emptydir);
	dirty(bp);
	dp->di_nlink = 2;
	inodirty();
	if (ino == ROOTINO) {
		lncntp[ino] = dp->di_nlink;
		return(ino);
	}
	if (statemap[parent] != DSTATE && statemap[parent] != DFOUND) {
		freeino(ino);
		return (0);
	}
	statemap[ino] = statemap[parent];
	if (statemap[ino] == DSTATE) {
		lncntp[ino] = dp->di_nlink;
		lncntp[parent]++;
	}
	dp = ginode(parent);
	dp->di_nlink++;
	inodirty();
	return (ino);
}

/*
 * free a directory inode
 */
static
freedir(ino, parent)
	ino_t ino, parent;
{
	DINODE *dp;

	if (ino != parent) {
		dp = ginode(parent);
		dp->di_nlink--;
		inodirty();
	}
	freeino(ino);
}

/*
 * generate a temporary name for the lost+found directory.
 */
static
lftempname(bufp, ino)
	char *bufp;
	ino_t ino;
{
	register ino_t in;
	register char *cp;
	int namlen;

	cp = bufp + 2;
	for (in = imax; in > 0; in /= 10)
		cp++;
	*--cp = 0;
	namlen = cp - bufp;
	in = ino;
	while (cp > bufp) {
		*--cp = (in % 10) + '0';
		in /= 10;
	}
	*cp = '#';
	return (namlen);
}

/*
 * Get a directory block.
 * Insure that it is held until another is requested.
 */
static BUFAREA *
getdirblk(blkno, size)
	daddr_t blkno;
	long size;
{
	static BUFAREA *pbp = 0;

	if (pbp != 0)
		pbp->b_flags &= ~B_INUSE;
	pbp = getdatablk(blkno, size);
	return (pbp);
}
