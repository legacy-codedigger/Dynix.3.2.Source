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
static	char	rcsid[] = "$Header: mmap_ifchr.c 2.1 90/06/03 $";
#endif

/*
 * mmap_ifchr.c
 *	Support for mapping character special files.
 *
 * Mostly this is just an interface that called drivers at their "d_mmap"
 * entry, using MM_xxx function codes to tell the driver what's wanted.
 *
 * Also contains a "null" mapops for representing unsupported vnode types.
 *
 * Could code a struct mapops pointer in d_mmap and be a bit more straight
 * forward, but the older MM_xxx interface is alredy in place (history).
 */

/* $Log:	mmap_ifchr.c,v $
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/conf.h"

/*
 * Define mapping operations for character special files.
 */

static	int	mmchr_new();
static	int	mmchr_dup();
static	int	mmchr_unmap();
static	int	mmchr_swpout();
static	int	mmchr_swpin();
static	int	mmchr_refpg();
static	int	mmchr_derefpg();
static	int	mmchr_realloc();
static	int	mmchr_pgout();
static	int	mmchr_stat();
static	int	mmchr_err();

struct	mapops	mmap_chr = {
	mmchr_new,		/* create a new map */
	mmchr_dup,		/* dup ref to map (fork) */
	mmchr_unmap,		/* release reference to map */
	mmchr_swpout,		/* swap out ref to map */
	mmchr_swpin,		/* swap in ref to map */
	mmchr_refpg,		/* get ref to page */
	mmchr_derefpg,		/* remove page ref */
	mmchr_realloc,		/* drop reclaim link to page */
	mmchr_pgout,		/* page-out page */
	mmchr_stat,		/* get information about the map */
	mmchr_err,		/* import an error to the map */
};

/* 
 * mmchr_new()
 *	Initial set up of character special file mapping.
 *
 * Called in mmap() to obtain handle on internal structures and
 * determine if legal mapping.
 *
 * Caller holds vp locked.
 */

static
mmchr_new(vp, pos, size, prot, handlep)
	register struct	vnode *vp;	/* locked node to map */
	u_long	pos;			/* file position (HW pages) */
	int	size;			/* size to map (HW pages) */
	int	prot;			/* PROT_BITS */
	u_long	*handlep;		/* return if succeed */
{
	*handlep = vp->v_rdev;
	return((*cdevsw[major(vp->v_rdev)].d_mmap)(vp->v_rdev, MM_MAP, pos, size, prot));
}

/*
 * mmchr_dup()
 *	Dup ref to a map during fork().
 *
 * Driver's MM_MAP function is used for both "new" and "dup".
 * Ignore return value for "dup" case.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmchr_dup(handle, off, size, prot)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
	int	size;			/* HW pages */
	int	prot;			/* PROT_READ|PROT_WRITE */
{
	(*cdevsw[major(handle)].d_mmap)(handle, MM_MAP, off, size, prot);
}

/*
 * mmchr_unmap()
 *	Release reference to a map.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmchr_unmap(handle, off, size, prot)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
	int	size;			/* HW pages */
	int	prot;			/* PROT_READ|PROT_WRITE */
{
	(*cdevsw[major(handle)].d_mmap)(handle, MM_UNMAP, off, size, prot);
}

/*
 * mmchr_swpout()
 *	Swap out a refernece to a map.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmchr_swpout(handle, off, size)
	u_long		handle;			/* dev_t of relevant driver */
	register u_long	off;			/* HW pages */
	register int	size;			/* HW pages */
{
	(*cdevsw[major(handle)].d_mmap)(handle, MM_SWPOUT, off, size);
}

/*
 * mmchr_swpin()
 *	Try to swap in a reference to a map.
 *
 * Returns 0 for success else error number.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmchr_swpin(handle, off, size)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
	int	size;			/* HW pages */
{
	return((*cdevsw[major(handle)].d_mmap)(handle, MM_SWPIN, off, size));
}

/*
 * mmchr_refpg()
 *	Return the physical address of a page.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmchr_refpg(handle, off)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
{
	return((*cdevsw[major(handle)].d_mmap)(handle, MM_REFPG, off));
}

/*
 * mmchr_derefpg()
 *	Remove a reference to a page.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmchr_derefpg(handle, off, mod, lastfd)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
	bool_t	mod;			/* was page modified? */
	bool_t	lastfd;			/* is this last fd ref? */
{
	(*cdevsw[major(handle)].d_mmap)(handle, MM_DEREFPG, off, mod, lastfd);
}

/*
 * mmchr_realloc()
 *	Release claim on reclaimable page -- it's getting reallocated.
 *
 * Called holding memory locked.
 */

static
mmchr_realloc(handle, off)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
{
	(*cdevsw[major(handle)].d_mmap)(handle, MM_REALLOC, off);
}

/*
 * mmchr_pgout()
 *	Pageout process found a mapped page and wants it written out.
 *	Find adjacent pages if possible and tell pageout about them.
 *
 * Must munlink(c), turn off c_dirty, turn on c_pageout.
 *
 * Called by pageout process while holding memory locked.
 */

static
mmchr_pgout(handle, off, pgo)
	u_long	handle;			/* dev_t of relevant driver */
	u_long	off;			/* HW pages */
	struct	pgout	*pgo;
{
	(*cdevsw[major(handle)].d_mmap)(handle, MM_REALLOC, off, pgo);
}

/*
 * mmchr_stat()
 *	Return information about a map.
 *
 * For now, return constants (there are no implemented paging character
 * special files, and this is heuristic anyhow).
 */

static
mmchr_stat(handle, statp)
	u_long	handle;
	struct	map_stat *statp;
{
	statp->ms_count = statp->ms_ccount = 1;
#ifdef	lint
	lint_ref_int((int)handle);
#endif	lint
}

/*
 * mmchr_err()
 *	Assert an error on a page of a map.
 *
 * NOP for now.
 */

static
mmchr_err()
{
	/* NOP */
}

/*
 * Define a "null" mapping operations structure.
 */

static
mmnull_new()
{
	return(EINVAL);
}

static
mmnull_badop()
{
	panic("mmnull_badop");
	/*
	 *+ An unsupported memory map function has been called.
	 */
}

struct	mapops	mmap_null = {
	mmnull_new,		/* create a new map */
	mmnull_badop,		/* dup ref to map (fork) */
	mmnull_badop,		/* release reference to map */
	mmnull_badop,		/* swap out ref to map */
	mmnull_badop,		/* swap in ref to map */
	mmnull_badop,		/* get ref to page */
	mmnull_badop,		/* remove page ref */
	mmnull_badop,		/* drop reclaim link to page */
	mmnull_badop,		/* page-out page */
	mmnull_badop,		/* get information about the map */
	mmnull_badop,		/* import an error to the map */
};
