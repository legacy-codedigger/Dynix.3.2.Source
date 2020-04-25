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

/* $Header: mntent.h 1.3 90/07/27 $ */

/*
 * File system table, see mntent (5)
 *
 * Used by dump, mount, umount, swapon, fsck, df, ...
 *
 * Quota files are always named "quotas", so if type is "rq",
 * then use concatenation of mnt_file and "quotas" to locate
 * quota file.
 */

#define	MNTTAB		"/etc/fstab"
#define	MOUNTED		"/etc/mtab"

#define	MNTMAXSTR	128

#define	MNTTYPE_42	"4.2"	/* 4.2 file system */
#define	MNTTYPE_NFS	"nfs"	/* network file system */
#define	MNTTYPE_PC	"pc"	/* IBM PC (MSDOS) file system */
#define	MNTTYPE_SWAP	"swap"	/* swap file system */
#define	MNTTYPE_IGNORE	"ignore"/* No type specified, ignore this entry */

#ifndef MNTTYPE_OFS
#define MNTTYPE_OFS "ofs"	/* Optical File system */
#endif

#define	MNTOPT_RO	"ro"	/* read only */
#define	MNTOPT_RW	"rw"	/* read/write */
#define	MNTOPT_QUOTA	"quota"	/* quotas */
#define	MNTOPT_NOQUOTA	"noquota"	/* no quotas */
#define	MNTOPT_SOFT	"soft"	/* soft mount */
#define	MNTOPT_HARD	"hard"	/* hard mount */
#define	MNTOPT_NOSUID	"nosuid"	/* no set uid allowed */
#define	MNTOPT_INTR	"intr"	 /* allow interrupts on hard mount */

struct	mntent{
	char	*mnt_fsname;		/* name of mounted file system */
	char	*mnt_dir;		/* file system path prefix */
	char	*mnt_type;		/* MNTTYPE_* */
	char	*mnt_opts;		/* MNTOPT* */
	int	mnt_freq;		/* dump frequency, in days */
	int	mnt_passno;		/* pass number on parallel fsck */
};

struct	mntent *getmntent();
char	*hasmntopt();
FILE	*setmntent();
int	endmntent();
