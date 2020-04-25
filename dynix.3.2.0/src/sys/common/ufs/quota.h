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
 * $Header: quota.h 1.6 88/08/15 $
 *
 * quota.h
 *	UNIX file-system quota structures and constants.
 */

/* @(#)quota.h	1.4 87/09/10 3.2/4.3NFSSRC */
/* @(#)quota.h	1.3 86/11/18 NFSSRC */
/*	@(#)quota.h 1.1 86/09/25 SMI; from UCB 4.6 83/05/27	*/

/* $Log:	quota.h,v $
 */

/*
 * The dqblk structure defines the format of the disk quota file
 * (as it appears on disk) - the file is an array of these structures
 * indexed by user number.  The setquota sys call establishes the inode
 * for each quota file (a pointer is retained in the mount structure).
 */

struct	dqblk {
	u_long	dqb_bhardlimit;	/* absolute limit on disk blks alloc */
	u_long	dqb_bsoftlimit;	/* preferred limit on disk blks */
	u_long	dqb_curblocks;	/* current block count */
	u_long	dqb_fhardlimit;	/* maximum # allocated files + 1 */
	u_long	dqb_fsoftlimit;	/* preferred file limit */
	u_long	dqb_curfiles;	/* current # allocated files */
	u_long	dqb_btimelimit;	/* time limit for excessive disk use */
	u_long	dqb_ftimelimit;	/* time limit for excessive files */
};

#define dqoff(UID)	((UID) * sizeof(struct dqblk))

/*
 * The dquot structure records disk usage for a user on a filesystem.
 * There is one allocated for each quota that exists on any filesystem
 * for the current user. A cache is kept of recently used entries.
 * Active inodes have a pointer to the dquot associated with them.
 */
struct	dquot {
	struct	dquot *dq_forw, *dq_back;/* hash list, MUST be first entry */
	struct	dquot *dq_freef, *dq_freeb; /* free list */
	sema_t	dq_sema;		/* place to wait for dquot struct */
	lock_t	dq_lock;		/* lock for atomic updates of dquot */
	char	dq_busy;		/* structure busy (I/O in progress) */
	char	dq_flags;
#define	DQ_MOD		0x04		/* this quota modified since read */
#define	DQ_BLKS		0x10		/* has been warned about blk limit */
#define	DQ_FILES	0x20		/* has been warned about file limit */
	short	dq_uid;			/* user this applies to */
	long	dq_cnt;			/* count of active references */
	struct mount *dq_mp;		/* filesystem this relates to */
	struct dqblk dq_dqb;		/* actual usage & quotas */
};

#define	dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define	dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define	dq_curblocks	dq_dqb.dqb_curblocks
#define	dq_fhardlimit	dq_dqb.dqb_fhardlimit
#define	dq_fsoftlimit	dq_dqb.dqb_fsoftlimit
#define	dq_curfiles	dq_dqb.dqb_curfiles
#define	dq_btimelimit	dq_dqb.dqb_btimelimit
#define	dq_ftimelimit	dq_dqb.dqb_ftimelimit

/*
 * flags for m_qflags in mount struct
 */
#define Q_CLOSING	0x01		/* quotas in process of being closed */

#if defined(KERNEL) && defined(QUOTA)
/*
 * The dqfreelist, the hash lists and parts of the quota structures are
 * protected by the lock quota_list.  The parts of the quota structure
 * protected include the hash list pointers, free list pointers, the
 * user id field, the pointer to the mount table entry, and the count
 * field in the quota structures.  Holding this lock prevents the movement
 * of quota structres from one list to another.
 *
 * The rest of the data in the quota structures is protected by the
 * lock dq_quot.   Since I/O can be performed on items protected by this
 * lock, a semaphore, dq_sema, and a busy flag, dq_busy, are provided.
 * Each time dq_lock is locked a check is made to see the busy flag,
 * dq_busy, is set.  If dq_busy is set, the lock is released and we wait
 * for the semaphore dq_sema.  Before doing I/O and while still holding
 * the dq_lock lock, the busy flag is set to indicated I/O in progress
 * and to force other users of the structure to wait for the I/O completion.
 * The lock is then released, the I/O performed.  The quota structrue must
 * then be relocked, the busy flag cleared and then a vall_sema done on
 * the semaphore to wake any processes waithing on the semaphore.  These
 * processes must then relock dq_lock.
 *
 * To prevent deadlock the quota_list lock is always claimed before the
 * disk quota structure lock.
 *
 * The following macros lock, unlock, mark as busy (I/O being done), and
 * clear busy indications for disk quota strucutres.  All locking and
 * unlocking of disk quota structures must be done with these macros.
 */

/*
 * LOCK_DQ locks the disk quota block (dqp).  If the block is BUSY it waits
 * on the disk quota block's semaphore for it to become available.
 */
#define LOCK_DQ(dqp) { \
	ASSERT_DEBUG(va_slic->sl_lmask == SPL0, "LOCK_DQ not spl 0"); \
	(void) p_lock(&(dqp)->dq_lock, SPLFS); \
	while ((dqp)->dq_busy) { \
		p_sema_v_lock(&(dqp)->dq_sema, PRSWAIT, &(dqp)->dq_lock, \
			      SPL0); \
		(void) p_lock(&(dqp)->dq_lock, SPLFS); \
	} \
}

/*
 * UNLOCK_Q unlocks the disk quota structure pointed to by dpq and restores
 * the spl level to oldspl
 */
#define UNLOCK_DQ(dqp)		v_lock(&(dqp)->dq_lock, SPL0)

/*
 * SET_BUSY assumes the disk quota structure pointed at by dqp is locked.
 * It is used to indicate that I/O is being done on the disk_quota structure.
 * It marks the structure as busy and unlocks it.  Other users of the
 * structure must detect that it is busy and wait on the dq_sema semaphore
 * for the structure to become available.
 */
#define SET_BUSY(dqb) { \
	(dqp)->dq_busy = 1; \
	v_lock(&(dqp)->dq_lock, SPL0); \
}

/*
 * CLEAR_BUSY re-obtains the dq_lock for the disk quota structure, clears
 * the busy flag and wakes any blocked proccess waiting for this disk
 * quota structure.  It may only be used after a SET_BUSY.  It leave the
 * disk quota structure locked.
 */
#define CLEAR_BUSY(dqp) { \
	(void) p_lock(&(dqp)->dq_lock, SPLFS); \
	(dqp)->dq_busy = 0; \
	if (blocked_sema(&(dqp)->dq_sema)) \
		vall_sema(&(dqp)->dq_sema); \
}

struct	dqhead	{
	struct	dquot	*dqh_forw;	/* MUST be first */
	struct	dquot	*dqh_back;	/* MUST be second */
};

/*
 * Dquot in core hash chain headers
 */
extern	struct	dqhead	*dqhead;
extern	int	ndqhash;

extern	struct	dquot *dquot, *dquotNDQUOT;
extern	int	ndquot;

extern	int	dq_ftimedefault;	/* default files over quota time */
extern	int	dq_btimedefault;	/* default space over quota time */
extern	lock_t	quota_list;		/* list (hash+free) locker */
extern	sema_t	quota_sema;		/* semaphore for turning off quotas */

extern	void	qtinit();		/* initialize quota system */
extern	struct	dquot *getinoquota();	/* establish quota for an inode */
extern	int	chkdq();		/* check disk block usage */
extern	int	chkiq();		/* check inode usage */
extern	void	dqrele();		/* release dquot */
extern	int	closedq();		/* close quotas */

extern	struct	dquot *getdiskquota();	/* get dquot for uid on filesystem */
extern	void	dqupdate();		/* update dquot on disk */
#endif

/*
 * Definitions for the 'quotactl' system call.
 */
#define Q_QUOTAON	1	/* turn quotas on */
#define Q_QUOTAOFF	2	/* turn quotas off */
#define	Q_SETQUOTA	3	/* set disk limits & usage */
#define	Q_GETQUOTA	4	/* get disk limits & usage */
#define	Q_SETQLIM	5	/* set disk limits only */
#define	Q_SYNC		6	/* update disk copy of quota usages */
