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
 * $Header: flock.h 2.2 1991/04/25 18:20:47 $
 */

/* $Log: flock.h,v $
 *
 */

/* file segment locking set data type - information passed to system by user */
#ifndef	F_RDLCK
#include "fcntl.h"
#endif

/* file locking structure (connected to file table entry) */
struct	filock	{
	struct	flock set;	/* contains type, start, and length */
	union	{
		int wakeflg;	/* for locks sleeping on this one */
		int blkpid;	/* pid of blocking lock
				 * (for sleeping locks only)
				 */
	}	stat;
	sema_t	wait;
	struct	filock *prev;
	struct	filock *next;
};

/* table to associate files with chain of locks */
struct	flino {
	struct	vnode *fl_vp;	 /* vp uniquely identifies an open file. */
	int	fl_refcnt;	 /* # procs currently referencing this flino */
	struct	filock *fl_flck; /* pointer to chain of locks for this file */
	lock_t	fl_lock;	 /* exclusion on fl_flck chain and filock's */
	spl_t	fl_ipl;		 /* priority level before exclusion */
	struct	flino  *prev;
	struct	flino  *next;
};

/* file and record locking configuration structure */
/* record and file use totals may overflow */
struct flckinfo {
	long recs;	/* number of records configured on system */
	long fils;	/* number of file headers configured on system */
	long reccnt;	/* number of records currently in use */
	long filcnt;	/* number of file headers currently in use */
	long rectot;	/* number of records used since system boot */
	long filtot;	/* number of file headers used since system boot */
};

extern struct flckinfo	flckinfo;
extern struct filock  	flox[];
extern struct flino   	flinotab[];

#define FLOCK_GATE	63
