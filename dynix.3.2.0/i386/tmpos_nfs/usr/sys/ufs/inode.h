/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
 * $Header: inode.h 2.20 1991/10/09 18:09:42 $
 *
 * inode.h
 *	Inode definitions.
 */

/* $Log: inode.h,v $
 *
 *
 *
 */

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair (see iget/ufs_inode.c).
 * Data in icommon is read in from permanent inode on volume.
 */

#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */
#define NSADDR	(NIADDR*sizeof(daddr_t)/sizeof(short))

/*
 * The i_chain, i_dev, i_number, i_fs fields are all locked by the
 * list-locker (ino_list).  Other fields (i_count, i_flag, i_common) are
 * locked via i_mutex.  i_un is multiplexed (locking depends on
 * context).
 */

struct 	icommon
{
	u_short		ic_mode;	/*  0: mode and type of file */
	short		ic_nlink;	/*  2: number of links to file */
	short		ic_uid;		/*  4: owner's user id */
	short		ic_gid;		/*  6: owner's group id */
	quad		ic_size;	/*  8: number of bytes in file */
#ifdef	KERNEL
	struct timeval	ic_atime;	/* 16: time last accessed */
	struct timeval	ic_mtime;	/* 24: time last modified */
	struct timeval	ic_ctime;	/* 32: last time inode changed */
#else
	time_t		ic_atime;	/* 16: time last accessed */
	long		ic_atspare;
	time_t		ic_mtime;	/* 24: time last modified */
	long		ic_mtspare;
	time_t		ic_ctime;	/* 32: last time inode changed */
	long		ic_ctspare;
#endif	KERNEL
	daddr_t		ic_db[NDADDR];	/* 40: disk block addresses */
	union {
		daddr_t		ic_ib[NIADDR];	/* 88: indirect blocks */

		/*
		 * CAUTION, ic_fifo MUST be smaller than ic_ib[] !!
		 */
		struct {
			short		if_data[4];
			lock_t		if_lock;
		} ic_fifo;
	} ic_un;
	long		ic_flags;	/* 100: status, currently unused */
	long		ic_blocks;	/* 104: blocks actually held */
	daddr_t		ic_ucbln;	/* 108: length of ucb link */
	daddr_t		ic_attln;	/* 112: length of att link */
	long		ic_gen;		/* 116: generation number */
	long		ic_spare[2];	/* 120: reserved, currently unused */
};

struct dinode {
	union {
		struct	icommon di_icom;
		char	di_size[128];
	} di_un;
};

#ifdef	KERNEL
struct inode {
	struct	inode	*i_chain[2];	/* must be first */
	struct	vnode	i_vnode;	/* vnode associated with this inode */
	u_short		i_flag;		/* see below */
	dev_t		i_dev;		/* device where inode resides */
	ino_t		i_number;	/* i number, 1-to-1 with dev address */
	int		i_diroff;	/* dir offset where last entry found */
	struct fs	*i_fs;		/* file sys assoc with this inode */
	struct	dquot *i_dquot;		/* quota structure controlling file */
	int		i_psize;	/* size used for SV pipe */
	struct	BmapCache *i_bmcache;	/* attached Bmap cache object */
	union {
		daddr_t	if_lastr;	/* last read (read-ahead) */
		struct	socket *is_socket;
		struct	{
			struct inode  *if_freef;	/* free list forward */
			struct inode **if_freeb;	/* free list back */
		} i_fr;
	} i_un;
	struct 	icommon i_ic;		/* disk portion of inode */
};

#define	i_mutex	i_vnode.v_nodemutex	/* "soft" locking sema */
#define	i_devvp	i_vnode.v_devvp		/* vnode for block I/O */

#endif	KERNEL

#define	i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define	i_uid		i_ic.ic_uid
#define	i_gid		i_ic.ic_gid
#define	i_size		i_ic.ic_size.val[0]		/* machine dependent */
#define	i_db		i_ic.ic_db
#define	i_ib		i_ic.ic_un.ic_ib
#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime
#define i_pflags	i_ic.ic_flags			/* "perm" flags */
#define i_ucbln		i_ic.ic_ucbln			/* ucb magic link */
#define i_attln		i_ic.ic_attln			/* att magic link */
#define i_blocks	i_ic.ic_blocks
#define	i_rdev		i_ic.ic_db[0]
#define	i_gen		i_ic.ic_gen
#define	i_lastr		i_un.if_lastr
#define	i_socket	i_un.is_socket
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define	i_freef		i_un.i_fr.if_freef
#define	i_freeb		i_un.i_fr.if_freeb

#define di_ic		di_un.di_icom
#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid
#define	di_size		di_ic.ic_size.val[0]		/* machine dependent */
#define	di_db		di_ic.ic_db
#define	di_ib		di_ic.ic_un.ic_ib
#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime
#define	di_rdev		di_ic.ic_db[0]
#define	di_pflags	di_ic.ic_flags			/* "perm" flags */
#define di_ucbln	di_ic.ic_ucbln			/* ucb magic link */
#define di_attln	di_ic.ic_attln			/* att magic link */
#define	di_blocks	di_ic.ic_blocks
#define	di_gen		di_ic.ic_gen

#define PIPSIZ	1*MAXBSIZE 
#define i_frptr		i_ic.ic_un.ic_fifo.if_data[0]
#define i_fwptr		i_ic.ic_un.ic_fifo.if_data[1]
#define i_frcnt		i_ic.ic_un.ic_fifo.if_data[2]
#define i_fwcnt		i_ic.ic_un.ic_fifo.if_data[3]
#define i_frwlock	i_ic.ic_un.ic_fifo.if_lock
#define i_rsema		i_vnode.v_shsema
#define i_wsema		i_vnode.v_exsema

#ifdef	KERNEL
extern	struct inode	*inode;		/* the inode table itself */
extern	struct inode	*inodeNINODE;	/* the end of the inode table */
extern	int		ninode;		/* number of slots in the table */
extern	lock_t		ino_list;	/* list (hash+free) locker */

extern struct vnodeops ufs_vnodeops;	/* vnode operations for ufs */

struct	inode *ialloc();
struct	inode *iget();
#ifdef QUOTA
void	iref();
#endif

ino_t	dirpref();
#endif	KERNEL

/*
 * Flags for i_flag.
 */

#define	IUPD		0x0002		/* file has been modified */
#define	IACC		0x0004		/* inode access time to be updated */
#define	ICHG		0x0040		/* inode has been changed */
#define	IFREE		0x0400		/* inode is on free-list */

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFSOCK		0140000		/* socket */
#define	IFLNK		0120000		/* symbolic link */
#define	IFREG		0100000		/* regular */
#define	IFBLK		0060000		/* block special */
#define	IFDIR		0040000		/* directory */
#define	IFCHR		0020000		/* character special */
#define IFIFO		0010000		/* fifo */

#define	ISUID		0004000		/* set user id on execution */
#define	ISGID		0002000		/* set group id on execution */
#define	ISVTX		0001000		/* save swapped text even after use */
#define	IREAD		0000400		/* read, write, execute permissions */
#define	IWRITE		0000200
#define	IEXEC		0000100

/*
 * Flags bits for i_pflags (disk-resident/permanent flags).
 */

#define IP_EXECMOD	0x00000001	/* Modified since last exec'ed */
#define	IP_CSYMLN	0x00000002	/* conditional symbolic link */

#ifdef	KERNEL

/*
 * Convert between inode pointers and vnode pointers
 */

#define VTOI(VP)	((struct inode *)(VP)->v_data)
#define ITOV(IP)	((struct vnode *)&(IP)->i_vnode)

/*
 * Convert between vnode types and inode formats
 */

extern enum vtype	iftovt_tab[];
extern int		vttoif_tab[];
#define IFTOVT(M)	(iftovt_tab[((M) & IFMT) >> 12])
#define VTTOIF(T)	(vttoif_tab[(int)(T)])

#define MAKEIMODE(T, M)	(VTTOIF(T) | (M))

/*
 * Lock, unlock, update (if necessary) an inode.  IUPDAT() assumes argument
 * is locked.
 * These names are retained for familiarity, but equated to the vnode calls
 * for locking/unlocking the underlying node.
 */

#define	ILOCK(ip)		VN_LOCKNODE(ITOV(ip))
#define TRY_ILOCK(ip)		VN_TRYLOCKNODE(ITOV(ip))
#define	IUNLOCK(ip)		VN_UNLOCKNODE(ITOV(ip))
#define ILOCKED(ip)		VN_LOCKEDNODE(ITOV(ip))

#define	IUPDAT(ip, waitfor) \
	{ if (ip->i_flag&(IUPD|IACC|ICHG)) iupdat(ip, waitfor); }

/*
 * IPUT()
 *	Decrement reference count of an inode structure and unlock the inode.
 *	(unlock actually done by iinactive, if it is called)
 *
 * On the last reference, write the inode out and if necessary,
 * truncate and deallocate the file.
 *
 * This is essentially a VN_PUT(ITOV(IP)) -- if VN_PUT() changes, this
 * should also.
 *
 * Inode is locked on entry.
 */
 
#define IPUT(IP) \
{\
	VN_LOCK(ITOV(IP));\
	if (--(ITOV(IP))->v_count == 0) {\
		iinactive((ITOV(IP)), NODEISLOCKED);\
	} else {\
		VN_UNLOCK(ITOV(IP));\
		IUNLOCK(IP);\
	}\
}

/*
 * IRELE() release reference to unlocked inode; similar to IPUT().
 *
 * This is essentially a VN_RELE(ITOV(IP)) -- if VN_RELE() changes, this
 * should also.
 */

#define IRELE(IP) \
{\
	VN_LOCK(ITOV(IP));\
	if (--(ITOV(IP))->v_count == 0) {\
		iinactive((ITOV(IP)), NODEISUNLOCKED);\
	} else {\
		VN_UNLOCK(ITOV(IP));\
	}\
}

/*
 * Mark the accessed, updated, or changed times in an inode
 * with the current time. The update time is always unique.
 */
#ifdef	lint
#define	IMARK(ip, flag)
#else
#define IMARK(ip, flag) { \
	struct timeval ltime; \
\
	ltime = time; \
	(ip)->i_flag |= (flag); \
	if ((flag) & IUPD) { \
		if ((ip)->i_mtime.tv_sec == ltime.tv_sec && \
		    (ip)->i_mtime.tv_usec >= ltime.tv_usec) { \
			ltime.tv_usec = (ip)->i_mtime.tv_usec + 1; \
			if (ltime.tv_usec >= 1000000) { \
				ltime.tv_usec = 0; \
				ltime.tv_sec++; \
			} \
		} \
		(ip)->i_mtime = ltime; \
	} \
	if ((flag) & IACC) \
		(ip)->i_atime = ltime; \
	if ((flag) & ICHG) { \
		(ip)->i_diroff = 0; \
		(ip)->i_ctime = ltime; \
	} \
}
#endif	/* lint */

#define ESAME (-1)		/* trying to rename linked files (special) */

/*
 * BMAP() calls BmapMap() or bmap() directly, depending on whether inode
 * has a bmap cache.  Currently only turn on bmap caching for VREG inodes;
 * can call bmap() directly in cases where file is known to not be VREG.
 */

extern	daddr_t	BmapMap();

#define	BMAP(ip, bn, rw, size) \
	((ip)->i_bmcache ? BmapMap((ip), (bn), (rw), (size)) \
			 : bmap((ip), (bn), (rw), (size)))

/*
 * Check that file is owned by current user or user is su.
 */
#define OWNER(CR, IP)	(((CR)->cr_uid == (IP)->i_uid)? 0: (suser()? 0: u.u_error))

/*
 * enums
 */
enum de_op	{ DE_CREATE, DE_LINK, DE_RENAME };	/* direnter ops */

#endif	KERNEL
