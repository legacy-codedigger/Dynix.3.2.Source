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
 * $Header: mount.h 2.6 88/08/15 $
 *
 * mount.h
 *	UNIX file-system mount structures.
 */

/* $Log:	mount.h,v $
 */

/*
 * Mount structure.  One allocated on every mount.
 */
struct	mount {
	struct vfs	m_vfs;		/* vfs structure for this filesystem */
	dev_t		m_dev;		/* device mounted */
	struct buf	*m_bufp;	/* pointer to superblock */
	lock_t		m_lock;		/* lock for unmount/fhtovp race */
	u_short		m_count;	/* # processes in fhtovp() */
	u_short		m_qflags;	/* QUOTA: filesystem flags */
	struct inode	*m_qinod;	/* QUOTA: pointer to quota file */
	u_long		m_btimelimit;	/* QUOTA: block time limit */
	u_long		m_ftimelimit;	/* QUOTA: file time limit */
	u_long		m_qcnt;		/* QUOTA: # dquot active structures */
};

#ifdef	KERNEL
/*
 * Convert vfs ptr to mount ptr. ONLY WORKS IF m_vfs IS FIRST.
 */
#define VFSTOM(VFSP)	((struct mount *)(VFSP))

extern	struct	mount	*mounttab;	/* mount table start */
extern	struct	mount	*mountNMOUNT;	/* mount table top */
extern	int		nmount;		/* # mount-table entries */

/*
 * Operations
 */
struct	mount	*getmp();
#endif	KERNEL
