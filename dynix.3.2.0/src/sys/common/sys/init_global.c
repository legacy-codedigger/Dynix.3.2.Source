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
static	char	rcsid[] = "$Header: init_global.c 2.7 88/08/15 $";
#endif

/*
 * init_global.c
 *
 * Declare various kernel global symbols that are only extern'd
 * elsewhere.
 */

/* $Log:	init_global.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../h/file.h"
#include "../h/callout.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/kernel.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif
#include "../balance/engine.h"

#include "../machine/pte.h"

/*
 * From kernel.h...
 */

/* 1.1 */
long	hostid;
char	hostname[32];
int	hostnamelen;
char	domainname[32];
int	domainnamelen;

/* 1.2 */
struct	timeval boottime;
struct	timeval time;
struct	timezone tz;
int	hz;
sema_t	lbolt;				/* awoken once a second */

struct	pte	zeropg[CLSIZE];		/* source of RO zeroes */

/*
 * From systm.h...
 */

int	mpid;			/* generic for unique process id's */

lock_t	select_lck;		/* used by select mechanism */
sema_t	selwait;
sema_t	unmount_mutex;		/* serialize unmounts */

struct	vnode	*swapdev_vp;	/* vnode equivalent of swapdev */
struct	vnode	*argdev_vp;	/* vnode equivalent of argdev */

/*
 * From engine.h
 */

lock_t	engtbl_lck;	/* lock all engine e_count fields */
sema_t	eng_wait;	/* on/off-line coordination */

/*
 * From proc.h
 */

short	pidhash[PIDHSZ];

/*
 * From vfs.h
 */
sema_t	vfs_mutex;

#ifdef QUOTA
/*
 * From quota.h
 */
lock_t	quota_list;	/* lock all dquot list */
sema_t	quota_sema;	/* wait here for dquots to be released when closing */
#endif

/*
 * The various things allocated at boot time.
 */

struct	proc	*proc		= NULL;
struct	proc	*procNPROC	= NULL;
struct	inode	*inode		= NULL;
struct	inode	*inodeNINODE	= NULL;
struct	mount	*mounttab	= NULL;
struct	mount	*mountNMOUNT	= NULL;
struct	file	*file		= NULL;
struct	file	*fileNFILE	= NULL;
struct	callout	*callout	= NULL;
struct	cblock	*cfree		= NULL;
struct	buf	*buf		= NULL;
struct	buf	*swbuf		= NULL;
char	*buffers		= NULL;
struct	cmap	*cmap		= NULL;
struct	cmap	*ecmap		= NULL;

#ifdef QUOTA
struct dquot	*dquot		= NULL;
struct dquot	*dquotNDQUOT	= NULL;
struct dqhead	*dqhead		= NULL;
#endif /* QUOTA */
