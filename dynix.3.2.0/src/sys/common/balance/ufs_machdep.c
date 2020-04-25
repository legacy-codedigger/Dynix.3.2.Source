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
static	char	rcsid[] = "$Header: ufs_machdep.c 2.2 87/09/30 $";
#endif

/*
 * ufs_machdep.c
 *	Machine dependent handling of the buffer cache.
 */

/* $Log:	ufs_machdep.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"

/*
 * allocbuf()
 *	Expand or contract the actual memory allocated to a buffer.
 *
 * If no memory is available, release buffer and take error exit
 * (this doesn't happen in this implementation; we block waiting for
 * more memory).
 *
 * This is expensive in terms of additional lock round-trips (on buf_lists).
 * Maybe consider effect of calling with buf_lists locked (trade latency with
 * overhead).
 */

#ifdef	DEBUG
int	albshrink = 0;
int	albgrow = 0;
#endif	DEBUG

allocbuf(tp, size)
	register struct buf *tp;
	int size;
{
	register struct buf *bp;
	register struct buf *ep;
	int	sizealloc;
	int	take;
	spl_t	s;

	sizealloc = roundup(size, CLBYTES);
	/*
	 * Buffer size does not change
	 */
	if (sizealloc == tp->b_bufsize)
		goto out;
	/*
	 * Buffer size is shrinking.
	 * Place excess space in a buffer header taken from the
	 * BQ_EMPTY buffer list and placed on the "most free" list.
	 * If no extra buffer headers are available, leave the
	 * extra space in the present buffer.
	 */
	if (sizealloc < tp->b_bufsize) {
		s = p_lock(&buf_lists, SPLBUF);
		ep = bfreelist[BQ_EMPTY].av_forw;
		if (ep == &bfreelist[BQ_EMPTY]) {
			v_lock(&buf_lists, s);
			goto out;
		}
		notavail(ep);
		v_lock(&buf_lists, s);
		pagemove(tp->b_un.b_addr + sizealloc, ep->b_un.b_addr,
		    (int)tp->b_bufsize - sizealloc);
		ep->b_bufsize = tp->b_bufsize - sizealloc;
		tp->b_bufsize = sizealloc;
		ep->b_flags = B_INVAL;
		ep->b_bcount = 0;
		brelse(ep);
#ifdef	DEBUG
		++albshrink;			/* ignore lock: simple stat */
#endif	DEBUG
		goto out;
	}
	/*
	 * More buffer space is needed. Get it out of buffers on
	 * the "most free" list, placing the empty headers on the
	 * BQ_EMPTY buffer header list.
	 *
	 * Note that this can deadlock (since we hold one buf (tp), and
	 * insist on getting others (bp's)) if we somehow wind up with
	 * some set of buffers holding all available bufpages and waiting
	 * for more.  In practice this shouldn't happen, and detection
	 * of the deadlock is expensive so we ignore it here (this needs
	 * to be measured under stress conditions, however).  [see Impl.Notes
	 * for ways to deal with this better].
	 */
	while (tp->b_bufsize < sizealloc) {
		take = sizealloc - tp->b_bufsize;
		bp = getnewbuf(0);			/* don't bremhash(bp) */
		if (take >= bp->b_bufsize)
			take = bp->b_bufsize;
		pagemove(&bp->b_un.b_addr[bp->b_bufsize - take],
		    &tp->b_un.b_addr[tp->b_bufsize], take);
		tp->b_bufsize += take;
		bp->b_bufsize -= take;
		if (bp->b_bcount > bp->b_bufsize)
			bp->b_bcount = bp->b_bufsize;
		if (bp->b_bufsize <= 0)
			bp->b_flags |= B_INVAL;
		brelse(bp);
#ifdef	DEBUG
		++albgrow;
#endif	DEBUG
	}
out:
	tp->b_bcount = size;
	return (1);
}
