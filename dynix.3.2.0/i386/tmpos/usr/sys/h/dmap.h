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
 * $Header: dmap.h 2.6 88/03/18 $
 *
 * dmap.h
 *	Definitions for the mapping of vitual swap
 *	space to the physical swap area - the disk map.
 */

/* $Log:	dmap.h,v $
 */

/*
 * The dmap structure represents "reasonable" size processes locally
 * in struct user.  Larger processes allocate an extion to the dmap
 * structure which is swapped with the process.  This keeps struct dmap
 * small for most processes, and allows very large processes to be
 * represented.
 */

#define	DMMIN		32		/* default min sectors in dmap chunk */
#define	DMMAX		512		/* default max sectors in dmap chunk */
#define	DMMAX_SW	2048		/* max # sectors in swap space chunk */

#define	NDMAP	9			/* sizeof "direct" swap map */

struct	dmap	{
	swblk_t			dm_map[NDMAP];	/* 1st block # in each chunk */
	union	{
		swblk_t		du_daddr;	/* disk addr when swapped out */
		struct	dmapext	*du_ext;	/* memory addr of extension */
	}			dm_un;		/* 0 if no extension alloc'd */
};

#define	dm_daddr	dm_un.du_daddr
#define	dm_ext		dm_un.du_ext

/*
 * dmap "extension" structure.  Allocated if the dmap structure can't hold
 * the swap space representation itself.  Proper size of the extension
 * structure is determined at boot time to fit an entire address space.
 *
 * The #ifdef avoids the need to move includes of "vm.h" before "user.h" all
 * over the place.
 */

#ifdef	MAXADDR
struct	dmapext	{
	u_long		dme_nent;		/* # valid entries */
	swblk_t		dme_daddr;		/* swap block(s) to hold this */
	swblk_t		dme_map[1];		/* 1st block # in each chunk */
};
#endif

/*
 * The following structure is that ``returned'' from a call to vstodb().
 */

struct	dblock	{
	swblk_t	db_base;	/* base of physical contig drum block */
	swblk_t	db_size;	/* size of block */
};

#ifdef	KERNEL
extern	struct	dmap	zdmap;
extern	int		dmmin, dmmax, dmmax_sw, maxdmap;
#endif	KERNEL
