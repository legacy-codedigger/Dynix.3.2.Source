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
 * $Header: ofile.h 1.1 90/02/20 $
 *
 * ofile.h
 *	Structures and definitions for open-file table objects.
 */

/* $Log:	ofile.h,v $
 */

#ifndef	OFILE_H
#define	OFILE_H

/*
 * An "open file" in a process is represented by a struct ofile.
 * Each process has by default LNOFILE struct ofile's.  If the process
 * needs more, it allocates an ofile_ext structure.
 */

struct	ofile	{
	struct	file	*of_file;		/* file-table entry */
	char		of_flags;		/* flags (see below) */
	u_short		of_inuse;		/* # concurrent uses */
};

#define	UF_EXCLOSE 	0x1			/* auto-close on exec */
#define	UF_ALLOC 	0x4			/* tentatively allocated */


/*
 * Open-file table object structure.
 *
 * oft_mutex listed first to allow asm functions to gen better code.
 */

struct	ofile_tab {
	lock_t		oft_mutex;	/* misc mutex */
	struct	ofile_tab *oft_next;	/* list of unused entries */
	int		oft_nofile;	/* # ofile entries */
	int		oft_lastfile;	/* oft_ofile high-water mark */
	int		oft_refcnt;	/* # proc's sharing this */
	struct	ofile	*oft_ofile;	/* array of open files */
	struct	ofile	oft_lofile[1];	/* local array */
};

extern	struct	file	  *ofile_getf();
extern	struct	file	  *ofile_close();
extern	struct	ofile_tab *ofile_clone();
extern	struct	ofile_tab *ofile_addref();

#define	OFILE_SHARED(oft)	((oft)->oft_refcnt > 1)
#define	OFILE_NOFILE(oft)	((oft)->oft_nofile)
#define	OFILE_LASTFILE(oft)	((oft)->oft_lastfile)

/*
 * Macros to get file-table pointers from open-file table.
 *
 * Ok to test refcnt outside lock since racing deref won't reference contents.
 */

#define	GETFP(fp, fd) { \
	if (OFILE_SHARED(u.u_ofile_tab)) \
		(fp) = ofile_getf(u.u_ofile_tab, fd); \
	else if ((unsigned)(fd) < OFILE_NOFILE(u.u_ofile_tab))  \
		(fp) = u.u_ofile_tab->oft_ofile[fd].of_file; \
	else \
		(fp) = NULL; \
}

#define	GETF(fp, fd) { \
	GETFP(fp, fd); \
	if ((fp) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}

#endif	OFILE_H
