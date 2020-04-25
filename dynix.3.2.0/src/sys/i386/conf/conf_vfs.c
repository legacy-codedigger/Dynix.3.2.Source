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
static	char	rcsid[] = "$Header: conf_vfs.c 1.3 90/11/04 $";
#endif

/*
 * conf_vfs.c
 * 	File System switch table.
 */

/* $Log:	conf_vfs.c,v $
 */

#include "../h/param.h"
#include "../h/vfs.h"

extern	struct vfsops ufs_vfsops;
#ifdef	NFS
extern	struct vfsops nfs_vfsops;
#endif	NFS
#ifdef	OFS
extern	struct vfsops ofs_vfsops;
#endif	OFS

struct vfsops *vfssw[] = {
	&ufs_vfsops,		/* 0 = MOUNT_UFS */
#ifdef	NFS
	&nfs_vfsops,		/* 1 = MOUNT_NFS */
#else
	(struct vfsops *)0,
#endif	NFS
#ifdef	OFS
	&ofs_vfsops,		/* 2 = MOUNT_OFS */
#endif	OFS			/* leave mount_ntypes == 2 if !OFS */
};

int	mount_ntypes = sizeof(vfssw) / sizeof(vfssw[0]);
