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
 * static char sccsid[] = "@(#)utilities.c	5.13 (Berkeley) 6/7/88";
 * #endif not lint
 */

#ident "$Header: utilities.c 1.2 90/01/30 $"

/* $Log:	utilities.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <sys/dir.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <strings.h>
#include "fsck.h"

long	diskreads, totalreads;	/* Disk cache statistics */
extern off_t lseek();
#ifdef STANDALONE
#undef 	getc
#else
extern char *valloc();
extern char *malloc();
#endif

ftypeok(dp)
	DINODE *dp;
{
	switch (dp->di_mode & IFMT) {

	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
	case IFSOCK:
	case IFIFO:
		return (1);

	default:
		if (debug)
			printf("bad file type 0%o\n", dp->di_mode);
		return (0);
	}
}

reply(s)
	char *s;
{
	char line[80];
	int cont = (strcmp(s, "CONTINUE") == 0);

	if (preen)
		pfatal("INTERNAL ERROR: GOT TO reply()");
	rplyflag = 1;
	printf("\n%s? ", s);
	if (!cont && (nflag || dfile.wfdes < 0)) {
		printf(" no\n\n");
		return (0);
	}
	if (yflag || (cont && nflag)) {
		printf(" yes\n\n");
		return (1);
	}
#ifdef STANDALONE
	(void) gets(line);
#else
	if (getline(stdin, line, sizeof(line)) == EOF)
		errexit("\n");
#endif
	printf("\n");
	if (line[0] == 'y' || line[0] == 'Y')
		return (1);
	else
		return (0);
}

#ifndef STANDALONE
getline(fp, loc, maxlen)
	FILE *fp;
	char *loc;
{
	register n;
	register char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF)
			return (EOF);
		if (!isspace(n) && p < lastloc)
			*p++ = n;
	}
	*p = 0;
	return (p - loc);
}
#endif

/*
 * Malloc buffers and set up cache.
 */
bufinit()
{
	register BUFAREA *bp;
	long bufcnt, i;
	char *bufp;

#ifdef STANDALONE
	callocrnd(1024);
	bufp = (char *)calloc((unsigned)sblock.fs_bsize);
#else
	bufp = (char *)valloc((unsigned)sblock.fs_bsize);
#endif
	if (bufp == 0)
		errexit("cannot allocate buffer pool\n");
	cgblk.b_un.b_buf = bufp;
	initbarea(&cgblk);
	bufhead.b_next = bufhead.b_prev = &bufhead;
	bufcnt = MAXBUFSPACE / sblock.fs_bsize;
	if (bufcnt < MINBUFS)
		bufcnt = MINBUFS;
	for (i = 0; i < bufcnt; i++) {
#ifdef STANDALONE
		bp = (BUFAREA *)calloc(sizeof(BUFAREA));
		callocrnd(1024);
		bufp = (char *)calloc((unsigned)sblock.fs_bsize);
#else
		bp = (BUFAREA *)malloc(sizeof(BUFAREA));
		bufp = (char *)valloc((unsigned)sblock.fs_bsize);
#endif
		if (bp == 0 || bufp == 0) {
			if (i >= MINBUFS)
				break;
			errexit("cannot allocate buffer pool\n");
		}
		bp->b_un.b_buf = bufp;
		bp->b_prev = &bufhead;
		bp->b_next = bufhead.b_next;
		bufhead.b_next->b_prev = bp;
		bufhead.b_next = bp;
		initbarea(bp);
	}
	bufhead.b_size = i;	/* save number of buffers */
}

/*
 * Manage a cache of directory blocks.
 */
BUFAREA *
getdatablk(blkno, size)
	daddr_t blkno;
	long size;
{
	register BUFAREA *bp;

	for (bp = bufhead.b_next; bp != &bufhead; bp = bp->b_next)
		if (bp->b_bno == fsbtodb(&sblock, blkno))
			goto foundit;
	for (bp = bufhead.b_prev; bp != &bufhead; bp = bp->b_prev)
		if ((bp->b_flags & B_INUSE) == 0)
			break;
	if (bp == &bufhead)
		errexit("deadlocked buffer pool\n");
	(void)getblk(bp, blkno, size);
	/* fall through */
foundit:
	totalreads++;
	bp->b_prev->b_next = bp->b_next;
	bp->b_next->b_prev = bp->b_prev;
	bp->b_prev = &bufhead;
	bp->b_next = bufhead.b_next;
	bufhead.b_next->b_prev = bp;
	bufhead.b_next = bp;
	bp->b_flags |= B_INUSE;
	return (bp);
}

BUFAREA *
getblk(bp, blk, size)
	register BUFAREA *bp;
	daddr_t blk;
	long size;
{
	register struct filecntl *fcp;
	daddr_t dblk;

	fcp = &dfile;
	dblk = fsbtodb(&sblock, blk);
	if (bp->b_bno == dblk)
		return (bp);
	flush(fcp, bp);
	diskreads++;
	bp->b_errs = bread(fcp, bp->b_un.b_buf, dblk, size);
	bp->b_bno = dblk;
	bp->b_size = size;
	return (bp);
}

flush(fcp, bp)
	struct filecntl *fcp;
	register BUFAREA *bp;
{
	register int i, j;

	if (!bp->b_dirty)
		return;
	if (bp->b_errs != 0)
		pfatal("WRITING %sZERO'ED BLOCK %d TO DISK\n",
		    (bp->b_errs == bp->b_size / dev_bsize) ? "" : "PARTIALLY ",
		    bp->b_bno);
	bp->b_dirty = 0;
	bp->b_errs = 0;
	bwrite(fcp, bp->b_un.b_buf, bp->b_bno, (long)bp->b_size);
	if (bp != &sblk)
		return;
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		bwrite(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize);
	}
}

rwerr(s, blk)
	char *s;
	daddr_t blk;
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %ld", s, blk);
	if (reply("CONTINUE") == 0)
		errexit("Program terminated\n");
}

ckflush()
{
	register BUFAREA *bp;
	int cnt = 0;

	flush(&dfile, &sblk);
	flush(&dfile, &cgblk);
	for (bp = bufhead.b_prev; bp != &bufhead; bp = bp->b_prev) {
		cnt++;
		flush(&dfile, bp);
	}
	if (bufhead.b_size != cnt)
		errexit("Panic: lost %d buffers\n", bufhead.b_size - cnt);
}

ckfini()
{
	if (havesb && sblk.b_bno != SBOFF / dev_bsize &&
	    !preen && reply("UPDATE STANDARD SUPERBLOCK")) {
		sblk.b_bno = SBOFF / dev_bsize;
		sbdirty();
		flush(&dfile, &sblk);
	}
	ckflush();
	if (debug)
		printf("cache missed %d of %d (%d%%)\n", diskreads,
		    totalreads, diskreads * 100 / totalreads);
	(void)close(dfile.rfdes);
	(void)close(dfile.wfdes);
}

/*
 * Local function to check the range of block numbers to make sure we
 * don't stray out into someone else's data
 */
#define INRANGE(block) \
	(!havesb || ((block) < fsbtodb(&sblock, sblock.fs_size)))

bread(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	long size;
{
	char *cp;
	int i, errs;

	if (!INRANGE(blk)) {
		pfatal("DATA BLOCK %ld NOT WITHIN FILE SYSTEM", (long)blk);
		rwerr("SEEK", blk);
	} else if (lseek(fcp->rfdes, (off_t)(blk * dev_bsize), 0) < (off_t)0)
		rwerr("SEEK", blk);
	else if (read(fcp->rfdes, buf, (int)size) == size)
		return (0);
	rwerr("READ", blk);
	if (lseek(fcp->rfdes, (off_t)(blk * dev_bsize), 0) < (off_t)0)
		rwerr("SEEK", blk);
	errs = 0;
	bzero(buf, (unsigned)size);
	printf("THE FOLLOWING DISK SECTORS COULD NOT BE READ:");
	for (cp = buf, i = 0; i < size; i += secsize, cp += secsize) {
		if (read(fcp->rfdes, cp, (int)secsize) < 0) {
			(void)lseek(fcp->rfdes, (off_t)(blk * dev_bsize +
				i + secsize), 0);
			if (secsize != dev_bsize && dev_bsize != 1)
				printf(" %d (%d),",
				    (blk * dev_bsize + i) / secsize,
				    blk + i / dev_bsize);
			else
				printf(" %d,", blk + i / dev_bsize);
			errs++;
		}
	}
	printf("\n");
	return (errs);
}

bwrite(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	long size;
{
	int i;
	char *cp;

	if (fcp->wfdes < 0)
		return;
	if (!INRANGE(blk)) {
		pfatal("DATA BLOCK %ld NOT WITHIN FILE SYSTEM", (long)blk);
		rwerr("SEEK", blk);
	} else if (lseek(fcp->wfdes, (off_t)(blk * dev_bsize), 0) < (off_t)0)
		rwerr("SEEK", blk);
	else if (write(fcp->wfdes, buf, (int)size) == size) {
		fcp->mod = 1;
		return;
	}
	rwerr("WRITE", blk);
	if (lseek(fcp->wfdes, (off_t)(blk * dev_bsize), 0) < (off_t)0)
		rwerr("SEEK", blk);
	printf("THE FOLLOWING SECTORS COULD NOT BE WRITTEN:");
	for (cp = buf, i = 0; i < size; i += dev_bsize, cp += dev_bsize)
		if (write(fcp->wfdes, cp, (int)dev_bsize) < 0) {
			(void)lseek(fcp->rfdes, (off_t)(blk * dev_bsize +
				i + dev_bsize), 0);
			printf(" %d,", blk + i / dev_bsize);
		}
	printf("\n");
	return;
}

/*
 * allocate a data block with the specified number of fragments
 */
allocblk(frags)
	long frags;
{
	register int i, j, k;

	if (frags <= 0 || frags > sblock.fs_frag)
		return (0);
	for (i = 0; i < fmax - sblock.fs_frag; i += sblock.fs_frag) {
		for (j = 0; j <= sblock.fs_frag - frags; j++) {
			if (getbmap(i + j))
				continue;
			for (k = 1; k < frags; k++)
				if (getbmap(i + j + k))
					break;
			if (k < frags) {
				j += k;
				continue;
			}
			for (k = 0; k < frags; k++)
				setbmap(i + j + k);
			n_blks += frags;
			return (i + j);
		}
	}
	return (0);
}

/*
 * Free a previously allocated block
 */
freeblk(blkno, frags)
	daddr_t blkno;
	long frags;
{
	struct inodesc idesc;

	idesc.id_blkno = blkno;
	idesc.id_numfrags = frags;
	(void)pass4check(&idesc);
}

/*
 * Find a pathname
 */
getpathname(namebuf, curdir, ino)
	char *namebuf;
	ino_t curdir, ino;
{
	int len;
	register char *cp;
	struct inodesc idesc;
	extern int findname();

	if (statemap[ino] != DSTATE && statemap[ino] != DFOUND) {
		(void)strcpy(namebuf, "?");
		return;
	}
	bzero((char *)&idesc, sizeof(struct inodesc));
	idesc.id_type = DATA;
	cp = &namebuf[BUFSIZ - 1];
	*cp = '\0';
	if (curdir != ino) {
		idesc.id_parent = curdir;
		goto namelookup;
	}
	while (ino != ROOTINO) {
		idesc.id_number = ino;
		idesc.id_func = findino;
		idesc.id_name = "..";
		if ((ckinode(ginode(ino), &idesc) & FOUND) == 0)
			break;
	namelookup:
		idesc.id_number = idesc.id_parent;
		idesc.id_parent = ino;
		idesc.id_func = findname;
		idesc.id_name = namebuf;
		if ((ckinode(ginode(idesc.id_number), &idesc) & FOUND) == 0)
			break;
		len = strlen(namebuf);
		cp -= len;
		if (cp < &namebuf[MAXNAMLEN])
			break;
		bcopy(namebuf, cp, (unsigned)len);
		*--cp = '/';
		ino = idesc.id_number;
	}
	if (ino != ROOTINO) {
		(void)strcpy(namebuf, "?");
		return;
	}
	bcopy(cp, namebuf, (unsigned)(&namebuf[BUFSIZ] - cp));
}

catch()
{

	ckfini();
	exit(12);
	/*NOTREACHED*/
}

/*
 * When preening, allow a single quit to signal
 * a special exit after filesystem checks complete
 * so that reboot sequence may be interrupted.
 */
catchquit()
{
	extern returntosingle;

	printf("returning to single-user after filesystem check\n");
	returntosingle = 1;
#ifndef STANDALONE
	(void)signal(SIGQUIT, SIG_DFL);
#endif
	return(0);
}

/*
 * Ignore a single quit signal; wait and flush just in case.
 * Used by child processes in preen.
 */
voidquit()
{
#ifndef STANDALONE
	(void)sleep(1);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_DFL);
#endif
	return(0);
}

/*
 * determine whether an inode should be fixed.
 */
dofix(idesc, msg)
	register struct inodesc *idesc;
	char *msg;
{

	switch (idesc->id_fix) {

	case DONTKNOW:
		if (idesc->id_type == DATA)
			direrr(idesc->id_number, msg);
		else
			pwarn(msg);
		if (preen) {
			printf(" (SALVAGED)\n");
			idesc->id_fix = FIX;
			return (ALTERED);
		}
		if (reply("SALVAGE") == 0) {
			idesc->id_fix = NOFIX;
			return (0);
		}
		idesc->id_fix = FIX;
		return (ALTERED);

	case FIX:
		return (ALTERED);

	case NOFIX:
		return (0);

	default:
		errexit("UNKNOWN INODESC FIX MODE %d\n", idesc->id_fix);
	}
	/* NOTREACHED */
}

/* VARARGS1 */
errexit(s1, s2, s3, s4)
	char *s1;
{
	printf(s1, s2, s3, s4);
	(void)exit(8);
}

/*
 * An inconsistency occured which shouldn't during normal operations.
 * Die if preening, otherwise just printf.
 */
/* VARARGS1 */
pfatal(s, a1, a2, a3)
	char *s;
{

	if (preen) {
		printf("%s: ", devname);
		printf(s, a1, a2, a3);
		printf("\n");
		printf("%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n",
			devname);
		exit(8);
	}
	printf(s, a1, a2, a3);
}

/*
 * Pwarn is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */
/* VARARGS1 */
pwarn(s, a1, a2, a3, a4, a5, a6)
	char *s;
{

	if (preen)
		printf("%s: ", devname);
	printf(s, a1, a2, a3, a4, a5, a6);
}

#ifndef lint
/*
 * Stub for routines from kernel.
 */
panic(s)
	char *s;
{

	pfatal("INTERNAL INCONSISTENCY:");
	errexit(s);
}
#endif
