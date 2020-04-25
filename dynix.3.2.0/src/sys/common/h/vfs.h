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
 * $Header: vfs.h 2.4 87/04/03 $
 *
 * vfs.h
 *	Virtual File-System structures and definitions.
 */

/* $Log:	vfs.h,v $
 */

/*
 * Structure per mounted file system.
 * Each mounted file system has an array of
 * operations and an instance record.
 * The file systems are put on a singly linked list.
 */
struct vfs {
	struct vfs	*vfs_next;		/* next vfs in vfs list */
	struct vfsops	*vfs_op;		/* operations on vfs */
	struct vnode	*vfs_vnodecovered;	/* vnode we mounted on */
	int		vfs_flag;		/* flags */
	int		vfs_bsize;		/* native block size */
	caddr_t		vfs_data;		/* private data */
};

/*
 * Sema to mutex operations on vfs lists
 */
extern	sema_t	vfs_mutex;

/*
 * vfs flags.
 */
#define	VFS_RDONLY	0x01		/* read only vfs */
#define	VFS_NOSUID	0x02		/* no suid */

/*
 * Operations supported on virtual file system.
 */
struct vfsops {
	int	(*vfs_mount)();
	int	(*vfs_unmount)();
	int	(*vfs_root)();
	int	(*vfs_statfs)();
	int	(*vfs_sync)();
};

#define	VFS_MOUNT(VFSP, PATH, DATA) \
				(*(VFSP)->vfs_op->vfs_mount)(VFSP, PATH, DATA)
#define	VFS_UNMOUNT(VFSP)	(*(VFSP)->vfs_op->vfs_unmount)(VFSP)
#define	VFS_ROOT(VFSP, VPP)	(*(VFSP)->vfs_op->vfs_root)(VFSP,VPP)
#define	VFS_STATFS(VFSP, SBP)	(*(VFSP)->vfs_op->vfs_statfs)(VFSP,SBP)
#define	VFS_SYNC(VFSP)		(*(VFSP)->vfs_op->vfs_sync)()

/*
 * File system statistics.
 */
typedef	long	fsid_t[2];		/* file system id type */

struct statfs {
	long	f_type;			/* type of info, zero for now */
	long	f_bsize;		/* fundamental file system block size */
	long	f_blocks;		/* total blocks in file system */
	long	f_bfree;		/* free block in fs */
	long	f_bavail;		/* free blocks avail to non-superuser */
	long	f_files;		/* total file nodes in file system */
	long	f_ffree;		/* free file nodes in fs */
	fsid_t	f_fsid;			/* file system id */
	long	f_spare[7];		/* spare for later */
};

#ifdef KERNEL

extern	void	vfs_mountroot();	/* mount the root */
extern	int	vfs_filldata();		/* fill out misc struct vfs fields */
extern	void	vfs_add();		/* add a new vfs to mounted vfs list */
extern	void	vfs_remove();		/* remove a vfs from mounted vfs list */

#define VFS_INIT(VFSP, OP, DATA)	{ \
	(VFSP)->vfs_next = (struct vfs *)0; \
	(VFSP)->vfs_op = (OP); \
	(VFSP)->vfs_flag = 0; \
	(VFSP)->vfs_data = (DATA); \
}

extern	struct vfs	*rootvfs;	/* ptr to root vfs structure */

#endif	KERNEL
