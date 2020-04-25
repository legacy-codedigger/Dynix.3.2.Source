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
 * $Header: vnode.h 2.15 89/10/30 $
 *
 * vnode.h
 *	Virtual file node definitions.
 */

/* $Log:	vnode.h,v $
 */

/*
 * The vnode is the focus of all file activity in UNIX.
 * There is a unique vnode allocated for each active file,
 * each current directory, each mounted-on file, mapped file, and the root.
 */

/*
 * Vnode types.  VNON means no type.
 */

enum vtype 	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };

/*
 * v_mutex is used to insure atomic manipulation of v_count.  When the HW
 * supports atomic inc/dec, some uses of v_mutex can go away (but not
 * all; see VN_RELE()).
 *
 * v_nodemutex is a "soft" lock (semaphore) for the vnode.  This is used
 * to insure consistent state across various operations.  File-systems
 * increment v_count only after hold v_nodemutex when hit in cache (eg,
 * iget()), or only when they already hold a reference (VN_HOLD()).
 * 
 * v_fllock protects the consistency of v_exlockc, v_shlockc,
 * and also the flock related flags in f_flag in the file structure.
 *
 * v_mapx is index to mfile entry if vnode is mapped (VMAPPED).  If being
 * used for shared text (cachable read-only map), the VTEXT is set also.
 */

struct	vnode	{
	u_short		v_flag;			/* vnode flags (see below)*/
	u_short		v_count;		/* reference count */
	dev_t		v_rdev;			/* device (VCHR, VBLK) */
	u_short		v_exlockc;		/* count of exclusive locks */
	u_long		v_shlockc;		/* count of shared locks */
	sema_t		v_shsema;		/* mutex for shared flocks */
	sema_t		v_exsema;		/* mutex for exclusive flocks */
	lock_t		v_fllock;		/* lock for use by flock() */
	lock_t		v_mutex;		/* lock for v_count mutex */
	rwsema_t	v_nodemutex;		/* mutex for underlying node */
	struct vfs	*v_vfsmountedhere; 	/* ptr to vfs mounted here */
	struct vnodeops	*v_op;			/* vnode operations */
	struct vfs	*v_vfsp;		/* ptr to vfs we are in */
	union {					/* file-sys specific field */
		struct	socket	*vun_socket;	/* For UNIX ipc */
	}		v_un;
	enum vtype	v_type;			/* vnode type */
	u_short		v_mapx;			/* mapped file index */
	u_char		v_badmap;		/* mapping is invalid */
	caddr_t		v_data;			/* private data for fs */
	struct	vnode	*v_devvp;		/* underlying "dev" vnode */
};

#define	NULLVP (struct vnode *)NULL

/*
 * Definitions for use of generic word.
 */

#define	v_socket	v_un.vun_socket

/*
 * Vnode flags.
 * VSVTX is used in va_mode and v_flag.
 * Values must NOT collide with VSVTX.
 */
#define	VROOT		00001		/* root of its file system */
#define	VNBACCT		00002		/* no "bio" vnode accounting */
#define	VTEXT		00004		/* vnode mapping is for execute */
#define	VMOUNTING	00010		/* mount on this vnode in progress */
#define	VBTAPE		00020		/* this devnode is for block tape */
#define	VMAPPED		00100		/* mapped vnode */
#define	VMAPSYNC	00200		/* sync vnode before mapping */
#define	VNOLINKS	00400		/* vnode has no links (heuristic) */
/*define VSVTX		01000		/* see va_mode flags, below */

/*
 * Operations on vnodes.
 */
struct vnodeops {
	int	(*vn_open)();
	int	(*vn_close)();
	int	(*vn_rdwr)();
	int	(*vn_ioctl)();
	int	(*vn_select)();
	int	(*vn_getattr)();
	int	(*vn_setattr)();
	int	(*vn_access)();
	int	(*vn_lookup)();
	int	(*vn_create)();
	int	(*vn_remove)();
	int	(*vn_link)();
	int	(*vn_rename)();
	int	(*vn_mkdir)();
	int	(*vn_rmdir)();
	int	(*vn_readdir)();
	int	(*vn_symlink)();
	int	(*vn_csymlink)();
	int	(*vn_readlink)();
	int	(*vn_readclink)();
	int	(*vn_fsync)();
	int	(*vn_inactive)();
	int	(*vn_bmap)();
	int	(*vn_strategy)();
	int	(*vn_bread)();
	int	(*vn_brelse)();
	int	(*vn_minphys)();
};

#ifdef KERNEL

#define	VOP_OPEN(VPP,F,C)		(*(*(VPP))->v_op->vn_open)(VPP, F, C)
#define	VOP_CLOSE(VP,F,C)		(*(VP)->v_op->vn_close)(VP,F,C)
#define	VOP_RDWR(VP,UIOP,RW,F,C)	(*(VP)->v_op->vn_rdwr)(VP,UIOP,RW,F,C)
#define	VOP_IOCTL(VP,C,D,F,CR)		(*(VP)->v_op->vn_ioctl)(VP,C,D,F,CR)
#define	VOP_SELECT(VP,W,C)		(*(VP)->v_op->vn_select)(VP,W,C)
#define	VOP_GETATTR(VP,VA,C)		(*(VP)->v_op->vn_getattr)(VP,VA,C)
#define	VOP_SETATTR(VP,VA,C)		(*(VP)->v_op->vn_setattr)(VP,VA,C)
#define	VOP_ACCESS(VP,M,C)		(*(VP)->v_op->vn_access)(VP,M,C)
#define	VOP_LOOKUP(VP,NM,VPP,C)		(*(VP)->v_op->vn_lookup)(VP,NM,VPP,C)
#define	VOP_CREATE(VP,NM,VA,E,M,VPP,C)	(*(VP)->v_op->vn_create) \
						(VP,NM,VA,E,M,VPP,C)
#define	VOP_REMOVE(VP,NM,C)		(*(VP)->v_op->vn_remove)(VP,NM,C)
#define	VOP_LINK(VP,TDVP,TNM,C)		(*(VP)->v_op->vn_link)(VP,TDVP,TNM,C)
#define	VOP_RENAME(VP,NM,TDVP,TNM,C)	(*(VP)->v_op->vn_rename) \
						(VP,NM,TDVP,TNM,C)
#define	VOP_MKDIR(VP,NM,VA,VPP,C)	(*(VP)->v_op->vn_mkdir)(VP,NM,VA,VPP,C)
#define	VOP_RMDIR(VP,NM,C)		(*(VP)->v_op->vn_rmdir)(VP,NM,C)
#define	VOP_READDIR(VP,UIOP,C)		(*(VP)->v_op->vn_readdir)(VP,UIOP,C)
#define	VOP_SYMLINK(VP,LNM,VA,TNM,C)	(*(VP)->v_op->vn_symlink) \
						(VP,LNM,VA,TNM,C)
#define	VOP_CSYMLINK(VP,LNM,VA,UCB,ATT,C)	(*(VP)->v_op->vn_csymlink) \
						(VP,LNM,VA,UCB,ATT,C)
#define	VOP_READLINK(VP,UIOP,C)		(*(VP)->v_op->vn_readlink)(VP,UIOP,C)
#define	VOP_READCLINK(VP,UIOP,F,C)	(*(VP)->v_op->vn_readclink)(VP,UIOP,F,C)
#define	VOP_FSYNC(VP,C)			(*(VP)->v_op->vn_fsync)(VP,C)
#define	VOP_INACTIVE(VP,F)		(*(VP)->v_op->vn_inactive)(VP,F)
#define	VOP_BMAP(VP,BN,VPP,BNP,RW,SZ)	(*(VP)->v_op->vn_bmap) \
						(VP,BN,VPP,BNP,RW,SZ)
#define	VOP_STRATEGY(BP)		(*(BP)->b_vp->v_op->vn_strategy)(BP)
#define	VOP_BREAD(VP,BN,BPP)		(*(VP)->v_op->vn_bread)(VP,BN,BPP)
#define	VOP_BRELSE(VP, BP)		(*(VP)->v_op->vn_brelse)(BP)
#define	VOP_MINPHYS(BP)			(*(BP)->b_vp->v_op->vn_minphys)(BP)

/*
 * Flags for above.
 *
 * Dynix/VFS: Caller must now lock vnode to accomplish IO_UNIT.
 */

/*#define IO_UNIT	0x01		/* do io as atomic unit for VOP_RDWR */
#define	IO_APPEND	0x02		/* append write for VOP_RDWR */
#define	IO_SYNC		0x04		/* sync io for VOP_RDWR */
#define IO_NDELAY	0x08		/* for named pipes, no delay I/O */

#endif

/*
 * Vnode attributes.  A field value of -1
 * represents a field whose value is unavailable
 * (getattr) or which is not to be changed (setattr).
 *
 * vattr_null() sets a vattr structure to all -1's.
 */

struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	u_short		va_flags;	/* See below */
	short		va_uid;		/* owner user id */
	short		va_gid;		/* owner group id */
	long		va_fsid;	/* file system id (dev for now) */
	long		va_nodeid;	/* node id */
	short		va_nlink;	/* number of references to file */
	u_long		va_size;	/* file size in bytes (quad?) */
	u_long		va_extfile;	/* file size to grow a file to */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct	timeval	va_atime;	/* time of last access */
	struct	timeval	va_mtime;	/* time of last modification */
	struct	timeval	va_ctime;	/* time file ``created */
	dev_t		va_rdev;	/* device the file represents */
	long		va_blocks;	/* kbytes of disk space held by file */
	int		va_psize;	/* FIFO size */
};

extern	struct	vattr	nullvattr;	/* init'd to all -1's */

#define	vattr_null(vap)	*(vap) = nullvattr

/*
 * Modes.  Some values same as Ixxx entries from inode.h for now.
 * ufs code assumes VSXTX==ISVTX.
 */

#define	VSUID	04000		/* set user id on execution */
#define	VSGID	02000		/* set group id on execution */
#define	VSVTX	01000		/* save text even after use */
#define	VREAD	00400		/* read, write, execute permissions */
#define	VWRITE	00200
#define	VEXEC	00100

/*
 * Flags. 
 */

#define	VA_CSYMLN	1	/* Modifier (if va_type = VLNK) */

#ifdef KERNEL
/*
 * Public vnode manipulation functions
 */

extern	int	vn_open();		/* open vnode */
extern	int	vn_create();		/* creat/mkdir vnode */
extern	int	vn_rdwr();		/* read or write vnode */
extern	int	vn_close();		/* close vnode */
extern	int	vn_link();		/* make hard link */
extern	int	vn_rename();		/* rename (move) */
extern	int	vn_remove();		/* remove/rmdir */
extern	int	getvnodefp();		/* get fp from vnode fd */

/*
 * Flags for above.
 */

enum rm		{ FILE, DIRECTORY };		/* rmdir or rm (remove) */
enum symfollow	{ NO_FOLLOW, FOLLOW_LINK };	/* follow symlinks (lookuppn) */
enum vcexcl	{ NONEXCL, EXCL};		/* (non)excl create (create) */

#define	VN_INIT(VP, VFSP, TYPE, DEV)	{ \
	(VP)->v_count = 1; \
	(VP)->v_shlockc = (VP)->v_exlockc = 0; \
	(VP)->v_vfsp = (VFSP); \
	(VP)->v_type = (TYPE); \
	(VP)->v_rdev = (DEV); \
}

/*
 * Ops to lock mutex access to vnode fields.
 */

#define	VN_LOCK(VP)	l.vnspl = p_lock(&(VP)->v_mutex, SPLFS)
#define	VN_UNLOCK(VP)	v_lock(&(VP)->v_mutex, l.vnspl)

/*
 * Ops to manipulate vnode reference counts.
 */

#define	NODEISLOCKED	1	/* To optimize locks/unlocks in VN_RELE & PUT */
#define	NODEISUNLOCKED	0

/*
 * Hold a vnode: adds a ref count.
 */

#define	VN_HOLD(VP)	{ VN_LOCK(VP); (VP)->v_count++; VN_UNLOCK(VP); }

/*
 * Release a vnode.  Decrements ref count and calls VOP_INACTIVE on last.
 * Vnode must be unlocked.
 */
 
#define	VN_RELE(VP) \
{\
	VN_LOCK(VP);\
	if (--(VP)->v_count == 0) {\
		(void) VOP_INACTIVE((VP), NODEISUNLOCKED);\
	} else {\
		VN_UNLOCK(VP);\
	}\
}

/*
 * Put a locked vnode.  Decrements ref count and calls VOP_INACTIVE on last.
 *
 * VOP_INACTIVE() unlocks the node if it is called.
 */
 
#define	VN_PUT(VP) \
{\
	VN_LOCK(VP);\
	if (--(VP)->v_count == 0) {\
		(void) VOP_INACTIVE((VP), NODEISLOCKED);\
	} else {\
		VN_UNLOCK(VP);\
		VN_UNLOCKNODE(VP);\
	}\
}

/*
 * Ops to lock the underlying node of a vnode.
 */

#define	VN_LOCKNODE(VP)		p_writer(&(VP)->v_nodemutex, PVNOD)
#define	VN_TRYLOCKNODE(VP)	cp_writer(&(VP)->v_nodemutex)
#define	VN_UNLOCKNODE(VP)	v_writer(&(VP)->v_nodemutex)

#define	VN_SHARENODE(VP)	p_reader(&(VP)->v_nodemutex, PVNOD)
#define	VN_TRYSHARENODE(VP)	cp_reader(&(VP)->v_nodemutex)
#define	VN_UNSHARENODE(VP)	v_reader(&(VP)->v_nodemutex)

/* Ops to query the lock state of an underlying node */

#define	VN_LOCKEDNODE(VP)	RWSEMA_WRBUSY(&(VP)->v_nodemutex)
#define	VN_WAITERS(VP)		RWSEMA_WRBLOCKED(&(VP)->v_nodemutex)
#define	VN_SHAREDNODE(VP)	RWSEMA_RDBUSY(&(VP)->v_nodemutex)

/*
 * Macros to maintain vnode mapping flags.
 *
 * List of mfiles (v_mapx) is mutually excluded via v_mutex.
 *
 * VN_LOCK_MAP_LIST() and VN_UNLOCK_MAP_LIST() are clones of VN_LOCK() and
 * VN_UNLOCK() which export the spl value.
 *
 * Note that in this model, VMAPPED and VTEXT flags may become stale (ie, the
 * mappings can go away without clearing these flags).  Code that looks at
 * these flags is assumed to tollerate this.
 */
#define	VMAPX_NULL			((u_short)-1)
#define	VN_MAPPED(vp)			((vp)->v_mapx != VMAPX_NULL)
#define	VN_LOCK_MAP_LIST(vp, s)		(s) = p_lock(&(vp)->v_mutex, SPLFS)
#define	VN_UNLOCK_MAP_LIST(vp, s)	v_lock(&(vp)->v_mutex, (s))

/*
 * Global vnode data.
 */

extern	struct	vnode	*rootdir;	/* root (i.e. "/") vnode */

/*
 * Device-vnode data and support.
 */

extern	int		ndevnode;	/* max # dev-vnodes */
extern	struct	vnode	*devnode;	/* boot-time allocated array */
extern	struct	vnode	*devnode_max;	/* -> last in use devnode */
extern	struct	vnode	*devtovp();

#define DEVTOVP(dev,dvp) \
	for (dvp = devnode_max; dvp >= devnode; --dvp) \
		if (dvp->v_rdev == dev) break; \
	if (dvp < devnode) dvp = devtovp(dev);

#endif	KERNEL
