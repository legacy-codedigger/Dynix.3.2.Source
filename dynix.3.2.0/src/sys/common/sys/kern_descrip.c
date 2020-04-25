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
static	char	rcsid[] = "$Header: kern_descrip.c 2.12 90/06/10 $";
#endif

/*
 * kern_descrip.c
 *	Descriptor management.
 *
 * TODO:
 *	eliminate u.u_error side effects
 */

/* $Log:	kern_descrip.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/stat.h"
#include "../h/flock.h"
#include "../h/vm.h"

#include "../h/ioctl.h"

#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

/*
 * getdtablesize()
 *	Return max number of per-process file-descriptors.
 *
 * Also return high-water mark (highest in-use file-descriptor index).
 */

getdtablesize()
{
	u.u_r.r_val1 = OFILE_NOFILE(u.u_ofile_tab);
	u.u_r.r_val2 = OFILE_LASTFILE(u.u_ofile_tab);
}

/*
 * setdtablesize()
 *	System call to alter size of the calling process's open file
 *	descriptor table.
 */

setdtablesize()
{
	struct a {
		int	nfds;			/* # fd's desired */
	} *uap = (struct a *) u.u_ap;

	u.u_error = ofile_capacity(u.u_ofile_tab, uap->nfds);
	u.u_r.r_val1 = OFILE_NOFILE(u.u_ofile_tab);
}

/*
 * dup()
 *	System call.  Duplicate a given file-descriptor.
 */

dup()
{
	struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	struct	file	*fp;
	int	j;

	GETF(fp, uap->i);
	if ((j = ofile_alloc(u.u_ofile_tab, 0)) < 0) {
		u.u_error = EMFILE;
		return;
	}
	u.u_error = ofile_dup(u.u_ofile_tab, j, fp);
	u.u_r.r_val1 = j;
}

/*
 * dup2()
 *	System call.  Duplicate a given file-descriptor to given file #.
 */

dup2()
{
	struct a {
		int	i, j;
	} *uap = (struct a *) u.u_ap;
	struct	file	*fp;

	GETF(fp, uap->i);
	u.u_r.r_val1 = uap->j;
	u.u_error = ofile_dup(u.u_ofile_tab, uap->j, fp);
}

/*
 * fcntl()
 *	The file control system call.
 */

fcntl()
{
	register struct a {
		int	fdes;
		int	cmd;
		int	arg;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;
	register i;
	struct	flock	bf;
	int	value;
	spl_t	s_ipl;

	GETF(fp, uap->fdes);

	switch(uap->cmd) {
	case F_DUPFD:
		/*
		 * Test of arg range is heuristic if shared ofile table
		 * (vs concurrent setdtablesize()).  Test is only necessary to
		 * preserve EINVAL return code; ofile_alloc() is
		 * self-protecting, returns error if arg out of range.
		 */
		if (uap->arg < 0 || uap->arg >= OFILE_NOFILE(u.u_ofile_tab)) {
			u.u_error = EINVAL;
			return;
		}
		if ((i = ofile_alloc(u.u_ofile_tab, uap->arg)) < 0) {
			u.u_error = EMFILE;
			return;
		}
		u.u_error = ofile_dup(u.u_ofile_tab, i, fp);
		u.u_r.r_val1 = i;
		break;

	case F_GETFD:
		u.u_error = ofile_get_flags(u.u_ofile_tab, uap->fdes, &u.u_r.r_val1);
		break;

	case F_SETFD:
		u.u_error = ofile_set_flags(u.u_ofile_tab, uap->fdes, uap->arg);
		break;

	case F_GETFL:
		u.u_r.r_val1 = fp->f_flag + FOPEN;
		break;

	case F_SETFL:
		/*
		 * The file lock is held until all operations completed. This
		 * means that the lower level code must not block on a 
		 * semaphore. In the case of FIONBIO and FIOASYNC this is
		 * true. This operation is kept atomic to avoid race with
		 * concurrent F_SETFL.
		 */
		s_ipl = FDLOCK(fp);
		fp->f_flag &= FCNTLCANT;
		fp->f_flag |= (uap->arg-FOPEN) &~ FCNTLCANT;
		value = fp->f_flag & FNDELAY;
		u.u_error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&value);
		if (u.u_error) {
			FDUNLOCK(fp, s_ipl);
			break;
		}
		value = fp->f_flag & FASYNC;
		u.u_error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (caddr_t)&value);
		if (u.u_error) {
			value = 0;
			(void)(*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&value);
		}
		FDUNLOCK(fp, s_ipl);
		break;

	case F_GETOWN:
		u.u_error = fgetown(fp, &u.u_r.r_val1);
		break;

	case F_SETOWN:
		u.u_error = fsetown(fp, uap->arg);
		break;

	case F_GETLK:
		/* get record lock */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof bf))
			u.u_error = EFAULT;
		else if ((i = getflck(fp, &bf)) != 0)
			u.u_error = i;
		else if (copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf))
			u.u_error = EFAULT;
		break;

	case F_SETLK:
		/* set record lock and return if blocked */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof bf))
			u.u_error = EFAULT;
		else if ((i = setflck(fp, &bf, 0)) != 0)
			u.u_error = i;
		break;

	case F_SETLKW:
		/* set record lock and wait if blocked */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof bf))
			u.u_error = EFAULT;
		else if ((i = setflck(fp, &bf, 1)) != 0)
			u.u_error = i;
		break;

	default:
		u.u_error = EINVAL;
	}
}

/*
 * fset()
 *	Called by ioctl to set/clear FIONBIO and FIOASYNC.
 *
 * see fcntl() FSETFL case.
 */

fset(fp, bit, value)
	register struct file *fp;
	int	bit;
	int	value;
{
	spl_t	s_ipl;
	int	retval;

	s_ipl = FDLOCK(fp);

	if (value)
		fp->f_flag |= bit;
	else
		fp->f_flag &= ~bit;
	retval = fioctl(fp, (int)(bit == FNDELAY ? FIONBIO : FIOASYNC),
			(caddr_t)&value);

	FDUNLOCK(fp, s_ipl);

	return(retval);
}

fgetown(fp, valuep)
	struct	file	*fp;
	int	*valuep;
{
	int	error;

	switch (fp->f_type) {

	case DTYPE_SOCKET:
		*valuep = ((struct socket *)fp->f_data)->so_pgrp;
		return(0);

	default:
		error = fioctl(fp, (int)TIOCGPGRP, (caddr_t)valuep);
		*valuep = -*valuep;
		return(error);
	}
}

fsetown(fp, value)
	register struct file *fp;
	int	value;
{
	if (fp->f_type == DTYPE_SOCKET) {
		((struct socket *)fp->f_data)->so_pgrp = value;
		return(0);
	}
	if (value > 0) {
		register struct proc *p;
		/*
		 * If process found, pfind locks its p_state.
		 */
		p = pfind(value);
		if (p == 0)
			return (EINVAL);
		value = p->p_pgrp;
		/*
		 * Free lock and back to base level.
		 */
		v_lock(&p->p_state, SPL0);
	} else
		value = -value;
	return(fioctl(fp, (int)TIOCSPGRP, (caddr_t)&value));
}

static
fioctl(fp, cmd, value)
	struct	file	*fp;
	int	cmd;
	caddr_t	value;
{
	return((*fp->f_ops->fo_ioctl)(fp, cmd, value));
}

/*
 * close()
 *	`close' system call.  Close a given file-descriptor.
 */

close()
{
	register struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	struct file *fp;

	fp = ofile_close(u.u_ofile_tab, uap->i);
	if (fp == NULL) {
		u.u_error = EBADF;
		return;
	}

	/*
	 * Finally close the file-table entry.
	 */

	closef(fp);
	/* WHAT IF u.u_error ? */
}

/*
 * fstat()
 *	System-call.  Does a "stat" on an already open file.
 */

fstat()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		struct	stat *sb;
	} *uap = (struct a *) u.u_ap;
	struct stat ub;

	fp = getf(uap->fdes);
	if (fp == 0)
		return;

	switch (fp->f_type) {

	case DTYPE_VNODE:
		/*
		 * Need locked vnode to insure consistent data.
		 */
		VN_LOCKNODE((struct vnode *)fp->f_data);
		u.u_error = vno_stat((struct vnode *)fp->f_data, &ub);
		VN_UNLOCKNODE((struct vnode *)fp->f_data);
		break;

	case DTYPE_SOCKET:
		if (u.u_tuniverse == U_ATT) {
 			att_pstat(&ub, fp);
			break;
		}
		u.u_error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*
		 *+ The fstat() system call was called on a file
		 *+ descriptor whos type is not a vnode nor a socket.
		 *+ The file pointer has likely been corrupted.
		 */
		/*NOTREACHED*/
	}
	if (u.u_error == 0)
		u.u_error = copyout((caddr_t)&ub, (caddr_t)uap->sb, sizeof(ub));
}

/*
 * finit()
 *	Init the file-table and file entries.
 *
 * Called in main() when the rest of the world is comming alive.
 */

lock_t	file_list;			/* to lock the list */
int	ffree_head;			/* head of free-list */

finit()
{
	register struct file *fp;
	register int	gateno;

	init_lock(&file_list, G_FILELIST);
	ffree_head = -1;			/* end of list indicator */
	gateno = 0;
	for (fp = file; fp < fileNFILE; fp++) {
		fp->f_next = ffree_head;
		ffree_head = fp - file;
		init_lock(&fp->f_mutex,
		       (gate_t)(G_FILMIN + (gateno++ % (G_FILMAX-G_FILMIN+1))));
	}
#ifdef	lint
	lint_ref_int(gateno);
#endif	lint

	/*
	 * Also initialize callers open-file table.
	 */

	ofile_init();
}

/*
 * falloc()
 *	Allocate a user file descriptor and a file structure.
 *
 * Initialize the descriptor to point at the file structure.
 */

struct file *
falloc()
{
	register struct file *fp;
	register i;
	spl_t	s;

	if ((i = ofile_alloc(u.u_ofile_tab, 0)) < 0) {
		u.u_error = EMFILE;
		return (NULL);
	}

	/*
	 * Lock file-list for allocation.
	 * If there is a free element, take it; else bitch.
	 */

	s = p_lock(&file_list, SPLFS);
	if (ffree_head < 0) {
		v_lock(&file_list, s);
		tablefull("file");
		u.u_error = ENFILE;
		ofile_install(u.u_ofile_tab, i, (struct file *) NULL);
		return (NULL);
	}
	fp = &file[ffree_head];
	ffree_head = fp->f_next;

	fp->f_data = 0;
	fp->f_count = 1;

#ifdef	ns32000
	ASSERT_DEBUG(fp->f_mutex.l_state == L_UNLOCKED, "falloc: locked");
#endif	ns32000
#ifdef	i386
	ASSERT_DEBUG(fp->f_mutex == L_UNLOCKED, "falloc: locked");
#endif	i386

	v_lock(&file_list, s);

	fp->f_offset = 0;			/* ok to do outside list lock */
	crhold(u.u_cred);
	fp->f_cred = u.u_cred;
	u.u_r.r_val1 = i;			/* return value */
	return (fp);
}

/*
 * ffree()
 *	Free a file-node.
 *
 * Called various places (closef(), unwinding in copen()) to give back
 * a previously allocated file node.
 */

ffree(fp)
	register struct file *fp;
{
	struct	ucred	*cr;
	spl_t	s;

	s = p_lock(&file_list, SPLFS);

	fp->f_next = ffree_head;
	ffree_head = fp - file;
	fp->f_type = 0;
	fp->f_count = 0;		/* for cleanliness */
					/* Not redundant if copen() unwind */
	cr = fp->f_cred;
	v_lock(&file_list, s);

	crfree(cr);
}

/*
 * getf()
 *	Convert a user supplied file descriptor into a pointer
 *	to a file structure.
 *
 * Critical paths should use the GETF macro directly.
 */

struct file *
getf(fd)
	int	fd;
{
	register struct	ofile_tab *oft = u.u_ofile_tab;
	struct	file	*fp;

	if (OFILE_SHARED(oft))
		fp = ofile_getf(oft, fd);
	else if ((unsigned)(fd) < oft->oft_nofile)
		fp = oft->oft_ofile[fd].of_file;
	else
		fp = NULL;
	if (fp == NULL)
		u.u_error = EBADF;
	return fp;
}

/*
 * closef()
 *	Internal form of close.
 *
 * Decrement reference count on file structure, closes underling object
 * if count hits zero.
 */

closef(fp)
	register struct file *fp;
{
	spl_t	s;

	if (fp == NULL)
		return;
	ASSERT_DEBUG(fp->f_count > 0, "closef: f_count");
	cleanlocks(fp);
	s = FDLOCK(fp);
	if (fp->f_count > 1) {
		fp->f_count--;
		FDUNLOCK(fp, s);
		return;
	}

	/*
	 * Last ref going away.  We can unlock, since nobody's going
	 * to play with it any more.  Note that the "fo_close" procedure
	 * must ffree() the file-node.
	 *
	 * Must also decrement the vnode writer count if appropriate.
	 */

	fp->f_count = 0;	   /* for forceclose() and cleanliness */

	FDUNLOCK(fp, s);

	(*fp->f_ops->fo_close)(fp);
}

/*
 * deref_file()
 *	Clone of closef() for shared-ofile-table semantics.
 *
 * No record-lock cleanup (cleanlocks(): see closef()), since not used to
 * close open-file table entry, rather drop reference to file-table entry.
 *
 * Decrement reference count on file structure, closes underling object
 * if count hits zero.
 */

deref_file(fp)
	register struct file *fp;
{
	spl_t	s;

	ASSERT_DEBUG(fp != NULL, "deref_file: NULL fp");
	ASSERT_DEBUG(fp->f_count > 0, "deref_file: f_count");

	s = FDLOCK(fp);
	if (fp->f_count > 1) {
		fp->f_count--;
		FDUNLOCK(fp, s);
		return;
	}

	/*
	 * Last ref going away.  We can unlock, since nobody's going
	 * to play with it any more.  Note that the "fo_close" procedure
	 * must ffree() the file-node.
	 */

	fp->f_count = 0;	   /* for forceclose() and cleanliness */

	FDUNLOCK(fp, s);

	(*fp->f_ops->fo_close)(fp);
}

/*
 * flock()
 *	Apply an advisory lock on a file descriptor.
 */

flock()
{
	register struct a {
		int	fd;
		int	how;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;

	fp = getf(uap->fd);
	if (fp == NULL)
		return;

	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EOPNOTSUPP;
		return;
	}

	if (((struct vnode *)fp->f_data)->v_type == VFIFO) {
		u.u_error = EINVAL;
 		return;
	}

	if (!(uap->how & (LOCK_UN | LOCK_EX | LOCK_SH))) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->how & LOCK_UN) {
		vno_unlock(fp, FSHLOCK|FEXLOCK);
		return;
	}

	/*
	 * No lock needed on fp here.  It wouldn't avoid any
	 * races that aren't already there.
	 */

	if ((fp->f_flag & FEXLOCK) && (uap->how & LOCK_EX) ||
	    (fp->f_flag & FSHLOCK) && (uap->how & LOCK_SH))
		return;
	u.u_error = vno_lock(fp, uap->how);
}
