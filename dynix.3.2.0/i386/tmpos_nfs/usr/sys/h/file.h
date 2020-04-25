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

/*
 * $Header: file.h 2.14 1991/05/28 23:37:38 $
 *
 * file.h
 *	File-descriptor structure.
 */

/* $Log: file.h,v $
 *
 *
 *
 *
 */

#ifndef _SYS_FILE_H_

#ifdef	KERNEL
#include "fcntl.h"
#include "unistd.h"

#define	DTYPE_VNODE	1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	long	f_count;	/* reference count */
	short	f_type;		/* descriptor type */
	short	f_msgcount;	/* references from message queue */
				/* with uipc, access rights sending */
	lock_t	f_mutex;	/* lock access to this descriptor */
	short	f_next;		/* free-list thread */
	struct	fileops {
		int	(*fo_rw)();
		int	(*fo_ioctl)();
		int	(*fo_select)();
		int	(*fo_close)();
	} *f_ops;
 	caddr_t	f_data;		/* ptr to file specific struct (vnode/socket) */
	off_t	f_offset;
	off_t	f_uoffset;	/* offset for sysV dir translations */
	off_t	f_ucbboff;	/* another offset for sysV */
 	struct	ucred *f_cred;	/* credentials of user who opened file */
};

extern	struct	file *file, *fileNFILE;
extern	int	nfile;
extern	lock_t	file_list;
struct	file	*getf();
struct	file	*falloc();

/*
 * Lock/unlock, bump f_count macros.
 */

#define	FDLOCK(fp)	p_lock(&(fp)->f_mutex, SPLFD);
#define	FDUNLOCK(fp,s)	v_lock(&(fp)->f_mutex, (s));
#define	FDBUMP(fp)	{spl_t fd_spl; fd_spl = FDLOCK(fp); ++(fp)->f_count; FDUNLOCK(fp,fd_spl);}

/*
 * flags- also for fcntl call.
 */
#define	FOPEN		0xffffffff
#define	FREAD		0x00000001	/* descriptor read/receive'able */
#define	FWRITE		0x00000002	/* descriptor write/send'able */

/* kernel only versions -- deprecated, should be removed */
#define	FCREAT		O_CREAT	
#define	FDEFER		O_DEFER
#define	FEXCL		O_EXCL	
#define	FEXLOCK		O_EXLOCK
#define	FMARK		O_MARK	
#define	FSHLOCK		O_SHLOCK
#define	FTRUNC		O_TRUNC

#define FSYNC           O_SYNC

/* bits to save after open */
#define	FMASK		(FREAD|FWRITE|O_APPEND|O_ASYNC|O_SYNC)
#define	FCNTLCANT	(FREAD|FWRITE|O_DEFER|O_EXLOCK|O_MARK|O_SHLOCK)

/*
 * open useage bits.
 */
#define	FUSEM		0x00f00000	/* usage mask */
#define	FNORM		0x00100000	/* Normal Block or character open */
#define	FMOUNT		0x00200000	/* open for mount */
#define	FSWAP		0x00400000	/* Swap Block or character open */
#define	FSPEC		0x00800000	/* Special use open */

#else

#include "sys/fcntl.h"
#include "sys/unistd.h"

/*
 * added since some old 4.2 stuff referes to it. Base code is clean however.
 */
#define	FREAD		0x00000001	/* descriptor read/receive'able */
#define	FWRITE		0x00000002	/* descriptor write/send'able */
#endif	/* KERNEL */

/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#define  _SYS_FILE_H_ 1
#endif /* _SYS_FILE_H_ */
