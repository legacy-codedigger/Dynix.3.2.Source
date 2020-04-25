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
 * $Header: nfs_clnt.h 1.5 89/09/25 $
 */

/* $Log:	nfs_clnt.h,v $
 */

/*
 * vfs pointer to mount info
 */
#define	vftomi(vfsp)	((struct mntinfo *)((vfsp)->vfs_data))

/*
 * vnode pointer to mount info
 */
#define	vtomi(vp)	((struct mntinfo *)(((vp)->v_vfsp)->vfs_data))

/*
 * NFS vnode to server's block size
 */
#define	vtoblksz(vp)	(vtomi(vp)->mi_bsize)

#define	HOSTNAMESZ      32

/*
 * NFS private data per mounted file system
 */
struct mntinfo {
	struct sockaddr_in mi_addr;	/* server's address */
	struct vnode	*mi_rootvp;	/* root vnode */
	u_int		mi_hard : 1;	/* hard or soft mount */
	u_int		mi_printed : 1;	/* not responding message printed */
	u_int		mi_int : 1;	/* interrupts allowed on hard mount */
	int		mi_refct;	/* active vnodes for this vfs */
	long		mi_tsize;	/* transfer size (bytes) */
	long		mi_stsize;	/* server's max transfer size (bytes) */
	long		mi_bsize;	/* server's disk block size */
	int		mi_mntno;	/* kludge to set client rdev for stat*/
	int		mi_timeo;	/* inital timeout in 10th sec */
	int		mi_retrans;	/* times to retry request */
	char		mi_hostname[HOSTNAMESZ];	/* server name */
	struct	vfs	mi_vfs;		/* vfs structure for this filesystem */
};

/*
 * enum to specifiy cache flushing action when file data is stale
 */
enum staleflush	{NOFLUSH, SFLUSH};

#ifdef	KERNEL
/*
 * Client handle.
 */
struct chtab {
	int	ch_timesused;
	bool_t	ch_inuse;
	CLIENT	*ch_client;
};
#endif	KERNEL
