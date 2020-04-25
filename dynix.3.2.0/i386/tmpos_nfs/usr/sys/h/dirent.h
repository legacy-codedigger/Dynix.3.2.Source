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

/* $Header: dirent.h 1.1 90/10/09 $
 */

/* $Log:	dirent.h,v $
 */

/*
 * The following structure defines the file
 * system independent directory entry.
 *
 * NOTE: Currently this is *only* for support of odd sized NFS readdir
 * operations.  Nothing else should be using it.
 *
 */

struct dirent
	{
	ino_t		d_ino;		/* inode number of entry */
	off_t		d_off;		/* offset of disk direntory entry */
	unsigned short	d_reclen;	/* length of this record */
	char		d_name[1];	/* name of file */
	};

#ifdef KERNEL
/*
 * macro to calculate number of bytes to hold a "translated" dirent
 * structure (system V), based on the namelength (without terminating
 * null) of the name to be placed in the entry.
 */

#ifndef NBPW
#define NBPW    sizeof(int)
#endif /* NBPW */

#define DNTSIZ(namelen) \
(((int)(((struct dirent *)0)->d_name)+ (namelen) +1 + (NBPW-1)) & ~(NBPW-1))
#endif /* KERNEL */
