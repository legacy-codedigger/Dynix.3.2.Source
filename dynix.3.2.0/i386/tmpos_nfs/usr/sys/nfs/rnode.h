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
 * $Header: rnode.h 1.4 87/04/18 $
 */

/* $Log:	rnode.h,v $
 */

/*
 * Remote file information structure.
 * The rnode is the "inode" for remote files.  It contains
 * all the information necessary to handle remote file on the
 * client side.
 */
struct rnode {
	struct rnode	*r_next;	/* active rnode list */
	struct vnode	r_vnode;	/* vnode for remote file */
	fhandle_t	r_fh;		/* file handle */
	short		r_nopen;	/* Number of concurrent opens */
	u_short		r_flags;	/* flags, see below */
	short		r_error;	/* async write error */
	lock_t		r_credlock;	/* lock for r_cred */
	u_char		r_flushing;	/* Flushing bufs due to attr change */
	struct ucred	*r_cred;	/* current credentials */
	daddr_t		r_lastr;	/* last block read (read-ahead) */
	u_long		r_size;		/* file size in bytes */
	struct ucred	*r_unlcred;	/* unlinked credentials */
	char		*r_unlname;	/* unlinked file name */
	struct vnode	*r_unldvp;	/* parent dir of unlinked file */
	sema_t		r_attrsema;	/* sema to mutex attribute cache */
	struct nfsfattr	r_nfsattr;	/* cached nfs attributes */
	struct timeval	r_nfsattrtime;	/* time attributed cached */
};

/*
 * Flags
 */
#define	REOF		0x01		/* EOF encountered on read */
#define	RDIRTY		0x02		/* dirty buffers may be in buf cache */

/*
 * Convert between vnode and rnode
 */
#define	rtov(rp)	(&(rp)->r_vnode)
#define	vtor(vp)	((struct rnode *)((vp)->v_data))
#define	vtofh(vp)	(&(vtor(vp)->r_fh))
#define	rtofh(rp)	(&(rp)->r_fh)

/*
 * Sema operations used to mutex access to the following rnode fields.
 * r_size, r_nfsattr, r_nfsattrtime, and r_flushing.
 *
 * r_cred is mutexed with r_credlock.
 *
 * Note: other fields where mutex is necessary are mutex'd via VN_LOCKNODE.
 */
#define	RLOCK_ATTR(rp)		p_sema(&(rp)->r_attrsema, PVNOD)
#define	TRY_RLOCK_ATTR(rp)	cp_sema(&(rp)->r_attrsema)
#define	RUNLOCK_ATTR(rp)	v_sema(&(rp)->r_attrsema)

/*
 * Lock and unlock rnode table
 */
extern	lock_t	rtab_lock;

#define	LOCK_RTAB		p_lock(&rtab_lock, SPLFS)
#define	UNLOCK_RTAB(ipl)	v_lock(&rtab_lock, ipl)
