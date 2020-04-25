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
static	char	rcsid[] = "$Header: vfs_dnlc.c 2.8 1991/06/07 03:52:54 $";
#endif

/*
 * vfs_dnlc.c
 *	Virtual File-System directory-name lookup cache.
 */

/*
 * Copyright (c) 1984 by Sun Microsystems.
 */

/* $Log: vfs_dnlc.c,v $
 *
 */

#include "../h/param.h"
#include "../h/time.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/dnlc.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"
#include "../machine/plocal.h"

/*
 * Directory name lookup cache.
 * Based on code originally done by Robert Els at Melbourne.
 *
 * Names found by directory scans are retained in a cache
 * for future referene.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (vp, name) where the vp refers to the
 * directory containing the name.
 *
 * For simplicity (and economy of storage), names longer than
 * some (small) maximum length are not cached, they occur
 * infrequently in any case, and are almost never of interest.
 */

extern	void	crhold();
extern	void	crfree();

#define NC_SIZE			128	/* size of name cache */
#define	NC_REL_SIZE		10	/* size of auto arrays for releasing */
#define	NC_HASH_SIZE		32	/* size of hash table */

#define	NC_BADLEN	(NC_NAMLEN+1)	/* impossible name-length */

#define	NC_HASH(namep, namlen, vp)	\
	((namep[0] + namep[namlen-1] + namlen + (int) vp) & (NC_HASH_SIZE-1))

/*
 * Macros to insert, remove cache entries from hash, LRU lists.
 */

#define	INS_HASH(ncp,nch)	insque(ncp, nch)
#define	RM_HASH(ncp)		remque(ncp)

#define	INS_LRU(ncp1,ncp2)	insque2((struct ncache *) ncp1, (struct ncache *) ncp2)
#define	RM_LRU(ncp)		remque2((struct ncache *) ncp)

#define	NULL_HASH(ncp)		(ncp)->hash_next = (ncp)->hash_prev = (ncp)

/*
 * Stats on usefulness of name cache.
 */

struct	ncstats {
	int	hits;		/* hits that we can really use */
	int	misses;		/* cache misses */
	int	long_enter;	/* long names tried to enter */
	int	long_look;	/* long names tried to look up */
	int	lru_empty;	/* LRU list empty */
	int	purges;		/* number of purges of cache */
	int	badname;	/* # times name purge made bad name */
};

/*
 * Hash list of name cache entries for fast lookup.
 */

struct	nc_hash	{
	struct	ncache	*hash_next, *hash_prev;
} nc_hash[NC_HASH_SIZE];

/*
 * LRU list of cache entries for aging.
 */

struct	nc_lru	{
	struct	ncache	*hash_next, *hash_prev;	/* hash chain, unused */
	struct 	ncache	*lru_next, *lru_prev;	/* LRU chain */
} nc_lru;

struct	ncstats ncstats;		/* cache effectiveness statistics */

struct ncache *dnlc_search();

#ifdef	DEBUG
int	doingcache = 1;			/* turn off directory caching */
#endif	DEBUG

/*
 * All operations on the name cache are mutexed by a single lock, dnlc_mutex.
 * This is straightforward, except that we cannot release any vnodes using
 * VN_RELE() while holding a lock, as we may block if we hold the last ref
 * to the vnode. Therefore we always save the address of vnodes to be released
 * in private stack variables, and do the VN_RELE()s after we release the lock.
 *
 * All procedures that manipulate the cache hold the directory vnode locked
 * during the manipulation, to insure consistent state.
 */

static	lock_t	dnlc_mutex;

struct	ncache	ncache[NC_SIZE];

/*
 * dnlc_init()
 *	Initialize the directory cache.
 *
 * Put all the entries on the LRU chain and clear out the hash links.
 */

dnlc_init()
{
	register struct ncache *ncp;
	register int i;

	init_lock(&dnlc_mutex, G_NFS);

	nc_lru.lru_next = (struct ncache *) &nc_lru;
	nc_lru.lru_prev = (struct ncache *) &nc_lru;
	for (i = 0; i < NC_SIZE; i++) {
		ncp = &ncache[i];
		INS_LRU(ncp, &nc_lru);
		NULL_HASH(ncp);
		ncp->dp = ncp->vp = NULLVP;
	}
	for (i = 0; i < NC_HASH_SIZE; i++) {
		ncp = (struct ncache *) &nc_hash[i];
		NULL_HASH(ncp);
	}
}

/*
 * dnlc_enter()
 *	Add a name to the directory cache.
 *
 * Caller holds dp node locked, holds a ref to vp.
 */

dnlc_enter(dp, name, vp, cred)
	register struct vnode *dp;
	register char *name;
	struct vnode *vp;
	struct ucred *cred;
{
	register unsigned namlen;
	register struct ncache *ncp;
	register int hash;
	spl_t	spl;
	struct vnode *olddp;
	struct vnode *oldvp;

#ifdef	DEBUG
	if (!doingcache)
		return;
#endif	DEBUG

	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		ncstats.long_enter++;
		return;
	}

	/*
	 * Lock cache and see if name exists there (ie, avoid duplicates).
	 */

	hash = NC_HASH(name, namlen, dp);

	spl = p_lock(&dnlc_mutex, SPLFS);

	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp != (struct ncache *) 0) {
		/* 
		 * Lost race -- someone already entered it.
		 */
		v_lock(&dnlc_mutex, spl);
		return;
	}

	/*
	 * Take least recently used cache struct that's either null or
	 * we can lock the "dp" node.
	 *
	 * Must lock the "dp" of the entry to insure don't race with
	 * a concurrent hit in the cache.  Keep trying until find
	 * entry we can take, or run out of list.
	 */

	ncp = nc_lru.lru_next;
	for (; ncp != (struct ncache *) &nc_lru; ncp = ncp->lru_next) {
		if (ncp->dp == NULLVP || VN_TRYLOCKNODE(ncp->dp)) {
			/*
			 * Either null entry or we locked the "dp" part
			 * to avoid races with searches.
			 *
			 * Remove from LRU, hash chains.
			 */
			RM_LRU(ncp);
			RM_HASH(ncp);

			/*
			 * Save vnode pointers to drop holds on (if any).
			 */
			olddp = ncp->dp;
			oldvp = ncp->vp;

			/*
			 * Hold new entry vnodes and fill in cache info.
			 */
			VN_HOLD(dp); ncp->dp = dp;
			VN_HOLD(vp); ncp->vp = vp;
			ncp->namlen = namlen;
			bcopy(name, ncp->name, namlen);
			if (ncp->cred != NOCRED)
				crfree(ncp->cred);
			ncp->cred = cred;
			if (cred != NOCRED)
				crhold(cred);

			/*
			 * Insert in LRU, hash chains.
			 */
			INS_LRU(ncp, nc_lru.lru_prev);
			INS_HASH(ncp, &nc_hash[hash]);
			v_lock(&dnlc_mutex, spl);

			/*
			 * Release old references, if any.
			 */
			if (olddp != NULLVP) {
				ASSERT_DEBUG(oldvp!= NULLVP,"dnlc_enter: NULL");
				VN_RELE(oldvp);
				VN_PUT(olddp);	
			}
			return;
		}
	}

	/*
	 * Couldn't get an avail LRU entry...
	 */

	ncstats.lru_empty++;
	v_lock(&dnlc_mutex, spl);
}

/*
 * dnlc_lookup()
 *	Look up a name in the directory name cache.
 *
 * Caller holds "dp" node locked.
 */

struct vnode *
dnlc_lookup(dp, name, cred)
	struct vnode *dp;
	register char *name;
	struct ucred *cred;
{
	register unsigned namlen;
	register int hash;
	register struct ncache *ncp;
	register struct ncache *prev;
	spl_t	spl;

#ifdef	DEBUG
	if (!doingcache)
		return (NULLVP);
#endif	DEBUG

	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		ncstats.long_look++;
		return (NULLVP);
	}

	hash = NC_HASH(name, namlen, dp);
	spl = p_lock(&dnlc_mutex, SPLFS);

	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp == (struct ncache *) 0) {
		v_lock(&dnlc_mutex, spl);
		ncstats.misses++;
		return (NULLVP);
	}

	/*
	 * Move this slot to the end of LRU chain.
	 */

	RM_LRU(ncp);
	INS_LRU(ncp, nc_lru.lru_prev);

	/*
	 * If not at the head of the hash chain,
	 * move forward so will be found earlier if looked up again.
	 */

	if (ncp->hash_prev != (struct ncache *) &nc_hash[hash]) {
		/* don't assume that remque() preserves links! */
		prev = ncp->hash_prev->hash_prev;
		RM_HASH(ncp);
		INS_HASH(ncp, prev);
	}

	v_lock(&dnlc_mutex, spl);

	ncstats.hits++;
	return (ncp->vp);
}

/*
 * dnlc_remove()
 *	Remove an entry in the directory name cache.
 *
 * Can have multiple entries of same "dp" and "name" if different
 * "cred"s are used.
 *
 * Caller holds "dp" locked.
 */

dnlc_remove(dp, name)
	struct	vnode	*dp;
	char	*name;
{
	register struct ncache *ncp;
	register int	relcnt;
	register int	i;
	register unsigned namlen;
	struct	vnode	*relvp[NC_REL_SIZE];
	int	hash;
	spl_t	spl;

	namlen = strlen(name);
	if (namlen > NC_NAMLEN)
		return;

	hash = NC_HASH(name, namlen, dp);

	/*
	 * Collect a bunch, release, and iterate.  Use small array
	 * on stack to collect vnodes to release, since can't release
	 * while hold cache locked.  Avoid use of NC_SIZE to size array
	 * since this can get arbitrarily big (and can't support boot-time
	 * sizing of array).
	 *
	 * Since all found entries list "dp" as ncp->dp, don't
	 * need to specifically remember these.
	 *
	 * No livelock possible, since "dp" is locked.
	 */

	do {
		relcnt = 0;
		spl = p_lock(&dnlc_mutex, SPLFS);

		while (ncp = dnlc_search(dp, name, namlen, hash, ANYCRED)) {
			ASSERT_DEBUG(ncp->vp != NULLVP, "dnlc_remove: NULLVP");
			ASSERT_DEBUG(ncp->dp == dp, "dnlc_remove: skew");
			relvp[relcnt] = ncp->vp;
			dnlc_rm(ncp);			/* zap entry */
			if (++relcnt >= NC_REL_SIZE)
				break;
		}

		v_lock(&dnlc_mutex, spl);

		/*
		 * Release vnodes for entries found in the cache.
		 */

		for (i = 0; i < relcnt; i++) {
			VN_RELE(dp);
			VN_RELE(relvp[i]);
		}

	} while (relcnt >= NC_REL_SIZE);
}

/*
 * dnlc_purge()
 *	Purge the entire cache.
 *
 * Is heuristic; makes best effort, no guarantees.  If an entry if found and
 * can't be sufficiently locked to remove from cache, the entry is made
 * unusable (ie, it can't "hit" again).
 *
 * Re-locks cache each time; this is more overhead, but no big deal
 * since dnlc_purge() happens rarely.
 *
 * Returns # entries released.
 */

dnlc_purge()
{
	register struct ncache	*ncp;
	register struct	vnode	*olddp;
	register struct	vnode	*oldvp;
	register int	nrel = 0;
	register spl_t	spl;

	ncstats.purges++;

	/*
	 * Look thru the entire cache and try to purge each entry.
	 * Must lock the "dp" node for each entry before delete it.
	 */

	for (ncp = ncache; ncp < &ncache[NC_SIZE]; ncp++) {
		if (ncp->dp != NULLVP) {
			/*
			 * Found a possible.  Lockup cache, see if
			 * still interesting and can lock it.  If so,
			 * zap entry and release references to held vnodes.
			 * If not, insure name won't match again.
			 *
			 * Note that ncp->dp != NULLVP <==> entry is currently
			 * valid in the cache.
			 */
			spl = p_lock(&dnlc_mutex, SPLFS);
			if (ncp->dp != NULLVP && VN_TRYLOCKNODE(ncp->dp)) {
				olddp = ncp->dp;
				oldvp = ncp->vp;
				dnlc_rm(ncp);			/* zap entry */
				v_lock(&dnlc_mutex, spl);
				VN_RELE(oldvp);
				VN_PUT(olddp);
				++nrel;
			} else {				/* lost race */
				/*
				 * Couldn't lock the node (or dp went NULL).
				 * Arrange that the name cant't hit in
				 * cache any more (and put at front of LRU
				 * list to get it reclaimed sooner).
				 * Could remove from hash chain, but zap name
				 * is sufficient.
				 */
				ncp->namlen = NC_BADLEN;
				RM_LRU(ncp);
				INS_LRU(ncp, &nc_lru);
				ncstats.badname++;
				v_lock(&dnlc_mutex, spl);
			}
		}
	}

	return(nrel);
}

/*
 * dnlc_purge_vp()
 *	Purge the cache of any entries with argument "vp" as directory or
 *	target name.
 *
 * Similar to dnlc_purge(), this is heuristic: no guarantees.
 *
 * Caller holds "vp" locked.
 */

dnlc_purge_vp(vp)
	register struct vnode *vp;
{
	register struct ncache	*ncp;
	register struct	vnode	*old;
	register spl_t	spl;

	/*
	 * Look thru the entire cache and try to purge each matching entry.
	 * If matches "dp" is easy -- already locked.
	 * If matches only "vp" have to try to lock "dp".
	 */

	for (ncp = ncache; ncp < &ncache[NC_SIZE]; ncp++) {
		if (ncp->dp == vp) {
			/*
			 * Easy match -- lock up cache, blast entry, release
			 * ref's to held vnodes.  Entry is already stable
			 * since hold dp (==vp) locked.
			 */
			old = ncp->vp;
			spl = p_lock(&dnlc_mutex, SPLFS);
			dnlc_rm(ncp);
			v_lock(&dnlc_mutex, spl);
			VN_RELE(old);
			VN_RELE(vp);
		} else if (ncp->vp == vp) {
			/*
			 * Found a possible.  Lockup cache, see if
			 * still interesting and can lock it.  If so,
			 * zap entry and release references to held vnodes.
			 * If not, arrange entry won't "hit".
			 *
			 * Note that ncp->vp != NULLVP <==> entry is currently
			 * valid in the cache.
			 */
			spl = p_lock(&dnlc_mutex, SPLFS);
			if (ncp->vp == vp && VN_TRYLOCKNODE(ncp->dp)) {
				old = ncp->dp;
				dnlc_rm(ncp);			/* zap entry */
				v_lock(&dnlc_mutex, spl);
				VN_RELE(vp);
				VN_PUT(old);
			} else {				/* lost race */
				/*
				 * Couldn't lock the node (or vp changed).
				 * Arrange that the name can't hit in
				 * cache any more (and put at front of LRU
				 * list to get it reclaimed sooner).
				 * Could remove from hash chain, but zap name
				 * is sufficient.
				 */
				if (ncp->vp == vp) {
					ncp->namlen = NC_BADLEN;
					RM_LRU(ncp);
					INS_LRU(ncp, &nc_lru);
					ncstats.badname++;
				}
				v_lock(&dnlc_mutex, spl);
			}
		}
	}
}

/*
 *  Dnlc_purge_vfsp()
 *	Purge the cache of names in the specified file system, and
 *	also any cache references to the name of the vnode upon
 *	which the file system (vfsp) is mounted.
 *
 *  This function is not heuristic.  On return, all entries referring
 *  to the specified file system WILL be removed from the cache.
 *
 *  Re-locks cache each time; this is more overhead, but no big deal
 *  since dnlc_purge_vfsp() happens rarely.
 *
 *	WARNING:
 *	  This function is only called by cumount().  Other usages
 *	  of this function can create deadlock situations, since this
 *	  function can block (VN_LOCKNODE) on the parent vnode of the
 *	  selected cache entries.
 */
void
dnlc_purge_vfsp (vfsp)
register struct vfs *vfsp;
{
	register struct ncache	*ncp;
	register struct	vnode	*olddp;
	register struct vnode	*coveredvp;
	register struct	vnode	*oldvp;
	register spl_t	spl;

	ncstats.purges++;
	coveredvp = vfsp->vfs_vnodecovered;

	/*
	 * Look thru the entire cache and purge each entry whose parent
	 * (and hence the child as well) resides in the specified file
	 * system.  Also purge any entry referring to the vnode upon which
	 * the specified file system is mounted.  We must lock the "dp"
	 * node for each entry before we delete it.
	 */

	for (ncp = ncache; ncp < &ncache[NC_SIZE]; ncp++) {
		olddp = ncp->dp;
		if (olddp != NULLVP) {			/* entry in use */
			if (olddp->v_vfsp == vfsp || olddp == coveredvp) {
				/*
				 * Found a possible.  Lockup cache, see
				 * if it's still interesting after we lock
				 * the cache.  If so, zap the entry and
				 * release references to held vnodes.
				 */
				spl = p_lock(&dnlc_mutex, SPLFS);
				olddp = ncp->dp;
				if (olddp != NULLVP &&
				    (olddp->v_vfsp == vfsp ||
				     olddp == coveredvp)) {	/* got one! */
					oldvp = ncp->vp;
					dnlc_rm(ncp);		/* zap entry */
					v_lock(&dnlc_mutex, spl);
					VN_LOCKNODE(olddp);
					VN_RELE(oldvp);
					VN_PUT(olddp);
				} else {			/* lost race */
					/*
					 * Either dp went null, or the entry
					 * was reused by someone else.  Either
					 * way, let go of the cache.
					 */
					v_lock(&dnlc_mutex, spl);
				}
			} else if (ncp->vp == coveredvp) {
				/*
				 * Found a possible.  Lockup cache, see
				 * if it's still interesting after we lock
				 * the cache.  If so, zap the entry and
				 * release references to held vnodes.
				 *
				 * Note that ncp->vp != NULLVP <==> entry is
				 * currently valid in the cache.
				 */
				spl = p_lock(&dnlc_mutex, SPLFS);
				if (ncp->vp == coveredvp) {
					olddp = ncp->dp;
					dnlc_rm(ncp);		/* zap entry */
					v_lock(&dnlc_mutex, spl);
					VN_LOCKNODE(olddp);
					VN_RELE(coveredvp);
					VN_PUT(olddp);
				} else {			/* lost race */
					/*
					 * Either vp went null, or the entry
					 * was reused by someone else.  Either
					 * way, let go of the cache.
					 */
					v_lock(&dnlc_mutex, spl);
				}
			}
		}
	}
}

/*
 * dnlc_rm()
 *	Obliterate a cache entry.
 *
 * Caller holds cache locked (dnlc_mutex).
 */

static
dnlc_rm(ncp)
	register struct ncache *ncp;
{
	ncp->dp = ncp->vp = NULLVP;
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	RM_HASH(ncp);
	if (ncp->cred != NOCRED) {
		crfree(ncp->cred);
		ncp->cred = NOCRED;
	}
	/*
	 * Insert at head of LRU list (first to grab).
	 */
	INS_LRU(ncp, &nc_lru);
	/*
	 * And make a dummy hash chain.
	 */
	NULL_HASH(ncp);
}

/*
 * dnlc_search()
 *	Utility routine to search for a cache entry.
 *
 * Caller holds cache locked (dnlc_mutex).
 */

static struct ncache *
dnlc_search(dp, name, namlen, hash, cred)
	register struct vnode *dp;
	char	*name;
	register unsigned namlen;
	int	hash;
	struct	ucred *cred;
{
	register struct ncache *ncp;
	register struct nc_hash *nhp;

	nhp = &nc_hash[hash];
	for (ncp = nhp->hash_next; ncp != (struct ncache *) nhp;
	    ncp = ncp->hash_next) {
		if (ncp->dp == dp && ncp->namlen == namlen &&
		    (cred == ANYCRED || ncp->cred == cred) &&
		    *ncp->name == *name &&		/* fast chk 1st char */
		    bcmp(ncp->name, name, namlen) == 0)
			return (ncp);
	}
	return ((struct ncache *) 0);
}

/*
 * insque2()
 *	Insert into queue, where the queue pointers are
 *	in the second two longwords.
 *
 * Should be in assembler like insque.
 */

static
insque2(ncp2, ncp1)
	register struct ncache *ncp2, *ncp1;
{
	register struct ncache *ncp3;

	ncp3 = ncp1->lru_next;
	ncp1->lru_next = ncp2;
	ncp2->lru_next = ncp3;
	ncp3->lru_prev = ncp2;
	ncp2->lru_prev = ncp1;
}

/*
 * remque2()
 *	Remove from queue, like insque2.
 */

static
remque2(ncp)
	register struct ncache *ncp;
{
	ncp->lru_prev->lru_next = ncp->lru_next;
	ncp->lru_next->lru_prev = ncp->lru_prev;
}
