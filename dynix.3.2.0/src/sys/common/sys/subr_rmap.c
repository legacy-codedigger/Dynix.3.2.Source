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
static	char	rcsid[] = "$Header: subr_rmap.c 2.4 90/06/03 $";
#endif

/*
 * subr_rmap.c
 *	Resource map handling routines.
 *
 * These routines do *not* handle their own mutex; all assume caller
 * handles this.
 */

/* $Log:	subr_rmap.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/mutex.h"
#include "../h/proc.h"
#include "../h/kernel.h"

/*
 * A resource map is an array of structures each
 * of which describes a segment of the address space of an available
 * resource.  The segments are described by their base address and
 * length, and sorted in address order.  Each resource map has a fixed
 * maximum number of segments allowed.  Resources are allocated
 * by taking part or all of one of the segments of the map.
 *
 * Returning of resources will require another segment if
 * the returned resources are not adjacent in the address
 * space to an existing segment.  If the return of a segment
 * would require a slot which is not available, then one of
 * the resource map segments is discarded after a warning is printed.
 * Returning of resources may also cause the map to collapse
 * by coalescing two existing segments and the returned space
 * into a single segment.  In this case the resource map is
 * made smaller by copying together to fill the resultant gap.
 *
 * N.B.: the current implementation uses a dense array and does
 * not admit the value ``0'' as a legal address, since that is used
 * as a failure return from rmalloc().
 */

/*
 * Initialize map mp to have (mapsize-2) segments
 * and to be called ``name'', which we print if
 * the slots become so fragmented that we lose space.
 * The map itself is initialized with size elements free
 * starting at addr.
 */
rminit(mp, size, addr, name, mapsize)
	register struct map *mp;
	long size, addr;
	char *name;
	int mapsize;
{
	register struct mapent *ep = (struct mapent *)(mp+1);

	mp->m_name = name;
/* N.B.: WE ASSUME HERE THAT sizeof (struct map) == sizeof (struct mapent) */
	/*
	 * One of the mapsize slots is taken by the map structure.
	 * Another of the
	 * segments has size 0 and addr 0, and acts as a delimiter.
	 * We insure that we never use segments past the end of
	 * the array which is given by mp->m_limit.
	 * Instead, when excess segments occur we discard some resources.
	 */
	mp->m_limit = (struct mapent *)&mp[mapsize];
	/*
	 * Simulate a rmfree(), but with the option to
	 * call with size 0 and addr 0 when we just want
	 * to initialize without freeing.
	 */
	ep->m_size = size;
	ep->m_addr = addr;
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 *
 * This routine knows about the interleaving of the swapmap
 * and handles that.
 *
 * Assumes caller locked the map structure.
 */
long
rmalloc(mp, size)
	register struct map *mp;
	register long size;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register int addr;
	register struct mapent *bp;
	swblk_t first, rest;

	ASSERT(size > 0 && (mp != swapmap || size <= dmmax_sw), "rmalloc");
	/*
	 *+ Invalid arguments were passed to the resource map
	 *+ allocation routine rmalloc().
	 */

	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	for (bp = ep; bp->m_size; bp++) {
		if (bp->m_size >= size) {
			/*
			 * If allocating from swapmap, then have to
			 * respect interleaving boundaries.
			 *
			 * This is a bit conservative; if m_size - first < size,
			 * but first <= size then can take the 1st part of the
			 * chunk.  This is extra code, and doesn't really buy
			 * much, since rarely run out of swap-space due to
			 * something like this.
			 */
			if (mp == swapmap && nswdev > 1 &&
			    (first = dmmax_sw - bp->m_addr % dmmax_sw)
								< bp->m_size) {
				if (bp->m_size - first < size)
					continue;
				addr = bp->m_addr + first;
				rest = bp->m_size - first - size;
				bp->m_size = first;
				if (rest)
					rmfree(swapmap, rest, addr+size);
				return (addr);
			}
			/*
			 * Allocate from the map.
			 * If there is no space left of the piece
			 * we allocated from, move the rest of
			 * the pieces to the left.
			 */
			addr = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while ((bp-1)->m_size = bp->m_size);
			}
			ASSERT(mp != swapmap || addr % CLSIZE == 0, "rmalloc swapmap");
			/*
			 *+ An attempt was made to allocate a swap region that
			 *+ is not a multiple of the system page size.
			 */
			return (addr);
		}
	}
	return (0);
}

/*
 * Free the previously allocated space at addr
 * of size units into the specified map.
 * Sort addr into map and combine on
 * one or both ends if possible.
 *
 * Assumes caller locked the map structure.
 *
 * Note: this procedure no longer special cases kernelmap and deals with
 * kmapwnt.  This is done at a higher level (see uptalloc(), uptfree()).
 */
rmfree(mp, size, addr)
	struct map *mp;
	register long size;
	register long addr;
{
	struct mapent *firstbp;
	register struct mapent *bp;
	register int t;

	/*
	 * Both address and size must be
	 * positive, or the protocol has broken down.
	 */

	ASSERT(addr > 0 && size > 0, "rmfree 1");
	/*
	 *+  An attempt was made to free an invalid resource map area.
	 */

	/*
	 * Locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */

	firstbp = bp = (struct mapent *)(mp + 1);
	for (; bp->m_addr <= addr && bp->m_size != 0; bp++)
		continue;

	/*
	 * If the piece on the left abuts us,
	 * then we should combine with it.
	 */

	if (bp > firstbp && (bp-1)->m_addr+(bp-1)->m_size >= addr) {

		/*
		 * Check no overlap (internal error).
		 */

		ASSERT((bp-1)->m_addr+(bp-1)->m_size <= addr, "rmfree 2");
		/*
		 *+ The kernel has freed a resource map region that overlaps
		 *+ with a map region that is already free.
		 */

		/*
		 * Add into piece on the left by increasing its size.
		 */

		(bp-1)->m_size += size;

		/*
		 * If the combined piece abuts the piece on
		 * the right now, compress it in also,
		 * by shifting the remaining pieces of the map over.
		 */

		if (bp->m_addr && addr+size >= bp->m_addr) {
			ASSERT(addr+size <= bp->m_addr, "rmfree 3");
			/*
			 *+ An overlap has occurred between a map region
			 *+ already in the resource map
			 *+ structure and a map region that is already
			 *+ free.
			 */
			(bp-1)->m_size += bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
		}
		return;
	}

	/*
	 * Don't abut on the left, check for abutting on
	 * the right.
	 */

	if (addr+size >= bp->m_addr && bp->m_size) {
		ASSERT(addr+size <= bp->m_addr, "rmfree 4");
		/*
		 *+ There is an invalid overlap between a map region on the
		 *+ map list and the map region being freed.
		 */
		bp->m_addr -= size;
		bp->m_size += size;
		return;
	}

	/*
	 * Don't abut at all.  Make a new entry
	 * and check for map overflow.
	 */

	do {
		t = bp->m_addr;
		bp->m_addr = addr;
		addr = t;
		t = bp->m_size;
		bp->m_size = size;
		bp++;
	} while (size = t);

	/*
	 * Segment at bp is to be the delimiter;
	 * If there is not room for it 
	 * then the table is too full
	 * and we must discard something.
	 */

	if (bp+1 > mp->m_limit) {
		/*
		 * Back bp up to last available segment.
		 * which contains a segment already and must
		 * be made into the delimiter.
		 * Discard second to last entry,
		 * since it is presumably smaller than the last
		 * and move the last entry back one.
		 */
		bp--;
		printf("%s: rmap ovflo, lost [%d,%d)\n", mp->m_name,
		    (bp-1)->m_addr, (bp-1)->m_addr+(bp-1)->m_size);
		/*
		 *+ A resource map structure overflowed.  This means
		 *+ that during a resource free operation, more
		 *+ resource fragments were created than could be
		 *+ contained in the structure allocated to keep track
		 *+ of those fragments.  Therefore, a fragment has been
		 *+ discarded.  That portion of the
		 *+ resource cannot be used again until the
		 *+ system is rebooted.  It is suggested that the
		 *+ system be reconfigured with a larger map
		 *+ structure for this resource.
		 */
		bp[-1] = bp[0];
		bp[0].m_size = bp[0].m_addr = 0;
	}
}
