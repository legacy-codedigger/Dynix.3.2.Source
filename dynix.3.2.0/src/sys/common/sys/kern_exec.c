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

#ifndef	lint
static	char	rcsid[] = "$Header: kern_exec.c 2.29 90/06/03 $";
#endif

/*
 * kern_exec.c
 *	Exec system calls and argument-map management.
 */

/* $Log:	kern_exec.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/vm.h"
#include "../h/seg.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/acct.h"
#include "../h/vfs.h"

#include "../machine/exec.h"
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"

dev_t		argdev;				/* device for arguments */
struct vnode	*argdev_vp;			/* vnode equiv to above */
struct	map	*argmap;			/* argument space map */
long		argalloc();			/* for map allocation */
int		suid_script = 0;		/* Allow suid shell scripts */

/*
 * Subset of struct vattr for execve() (allows smaller stack frame).
 */

struct	exec_vattr {
	u_short		va_mode;	/* files access mode and type */
	short		va_uid;		/* owner user id */
	short		va_gid;		/* owner group id */
	u_long		va_size;	/* file size in bytes (quad?) */
};

/*
 * exec system call, with and without environments.
 */

struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

execv()
{
	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

execve()
{
	register nc;
	register char *cp;
	register struct buf *bp;
	register struct execa *uap;
	int	na, ne, ucp, ap, cc, len, error;
	int	indir, uid, gid;
	char	*execnamep;
	struct	vnode	*vp;
	struct	ofile_tab *oft = NULL;
	struct	exec_vattr vattr;
	daddr_t	bno;
	struct	pathname pn;
	char	*sharg;
	char	cfarg[SHSIZE];
	union {
		char	ex_shell[SHSIZE];
		struct	exec ex_exec;
	} exdata;
	int	resid;

	uap = (struct execa *)u.u_ap;
	u.u_error = pn_get(uap->fname, UIOSEG_USER, &pn);
	if (u.u_error)
		return;
	u.u_error = lookuppn(&pn, FOLLOW_LINK, (struct vnode **)0, &vp);
	if (u.u_error) {
		pn_free(&pn);
		return;
	}
	bno = 0;
	bp = 0;
	indir = 0;
	uid = u.u_uid;
	gid = u.u_gid;
	if (u.u_error = exec_getattr(vp, &vattr))
		goto bad;
	if ((vp->v_vfsp->vfs_flag & VFS_NOSUID) == 0) {
		if (vattr.va_mode & VSUID)
			uid = vattr.va_uid;
		if (vattr.va_mode & VSGID)
			gid = vattr.va_gid;
	} else if (vattr.va_mode & (VSUID | VSGID)) {
		exec_nosuid(uap->fname);
	}

	/*
	 * Dynix note:
	 * Must have 'x' permission, and 'r' permission if tracing.
	 * 'r' permission on tracing avoids security hole (reading
	 * copy of binary without read permission on file).
	 */
again:
	if (u.u_error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto bad;
	if ((u.u_procp->p_flag & SDBG)
	&&  (u.u_error = VOP_ACCESS(vp, VREAD, u.u_cred)))
		goto bad;
	if (vp->v_type != VREG
	||  (vattr.va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) == 0) {
		u.u_error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for segment sizes, ux_mag:
	 *	ZMAGIC = default zero at zero demand paged RO text
	 *	XMAGIC = invalid page at zero demand paged RO text
	 * Also an ASCII line beginning with #! is the file name of a ``shell''
	 * and arguments may be prepended to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 *
	 * Should cache tsize, dsize, bsize, and magic # indication in struct
	 * vnode for already mapped VTEXT vnode, and avoid the vn_rdwr().
	 * When/if do this, may need care to purge bad map (currently done in
	 * mmreg_rw(), since do actually read the mapped file).
	 */

	exdata.ex_shell[0] = '\0';	/* for zero length files */
	u.u_error = vn_rdwr(UIO_READ, vp, (caddr_t)&exdata, sizeof (exdata),
						0, UIOSEG_KERNEL, 0, &resid);
	if (u.u_error)
		goto bad;

	/*
	 * If file header doesn't have a standard magic number, see if
	 * it's a "#!" file.  getxfile() checks format of header further.
	 */

	if (exdata.ex_exec.a_magic!=ZMAGIC && exdata.ex_exec.a_magic!=XMAGIC) {
		if (exdata.ex_shell[0] != '#' || exdata.ex_shell[1] != '!' ||
		    indir) {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &exdata.ex_shell[2];		/* skip "#!" */
		while (cp < &exdata.ex_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			u.u_error = ENOEXEC;
			goto bad;
		}
		cp = &exdata.ex_shell[2];
		while (*cp == ' ')
			cp++;
		execnamep = cp;
		while (*cp && *cp != ' ')
			cp++;
		sharg = NULL;
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp) {
				bcopy((caddr_t)cp, (caddr_t)cfarg, SHSIZE);
				sharg = cfarg;
			}
		}
		indir = 1;
		VN_PUT(vp);
		u.u_error = lookupname(execnamep, UIOSEG_KERNEL, FOLLOW_LINK,
						(struct vnode **)0, &vp);
		if (u.u_error) {
			vp = (struct vnode *)0;
			goto bad;
		}
		if (u.u_error = exec_getattr(vp, &vattr))
			goto bad;
		/*
		 * Disallow setuid/setgid on shell scripts.  They provide
		 * a well-known security hole, and were flagged for
		 * deletion in the 3.0.17 release notes.
		 */
		if (suid_script == 0) {
			uid = u.u_uid;
			gid = u.u_gid;
		}

		goto again;
	}

	/*
	 * If shared open file table, back out for new program image.
	 * Note: ok to race with concurrent dereference; worst case is
	 * additional work in creating new copy.
	 */
	if (OFILE_SHARED(u.u_ofile_tab)) {
		oft = ofile_clone(u.u_ofile_tab, &u.u_error);
		if (oft == NULL)
			goto bad;
	}

	/*
	 * Collect arguments on "file" in swap space.
	 */
	na = 0;
	ne = 0;
	nc = 0;
	cc = 0;
	uap = (struct execa *)u.u_ap;
	bno = argalloc();
	ASSERT_DEBUG((bno % CLSIZE) == 0, "execve: argalloc");
	if (uap->argp) for (;;) {
		ap = NULL;
		if (indir && (na == 1 || na == 2 && sharg))
			ap = (int)uap->fname;
		else if (uap->argp) {
			ap = fuword((caddr_t)uap->argp);
			uap->argp++;
		}
		if (ap==NULL && uap->envp) {
			uap->argp = NULL;
			if ((ap = fuword((caddr_t)uap->envp)) == NULL)
				break;
			uap->envp++;
			ne++;
		}
		if (ap == NULL)
			break;
		na++;
		do {
			if (ap == -1) {
				error = EFAULT;
				break;
			}
			if (cc <= 0) {
				/*
				 * We depend on NCARGS being a multiple of
				 * CLBYTES.  This way we need only check
				 * overflow before each buffer allocation.
				 */
				if (nc >= NCARGS-1) {
					error = E2BIG;
					break;
				}
				if (bp)
					bdwrite(bp);
				cc = CLBYTES;
				bp = getblk(argdev_vp, bno + ctod(nc / NBPG),
				    cc, 0);
				cp = bp->b_un.b_addr;
			}
			if (indir && na == 2 && sharg != NULL) {
				error = copystr(sharg, cp, cc, &len);
				sharg += len;
			} else {
				error = copyinstr((caddr_t)ap, cp, cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error) {
			if (bp)
				brelse(bp);
			bp = 0;
			goto badarg;
		}
	}
	if (bp)
		bdwrite(bp);
	bp = 0;
	nc = (nc + NBPW-1) & ~(NBPW-1);
	/*
	 * Note: getxfile() is called with vp locked, returns it unlocked.
	 */
	error = getxfile(vp, &exdata.ex_exec, nc + (na+4)*NBPW + USRSTACKADJ,
							uid, gid, &vattr);
	VN_RELE(vp);
	vp = (struct vnode *)NULL;
	if (error) {
badarg:
		for (cc = 0; cc < nc; cc += CLBYTES) {
			/*
			 * Note: could mod baddr() to not return buffer if
			 * IO already started, if do FIFO IO's
			 */
			bp = baddr(argdev_vp, bno + ctod(cc / NBPG), CLBYTES);
			if (bp) {
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				brelse(bp);
				bp = 0;
			}
		}
		u.u_error = error;
		goto bad;
	}
	l.cnt.v_cntexec++;

	/*
	 * copy back arglist
	 */
	ucp = USRSTACK - USRSTACKADJ - nc - NBPW;
	ap = ucp - na*NBPW - 3*NBPW;
	u.u_ar0[SP] = ap;
	(void) suword((caddr_t)ap, na-ne);
	nc = 0;
	cc = 0;
	for (;;) {
		ap += NBPW;
		if (na==ne) {
			(void) suword((caddr_t)ap, 0);
			ap += NBPW;
		}
		if (--na < 0)
			break;
		(void) suword((caddr_t)ap, ucp);
		do {
			if (cc <= 0) {
				if (bp)
					brelse(bp);
				cc = CLBYTES;
				bp = bread(argdev_vp, bno + ctod(nc/NBPG), cc);
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				cp = bp->b_un.b_addr;
			}
			error = copyoutstr(cp, (caddr_t)ucp, cc, &len);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		ASSERT_DEBUG(error != EFAULT, "execve: EFAULT");
	}
	(void) suword((caddr_t)ap, 0);

	/*
	 * Reset caught signals.  Held signals remain held through p_sigmask.
	 * No process-state locking, since only process can modify own
	 * p_sigcatch, and races with psignal() exist anyhow.
	 */

	while (u.u_procp->p_sigcatch) {
		nc = ffs(u.u_procp->p_sigcatch);
		u.u_procp->p_sigcatch &= ~sigmask(nc);
		u.u_signal[nc] = SIG_DFL;
	}

	/*
	 * Allow custom system calls to deal with exec.
	 * Done here in case "close-on-exec" will close some
	 * file-descriptors the custom syscalls might use.
	 */

	cust_sys_exec();

	/*
	 * If allocated a new open file table, install it and loose old one.
	 * Then close all "close-on-exec" file-descriptors.
	 */

	if (oft) {
		ofile_deref(u.u_ofile_tab);
		u.u_ofile_tab = oft;
		oft = NULL;
	}
	ofile_exclose(u.u_ofile_tab);

	setregs(exdata.ex_exec.a_entry);

	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	if (pn.pn_pathlen > MAXCOMLEN)
		pn.pn_pathlen = MAXCOMLEN;
	bcopy((caddr_t)pn.pn_buf, (caddr_t)u.u_comm,
			(unsigned)(pn.pn_pathlen + 1));
bad:
	pn_free(&pn);
	if (bp) {
		brelse(bp);
	}
	if (bno) {
		argfree(bno);
	}
	if (vp) {
		VN_PUT(vp);
	}
	if (oft) {
		error = u.u_error;
		ofile_deref(oft);		/* closef() may alter u_error */
		u.u_error = error;
	}

	/*
	 * If process traced by a multi-process debugger, stop for it.
	 * Stop here (rather than in getxfile()) since have args/etc on
	 * stack and all resouces released.
	 */

	if ((u.u_procp->p_flag & SMPTRC) && !u.u_error) {
		(void) p_lock(&u.u_procp->p_state, SPLHI);
		if (u.u_procp->p_flag & SMPTRC)			/* can race */
			mpt_stop(PTS_EXEC);
		v_lock(&u.u_procp->p_state, SPL0);
	}
}

/*
 * getxfile()
 *	Set up memory for executed file.
 *
 * Arranges text to be RO ref to mapped (VTEXT) vnode, data is copy-on-ref
 * to same vnode.
 *
 * Called with LOCKED vnode; vnode is UNLOCKED before return.
 */

static
getxfile(vp, ep, nargc, uid, gid, va)
	register struct vnode *vp;
	register struct exec *ep;
	int	nargc;
	int	uid;
	int	gid;
	struct	exec_vattr *va;
{
	register struct	proc *p = u.u_procp;
	size_t	ts;				/* text only size (pages) */
	size_t	ds;				/* total of text+init_data */
	size_t	bs;				/* bss size (pages) */
	size_t	ss;				/* initial stack size (pages) */
	u_long	thandle;			/* handle of mmap'd text+data */
	u_long	dhandle;			/* handle of mmap'd data */
	int	error;
	spl_t	s;

	/*
	 * Compute text and data sizes and make sure not too large.
	 * Check both dssize+bsize and individually (wrap-around).
	 *
	 * Also insure text and data sizes are multiple of system page-size,
	 * and executable file is large enough to hold the executable.
	 */

	ts = btop(ep->a_text);
	ds = ts + btop(ep->a_data);
	bs = clrnd(btoc(ep->a_bss));
	ss = clrnd(SSIZE + btoc(nargc));

	if (((ep->a_text | ep->a_data) & CLOFSET) || ep->a_text <= LOWPAGES*NBPG
	||  va->va_size < (ep->a_text + ep->a_data - LOWPAGES*NBPG))
		error = ENOEXEC;
	else if (ep->a_text > MAXTSIZ*NBPG || ep->a_data > MAXDSIZ*NBPG
					   || ep->a_bss > MAXDSIZ*NBPG)
		error = ENOMEM;
	else
		error = chksize((unsigned)ds+bs, (unsigned)ss);

	if (error) {
		VN_UNLOCKNODE(vp);
		return(error);
	}

	/*
	 * Get handle on mapping.  Do this before release old space to
	 * be sure we can get it, and avoid vnode deadlocks.
	 */

	error = xalloc(vp, ts, ds, &thandle, &dhandle);
	VN_UNLOCKNODE(vp);
	if (error)
		return(error);

	/*
	 * Make sure enough swap space to start process.
	 */

	u.u_cdmap = zdmap;
	u.u_csmap = zdmap;
	if (!vsalloc(&u.u_cdmap, ts, ds+bs-ts) || !vsalloc(&u.u_csmap, 0, ss)) {
		vsrele(&u.u_cdmap);
		xfree(ts, ds, thandle, dhandle);
		return(ENOMEM);
	}

	/*
	 * At this point, committed to the new image!
	 * Release virtual memory resources of old process, and
	 * initialize the virtual memory of the new process.
	 * If we resulted from vfork(), instead wakeup our
	 * parent who will wake us up when he has taken back
	 * our resources.  Note that p_pptr is stable since
	 * parent is sleeping on its own p_vfork sema and cannot
	 * have exited.
	 *
	 * Avoid swap while page-table is in state of transition.
	 */

	++p->p_noswap;

	if ((p->p_flag & SVFORK) == 0)
		vrelvm();
	else {
		s = p_lock(&p->p_state, SPLHI);
		p->p_flag &= ~SVFORK;
		v_lock(&p->p_state, s);
		/*
		 * Ok to race with parent switching VM resources here,
		 * since we're not swappable and p_sema doesn't need
		 * those resources.
		 */
		v_sema(&p->p_pptr->p_vfork);
		p_sema(&p->p_vfork, PZERO-1);
		u.u_pmapcnt = 0;
	}
	u.u_mmapmax = u.u_mmap;			/* no mmap's in effect now */

#ifdef	USESOUSIG
	/*
	 * New process image doesn't yet use old flavor signals.
	 */
	s = p_lock(&p->p_state, SPLHI);
	p->p_flag &= ~(SOUSIG);
	v_lock(&p->p_state, s);
#endif	USESOUSIG

	u.u_dmap = u.u_cdmap;
	u.u_smap = u.u_csmap;
	if (error = vgetvm(ds+bs, ss)) {
		/*
		 * Failed to expand -- swap-space.  Process has been
		 * swkill()'d, so need to unwind and die quietly.
		 */
		xfree(ts, ds, thandle, dhandle);
		return(error);
	}

	--p->p_noswap;

	/*
	 * Set up initial Rset and text+data part of page-table.
	 * Then set up bss part of page-table.
	 */

	vinitRS(ds+bs, ss);
	xinitpt(ts, ds, thandle, dhandle, ep->a_magic);
	ptefill(dptopte(p, ds), PG_ZFOD, bs);
	l.cnt.v_nzfod += bs;

	/*
	 * Traced ==> post SIGTRAP if traced under old ptrace.
	 * Set up SUID/SGID protections for untraced process.
	 */

	if (p->p_flag & SDBG) {
		s = p_lock(&p->p_state, SPLHI);
		if (p->p_flag & STRC)		/* old-flavor debugger */
			lpsignal(p, SIGTRAP);
		v_lock(&p->p_state, s);
	} else {
		if (uid != u.u_uid || gid != u.u_gid)
			u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
		p->p_uid = uid;
		p->p_suid = uid;
		u.u_gid = gid;
	}

	u.u_tsize = ts;
	u.u_dsize = ds+bs;
	u.u_ssize = ss;
	u.u_prof.pr_scale = 0;

	return(0);				/* success! */
}

/*
 * exec_getattr()
 *	Get a subset of struct vattr fields.  This keeps execve()'s stack
 *	smaller.
 */

static
exec_getattr(vp, va)
	struct	vnode	*vp;
	register struct	exec_vattr *va;
{
	struct	vattr	vattr;
	int	error;

	if (error = VOP_GETATTR(vp, &vattr, u.u_cred))
		return(error);

	va->va_mode = vattr.va_mode;
	va->va_uid = vattr.va_uid;
	va->va_gid = vattr.va_gid;
	va->va_size = vattr.va_size;
	return(0);
}

/*
 * exec_nosuid()
 *	Complain about not allowing SUID/SGID bits on this file-system.
 *
 * Seperate procedure to keep execve() stack small.
 */

static
exec_nosuid(name)
	caddr_t	name;
{
	struct pathname pn;

	if (pn_get(name, UIOSEG_USER, &pn) == 0) {
		printf("%s: Setuid execution not allowed\n", pn.pn_buf);
		/*
		 *+ setuid programs cannot be executed off this filesystem.
		 *+ This is a result of the way the filesystem was mounted.
		 */
		pn_free(&pn);
	}
}

/*
 * arginit()
 *	Init the argument space map.
 *
 * Called in swfree(0), gets half the 1st dmmax_sw portion of the 1st swap
 * partition.
 */

static	lock_t	arg_mutex;		/* guards argmap */
static	sema_t	arg_wait;		/* wait here when no arg space */
static	int	arg_nwait;		/* # times wait in argalloc() */

arginit(dev, size, addr)
	dev_t	dev;
	long	size;
	long	addr;
{
	argdev = dev;
	argdev_vp = devtovp(argdev);
	rminit(argmap, size, addr, "argmap", ARGMAPSIZE);
	init_lock(&arg_mutex, G_ARGMAP);
	init_sema(&arg_wait, 0, 0, G_ARGMAP);
}

/*
 * argalloc()
 *	Allocate a chunk of argument map.
 *
 * Assumes all requests for the same size chunk.
 *
 * Unconditionally succeeds.
 */

static long
argalloc()
{
	long	addr;
	spl_t	s;

	s = p_lock(&arg_mutex, SPL6);
	while ((addr = rmalloc(argmap, (long)ctod(clrnd(btoc(NCARGS))))) == 0) {
		++arg_nwait;
		p_sema_v_lock(&arg_wait, PZERO-1, &arg_mutex, s);
		s = p_lock(&arg_mutex, SPL6);
	}
	v_lock(&arg_mutex, s);
	return(addr);
}

/*
 * argfree()
 *	Release a piece of argument map space.
 *
 * Assumes all requests for same size chunk.
 */

static
argfree(addr)
	long	addr;
{
	spl_t	s;

	s = p_lock(&arg_mutex, SPL6);
	rmfree(argmap, (long)ctod(clrnd(btoc(NCARGS))), addr);
	if (blocked_sema(&arg_wait))		/* if waiting for arg space */
		v_sema(&arg_wait);		/* ... wake one */
	v_lock(&arg_mutex, s);
}
