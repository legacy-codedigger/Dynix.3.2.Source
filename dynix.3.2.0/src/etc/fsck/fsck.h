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
 *
 *	@(#)fsck.h	5.7 (Berkeley) 5/7/88
 */

#ident "$Header: fsck.h 1.3 90/04/10 $"

/* $Log:	fsck.h,v $
 */

#define	MAXDUP		10	 /* limit on dup blks (per inode) */
#define	MAXBAD		10	 /* limit on bad blks (per inode) */
#define MAXBUFSPACE	128*1024 /* maximum space to allocate to buffers */

typedef	int	(*SIG_TYP)();

/* Compat w. sysV */
typedef unsigned char unchar;

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#define	USTATE	01		/* inode not allocated */
#define	FSTATE	02		/* inode is file */
#define	DSTATE	03		/* inode is directory */
#define	DFOUND	04		/* directory found during descent */
#define	DCLEAR	05		/* directory is to be cleared */
#define	FCLEAR	06		/* file is to be cleared */

typedef struct dinode	DINODE;
typedef struct direct	DIRECT;

#define	ALLOC(dip)	(((dip)->di_mode & IFMT) != 0)
#define	DIRCT(dip)	(((dip)->di_mode & IFMT) == IFDIR)
#define	SPECIAL(dip) \
	(((dip)->di_mode & IFMT) == IFBLK || ((dip)->di_mode & IFMT) == IFCHR)

/*
 * buffer cache structure.
 */
struct bufarea {
	struct bufarea	*b_next;		/* free list queue */
	struct bufarea	*b_prev;		/* free list queue */
	daddr_t	b_bno;
	int	b_size;
	int	b_errs;
	int	b_flags;
	union {
		char	*b_buf;			/* buffer space */
		daddr_t	*b_indir;		/* indirect block */
		struct	fs *b_fs;		/* super block */
		struct	cg *b_cg;		/* cylinder group */
		struct dinode *b_dinode;	/* inode block */
	} b_un;
	char	b_dirty;
};

#define	B_INUSE 1
typedef struct bufarea BUFAREA;

#define	MINBUFS		5	/* minimum number of buffers required */
BUFAREA	bufhead;		/* head of list of other blks in filesys */
BUFAREA	sblk;			/* file system superblock */
BUFAREA	cgblk;			/* cylinder group blocks */
BUFAREA	*getdatablk();

#define	dirty(x)	(x)->b_dirty = 1
#define	initbarea(x) \
	(x)->b_dirty = 0; \
	(x)->b_bno = (daddr_t)-1; \
	(x)->b_flags = 0;

#define	sbdirty()	sblk.b_dirty = 1
#define	cgdirty()	cgblk.b_dirty = 1
#define	sblock		(*sblk.b_un.b_fs)
#define	cgrp		(*cgblk.b_un.b_cg)

struct filecntl {
	int	rfdes;
	int	wfdes;
	int	mod;
} dfile;			/* file descriptors for filesys */

enum fixstate {DONTKNOW, NOFIX, FIX};

struct inodesc {
	enum fixstate id_fix;	/* policy on fixing errors */
	int (*id_func)();	/* function to be applied to blocks of inode */
	ino_t id_number;	/* inode number described */
	ino_t id_parent;	/* for DATA nodes, their parent */
	daddr_t id_blkno;	/* current block number being examined */
	int id_numfrags;	/* number of frags contained in block */
	long id_filesize;	/* for DATA nodes, the size of the directory */
	int id_loc;		/* for DATA nodes, current location in dir */
	int id_entryno;		/* for DATA nodes, current entry number */
	DIRECT *id_dirp;	/* for DATA nodes, ptr to current entry */
	char *id_name;		/* for DATA nodes, name to find or enter */
	int id_dotflag;		/* for DATA nodes, does "." exist? */
	int id_dotdotflag;	/* for DATA nodes, does ".." exist? */
	char id_type;		/* type of descriptor, DATA or ADDR */
};
/* file types */
#define	DATA	1
#define	ADDR	2

/*
 * Linked list of duplicate blocks.
 * 
 * The list is composed of two parts. The first part of the
 * list (from duplist through the node pointed to by muldup)
 * contains a single copy of each duplicate block that has been 
 * found. The second part of the list (from muldup to the end)
 * contains duplicate blocks that have been found more than once.
 * To check if a block has been found as a duplicate it is only
 * necessary to search from duplist through muldup. To find the 
 * total number of times that a block has been found as a duplicate
 * the entire list must be searched for occurences of the block
 * in question. The following diagram shows a sample list where
 * w (found twice), x (found once), y (found three times), and z
 * (found once) are duplicate block numbers:
 *
 *    w -> y -> x -> z -> y -> w -> y
 *    ^		     ^
 *    |		     |
 * duplist	  muldup
 */
struct dups {
	struct dups *next;
	daddr_t dup;
};
struct dups *duplist;		/* head of dup list */
struct dups *muldup;		/* end of unique duplicate dup block numbers */

/*
 * Linked list of inodes with zero link counts.
 */
struct zlncnt {
	struct zlncnt *next;
	ino_t zlncnt;
};
struct zlncnt *zlnhead;		/* head of zero link count list */

char	*devname;
char	rawflg;
long	dev_bsize;		/* computed value of DEV_BSIZE */
long	secsize;		/* actual disk sector size */
char	nflag;			/* assume a no response */
char	yflag;			/* assume a yes response */
int	xxflag;			/* reboot if root was modified */
int	bflag;			/* location of alternate super block */
int	debug;			/* output debugging info */
int	cvtflag;		/* convert to old file system format */
char	preen;			/* just fix normal inconsistencies */
char	Preen;			/* like "preen", but generate own list of */
				/* file systems per pass */
char	rplyflag;		/* any questions asked? */
char	hotroot;		/* checking root device */
char	havesb;			/* superblock has been read */

char	*blockmap;		/* ptr to primary blk allocation map */
char	*statemap;		/* ptr to inode state table */
short	*lncntp;		/* ptr to link count table */

char	pathname[BUFSIZ];	/* current pathname */
char	*pathp;			/* pointer to pathname position */
char	*endpathname;

daddr_t	fmax;			/* number of blocks in the volume */
ino_t	imax;			/* number of inodes */
ino_t	lastino;		/* hiwater mark of inodes */
ino_t	lfdir;			/* lost & found directory inode number */
char	*lfname;		/* lost & found directory name */

off_t	maxblk;			/* largest logical blk in file */
off_t	bmapsz;			/* num chars in blockmap */

daddr_t	n_blks;			/* number of blocks used */
daddr_t	n_files;		/* number of files seen */

#define	zapino(x)	zino.di_gen = (x)->di_gen+1; (*(x) = zino)
struct	dinode zino;

#define	setbmap(x)	setbit(blockmap, x)
#define	getbmap(x)	isset(blockmap, x)
#define	clrbmap(x)	clrbit(blockmap, x)

#define	FOUND	020
#define	ALTERED	010
#define	KEEPON	04
#define	SKIP	02
#define	STOP	01

time_t	time();
DINODE	*ginode();
BUFAREA	*getblk();
int	findino();

/*
 * These macros are to convert from BSD to system V libraries.
 */
#ifdef SYSV
#define bcopy(b1, b2, len)	(void)memcpy(b2, b1, len)
#define bcmp(b1, b2, len)	memcmp(b1, b2, len)
#define bzero(b, len)		(void)memset(b, 0, len)
#endif

/*
 * Calculate the size of an old-style cylinder group structure.
 *	size of ocg plus number of bytes to hold 
 */
#define OCGSIZE(fs) (sizeof(struct ocg) + howmany((fs)->fs_fpg, NBBY))

/*
 * The size of a cylinder group is calculated by CGSIZE. The maximum size
 * is limited by the fact that cylinder groups are at most one block.
 * Its size is derived from the size of the maps maintained in the 
 * cylinder group and the (struct cg) size.
 */
#define CGSIZE(fs) \
    /* base cg */	(sizeof(struct cg) + \
    /* blktot size */	(fs)->fs_cpg * sizeof(long) + \
    /* blks size */	(fs)->fs_cpg * (fs)->fs_nrpos * sizeof(short) + \
    /* inode map */	howmany((fs)->fs_ipg, NBBY) + \
    /* block map */	howmany((fs)->fs_cpg * (fs)->fs_spc / NSPF(fs), NBBY))