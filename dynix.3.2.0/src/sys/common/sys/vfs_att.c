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

#ifndef lint
static char rcsid[] = "$Header: vfs_att.c 2.14 89/06/26 $";
#endif

/*
 * vfs_att.c
 *	System V specific VFS support routines, including
 *	directory translation from att to ucb.
 */

/* $Log:	vfs_att.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/dir.h"
#include "../h/dir_att.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/buf.h"
#undef	NFS
#include "../h/mount.h"
#include "../ufs/fs.h"
#include "../ufs/mount.h"
#include "../h/ustat.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"

extern	int	dirchk;

att_creat()
{
	u.u_tuniverse = U_ATT;
	creat();
	u.u_tuniverse = U_UCB;
}

att_open()
{
	u.u_tuniverse = U_ATT;
	open();
	u.u_tuniverse = U_UCB;
}

att_mknod()
{
	u.u_tuniverse = U_ATT;
	mknod();
	u.u_tuniverse = U_UCB;
}

att_read()
{
	u.u_tuniverse = U_ATT;
	read();
	u.u_tuniverse = U_UCB;
}

att_fstat()
{
	u.u_tuniverse = U_ATT;
	fstat();
	u.u_tuniverse = U_UCB;
}

att_stat()
{
	u.u_tuniverse = U_ATT;
	stat();
	u.u_tuniverse = U_UCB;
}

/*
 * State structure for reading directories in "SysV" format.
 */

struct att_garbage {
	int		dentoffset;	/* dir entry offset in file-sys block */
	struct	buf	*dbuf;		/* buffer holding directory data */
	struct	vnode	*dvp;		/* the directory being read */
	int		fsbsize;	/* underlying file-sys block size */
	struct	direct	*ep;		/* current directory entry */
	struct	ucred	*cred;		/* credentials for reading */
	off_t		attoff;		/* current att directory offset */
	off_t		ucboff;		/* current ucb directory offset */
	off_t		ucbboff;	/* last guio.uio_offset */
	off_t		fuoffset;	/* copy of f_uoffset, for sane races */
	bool_t		first_time;	/* set ==> must read a dir block */
	struct uio	guio;		/* uio struct to read dirs */
	struct iovec	giovec;		/* iovec "	" */
};

/*
 * att_ginit()
 *	Initialize for reading or sizing directory in SysV format.
 */

static
att_ginit(gp, dvp, fp, offset)
	register struct att_garbage *gp;
	struct	vnode *dvp;
	register struct file *fp;
	off_t	offset;
{
	if (fp == NULL) {
		gp->cred = u.u_cred;
		gp->fuoffset = 0;
	} else {
		gp->cred = fp->f_cred;
		gp->fuoffset = fp->f_uoffset;
	}
	gp->fsbsize = dvp->v_vfsp->vfs_bsize;
	gp->dbuf = geteblk(gp->fsbsize);
	gp->dentoffset = 0;
	gp->dvp = dvp;
	gp->ep = NULL;
	gp->attoff = -1;
	gp->guio.uio_iov = &gp->giovec;
	gp->guio.uio_iovcnt = 1;
	gp->guio.uio_resid = 0;	     /* needed for first time thru att_skip */
	gp->guio.uio_segflg = UIOSEG_KERNEL;
	if (offset == 0)
		gp->fuoffset = 0;
	if (gp->fuoffset == 0) {
		gp->guio.uio_offset = gp->ucbboff = 0;
		gp->ucboff = 0;
	} else {
		gp->guio.uio_offset = gp->ucbboff = fp->f_ucbboff;
		gp->ucboff = fp->f_ucbboff;
	}
	gp->first_time = 1;
	att_skip(gp);
}

/*
 * att_gclean()
 *	Finish up after reading/sizing directory in SysV format.
 */

static
att_gclean(gp, fp)
	register struct att_garbage *gp;
	register struct file *fp;
{
	brelse(gp->dbuf);
	if (fp != NULL) {
		fp->f_ucbboff = gp->ucbboff;
		fp->f_uoffset = gp->ucboff;
	}
}

/*
 * att_skip()
 *	Skip directory entries.
 */

static
att_skip(gp)
	register struct att_garbage *gp;
{
	register struct direct *gep;
	int spcleft, error;

	if (gp->attoff != -1) {
		gp->ucboff += (gp->ep)->d_reclen;
		gp->dentoffset += (gp->ep)->d_reclen;
		gp->attoff += sizeof (struct direct_att);
	} else
		gp->attoff = 0;

	for (;;) {
		/*
		 * If offset is at end of last read or 1st time,
		 * read the next directory block.
		 */
		if (gp->dentoffset >= (gp->fsbsize - gp->guio.uio_resid)
		   || gp->first_time) {
			gp->ucbboff = gp->guio.uio_offset;
			gp->giovec.iov_base = gp->dbuf->b_un.b_addr;
			gp->guio.uio_resid = gp->giovec.iov_len = gp->fsbsize;
			error = VOP_READDIR(gp->dvp, &gp->guio, gp->cred);
			/*
			 * An EINVAL error from ufs_readdir indicates
			 * that we tried to read past the end of an
			 * empty (only . and ..) directory, who's size
			 * in 4.2 is 24 bytes. It should really be
			 * DIRBLKSIZ (which it is in 4.3). When we go
			 * to 4.3, we should check for the error
			 * condition and do something meaningful.
			 */
			if (error ||
			    ((gp->fsbsize - gp->guio.uio_resid) == 0)) {
				gp->ep = NULL;
				return;
			}
			gp->first_time = 0;
			gp->dentoffset = 0;
		}

		/*
		 * Get pointer to next entry, and do consistency checking:
		 *	record length must be multiple of 4
		 *	record length must not be zero
		 *	entry must fit in rest of this DIRBLKSIZ block
		 *	record must be large enough to contain name
		 * When dirchk is set we also check:
		 *	name is not longer than MAXNAMLEN
		 *	name must be as long as advertised, and null terminated
		 * Checking last two conditions is done only when dirchk is
		 * set, to save time.
		 *
		 * The "bad dir" printf is redundant with dirbad() in the
		 * UFS file-system, but is file-sys independent.
		 */
		gep = gp->ep = (struct direct *)
				(gp->dbuf->b_un.b_addr + gp->dentoffset);
		spcleft = DIRBLKSIZ-(gp->dentoffset & (DIRBLKSIZ - 1));
		if ((gep->d_reclen & 0x3) || gep->d_reclen == 0 ||
		    gep->d_reclen > spcleft || DIRSIZ(gep) > gep->d_reclen ||
		    dirchk && (gep->d_namlen > MAXNAMLEN || 
			dirbadname(gep->d_name, (int)gep->d_namlen))) {
#ifdef	DEBUG
			printf("bad dir:reclen=0x%x namlen=0x%x spcleft=0x%x\n",
				gep->d_reclen, gep->d_namlen, spcleft); 
#endif	DEBUG
			gp->ucboff += spcleft;
			gp->dentoffset += spcleft;
			continue;
		}

		if (gep->d_ino == 0) {
			gp->ucboff += gep->d_reclen;
			gp->dentoffset += gep->d_reclen;
			continue;
		}
		return;
	}
}

/*
 * readdir_att()
 *	Read a directory, returning data as if it were a SysV format
 *	directory.
 */

readdir_att(dvp, uio, fp)
	register struct vnode *dvp;
	register struct uio *uio;
	register struct file *fp;
{
	struct att_garbage g;
	struct direct_att dirbuf;
	register int skip, n;

	VN_LOCKNODE(dvp);		/* File size can't change during this */

	att_ginit(&g, dvp, fp, uio->uio_offset);

	skip = uio->uio_offset % sizeof(struct direct_att);

	if (g.fuoffset == 0)  
		while(g.attoff != (uio->uio_offset - skip) && g.ep != NULL)  
			att_skip(&g);
	else 
		while(g.ucboff < g.fuoffset && g.ep != NULL) 
			att_skip(&g);
		
	/*
	 * Could speed up by building big sysV buffer and moving large chunks.
	 */

	while(uio->uio_resid && g.ep != NULL) {
		/*
	 	 * Translate to sysV format, and move to user buffer.
	 	 */
		bzero((caddr_t)&dirbuf, sizeof(dirbuf));
		dirbuf.d_ino = g.ep->d_ino;
		bcopy(g.ep->d_name, dirbuf.d_name, g.ep->d_namlen + 1);
		n = min((unsigned)(sizeof(struct direct_att) - skip), 
					(unsigned)uio->uio_resid);
		u.u_error = uiomove((caddr_t)&dirbuf + skip, n, UIO_READ, uio);
		if (u.u_error != 0)
			break;
		if ((skip + n) == sizeof(struct direct_att))
			att_skip(&g);
		skip = 0;
	}
	att_gclean(&g, fp);
	VN_UNLOCKNODE(dvp);
	return(u.u_error);
}

/*
 * sizedir_att()
 *	Return "logical" size of directory for SysV programs.
 *	Temp kludge: Sizing the dir requires that the data
 *	blocks be read to count the number of entries. This
 *	sets the accessed time in the inode. The U_NOACC bit
 *	tells rwip() not to set it. This kludge will go away
 *	in the next release when we keep track of the number
 *	of directory entries in the inode itself. Thus, the
 *	size can be obtained by multiplying by the dir size.
 *
 * Called with dvp locked.
 */

sizedir_att(dvp)
	register struct vnode *dvp;
{
	register int count;
	struct att_garbage g;

	u.u_tuniverse |= U_NOACC;	/* Don't set I_ACC in inode */
	att_ginit(&g, dvp, (struct file *)NULL, 0);

	count = 0;
	while (g.ep != NULL) {
		count++;
		att_skip(&g);
	}

	att_gclean(&g, (struct file *)NULL);
	u.u_tuniverse &= ~U_NOACC;
	return(count * sizeof(struct direct_att));
}

att_chown()
{
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;
	struct vattr vattr;

	uap = (struct a *)u.u_ap;

	u.u_tuniverse = U_ATT;
	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = namesetattr(uap->fname, NO_FOLLOW, &vattr);
	u.u_tuniverse = U_UCB;
}

/*
 * system 5 mount system call.
 */
att_mount()
{
	register struct a {
		caddr_t	data;	/* (actually struct ufs_args *) */
		char	*reg;
		int	ronly;
	} *uap = (struct a *)u.u_ap;

	cmount(MOUNT_UFS, uap->reg, uap->ronly ? M_RDONLY : 0, uap->data);
}

/*
 * system 5 umount system call.
 * The common unmount function requires the mounted-on directory
 * rather than the special file. The directory is stored in
 * the superblock so we cheat and pick it up from there. Note
 * that this will only work for ufs-type file systems. This
 * violates all the vnode/vfs layering but in the interest of
 * simplicity/efficiency.... Anyway, sys5 allows only 1 type
 * of file system.
 */
att_umount()
{
	register struct a {
		char	*fspec;
	} *uap = (struct a *)u.u_ap;
	register struct fs *fs;
	struct vnode *vp;
	dev_t dev;
	extern struct vnodeops ufs_vnodeops;
	extern struct fs *trygetfs();

	if (!suser())
		return;

	u.u_error = lookupname(uap->fspec, UIOSEG_USER, FOLLOW_LINK,
			(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	
	/*
	 * Operation allowed only on local unix file systems.
	 */
	if (vp->v_op != &ufs_vnodeops) {
		VN_PUT(vp);
		u.u_error = EINVAL;
		return;
	}
	if (vp->v_type != VBLK) {
		VN_PUT(vp);
		u.u_error = ENOTBLK;
		return;
	}
	dev = vp->v_rdev;
	VN_PUT(vp);
	if (major(dev) >= nblkdev) {
		u.u_error = ENXIO;
		return;
	}

	/*
	 * Getting the name of mounted-on directory and
	 * the unmount must be indivisible relative to
	 * other mounts/u[n]mounts.
	 */
	p_sema(&unmount_mutex, PVFS);
	if ((fs = trygetfs(dev)) == NULL) {
		v_sema(&unmount_mutex);
		u.u_error = EINVAL;
		return;
	}
	fs->fs_fsmnt[MAXMNTLEN-1] = '\0';	/* ensure null-terminated */
	cunmount(fs->fs_fsmnt, UIOSEG_KERNEL);
	v_sema(&unmount_mutex);		/* u.u_error set by cunmount */
}
/*
 * ustat
 * This is really ugly. The name/pack fields are in
 * the superblock while we get the free blocks/inodes
 * from the statfs structure.
 */
ustat()
{
	register struct a {
		int	dev;
		struct ustat *buf;
	} *uap = (struct a *)u.u_ap;
	register struct fs *fs;
	register struct mount *mp;
	extern struct mount *getmp();
	struct statfs sb;
	struct ustat ust;

	/*
	 * Need to mutex with u[n]mount since no
	 * guarantee that fs is busy and hence, locked.
	 * Calling getmp() is really a no-no since it's
	 * fs-specific. vfs_mutex stays locked until
	 * name/pack have been extracted from the superblock.
	 */
	p_sema(&vfs_mutex, PVFS);
	if ((mp = getmp((dev_t)uap->dev)) == NULL) {
		v_sema(&vfs_mutex);
		u.u_error = EINVAL;
		return;
	}
	u.u_error = VFS_STATFS(&mp->m_vfs, &sb);
	if (u.u_error) {
		v_sema(&vfs_mutex);
		return;
	}
	
	fs = mp->m_bufp->b_un.b_fs;
	bcopy(fs->fs_fname, ust.f_fname, sizeof(ust.f_fname));
	bcopy(fs->fs_fpack, ust.f_fpack, sizeof(ust.f_fpack));
	v_sema(&vfs_mutex);
	ust.f_tfree = sb.f_bfree;
	ust.f_tinode = sb.f_ffree;
	u.u_error = copyout((caddr_t)&ust, (caddr_t)uap->buf, sizeof(struct ustat));
}

att_unlink()
{
	u.u_tuniverse = U_ATT;
	unlink();
	u.u_tuniverse = U_UCB;
}

att_acct()
{
	u.u_tuniverse = U_ATT;
	sysacct();
	u.u_tuniverse = U_UCB;
}
