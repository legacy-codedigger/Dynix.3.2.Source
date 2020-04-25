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
static char rcsid[] = "$Header: ufs_pipe.c 2.7 89/12/28 $";
#endif

/*
 * ufs_pipe.c
 *	System V pipe implementation.
 */

/* $Log:	ufs_pipe.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../ufs/mount.h"
#include "../h/stat.h"
#include "../h/file.h"
#include "../h/kernel.h"

#include "../machine/intctl.h"

#ifdef notyet
/*
 * The sys-pipe entry.
 * Allocate an inode on the root device.
 * Allocate 2 file structures.
 * Put it all together with flags.
 */

Vpipe()
{
	register struct inode *ip;
	register struct file *rf, *wf;
	int r;
	extern struct fileops inodeops;


	ip = ialloc(rootdir, dirpref(rootdir->i_fs), IFIFO);
	if (ip == NULL) {
		return;
	}
/* maybe imark() here - mjw */
	ip->i_flag |= IACC|IUPD|ICHG;
	ip->i_mode = IFIFO;
	ip->i_nlink = 0;
	ip->i_uid = u.u_uid;
	ip->i_gid = u.u_gid;
/* 2 args */
	iupdat(ip, &time, &time, 1);
	rf = falloc();
	if (rf == (struct file *) NULL)
		goto free2;
	r = u.u_r.r_val1;
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_INODE;
	rf->f_ops = &inodeops;
	rf->f_data = (caddr_t)ip;
	wf = falloc();
	if (wf == (struct file *) NULL)
		goto free3;
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_INODE;
	wf->f_ops = &inodeops;
	wf->f_data = (caddr_t)ip;
	u.u_r.r_val2 = u.u_r.r_val1;
	u.u_r.r_val1 = r;
	ip->i_count = 2;
	ip->i_frcnt = 1;
	ip->i_fwcnt = 1;
	ip->i_size = PIPSIZ;
	ip->i_psize = 0;
	IUNLOCK(ip);
	return;
free3:
	u.u_ofile[r].of_file = NULL;
	ffree(rf);

free2:
	iput(ip);
	return;
}
#endif notyet

/*
 * Open a pipe
 * Check read and write counts, delay as necessary
 * inode has been locked before calling this routine.
 *
 * This is called with the inode locked.
 */

openp(ip, mode)
	register struct inode *ip;
	register mode;
{
	register i;
	register daddr_t *ibp;
	spl_t s;

	/*
	 * On first open, initialize the fifo inode lock with gate
	 * from inode semaphore.
	 */
	if ((ip->i_frcnt == 0) && (ip->i_fwcnt == 0))  {
		init_lock(&ip->i_frwlock, ITOV(ip)->v_nodemutex.rw_mutex);
		ip->i_frptr = 0;
		ip->i_fwptr = 0;
		ip->i_size = PIPSIZ;
		ip->i_psize = 0;
	}

	if (mode&FREAD) {
		if (ip->i_frcnt++ == 0) {
			s = p_lock(&ip->i_frwlock, SPLFS);
			vall_sema(&ip->i_rsema);
			v_lock(&ip->i_frwlock, s);
		}
	}
	if (mode&FWRITE) {
		if (mode&FNDELAY && ip->i_frcnt == 0) {
			/*
			 * if there are no writers, then we must
			 * exit with the pipe in its "closed" state.
			 */
			if (ip->i_fwcnt == 0) {
				ibp = ip->i_ib;
				for (i = 0; i < NIADDR; i++)
					*ibp++ = (daddr_t)0;
				ip->i_size = 0;
			}
			return(ENXIO);
		}
		if (ip->i_fwcnt++ == 0) {
			s = p_lock(&ip->i_frwlock, SPLFS);
			vall_sema(&ip->i_wsema);
			v_lock(&ip->i_frwlock, s);
		}
	}
	if (mode&FREAD) {
		while (ip->i_fwcnt == 0) {
			if (mode&FNDELAY || ip->i_psize)
				return(0);
			s = p_lock(&ip->i_frwlock, SPLFS);
			IUNLOCK(ip);
			/*
			 * Note, if signaled out here, the inode is in
			 * a "sane" state.  Also, the open routines sense
			 * that we've been signalled out, and call closep
			 * to close out the inode before returning to the
			 * user.
			 */
			p_sema_v_lock(&ip->i_wsema, PPIPE, 
					&ip->i_frwlock, s);
			ILOCK(ip);
		}
	}
	if (mode&FWRITE) {
		while (ip->i_frcnt == 0) {
			s = p_lock(&ip->i_frwlock, SPLFS);
			IUNLOCK(ip);
			/*
			 * Note, if signaled out here, the inode is in
			 * a "sane" state.  Also, the open routines sense
			 * that we've been signalled out, and call closep
			 * to close out the inode before returning to the
			 * user.
			 */
			p_sema_v_lock(&ip->i_rsema, PPIPE,
					&ip->i_frwlock, s);
			ILOCK(ip);
		}
	}
	return(0);
}

/*
 * Close a pipe
 * Called with inode locked.
 * Update counts and cleanup
 */

closep(ip, mode)
	register struct inode *ip;
	register mode;
{
	register i;
	register daddr_t *ibp;
	spl_t s;

	if (mode&FREAD) {
		if (--ip->i_frcnt == 0) {
			s = p_lock(&ip->i_frwlock, SPLFS);
			vall_sema(&ip->i_wsema);
			v_lock(&ip->i_frwlock, s);
		}
	}
	if (mode&FWRITE) {
		if (--ip->i_fwcnt == 0) {
			s = p_lock(&ip->i_frwlock, SPLFS);
			vall_sema(&ip->i_rsema);
			v_lock(&ip->i_frwlock, s);
		}
	}
	/*
	 * on last close, zero the fifo information from the inode
	 * (note, we use the indirect block references to do this)
	 * then truncate to zero length
	 */
	if ((ip->i_frcnt == 0) && (ip->i_fwcnt == 0)) {
		ibp = ip->i_ib;
		ip->i_psize = 0;
		for (i = 0; i < NIADDR; i++)
			*ibp++ = (daddr_t)0;
		itrunc(ip, (u_long)0);
	}
}

/*
 * att_pstat()
 *	Return (somewhat fake) stats for a FIFO
 *
 */

extern struct timeval time;

att_pstat(sb, fp)
	register struct stat *sb;
	register struct file *fp;
{
	sb->st_dev = makedev(255,255);
	sb->st_ino = (fp - file);
	sb->st_mode = IFIFO;
	sb->st_blksize = MAXBSIZE;
	sb->st_size = PIPSIZ;
	sb->st_nlink = 0;
	sb->st_uid = u.u_uid;
	sb->st_gid = u.u_gid;
	sb->st_rdev = 0;
	sb->st_atime = time.tv_sec;
	sb->st_spare1 = 0;
	sb->st_mtime = time.tv_sec;
	sb->st_spare2 = 0;
	sb->st_ctime = time.tv_sec;
	sb->st_spare3 = 0;
	sb->st_blocks = 0;
	sb->st_spare4[0] = 0;
	sb->st_spare4[1] = 0;
}
