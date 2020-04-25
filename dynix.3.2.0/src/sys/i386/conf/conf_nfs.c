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

#ifndef	lint
static	char	rcsid[] = "$Header: conf_nfs.c 1.5 1991/10/09 18:20:57 $";
#endif	lint

/*
 * conf_nfs.c
 * 	NFS configurable parameters.
 */

/* $Log: conf_nfs.c,v $
 *
 */

#include "../h/types.h"
#ifdef	NFS
#include "../h/mutex.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/svc_dup.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"

/*
 * Binary configurable NFS parameters.
 *	This module contains numerous NFS parameters. Most of these
 *	should *NEVER* need to change.
 */

/*
 * NFS Server Parameters
 */

/*
 * When a client becomes superuser, it is denied superuser permissions on
 * the server. This is implemented by the server mapping the superuser to
 * "nobody". This is by default -2. If superuser access by the client is
 * to be allowed on the server, then set "nobody" to 0.
 */
int nobody = -2;

/*
 * If nfs_portmon is set, then clients are required to use
 * privileged ports (ports < IPPORT_RESERVED) in order to get NFS services.
 */
int nfs_portmon = 0;

/*
 * If nfs_createmask is set, the NFS server will strip type bits of
 * mode to prevent bogus special file creations.
 */
int nfs_createmask = 1;

/*
 * The dup cacheing routines provide a cache of non-failure
 * transaction id's. Rpc service routines can use this to detect
 * retransmissions and re-send a non-failure response.
 *
 * MAXDUPREQS is the number of cached items. It should be adjusted
 * to the service load so that there is likely to be a response entry
 * when the first retransmission comes in. If the system will be used
 * only as a Client, then MAXDUPREQS may be set very small.
 */
#ifndef	MAXDUPREQS
#define	MAXDUPREQS	400
#endif	MAXDUPREQS
int maxdupreqs = MAXDUPREQS;
struct dupreq drtable[MAXDUPREQS];

/*
 * Duplicate request cache hash table.
 * NOTE: DRHASHSZ must be a power of 2 for duplicate request cache
 * hashing to work!
 */
#define	DRHASHSZ	64
int drhashsz = DRHASHSZ;
struct dupreq *drhashtbl[DRHASHSZ];


/*
 * NFS Client Parameters
 */

/*
 * Client handle table.
 */
#ifndef	MAXCLIENTS
#define MAXCLIENTS	6
#endif	MAXCLIENTS
int maxclients = MAXCLIENTS;
struct chtab chtable[MAXCLIENTS];

/*
 * Rnode hash table size.
 * NOTE: RTABLESIZE must be a power of 2 for rtable hashing to work!
 */
#define	RTABLESIZE	64
int rtablesize = RTABLESIZE;
struct rnode *rtable[RTABLESIZE];

/*
 * Attribute cache timeout values for attributes for regular files,
 * and for directories.
 * These should not be changed unless you really know what you are doing!!
 */
int nfsac_regtimeo = 3;
int nfsac_dirtimeo = 30;

/*
 * Maximum timeout backoff waiting for response from server. This value
 * will be multiplied by "hz" to obtain the maximum.
 *
 * Each NFS request made in the kernel waits "timeo" tenths of a second
 * for a response. See mount(8). If no response arrives, the timeout is
 * multiplied by 2 and the request is retransmitted. "clntkudp_maxtimo"
 * places an upper limit on this timeout value. The initial timeout value
 * is settable when the remote filesystem is mounted.
 */
int clntkudp_maxtimo = 60;	/* 60 * hz */

/*
 * Number of receive tries per rpc transmit from the client (clntkudp_callit()).
 */
int clntkudp_recvtries = 2;

/*
 * Hard mount maximum backoff timeout.
 * If "retrans" (see mount(8)) retransmissions have been sent with no reply
 * a soft mounted filesystem returns an error on the request and a hard
 * mounted filesystem retries the request after adjusting the time delay
 * "timeo" used to wait for a response. This is determined by multiplying
 * the "timeo" value last passed to the rpc call by 4. "hard_maxtimo"
 * places an upper limit on this timeout value. The intial timeout value in
 * tenths of a second is settable when the remote filesystem is mounted.
 */
int hard_maxtimo = 300;		/* 10ths of a second */

#else	!NFS

/*
 * Binary configuration stubs.
 *
 * Used to tie off system calls and initialization routines when NFS
 * is not defined.
 */

/*
 * Initialization routines
 */
svc_mutexinit() {};
clnt_init() {};
nfs_init() {};


/*
 * System calls stubs
 */
nfs_svc() {
	nosys();
};

async_daemon() {
	nosys();
};

nfs_getfh() {
	nosys();
};

#endif	NFS
