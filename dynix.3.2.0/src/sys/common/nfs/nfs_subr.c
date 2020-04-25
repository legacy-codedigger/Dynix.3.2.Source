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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: nfs_subr.c 1.21 1991/10/09 18:03:55 $";
#endif	lint

/*
 * nfs_subr.c
 *	NFS client and server side support routines.
 */

/* $Log: nfs_subr.c,v $
 *
 *
 * NFSSRC @(#)nfs_subr.c	2.1 86/04/14
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"	/* for fhtovp() */
#include "../ufs/fs.h"
#include "../h/uio.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"

#ifdef	NFSDEBUG
int nfsdebug = 2;
#endif	NFSDEBUG

extern int nfs_createmask;

/*
 * Client side utilities
 */

/*
 * client side statistics
 */
struct {
	int	nclsleeps;		/* client handle waits */
	int	nclgets;		/* client handle gets */
	int	ncalls;			/* client requests */
	int	nbadcalls;		/* rpc failures */
	int	reqs[32];		/* count of each request */
} clstat;

/*
 * These are sized in conf_nfs.c.
 */
extern int maxclients;
extern struct chtab chtable[];

/*
 * Misc. client side mutex.
 */
static	lock_t	chtab_lock;
static	sema_t	chtab_sema;
lock_t	rtab_lock;

/*
 * Initialize client-side locks and semaphores
 */
nfs_init()
{
	extern lock_t	async_buf_lock;
	extern sema_t	async_daemon_sema;

	init_lock(&chtab_lock, G_NFS);
	init_sema(&chtab_sema, 0, 0, G_NFS);
	init_lock(&rtab_lock, G_NFS);
	init_lock(&async_buf_lock, G_NFS);
	init_sema(&async_daemon_sema, 0, 0, G_NFS);
}

static CLIENT *
clget(mi, cred)
	struct mntinfo *mi;
	struct ucred *cred;
{
	register struct chtab *ch;
	spl_t	s_ipl;		/* saved spl level */

	/*
	 * Find an unused handle or create one if not at limit yet.
	 * If at limit, sleep until one available.
	 */
	for (;;) {
		s_ipl = p_lock(&chtab_lock, SPL6);
		clstat.nclgets++;
		for (ch = chtable; ch < &chtable[maxclients]; ch++) {
			if (!ch->ch_inuse) {
				/*
				 * Allocate the client handle
				 */
				ch->ch_inuse = TRUE;
				v_lock(&chtab_lock, s_ipl);
				if (ch->ch_client == NULL) {
					ch->ch_client =
					    clntkudp_create(&mi->mi_addr,
							NFS_PROGRAM,
							NFS_VERSION,
							mi->mi_retrans, cred);
				} else {
					clntkudp_init(ch->ch_client,
							&mi->mi_addr,
							mi->mi_retrans, cred);
				}
				ASSERT(ch->ch_client != NULL,
					"clget: null client");
				ch->ch_timesused++;
				return (ch->ch_client);
			}
		}
		/*
		 * If we got here there are no available handles
		 */
		clstat.nclsleeps++;
		p_sema_v_lock(&chtab_sema, PRIBIO, &chtab_lock, s_ipl);
	}
}

static
clfree(cl)
	CLIENT *cl;
{
	register struct chtab *ch;
	spl_t	s_ipl;

	s_ipl = p_lock(&chtab_lock, SPL6);
	for (ch = chtable; ch < &chtable[maxclients]; ch++) {
		if (ch->ch_client == cl) {
			ch->ch_inuse = FALSE;
			break;
		}
	}
	v_lock(&chtab_lock, s_ipl);
	/*
	 * If there are waiters, awaken them.
	 */
	if (blocked_sema(&chtab_sema))
		(void)cv_sema(&chtab_sema);
}

#define	RPC_INTR	18

char *rpcstatnames[] = {
	"SUCCESS", "CANT ENCODE ARGS", "CANT DECODE RES", "CANT SEND",
	"CANT RECV", "TIMED OUT", "VERS MISMATCH", "AUTH ERROR", "PROG UNAVAIL",
	"PROG VERS MISMATCH", "PROC UNAVAIL", "CANT DECODE ARGS", "SYSTEM ERROR",
	"UNKNOWN HOST", "PMAP FAILURE", "PROG NOT REGISTERED", "FAILED",
	"UNKNOWN ERROR", "INTERRUPTED HARD MOUNT"
};
char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat"
};

/*
 * Back off for retransmission timeout, hard_maxtimo is in 10ths of a sec
 */
extern	int hard_maxtimo;

#define backoff(tim)	\
	((((tim) << 2) > hard_maxtimo) ? hard_maxtimo : ((tim) << 2))

int
rfscall(mi, which, xdrargs, argsp, xdrres, resp, cred)
	register struct mntinfo *mi;
	int	 which;
	xdrproc_t xdrargs;
	caddr_t	argsp;
	xdrproc_t xdrres;
	caddr_t	resp;
	struct ucred *cred;
{
	CLIENT *client;
	register enum clnt_stat status;
	struct ucred *newcred;
	int timeo;
	bool_t tryagain;
	struct rpc_err rpcerr;
	struct timeval wait;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 6, "rfscall: %x, %d, %x, %x, %x, %x\n",
		mi, which, xdrargs, argsp, xdrres, resp);
#endif	NFSDEBUG
	rpcerr.re_errno = 0;
	clstat.ncalls++;
	clstat.reqs[which]++;
	newcred = NULL;
	timeo = mi->mi_timeo;
retry:
	client = clget(mi, cred);

	/*
	 * If hard mounted fs, retry call forever unless hard error occurs
	 */
	do {
		tryagain = FALSE;

		wait.tv_sec = timeo / 10;
		wait.tv_usec = 100000 * (timeo % 10);
		status = CLNT_CALL(client, which, xdrargs, argsp,
				xdrres, resp, wait);
		switch (status) {
		case RPC_SUCCESS:
			break;

		/*
		 * Unrecoverable errors: give up immediately
		 */
		case RPC_AUTHERROR:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_VERSMISMATCH:
		case RPC_PROGVERSMISMATCH:
		case RPC_CANTDECODEARGS:
			break;

		default:
			if (mi->mi_hard) {
				if (mi->mi_int && interrupted()) {
					status = (enum clnt_stat)RPC_INTR;
					rpcerr.re_status = RPC_SYSTEMERROR;
					rpcerr.re_errno = EINTR;
					tryagain = FALSE;
					break;
				} else {
					tryagain = TRUE;
					timeo = backoff(timeo);
				}
			}
			if (!mi->mi_printed) {
				mi->mi_printed = 1;
				printf("NFS server %s not responding, %s\n",
					mi->mi_hostname,
					(mi->mi_hard) ? "still trying"
						      : "giving up");
			}
		}
	} while (tryagain);

	if (status != RPC_SUCCESS) {
		clstat.nbadcalls++;
		if (status != (enum clnt_stat)RPC_INTR) {
			CLNT_GETERR(client, &rpcerr);
			printf("NFS %s failed for server %s: %s\n",
				rfsnames[which], mi->mi_hostname, 
				rpcstatnames[(int)status]);
		}
	} else if (resp && *(int *)resp == EACCES &&
		   newcred == NULL && cred->cr_uid == 0 && cred->cr_ruid != 0) {
		/*
		 * Boy is this a kludge!  If the reply status is EACCES
		 * it may be because we are root (no root net access).
		 * Check the real uid, if it isn't root make that
		 * the uid instead and retry the call.
		 */
		newcred = crdup(cred);
		cred = newcred;
		cred->cr_uid = cred->cr_ruid;
		clfree(client);
		goto retry;
	} else if (mi->mi_printed && mi->mi_hard) {
		printf("NFS server %s ok\n", mi->mi_hostname);
	}
	mi->mi_printed = 0;

	clfree(client);
#ifdef	NFSDEBUG
	dprint(nfsdebug, 7, "rfscall: returning %d\n", rpcerr.re_errno);
#endif	NFSDEBUG
	if (newcred) {
		crfree(newcred);
	}
	return (rpcerr.re_errno);
}

/*
 * Check if this process got an interrupt from the keyboard while sleeping
 */
int
interrupted() 
{
	int smask;
	struct proc *p = u.u_procp;

	smask = p->p_sigmask | ~(sigmask(SIGHUP)  | sigmask(SIGINT) | 
				 sigmask(SIGQUIT) | sigmask(SIGTERM));

	if ((p)->p_sig & ~((p)->p_sigignore | smask)) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

nattr_to_vattr(na, vap)
	register struct nfsfattr *na;
	register struct vattr *vap;
{
	vap->va_type = (enum vtype)na->na_type;
	vap->va_flags = 0;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;
	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;
	vap->va_size = na->na_size;
	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_usec = na->na_atime.tv_usec;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_usec = na->na_mtime.tv_usec;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_usec = na->na_ctime.tv_usec;
	vap->va_blocks = na->na_blocks;
	switch(na->na_type) {

	case NFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;

	case NFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = na->na_blocksize;
		break;
	}

	/*
	 * Convert 16-bit dev_t to dynix 32-bit dev_t.
	 *
	 * DYNIX to DYNIX
	 * The least significant bits of the high order short contain
	 * the high order byte of the minor number. The most significant
	 * bits contain the high order byte of the major number.
	 * If top half -1 don't use it. It may be due to sign extension from
	 * a short dev_t machine.
	 */
	if ((na->na_fsid & 0xffff0000) == (u_long)0xffff0000) {
		vap->va_fsid = na->na_fsid & 0xff;	/* dont use top half */
	} else {
		vap->va_fsid = na->na_fsid & 0xff0000ff;
		vap->va_fsid |= (na->na_fsid >> 8) & 0xff00;	/* high minor */
	}
	vap->va_fsid |= (na->na_fsid & 0xff00) << 8;	/* low  major */

	if ((na->na_rdev & 0xffff0000) == (u_long)0xffff0000) {
		vap->va_rdev = na->na_rdev & 0xff;	/* dont use top half */
	} else {
		vap->va_rdev = na->na_rdev & 0xff0000ff;
		vap->va_rdev |= (na->na_rdev >> 8) & 0xff00;	/* high minor */
	}
	vap->va_rdev |= (na->na_rdev & 0xff00) << 8;	/* low  major */
}

vattr_to_sattr(vap, sa)
	register struct vattr *vap;
	register struct nfssattr *sa;
{
	sa->sa_mode = vap->va_mode;
	sa->sa_uid = vap->va_uid;
	sa->sa_gid = vap->va_gid;
	sa->sa_size = vap->va_size;
	sa->sa_atime.tv_sec  = vap->va_atime.tv_sec;
	sa->sa_atime.tv_usec = vap->va_atime.tv_usec;
	sa->sa_mtime.tv_sec  = vap->va_mtime.tv_sec;
	sa->sa_mtime.tv_usec = vap->va_mtime.tv_usec;
}

setdiropargs(da, nm, dvp)
	struct nfsdiropargs *da;
	char *nm;
	struct vnode *dvp;
{

	da->da_fhandle = *vtofh(dvp);
	da->da_name = nm;
}

/*
 * cmptim()
 *	compare time as in struct timeval.
 *
 * Return: -: t1 <  t2
 *	   0: t1 == t2
 *	   +: t1 >  t2
 */
static int
cmptim(t1, t2)
	register struct timeval *t1;
	register struct timeval *t2;
{
	if (t1->tv_sec == t2->tv_sec)
		return (t1->tv_usec - t2->tv_usec);
	return (t1->tv_sec - t2->tv_sec);
}

/*
 * return a vnode for the given fhandle.
 * If no rnode exists for this fhandle create one and put it
 * in a table hashed by fh_fsid and fs_fid.  If the rnode for
 * this fhandle is already in the table return it (ref count is
 * incremented by rfind.  The rnode will be flushed from the
 * table when nfs_inactive calls runsave.
 *
 * Returns locked vnode.
 */
struct vnode *
makenfsnode(fh, attr, vfsp)
	fhandle_t *fh;
	struct nfsfattr *attr;
	struct vfs *vfsp;
{
	register struct rnode *rp;
	struct rnode *nrp;
	int newnode = 0;
	int diff;	/* (-, 0, +) for (<, ==, >) */
	spl_t	s_ipl;
	struct rnode *rfind();
	extern struct vnodeops nfs_vnodeops;

	s_ipl = LOCK_RTAB;
	if ((rp = rfind(fh, vfsp)) == NULL) {
		UNLOCK_RTAB(s_ipl);
		rp = (struct rnode *)kmem_alloc((u_int)sizeof(*rp));
		bzero((caddr_t)rp, sizeof(*rp));
		rp->r_fh = *fh;
		init_rwsema(&rtov(rp)->v_nodemutex, RWSEMA_SWP, G_NFS);
		RWSEMA_SETWRBUSY(&rtov(rp)->v_nodemutex);	/* locked */
		init_lock(&rtov(rp)->v_mutex, G_NFS);
		init_sema(&rtov(rp)->v_exsema, 0, 0, G_NFS);
		init_sema(&rtov(rp)->v_shsema, 0, 0, G_NFS);
		init_lock(&rtov(rp)->v_fllock, G_NFS);
		init_lock(&rp->r_credlock, G_NFS);
		rtov(rp)->v_count = 1;				/* held */
		rtov(rp)->v_op = &nfs_vnodeops;
		rtov(rp)->v_mapx = VMAPX_NULL;			/* not mapped */
		if (attr) {
			rtov(rp)->v_type = (enum vtype)attr->na_type;
			rtov(rp)->v_flag |= (attr->na_mode & VSVTX);

			/*
			 * Convert 16-bit dev_t to dynix 32-bit dev_t.
			 *
			 * DYNIX to DYNIX
			 * The least significant bits of the high order short
			 * contain the high order byte of the minor number. The
			 * most significant bits contain the high order byte of
			 * the major number. If top half is -1 don't use it.
			 * It may be due to sign extension from a short dev_t
			 * machine.
			 */
			if ((attr->na_rdev & 0xffff0000) == (u_long)0xffff0000){
				/* don't use top half */
				rtov(rp)->v_rdev = attr->na_rdev & 0xff;
			} else {
				rtov(rp)->v_rdev = attr->na_rdev & 0xff0000ff;
				/* high minor */
				rtov(rp)->v_rdev |= (attr->na_rdev>>8) & 0xff00;
			}
			/* low  major */
			rtov(rp)->v_rdev |= (attr->na_rdev & 0xff00) << 8;

			init_sema(&rp->r_attrsema, 0, 0, G_NFS); /* held */
		} else {
			init_sema(&rp->r_attrsema, 1, 0, G_NFS);
		}
		rtov(rp)->v_data = (caddr_t)rp;
		rtov(rp)->v_vfsp = vfsp;
		rtov(rp)->v_devvp = rtov(rp);
		s_ipl = LOCK_RTAB;
		/*
		 * Must check again since another process may have
		 * entered the rnode into the rtable if this process
		 * slept in kmem_alloc().
		 * Note: mi_refct is actually mutex'd via the rnode table
		 * lock. See also nfs_inactive().
		 */
		if ((nrp = rfind(fh, vfsp)) == NULL) {
			rsave(rp);
			((struct mntinfo *)(vfsp->vfs_data))->mi_refct++;
			UNLOCK_RTAB(s_ipl);
			newnode = 1;
		} else {
			/*
			 * rfind() has unlocked the rtab_lock.
			 */
			kmem_free((caddr_t)rp, (u_int)sizeof(*rp));
			rp = nrp;
		}
	}
	if (attr) {
		if (newnode) {
			/*
			 * Brand spankin' new. No need to flush.
			 */
			nfs_attrcache(rtov(rp), attr, NOFLUSH);
		} else {
			/*
			 * Need to flush only if new data to update.
			 * If the times are less than that cached, then
			 * do nothing further. If times are later, then
			 * do the update, as the currently cached data
			 * may have been due to race in nfs_lookup().
			 */
			RLOCK_ATTR(rp);
			diff = cmptim(&attr->na_atime, &rp->r_nfsattr.na_atime);
			if (diff > 0) {
				nfs_attrcache(rtov(rp), attr, SFLUSH);
				goto done;
			}
			if (diff < 0) {
				RUNLOCK_ATTR(rp);
				goto done;
			}
			diff = cmptim(&attr->na_ctime, &rp->r_nfsattr.na_ctime);
			if (diff > 0) {
				nfs_attrcache(rtov(rp), attr, SFLUSH);
				goto done;
			}
			if (diff < 0) {
				RUNLOCK_ATTR(rp);
				goto done;
			}
			diff = cmptim(&attr->na_mtime, &rp->r_nfsattr.na_mtime);
			if (diff >= 0) {
				nfs_attrcache(rtov(rp), attr, SFLUSH);
				goto done;
			}
			RUNLOCK_ATTR(rp);
		}
	}
done:
	return (rtov(rp));
}

/*
 * Rnode lookup stuff.
 * These routines maintain a table of rnodes hashed by fhandle so
 * that the rnode for an fhandle can be found if it already exists.
 */
extern int rtablesize;
extern struct rnode *rtable[];

#define	rtablehash(fh)	((fh)->fh_fno & (rtablesize - 1))


/*
 * Put a rnode in the hash table
 *
 * Called with rtab_lock held at SPLFS.
 */
static
rsave(rp)
	struct rnode *rp;
{

	rp->r_next = rtable[rtablehash(rtofh(rp))];
	rtable[rtablehash(rtofh(rp))] = rp;
}

/*
 * Remove a rnode from the hash table
 *
 * Called with rtab_lock held at SPLFS.
 */
runsave(rp)
	struct rnode *rp;
{
	struct rnode *rt;
	struct rnode *rtprev = NULL;

	rt = rtable[rtablehash(rtofh(rp))]; 
	while (rt != NULL) { 
		if (rt == rp) { 
			if (rtprev == NULL) {
				rtable[rtablehash(rtofh(rp))] = rt->r_next;
			} else {
				rtprev->r_next = rt->r_next;
			}
			return; 
		}	
		rtprev = rt;
		rt = rt->r_next;
	}	
}

/*
 * Lookup a rnode by fhandle.
 *
 * Called with rtab_lock held at SPLFS.
 * If rnode found, unlock rtab_lock at spl spl and return with locked rnode.
 * Otherwise return null.
 */
static struct rnode *
rfind(fh, vfsp)
	fhandle_t *fh;
	struct vfs *vfsp;
{
	register struct rnode *rt;

	rt = rtable[rtablehash(fh)]; 
	while (rt != NULL) { 
		if (bcmp((caddr_t)rtofh(rt), (caddr_t)fh, sizeof(*fh)) == 0 &&
		    vfsp == rtov(rt)->v_vfsp) { 
			/*
			 * Found one now grab and release rtab_lock.
			 */ 
			p_writer_v_lock(&rtov(rt)->v_nodemutex, PVNOD, &rtab_lock);
			VN_HOLD(rtov(rt));
			return (rt); 
		}	
		rt = rt->r_next;
	}	
	return (NULL);
}

/*
 * remove buffers from the buffer cache that have this vfs.
 *
 * NOTE: assumes buffers have been flushed already. If binval() encounters
 * a dirty buffer it will not invalidate and return. rinval() will check
 * the vnodes ref count (v_count) after the binval(). If the ref count isn't
 * 1 (from the VN_HOLD() in rinval()), then the rnode is still busy and we
 * return immediately.
 *
 * Called from nfs_unmount() with unmount_mutex locked.
 */
rinval(vfsp)
	struct vfs *vfsp;
{
	register struct rnode *rp;
	register int i;
	struct mntinfo *mi;
	spl_t s_ipl;

	/*
	 * First zap any buf cache references to the root vnode
	 * of the file system.
	 */
	mi = vftomi(vfsp);
	binval(mi->mi_rootvp);
	if (mi->mi_rootvp->v_count != 1)
		return;
	/*
	 * Now do the rest. Retry from ground zero if any rnodes need to be
	 * invalidated. The VN_RELE may change the rnode table. 
	 */
loop:
	s_ipl = LOCK_RTAB;
	for (i = 0; i < rtablesize; i++) {
		for (rp = rtable[i]; rp; rp = rp->r_next) {
			if (rp->r_vnode.v_vfsp == vfsp) {
				/*
				 * Ignore root vnode here.
				 */
				if (rtov(rp) == mi->mi_rootvp)
					continue;
				/*
				 * Found one. Hold here so that we can determine
				 * if the rnode is still busy. If so, return
				 * immediately.
				 */
				VN_HOLD(rtov(rp));
				UNLOCK_RTAB(s_ipl);
				binval(rtov(rp));
				if (rtov(rp)->v_count != 1) {
					VN_RELE(rtov(rp));
					return;
				}
				VN_RELE(rtov(rp));
				goto loop;
			}
		}
	}
	UNLOCK_RTAB(s_ipl);
}

/*
 * Flush dirty buffers for all vnodes
 * Only called from nfs_unmount().
 *
 * Called from nfs_unmount with unmount_mutex locked.
 */
rflush()
{
	bflush(NULLVP);
}

#define	PREFIXLEN	4
static char prefix[PREFIXLEN+1] = ".nfs";

/*
 * newname()
 *
 * Called from nfs_remove() create new name for file.
 * File and allocated heap memory removed by nfs_inactive().
 */
char *
newname()
{
	char *news;
	register char *s1, *s2;
	register int id;
	static int newnum;

	news = (char *)kmem_alloc((u_int)NFS_MAXNAMLEN);
	for (s1 = news, s2 = prefix; s2 < &prefix[PREFIXLEN]; ) {
		*s1++ = *s2++;
	}
	if (newnum == 0)
		newnum = time.tv_sec & 0xffff;
	id = newnum++;
	while (id) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id = id >> 4;
	}
	*s1 = '\0';
	return (news);
}

/*
 * Server side utilities
 */

vattr_to_nattr(vap, na)
	register struct vattr *vap;
	register struct nfsfattr *na;
{
	na->na_type = (enum nfsftype)vap->va_type;
	na->na_mode = vap->va_mode;
	na->na_uid = vap->va_uid;
	na->na_gid = vap->va_gid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime.tv_sec  = vap->va_atime.tv_sec;
	na->na_atime.tv_usec = vap->va_atime.tv_usec;
	na->na_mtime.tv_sec  = vap->va_mtime.tv_sec;
	na->na_mtime.tv_usec = vap->va_mtime.tv_usec;
	na->na_ctime.tv_sec  = vap->va_ctime.tv_sec;
	na->na_ctime.tv_usec = vap->va_ctime.tv_usec;
	na->na_blocks = vap->va_blocks;
	na->na_blocksize = vap->va_blocksize;

	/*
	 * Maintain 16 bit dev_t across the net for compatibility
	 *
	 * DYNIX to DYNIX
	 * The least significant bits of the high order short contain
	 * the high order byte of the minor number. The most significant
	 * bits contain the high order byte of the major number.
	 */
	na->na_fsid = vap->va_fsid & 0xff0000ff;
	na->na_fsid |= (vap->va_fsid >> 8) & 0xff00;	/* low  major */
	na->na_fsid |= (vap->va_fsid & 0xff00) << 8;	/* high minor */

	na->na_rdev = vap->va_rdev & 0xff0000ff;
	na->na_rdev |= (vap->va_rdev >> 8) & 0xff00;	/* low  major */
	na->na_rdev |= (vap->va_rdev & 0xff00) << 8;	/* high minor */
}

sattr_to_vattr(sa, vap)
	register struct nfssattr *sa;
	register struct vattr *vap;
{
	vattr_null(vap);
	/*
	 * when setting the mode bits, strip of file type to avoid
	 * later confusion and possible panic due to vnode and inode
	 * having different types
	 *
	 * note that we can't strip it if it is 0xffff since setattr
	 * relies on that value to decide whether it should change the
	 * mode or not.
	 */
	vap->va_mode = (u_short)sa->sa_mode;
	if ((vap->va_mode != 0xffff) && nfs_createmask) {
		vap->va_mode &= 07777;
	}
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;
	vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
	vap->va_atime.tv_usec = sa->sa_atime.tv_usec;
	vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
	vap->va_mtime.tv_usec = sa->sa_mtime.tv_usec;
}

/*
 * Make an fhandle from a ufs vnode
 */
makefh(fh, vp)
	register fhandle_t *fh;
	struct vnode *vp;
{
	struct inode *ip;

	if (vp->v_op != &ufs_vnodeops) {
		return (EREMOTE);
	}
	ip = VTOI(vp);
	bzero((caddr_t)fh, NFS_FHSIZE);
	fh->fh_fsid = ip->i_dev;
	fh->fh_fno = ip->i_number;
	fh->fh_fgen = ip->i_gen;
	return (0);
}

/*
 * Convert an fhandle into a vnode.
 * Uses the inode number in the fhandle (fh_fno) to get the locked inode.
 * The inode is locked and used to get the vnode which is returned locked.
 *
 * WARNING: users of this routine must do a VN_PUT on the vnode when they
 * are done with it.
 */
struct vnode *
fhtovp(fh)
	fhandle_t *fh;
{
	register struct inode *ip;
	register struct mount *mp;	/* ufs mount structure */
	spl_t	s_ipl;
	struct mount *nfsgetmp();

	/*
	 * Since fhtovp() is currently specific to the local UFS,
	 * call nfsgetmp() which returns a mount pointer and has updated
	 * counter as part of handskake with the ufs_unmount() code.
	 *
	 * Yes, this is UGLY but will go away with next release when the
	 * nfs3.2 style of fhtovp() is adopted.
	 */
	mp = nfsgetmp(fh->fh_fsid);
	if (mp == NULL) {
		return (NULL);
	}
	ip = iget(fh->fh_fsid, mp->m_bufp->b_un.b_fs, fh->fh_fno);

	/*
	 * Decrement reference to UFS mount table entry.
	 */
	s_ipl = p_lock(&mp->m_lock, SPLFS);
	mp->m_count--;
	v_lock(&mp->m_lock, s_ipl);

	if (ip == NULL) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 1, "fhtovp(%x, %x) couldn't iget\n",
			fh->fh_fsid, fh->fh_fno);
#endif	NFSDEBUG
		return (NULL);
	}
	if (ip->i_gen != fh->fh_fgen) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 2, "NFS stale fhandle %o %d\n",
			fh->fh_fsid, fh->fh_fno);
#endif	NFSDEBUG
#ifdef	DEBUG
		printf("NFS stale fhandle 0x%x %d\n", fh->fh_fsid, fh->fh_fno);
#endif	DEBUG
		/*
		 * idrop() will unlock the ip.
		 */
		idrop(ip);
		return (NULL);
	}
	return (ITOV(ip));
}

/*
 * General utilities
 */

#ifdef	NFSDEBUG
/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

/*VARARGS2*/
dprint(var, level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
	int var;
	int level;
	char *str;
	int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{
	if (var == level || (var > 10 && (var - 10) >= level))
		printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif	NFSDEBUG
#endif	NFS
