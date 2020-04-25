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
static	char	rcsid[] = "$Header: vm_pageout.c 2.9 1991/05/23 22:10:15 $";
#endif

/*
 * vm_pageout.c
 *	Page-out daemon.
 *
 * TODO:
 *	Don't bother with pages of process that's exiting or swapping.
 *	Retries on pageout IO errors?  Panic seems excessive.
 */

/* $Log: vm_pageout.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/conf.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

extern	struct	buf *swbuf_alloc();

sema_t	drain_dirty;			/* wait here for work */

lock_t	swpq_mutex;			/* swapout-queue manipulation mutex */
short	swpq_head;			/* swapout-queue head index */
short	swpq_tail;			/* swapout-queue tail index */

extern	int	pageout_maxb;		/* max # asynch IO buffers */

/*
 * pageout()
 *	Pageout daemon.
 *
 * This runs as process [2].  It waits to be told there is work to do
 * (eg, dirty-memory list reached high-water mark), then drains the list
 * until it reaches low-water mark.
 *
 * Also do all swapout writing to swap area in this process.  The idea is
 * to have *one* process managing all such writes; this guarantees we get
 * FIFO writes to swap-area pages (eg, no problems with races between
 * realloc swap space and concurrent IO).  Don't expect performance lags
 * (mostly wait on disk), and could start multiple concurrent IO's if
 * desired.
 *
 * The loop works as follows:
 *
 *	for (;;) {
 *		sleep on drain_dirty sema;
 *		while somebody wants to be swapped out: do it.
 *		while (dirtymem > dirtylow) {
 *			pull appropriate pages off head of dirty list
 *				(much policy tuning possible here);
 *			start output on these;
 *			wait for IO done's;
 *			foreach page we started IO on, if it hasn't been
 *				reclaimed, move it to tail of free-list
 *				(now cleaned);
 *		}
 *	}
 *
 * Much more detail available below...
 */

pageout()
{
	register struct proc *rp;
	register struct cmap *c;
	register struct pte *pte;
	register struct	buf *swb;	/* current buffer */
	register struct	buf *lb;	/* last buffer in chain */
	struct	buf	*fb;		/* 1st buffer in IO chain */
	struct	proc	*p;
	struct	dblock	db;
	struct	pgout	pgi;
	unsigned pg;
	bool_t	moreIO;
	int	numbuf;
	int	klsize;
	int	kldirty;
	int	delta;
	int	rev;
	int	error;
	int	result;
	struct	vnode *vp;
	GATESPL(s);

	p = u.u_procp;
	fb = lb = NULL;

	for (;;) {
		/*
		 * Wait for dirty-list to get too big or someone to
		 * swap out.  If there's already a proc to swap out,
		 * don't bother waiting.
		 */

		if (swpq_head == 0)
			p_sema(&drain_dirty, PSWP);

		/*
		 * If someone wants out, oblige them.
		 * Don't do other pageouts unless necessary.
		 *
		 * Note: no race with swapout() queuing proc and miss
		 * this when continue above; if swpq_head == 0 above
		 * then swapout() cannot have v_sema'd drain_dirty yet.
		 */

		if (swpq_head != 0) {
			pgout_swap();
			if (dirtymem <= vmtune.vt_dirtyhigh) {
				/*
				 * Must "lockup" the drain_dirty sema to avoid
				 * race with v_sema().
				 */
				lockup_sema(&drain_dirty, s);
				sema_count(&drain_dirty) = 0;
				unlock_sema(&drain_dirty, s);
				continue;
			}
		}

		/*
		 * Now drain dirty-list (from the front) until we drain
		 * below `dirtylow' or otherwise decide to stop.
		 */

		moreIO = 1;

		while (moreIO) {

			/*
			 * Get a buffer header for the IO.
			 * If can, try for a new one (asynch IO's).
			 * Else reuse old one.
			 *
			 * Could statically allocate pageout_maxb headers,
			 * but isn't easily run-time tuneable.
			 */

			if (fb == NULL) {
				swb = fb = swbuf_alloc(1);
				numbuf = 1;
			} else if (numbuf<pageout_maxb && (swb=swbuf_alloc(0))){
				lb->b_forw = swb;
				++numbuf;
			} else {
				swb = fb;
				if (fb = swb->b_forw)
					lb->b_forw = swb;
				else
					fb = swb;
				biowait(swb);
				pgout_rlse(swb);
			}

			/*
			 * Lock up memory-list and see if enough dirty.
			 * If not, give back buffer (overshot); careful to
			 * not free swap-header twice if have had mapped-file
			 * IO errors (<==> fb==swb || lb==swb).
			 */
		errloop:
			LOCK_MEM;

			if (dirtymem <= vmtune.vt_dirtylow) {
				UNLOCK_MEM;
				if (swb == fb) {
					fb = NULL;
					swbuf_free(swb);
				} else if (lb != swb) {
					lb->b_forw = NULL;
					swbuf_free(swb);
				}
				break;
			}
			swb->b_forw = NULL;
			lb = swb;

			/*
			 * Grab head of dirty list and handle based on type.
			 */

			c = cm_dirty.c_next;
			ASSERT_DEBUG(c->c_gone == 0, "pageout: c_gone");
			ASSERT_DEBUG(c->c_refcnt == 0, "pageout: c_refcnt");
			ASSERT_DEBUG(c->c_pageout == 0, "pageout: c_pageout");
			ASSERT_DEBUG(c->c_iowait == 0, "pageout: c_iowait");
			ASSERT_DEBUG(c->c_dirty == 1, "pageout: c_dirty");

			/*
			 * Figure type of page, and locate owner.
			 * Owner must exist, since page was on dirty list
			 * and we hold memory locked.
			 */

			switch(c->c_type) {
			
			case CDATA:
			case CSTACK:
				CM_UNLINK_DIRTY(c);
				c->c_pageout = 1;	/* we have it now! */
				c->c_dirty = 0;		/* will write it */

				rp = &proc[c->c_ndx];
				while (rp->p_flag & SNOVM)
					rp = rp->p_vflink;
				if (c->c_type == CDATA) {
					rev = 0;
					pte = dptopte(rp, c->c_page);
					result = vstodb((int)ctod(c->c_page), 
						ctod(1),
						&(rp->p_uarea->u_dmap), 
						&db, 0);
				} else {
					rev = 1;
					pte = sptopte(rp, c->c_page);
					result = vstodb((int)ctod(c->c_page), 
						ctod(1),
						&(rp->p_uarea->u_smap), 
						&db, 1);
				}

				/*
				 * Special case code for ZFOD pages
				 * which can get created in pagein
				 * if MMAP fails. These do not have
				 * swap locations, and are therefore
				 * simply put back on the free list
				 * and reset the ptes to ZFOD. 
				 */
				if (result == -1) {
					c->c_gone = 1;
					c->c_pageout = 0;
					c->c_refcnt = 0;
					ptefill(pte, PG_ZFOD, CLSIZE);
					CM_INS_HEAD_CLEAN(c);
					swkill(rp, "BAD PAGEOUT DMAP");
					swb->b_bufsize = 0;
					UNLOCK_MEM;
					goto errloop;
				}

				/*
				 * Try setting up kluster out...
				 */

				klsize = 1;
				kldirty = (dirtymem-vmtune.vt_dirtylow)/CLSIZE;
				if (kldirty>=KLMAX && vmtune.vt_klout_look>0) {
					delta = pgout_klust(c,
						MIN(kldirty, vmtune.vt_klout_look), rev, &klsize);
					db.db_base -= ctod(delta);
					pte -= delta;
				}
				vp = swapdev_vp;
				break;

			case CMMAP:
				/*
				 * Call mapper to get a set of dirty pages.
				 * Mapper must munlink(c), turn off c_dirty,
				 * turn on c_pageout.
				 *
				 * If mapper says there's an error, then
				 * it hasn't done anything -- implicitly
				 * "clean" the page and continue.
				 */
				rp = u.u_procp;
				error = (*c->c_mapf->map_pgout)
					    (c->c_ndx, (u_long)c->c_page, &pgi);
				if (error) {
					/*
					 * "Clean" page without IO.
					 */
					CM_UNLINK_DIRTY(c);
					c->c_dirty = 0;
					CM_INS_TAIL_CLEAN(c);
					UNLOCK_MEM;
					/*
					 * Buffer might not be re-used before
					 * done.  Mark it to be ignored.
					 */
					swb->b_bufsize = 0;
					goto errloop;
				}
				pte = pgi.po_pte;
				klsize = pgi.po_cnt / CLSIZE;
				db.db_base = pgi.po_blkno;
				vp = pgi.po_devvp;
				break;

			default:
				panic("pageout: type");
				/*
				 *+ The page replacement daemon tried to write
				 *+ out a modified page that had an invalid
				 *+ page type.
				 */
				/*NOTREACHED*/
			}

			if (dirtymem < vmtune.vt_dirtylow + KLMAX*CLSIZE
			||  swpq_head != 0) {
				moreIO = 0;		/* stop after this */
				/*
				 * Must "lockup" the drain_dirty sema to avoid
				 * race with v_sema().
				 */
				lockup_sema(&drain_dirty, s);
				sema_count(&drain_dirty) = 0;
				unlock_sema(&drain_dirty, s);
			}

			/*
			 * Must map pages into self.  This necessary to
			 * avoid races with exiting process deleting its
			 * page-table while we're waiting to map the pages
			 * on a driver's queue!  We map to a KLMAX*CLSIZE
			 * chunk of pte's into fixed (one) place in proc[2]
			 * data-space.  If we start multiple IOs, we map to
			 * place identified by `swb' index.
			 *
			 * Must do this before drop memory lock to mutex
			 * concurrent exit/exec from deleting page-tables
			 * (they must vmemfree() the reclaimables first...).
			 */

			pg = (swb - swbuf) * KLMAX * CLSIZE;
			bcopy(	(caddr_t)pte,
				(caddr_t)dptopte(p, pg),
				(unsigned)(klsize * CLSIZE * sizeof(struct pte))
			);

			UNLOCK_MEM;

			/*
			 * Have ref to pages (c_pageout) and know where they
			 * go.  Now start IO.
			 */

			swb->b_bufsize = klsize * CLSIZE;
			swapio(swb, rp, db.db_base, dptopte(p, pg),
				klsize * CLBYTES, B_WRITE|B_DIRTY, vp);

			/*
			 * Keep stats on # pageout operations and # pages
			 * written.
			 *
			 * No check for error in buffer since not much we
			 * can do, and is only process/file data anyhow.
			 * Error will have been printed on console.
			 */

			l.cnt.v_pgout++;
			l.cnt.v_pgpgout += klsize*CLSIZE;
		}

		/*
		 * Clean up outstanding IO.
		 * Careful about tail of list: may not have any associated
		 * data, if got mapper errors above.
		 */

		while (swb = fb) {
			fb = fb->b_forw;
			if (swb->b_bufsize) {
				biowait(swb);
				pgout_rlse(swb);
			}
			swbuf_free(swb);
		}
	}
}

/*
 * pgout_klust()
 *	Try to set up a multi-page pageout of contiguous swap-space pages.
 *
 * pgout_klust() locates pages adjacent to the argument pages that are
 * immediately available to include in the pageout, by looking "ahead" in
 * the dirty-list for pages having the same "owner" and "type" that fall
 * within a KLMAX envelope around the argument page (which has already
 * been unlinked from the dirty-list).  Done this way instead of looking
 * at source pte's to avoid (maybe non-existant) races on use of that
 * page-table, and since this is simpler (less code) and maybe more
 * amenable to other types of things (eg, shared-mem).  [NOTE: probably
 * should look at source pte's (rbk, 6/26/85)].
 *
 * Returns delta from argument `v' to start of virt area being written,
 * and kluster size in *pkl.
 *
 * Assumes caller holds memory locked to insure dirty-list is stable.
 */

#ifdef	PERFSTAT
int	klocnt[KLMAX+1];
#endif	PERFSTAT

static
pgout_klust(c, lookahead, rev, pkl)
	register struct cmap	*c;		/* page that must be included */
	int		rev;			/* reverse alloc; eg stack */
	int		lookahead;
	int		*pkl;
{
	register int	min_cl;
	register int	max_cl;
	register int	cl;
	register int	min_idx;
	struct	cmap	*klust[KLMAX];
	int	count;
	int	cl_idx;
	int	i;
	int	max_idx;
	unsigned type;
	unsigned ndx;

#ifdef	PERFSTAT
	klocnt[0]++;				/* # calls to pgout_klust */
#endif	PERFSTAT

	/*
	 * Zap array, and place argument at appropriate point.
	 */
  
	bzero((caddr_t)klust, sizeof(klust));
	cl = c->c_page / CLSIZE;
	cl_idx = cl & (KLMAX-1);
	/* klust[cl_idx] = c; */			/* not necessary */
	min_cl = cl & ~(KLMAX-1);
	max_cl = min_cl + KLMAX;

	count = 1;
	type = c->c_type;
	ndx = c->c_ndx;

	/*
	 * Look ahead as much as allowed (or until fill klust array)
	 * and find pages of same owner, type, and within range.
	 */

	c = cm_dirty.c_next;
	for (i = 0; i < lookahead && count < KLMAX; i++, c = c->c_next) {
		ASSERT_DEBUG(c->c_gone == 0, "pgout_klust: c_gone");
		ASSERT_DEBUG(c->c_refcnt == 0, "pgout_klust: c_refcnt");
		ASSERT_DEBUG(c->c_pageout == 0, "pgout_klust: c_pageout");
		ASSERT_DEBUG(c->c_dirty == 1, "pgout_klust: c_dirty");
		cl = c->c_page / CLSIZE;
		if (cl >= min_cl				/* within... */
		&&  cl <  max_cl				/* ...range */
		&&  c->c_type == type				/* same type */
		&&  c->c_ndx == ndx) {				/* same owner */
			ASSERT_DEBUG(klust[cl & (KLMAX-1)] == 0,
						"pgout_klust: klust != 0");
			klust[cl & (KLMAX-1)] = c;
			++count;
		}
	}

	/*
	 * See if we have any contiguity; look forward, back from
	 * starting cmap entry and see about neighbors.
	 * Unlink each neighbor from dirty-list.
	 */

	for (min_idx = cl_idx-1; min_idx >= 0; min_idx--) {
		if (c = klust[min_idx]) {
			CM_UNLINK_DIRTY(c);
			c->c_pageout = 1;
			c->c_dirty = 0;
		} else
			break;
	}
	++min_idx;

	for (max_idx = cl_idx+1; max_idx < KLMAX; max_idx++) {
		if (c = klust[max_idx]) {
			CM_UNLINK_DIRTY(c);
			c->c_pageout = 1;
			c->c_dirty = 0;
		} else
			break;
	}
	--max_idx;

	/*
	 * Return kluster size, and delta (in HW pages) from argument.
	 */

	*pkl = max_idx - min_idx + 1;
#ifdef	PERFSTAT
	klocnt[*pkl]++;					/* # at this size */
#endif	PERFSTAT
	return(rev ? (max_idx - cl_idx)*CLSIZE : (cl_idx - min_idx)*CLSIZE);
}

/*
 * pgout_rlse()
 *	Run thru the page(s) we just wrote and free them if possible.
 *
 * Note that page could have been reclaimed and re-modified (multiple
 * times) all before IO completes.
 *
 * Mapped file sync-ing can race with pageout, setting c_iowait if so --
 * don't free it, give it to the sync'r.
 *
 * Caller does NOT hold memory locked.
 */

static
pgout_rlse(swb)
	struct buf *swb;
{
	register struct pte *pte = swb->b_un.b_pte;
	register int pgcnt = swb->b_bufsize;
	register struct cmap *c;
	register int freed = 0;

	/*
	 * If got an IO error, kill process for private pages, tell
	 * mapper there was an error.
	 */

	if (swb->b_flags & B_ERROR) {
		c = PTETOCMAP(*pte);
		switch (c->c_type) {
		case CDATA:
		case CSTACK:
			swkill(swb->b_proc, "pageout IO error");
			break;
		case CMMAP:
			(*c->c_mapf->map_err)
				(c->c_ndx, (u_long)c->c_page, pgcnt, MM_IOERR);
			break;
		default:
			panic("pgout_rlse: c_type");
			/*
			 *+ The page replacement daemon tried to free a page
			 *+ that had just been written out to swap.  However,
			 *+ the daemon has an invalid page type.
			 */
			/*NOTREACHED*/
		}
	}

	/*
	 * Release pageout's reference to the pages.
	 */

	LOCK_MEM;

	for (; pgcnt > 0; pte += CLSIZE, pgcnt -= CLSIZE) {
		c = PTETOCMAP(*pte);
		ASSERT_DEBUG(c->c_pageout, "pgout_rlse");
		c->c_pageout = 0;
		if (c->c_iowait) {
			ASSERT_DEBUG(c->c_refcnt == 0 && c->c_type == CMMAP,
							"pgout_rlse: iowait");
			intrans_wake(c);
		} else if (c->c_refcnt == 0) {
			if (c->c_gone) {
				c->c_dirty = 0;
				CM_INS_HEAD_CLEAN(c);
				freed += CLSIZE;
			} else if (c->c_dirty) {
				CM_INS_TAIL_DIRTY(c);
			} else {
				CM_INS_TAIL_CLEAN(c);
				freed += CLSIZE;
			}
		}
	}

	UNLOCK_MEM;

	l.cnt.v_dfree += freed;
	if (freed && blocked_sema(&mem_wait))
		vall_sema(&mem_wait);
}

/*
 * intrans_wake()
 *	Wake up set of processes waiting on a given cmap[] entry
 *	to be brought into memory.
 *
 * Assumes caller holds memory locked to preserve state of cmap[] entry.
 */

intrans_wake(c)
	register struct cmap *c;
{
	register struct proc *p;
	register struct proc *pnext;

	ASSERT_DEBUG(c->c_iondx != 0, "intrans_wake: no proc");

	for (p = &proc[c->c_iondx]; p != &proc[0]; p = pnext) {
		pnext = p->p_spwait;
		v_sema(&p->p_pagewait);
	}

	c->c_iowait = 0;
	c->c_iondx = 0;
}

/*
 * pgout_swap()
 *	Drain swapout-queue.
 *
 * Do this in preference to draining dirty-list since process wants out
 * anyhow and have same effect (free memory space).
 */

static
pgout_swap()
{
	register struct proc *p;
	spl_t	s;

	/*
	 * Loop until nobody queued.
	 * Don't need lock to test -- once process is on queue, it
	 * doesn't get off until we take it off.
	 */

	while (swpq_head) {

		/*
		 * Dequeue process.
		 */

		s = p_lock(&swpq_mutex, SPLHI);

		ASSERT(swpq_head != 0, "pgout_swap: queue");
		/*
		 *+ The replacement daemon was asked to swap out a process
		 *+ and found the swap queue empty.  This daemon should be
		 *+ the only one removing processes from the swap queue.
		 */

		p = &proc[swpq_head];
		swpq_head = p->p_swpq;
		if (swpq_head == 0)
			swpq_tail = 0;

		v_lock(&swpq_mutex, s);

		/*
		 * Must sync with process by locking it's state, in
		 * case it's a self-swapout -- process holds self
		 * state-lock until it has switched away.  This is
		 * mostly paranoia, since probability of actual race
		 * that causes problem is extremely remote.
		 */

		s = p_lock(&p->p_state, SPLHI);
		/* proc must have switched away now */
		v_lock(&p->p_state, s);

		swapout_proc(p);
	}
}
