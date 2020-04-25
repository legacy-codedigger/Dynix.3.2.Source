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
static	char	rcsid[] = "$Header: uipc_mbuf.c 2.31 1991/09/11 01:04:22 $";
#endif

/*
 * upic_mbuf.c
 *	IPC mbuf management routines - use ../h/mbuf.h macros
 */

/* $Log: uipc_mbuf.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/mbuf.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/cmn_err.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

#define	indextom(x)	((struct mbuf *)((int)mbutl + ((x) << CLSHIFT)))

struct	mbstat	mbstat;
struct	mbuf	*mfree, *mclfree;

struct	map	*mbmap;

/*
 * These values maintain various statistics and sanity checks.
 */

struct	mbuf	*mbhirange; 
struct	mbuf	*mblorange;
struct	mbuf	*mclhirange;
struct	mbuf	*mcllorange;

short	mbxwaits;		/* how many calls to m_expandorwait */
bool_t	m_expandoff;		/* m_expandorwait on/off switch */
short	mwait;			/* number of times waited for memory */
 
extern	lock_t	mem_alloc;	/* memory allocation mutex */

/*
 * LOCK_MBUF() locks all mbuf allocation/deallocation.
 * UNLOCK_MBUF() unlocks.
 *
 * Defined this way to optimize for particular implementation.
 */

static	sema_t	mbuf_wait;
static	sema_t	clust_wait;

#ifdef	ns32000
#define	LOCK_MBUF(s)	P_GATE(G_MBUF, s)
#define	UNLOCK_MBUF(s)	V_GATE(G_MBUF, s)
#define	MBUF_WAIT(s) {	\
	mbstat.m_waits++; \
	V_GATE(G_MBUF, (s)); \
	p_sema(&mbuf_wait, PZERO+1); \
	P_GATE(G_MBUF, (s)); \
	}
#define	CLUST_WAIT(s) {	\
	mbstat.m_clwaits++; \
	V_GATE(G_MBUF, s); \
	p_sema(&clust_wait, PZERO+1); \
	P_GATE(G_MBUF, s); \
	}
#endif	/*ns32000*/

#ifdef	i386
/*
 * i386 uses a lock to mask interrupts at SLIC and allow nested use of gates.
 */
static	lock_t	mbuf_mutex = L_UNLOCKED;

#define	LOCK_MBUF(s)	s = p_lock(&mbuf_mutex, SPLHI)
#define	UNLOCK_MBUF(s)	v_lock(&mbuf_mutex, s)
#define	MBUF_WAIT(s) {	\
	mbstat.m_waits++; \
	p_sema_v_lock(&mbuf_wait, PZERO+1, &mbuf_mutex, (s)); \
	(s) = p_lock(&mbuf_mutex, SPLHI); \
	}
#define	CLUST_WAIT(s) {	\
	mbstat.m_clwaits++; \
	p_sema_v_lock(&clust_wait, PZERO+1, &mbuf_mutex, (s)); \
	(s) = p_lock(&mbuf_mutex, SPLHI); \
	}
#endif	/*i386*/

/*
 * mclinit()
 *	Allocate mbuf clusters.
 *
 * Called from sysinit() ==> gating not required.
 *
 * Allocated here as each cluster requires contiguous physical
 * memory that may cross a page boundary.
 */

mclinit()
{
	register struct mbuf *m;
	register int n = mbinitcl;

	/*
	 * allocate mbinitcl worth of clusters for mbuf clusters
	 * NOTE: only mbinitcl clusters are *ever* allocated - generally
	 * if all of the clusters are in use, mbufs are used instead.
	 */

	if (n == 0) {		/* allow for NO clusters */
		CPRINTF("NO mclusters configured\n");
		return;
	}

	init_sema(&mbuf_wait,0,0,G_MBUF);
	init_sema(&clust_wait,0,0,G_MBUF);

	callocrnd(512);		/* start on reasonable boundary */

	while (--n >= 0) {

		/* 
		 * initialize an mbuf cluster and link into mclfree
		 * Guarantee that cluster doesn't cross a 64 KB boundary
		 * since they are passed to SCED and its DMA can not cross
		 * such a boundary.
		 */

		if (((u_long)calloc(0) & 0xffff) + MCLBYTES  > 64 * 1024)
			callocrnd((int)64*1024);

		m = (struct mbuf *) calloc(MCLBYTES);

		if (mcllorange == (struct mbuf *)NULL)
			mcllorange = m;

		/*
		 * mcl_refcnt zeroed by calloc
		 */

		m->m_next = mclfree; 
		mclfree = m; 
		mbstat.m_clfree++; 
		mbstat.m_clusters++;
	}
	mclhirange = mclfree;

#ifdef IPCDEBUG
	printf("mbuf cluster range: 0x%x to 0x%x\n",
			((int)mclfree)-(mbinitcl-1)*1024, mclfree);
#endif /*IPCDEBUG*/
}


/*
 * mbinit()
 *	Initialize mbuffers - called from init_main().
 */

mbinit() 
{ 
	/*
	 * Allocate mbinitbufs (binary configurable) worth of pages for
	 * 128-byte mbufs - i.e. start out with this and m_expand as needed
	 * m_clalloc(npg) links into mfree.
	 */

	if (!mbinitbufs)
		CPRINTF("NOTE - no initial mbufs - will expand\n");
	else if (m_clalloc(mbinitbufs) == 0) {
		panic("mbinitbufs");
		/*
		 *+ Not enough space exists to allocate mbufs.
		 *+ Check the system configuration.
		 */
	}
	mblorange = (struct mbuf *) ((int) mfree - mbinitbufs*CLBYTES + MSIZE);

#ifdef IPCDEBUG
#define	KVIRTTOPHYS(addr) \
		(PTETOPHYS(Sysmap[btop(addr)]) + ((int)(addr) & (NBPG-1)))
	printf("mbuf range: 0x%x to 0x%x\n",
			((int)mfree)-mbinitbufs*CLBYTES, mfree);
	printf("mbuf phys range: 0x%x to 0x%x\n",
			KVIRTTOPHYS(((int)mfree)-(mbinitbufs-1)*1024),
			KVIRTTOPHYS(mfree));
#endif /*IPCDEBUG*/
} 

/*
 * m_clalloc()
 *	Allocate "clicks" and bust up into mbufs.
 *
 * Returns true if successful, else false.
 */

static bool_t
m_clalloc(ncl) 
	register int ncl; 
{ 
	register struct mbuf *m; 
	register int i; 
	int	npg; 
	int	mbx; 
	spl_t	splmem;

	/*
	 * The request for memory can fail in the following ways:
	 * 1) not enough Mbmap pte's left,
	 * 2) memory free-list is already locked (c'est le guru),
	 * 3) not enough system memory available right now.
	 *
	 * ncl is number of clusters requested.
	 */

	if (m_expandoff)
		return (0);

	if ((npg = (ncl * CLSIZE)) > freemem)
		return (0);			/* 0 => fail */

	/*
	 * rmalloc returns index into map for pte's
	 */

	mbx = rmalloc(mbmap, (long)npg); 

	if (mbx == 0) {				/* not enough pte's available */
		m_expandoff = 1;
		return (0);			/* 0 => allocation error */
	}

	m = indextom(mbx / CLSIZE);		/* figures mem address of pte */

	if (TRY_LOCK_MEM(splmem)) {	 	/* got it locked */
		if (freemem < npg) {	 	/* Not enough free memory */
			UNLOCK_MEM;
			rmfree(mbmap, (long)npg, (long)mbx); 
			return (0);	 	/* 0 => failure to allocate */
		}
		lmemall(&Mbmap[mbx], npg, proc, CSYS);
		UNLOCK_MEM;
		vmaccess(&Mbmap[mbx], (caddr_t)m, npg); 
	} else {				 /* couldn't lock mem lists */
		rmfree(mbmap, (long)npg, (long)mbx); 
		return (0);		 	/* 0 => failure to allocate */
	}

	/*
	 * Requested memory allocated; bust up into mbufs and link into mfree.
	 *
	 * Assumes sizeof(*m) divides CLBYTES evenly.
	 */

	for (i = ncl * CLBYTES / sizeof(*m); i > 0; i--) { 
		m->m_next = mfree;
		mfree = m;
		m->m_type = MT_FREE;
		m->m_off = 0; 
		m->m_act = 0;
		mbstat.m_mbufs++; 
		mbstat.m_mbfree++;
		m++; 
	} 
	mbhirange = mfree;

	return (1);			/* success */
}
 
/*
 * m_expand()
 *	Called whenever we run out of mbufs. 
 *
 * Note that once expanded, the buffers are not returned to the memory
 * allocator; up to nmbclusters of mbuf clusters can be allocated and/or
 * made into mbufs.
 *
 * Returns true if succeed, else false.
 */

static
m_expand() 
{ 
	/* 
	 * m_clalloc can fail because we failed to lock the memory
	 * allocation lists when another processor is allocating memory
	 * for something other than mbufs.
	 *
	 * Just for grins, try it twice, but give up if we can't get some
	 * memory allocated this time because this processor is spinning.
	 */

	if (m_clalloc(1))
		return (1);
	return (m_clalloc(1));
} 
 
/*
 * m_expandorwait()
 *	Expand or wait for more memory to become available.
 *
 * This routine unconditionally locks the memall allocation lists, and
 * waits for memory to be available if necessary.  (m_clalloc() can't
 * unconditionally lock memory lists in case running in interrupt that
 * interrupted code that held memory lists locked).
 *
 * Returns true if allocated some more mbufs, else false.  Also returns
 * true if there is expansion room in mbmap[] but no system memory
 * available "right now" -- waits for some free memory first.
 *
 * NOTE: success does not guarantee memory is available to caller, only
 * that memory is available just before the return.  Another processor
 * can still scarf it up.  This routine should not be called if other
 * mutex structures are held (e.g. locks) since doing so leads to deadlock.
 *
 * NOTE: to allow mem_alloc to be a lock_t, must properly nest p's & v's
 * of mem_alloc and mbuf lists to avoid getting wrong SPL when done.
 * This also works if mem_alloc is a sema_t.  This increases latency to
 * mem_alloc at rare times.
 *
 * NOTE: it may be the case that it is impossible to interrupt code
 * that holds mem_alloc and also call m_clalloc().  If so, m_clalloc()
 * can unconditionally lock memory lists (probably just call memall()).
 * This needs proof.
 */

bool_t
m_expandorwait() 
{ 
	register struct mbuf *m; 
	register int i; 
	int	mbx; 
	spl_t	ms;

	/*
	 * If limit already reached, fail from now on.
	 * This means that after saturation, m_expandorwait is disabled,
	 * and ENOBUFS error is returned.
	 * Syscalls are restarted cleanly till saturation.
	 */

	if (m_expandoff)
		return (0);

	LOCK_MEM;
	LOCK_MBUF(ms);

	mbxwaits++;		/* number of times m_expandorwait called */

	/*
	 * Allocate pte's to map 1 cluster (CLSIZE pages).
	 * rmalloc() returns index into map for pte's
	 */

	mbx = rmalloc(mbmap, (long)CLSIZE);

	if (mbx == 0) {			/* mbx==0 => not enuf pte's available */
		UNLOCK_MBUF(ms);
		UNLOCK_MEM;
		m_expandoff = 1;
		return (0);
	}

	m = indextom(mbx / CLSIZE);	/* macro figures mem address of pte */

	if (freemem < CLSIZE) {
		/*
		 * Unusual case -- no system memory is free at this
		 * moment, although there is room to map it (in mbmap[]);
		 * wait for some available memory and have caller try again.
		 */
		rmfree(mbmap, (long)CLSIZE, (long)mbx); 
		UNLOCK_MBUF(ms);
		UNLOCK_MEM;
		WAIT_MEM;
		mwait++;
		return (1);
	}

	/*
	 * There is memory.  Allocate it, make it accessible, and
	 * bust it up into mbuf's.
	 */

	lmemall(&Mbmap[mbx], CLSIZE, proc, CSYS);	/* allocate mem */
	vmaccess(&Mbmap[mbx], (caddr_t)m, CLSIZE); 	/* make it accessible */

	for (i = CLBYTES / sizeof(*m); i > 0; i--) { 
		m->m_next = mfree;
		mfree = m;
		m->m_type = MT_FREE;
		m->m_off = 0; 
		m->m_act = 0;
		mbstat.m_mbufs++; 
		mbstat.m_mbfree++;
		m++; 
	} 
	mbhirange = mfree;

	UNLOCK_MBUF(ms);
	UNLOCK_MEM; 

	return (1);
} 

/*
 * m_freem()
 *	Free a *chain* of mbufs.
 *
 * Note: this routine is modified from 4.2 to not expand the MFREE macro
 * within the loop.  This avoids multiple gate round trips when releasing
 * the chain, but means that all buffers are released within one
 * round-trip on the mbuf allocation lock; the time spent holding the
 * mbuf lock is dependent upon how many buffers are released.
 */

m_freem(m)
	register struct mbuf *m;
{
	register struct mbuf *n;
	spl_t	ms;

	if (m == (struct mbuf *)NULL)
		return;

	LOCK_MBUF(ms);

	do {
		ASSERT(m->m_type != MT_FREE, "mfree: MT_FREE");
		/*
		 *+ An attempt has been made to free a free mbuf.
		 */
		mbstat.m_mtypes[m->m_type]--;
		m->m_type = MT_FREE; 

		if (M_HASCL(m)) {		/* is this a cluster? */

			switch (m->m_cltype) {

			case MCLT_KHEAP:	/* call something first */
				/*
				 * Only current use is in NFS.  Most uses
				 * are "v_sema", which doesn't require a
				 * drop of mbuf mutex.  Other cases can cause
				 * long latency and/or badly nest SPL's,
				 * so drop mbuf mutex and re-acquire when done.
				 */
				if (m->m_clfun == (int (*)()) v_sema) {
					v_sema((sema_t *)(m->m_clarg));
				} else {
					UNLOCK_MBUF(ms);
					(*m->m_clfun)(m->m_clarg);
					LOCK_MBUF(ms);
				}
				break;

			case MCLT_MCLUST:	/* standard cluster */
				n = (struct mbuf *)m->m_clstart;
	
				ASSERT(n >= mcllorange && n <= mclhirange,
					"mclfreed out of bounds");
				/*
				 *+ An attempt has been made to free an mbuf
				 *+ but it dosn't appear to be a valid mbuf.
				 */
				ASSERT(((struct mclust *)n)->mcl_refcnt != 0,
					"m_freem: zero mcl_refcnt");
				/*
				 *+ An attempt has been made to deallocate
				 *+ an mbuf but it is still being referenced.
				 */
	
				if (--(((struct mclust *)n)->mcl_refcnt) == 0) {
					n->m_next = mclfree;
					mclfree = n;
				 	mbstat.m_clfree++;
					if (blocked_sema(&clust_wait))
						v_sema(&clust_wait);
				} 
				break;

			default:
				panic("m_freem defaulted!");
				/*
				 *+ The mbuf cluster type is not of the type
				 *+ MCLT_KHEAP nor MCLT_MCLUST the mbuf is
				 *+ likely corrupted.
				 */
				break;
			}
		} 

		n = m->m_next;
		m->m_next = mfree; 
		m->m_off = 0;
		m->m_act = 0;
		mfree = m;
		mbstat.m_mbfree++; 

	} while (m = n);	/* note assignment in condition */

	if (blocked_sema(&mbuf_wait))
		v_sema(&mbuf_wait);
	if (blocked_sema(&clust_wait))
		v_sema(&clust_wait);

	UNLOCK_MBUF(ms);
}

/*
 * Mbuffer utility routines.
 */

#ifdef	NFS
/*
 * mclgetx()
 *	Allocate a "funny" mbuf -- one whose data is owned by someone else.
 */
struct mbuf *
mclgetx(fun, arg, addr, len, canwait)
	int	(*fun)();
	int	arg;
	int	len;
	bool_t	canwait;
	caddr_t	addr;
{
	register struct mbuf *m;
	
	MGET(m, canwait, MT_DATA);
	if (m == (struct mbuf *)NULL) {
		if (canwait == M_DONTWAIT)
			return (m);
		do {
			if (!m_expandorwait()) {
				/*
				 * Can't expand mbuf pool. Give up.
				 */
				return (m);
			}
			MGET(m, canwait, MT_DATA);
		} while (m == (struct mbuf *)NULL);
	}

	m->m_off = (int)addr - (int)m;
	m->m_len = len;
	m->m_cltype = MCLT_KHEAP;
	m->m_clfun = fun;
	m->m_clarg = arg;
	m->m_clswp = NULL;
	return (m);
}

/*
 * mbuffree()
 *	Called at interrupt level to free an MCLT_KHEAP mbuf back into the heap.
 */
static 
mbuffree(arg)
	int arg;
{
	extern int kmem_free_intr();

	kmem_free_intr((caddr_t)arg, *(u_int *)arg);
}
#endif	/*NFS*/

/*
 * m_copy()
 *	Allocate and copy len bytes of an mbuf chain to a new mbuf chain.
 */

struct mbuf *
m_copy(m, off, len)
	register struct mbuf *m;
	int	off;
	register int len;
{
	register struct mbuf *n;
	register struct mbuf **np;
	struct	mbuf	*top;
	spl_t	ms;

	if (len == 0)
		return ((struct mbuf *)NULL);

	ASSERT_DEBUG(off >= 0 && len >= 0, "m_copy: negative off||len");

	while (off > 0) {	 /* skip off bytes in chain */
		ASSERT(m != (struct mbuf *)NULL, "m_copy: off > mbuf");
		/*
		 *+ In copying an mbuf, the chain has been broken.
		 */
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}

	np = &top;
	top = (struct mbuf *)NULL;

	while (len > 0) {

		/*
		 * If past end of chain, done.  Arg length should have
		 * been M_COPYALL in this case.
		 */

		if (m == (struct mbuf *)NULL) {
			ASSERT(len == M_COPYALL, "m_copy: off");
			/*
			 *+ Whilst copying an mbuf, the end of the chain
			 *+ has been reached before all of the data has
			 *+ been copied.
			 */
			break;
		}

		/*
		 * NOTE: 4.2bsd gets type always from 1st mbuf;
		 * here we assign type from mbuf being copied.
		 */

		LOCK_MBUF(ms);

		/*
		 * Do `MGET(n, M_WAIT, m->m_type)' holding MBUF's locked.
		 */

		if (!mfree && !m_expand() && !m_expand())
			mbstat.m_drops++;

		n = mfree;
		if (n == NULL) {
			UNLOCK_MBUF(ms);
			goto nospace;
		}
		mfree = n->m_next;
		n->m_next = (struct mbuf *)NULL;
		n->m_off = MMINOFF;
		n->m_type = m->m_type;
		mbstat.m_mtypes[m->m_type]++;
		mbstat.m_mbfree--;
		*np = n;
		n->m_len = MIN(len, m->m_len - off);

		if (m->m_off > MMAXOFF) {	 	/* dup a cluster */
			switch (m->m_cltype) {
			case MCLT_MCLUST:
				n->m_off = mtod(m, int) - (int)n;
				n->m_cltype = MCLT_MCLUST;
				n->m_clstart = m->m_clstart;
				++(((struct mclust *)m->m_clstart)->mcl_refcnt);
				UNLOCK_MBUF(ms);
				break;
#ifdef	NFS
			case MCLT_KHEAP: {
				register caddr_t copybuf;
				/*
				 * Must drop mbuf mutex since kmem_alloc()
				 * uses SPLIMP and can block on memory.
				 */
				UNLOCK_MBUF(ms);
				copybuf = (caddr_t) kmem_alloc(
						(u_int)(n->m_len+sizeof(int)));
				*(int *) copybuf = n->m_len + sizeof(int);
				bcopy(	mtod(m, caddr_t) + off,
					copybuf + sizeof(int),
					(unsigned) n->m_len);
				n->m_off = (int)copybuf + sizeof(int)
							- (int)n - off;
				n->m_cltype = MCLT_KHEAP;
				n->m_clfun = mbuffree;
				n->m_clarg = (int)copybuf;
				n->m_clswp = NULL;
				break;
			}
#endif	/*NFS*/
			default:
				panic("m_copy: cluster type");
				/*
				 *+ The mbuf cluster type is not of the type
				 *+ MCLT_KHEAP nor MCLT_MCLUST the mbuf is
				 *+ likely corrupted.
				 */
				/*NOTREACHED*/
			}
			n->m_off += off;
		} else {			 	/* copy mbuf */
			UNLOCK_MBUF(ms);
			bcopy(mtod(m, caddr_t)+off, mtod(n, caddr_t),
							(unsigned)n->m_len);
		}
		if (len != M_COPYALL)
			len -= n->m_len;
		off = 0;
		m = m->m_next;
		np = &n->m_next;
	}
	return (top);
nospace:
	m_freem(top);
	return ((struct mbuf *)NULL);
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
m_copydata(m, off, len, cp)
	register struct mbuf *m;
	register int off;
	register int len;
	caddr_t cp;
{
	register unsigned count;

	ASSERT_DEBUG(off >= 0 && len >= 0, "m_copydata: negative off||len");

	while (off > 0) {
		ASSERT_DEBUG(m != 0, "m_copydata: m1 = 0");
		if (off < m->m_len)
			break;
		off -= m->m_len;
		m = m->m_next;
	}
	while (len > 0) {
		ASSERT_DEBUG(m != 0, "m_copydata: m2 == 0");
		count = MIN(m->m_len - off, len);
		bcopy(mtod(m, caddr_t) + off, cp, count);
		len -= count;
		cp += count;
		off = 0;
		m = m->m_next;
	}
}

/*
 * m_cat()
 *	Concatenate two mbuf chains.
 */

m_cat(m, n)
	register struct mbuf *m;
	register struct mbuf *n;
{
	while (m->m_next)
		m = m->m_next;
	while (n) {
		if (m->m_off >= MMAXOFF ||
		    m->m_off + m->m_len + n->m_len > MMAXOFF) {
			m->m_next = n;	 /* just join the two chains */
			return;
		}

		/*
		 * Splat the data from one into the other.
		 */

		bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
		    (u_int)n->m_len);
		m->m_len += n->m_len;
		n = m_free(n);
	}
}

/*
 * m_adj()
 *	Remove length from the head or tail of an mbuf chain.
 */

m_adj(mp, len)
	struct mbuf *mp;
	register int len;
{
	register struct mbuf *m;
	register struct mbuf *n;

	if ((m = mp) == (struct mbuf *)NULL)
		return;

	if (len >= 0) {
		/*
		 * Remove length from the head of the mbuf chain.
		 */
		while (m != (struct mbuf *)NULL && len > 0) {
			if (m->m_len <= len) {
				len -= m->m_len;
				m->m_len = 0;
				m = m->m_next;
			} else {
				m->m_len -= len;
				m->m_off += len;
				break;
			}
		}
	} else {
		/*
		 * Remove length from the tail of the mbuf chain.
		 * A 2 pass algorithm might be better (sic 4.2).
		 */
		len = -len;
		while (len > 0 && m->m_len != 0) {
			while (m != (struct mbuf *)NULL && m->m_len != 0) {
				n = m;
				m = m->m_next;
			}
			if (n->m_len <= len) {
				len -= n->m_len;
				n->m_len = 0;
				m = mp;
			} else {
				n->m_len -= len;
				break;
			}
		}
	}
}

/*
 * m_pullup()
 *	Rearange an mbuf chain so that `len' bytes are contiguous
 *	and in the data area of an mbuf (so that mtod and dtom
 *	will work for a structure of size len).
 *
 * Returns the resulting mbuf chain on success, frees it and returns null
 * on failure.  If there is room, it add up to MPULL_EXTRA bytes to
 * the contiguous region in an attempt to avoid being called next time.
 */

struct mbuf *
m_pullup(n, len)
	register struct mbuf *n;
	int	len;
{
	register struct mbuf *m;
	register int count;
	int	space;

	if ((n->m_off + len) <= MMAXOFF && n->m_next) {
		m = n;
		n = n->m_next;
		len -= m->m_len;
	} else {
		if (len > MLEN)
			goto bad;
		MGET(m, M_DONTWAIT, (int)n->m_type);
		if (m == (struct mbuf*)NULL)
			goto bad;
		m->m_len = 0;
	}
	space = MMAXOFF - m->m_off;
	do {
		count = MIN(MIN(space - m->m_len, len + MPULL_EXTRA), n->m_len);
		bcopy(mtod(n,caddr_t), mtod(m,caddr_t)+m->m_len, (u_int)count);
		len -= count;
		m->m_len += count;
		n->m_len -= count;
		if (n->m_len)
			n->m_off += count;
		else
			n = m_free(n);
	} while (len > 0 && n);
	if (len > 0) {
		(void) m_free(m);
		goto bad;
	}
	m->m_next = n;
	return (m);
bad:
	/*
	 * Note - if failed to get buffers *all* buffers are released.
	 */
	m_freem(n);
	return ((struct mbuf*)NULL);
}

/*
 * subroutine replacements for MGET, MFREE and MCLGET macros 
 *
 * m_getm gets multiple mbufs - the MGET macro expands to an
 * m_getm(i,t,1) (i.e. getm one mbuf)
 */

/*ARGSUSED*/
struct mbuf *
m_getm(canwait, t, n)			/* get `n' mbufs of type `t' */
	bool_t	canwait;
	int	t;
	register int n;
{
	register struct mbuf *m;
	register struct mbuf *msave;
	spl_t	ms;

	ASSERT_DEBUG(n >= 0, "m_getm: mbuf request < 0");

	/*
	 * It is possible for m_expand() to fail because the kernel is
	 * allocating memory somewhere else (cannot be for mbufs since gate is
	 * held).  Therefore, retry a few times and eventually give up.  Note
	 * that the mbuf lists can be locked for up to four tries to malloc.
	 */

	LOCK_MBUF(ms);

	/*
	 * If not enough left, try twice to allocate more.
	 */

	if ((mbstat.m_mbfree < n) && !m_expand() && !m_expand()) {
		if (canwait == M_DONTWAIT)
			mbstat.m_drops++;
		else 
			while (mbstat.m_mbfree < n) {
				MBUF_WAIT(ms);
		}
	}

	if (mbstat.m_mbfree < n) {
		ASSERT_DEBUG(canwait == M_DONTWAIT,
		    "m_get: returning null to waiter");
		m = (struct mbuf *)NULL;
	} else {
		m = mfree;
		mbstat.m_mbfree -= n;
		mbstat.m_mtypes[t] += n; 
		for (; n > 0; n--) {
			msave = mfree;
			ASSERT(msave != NULL, "m_getm: null mfree");
			/*
			 *+ A broken mbuf chain has been detected.
			 */
			msave->m_type = t;
			msave->m_off = MMINOFF;
			mfree = mfree->m_next;
		}
		msave->m_next = (struct mbuf *)NULL;
	}

	if (mbstat.m_mbfree != 0)
		if (blocked_sema(&mbuf_wait))
			v_sema(&mbuf_wait);

	UNLOCK_MBUF(ms);
	return (m);
}

struct mbuf *
m_getclrm(canwait, t, n)	/* get `n' mbufs of type `t' and clear'em */
	bool_t	canwait;
	int	t;
	int	n;
{
	register struct mbuf *mclr;
	struct mbuf *m;

	m = m_getm(canwait, t, n);
	for (mclr = m; mclr; mclr = mclr->m_next) {
		bzero(mtod(mclr, caddr_t), MLEN);
	}
	return (m);
}

struct mbuf *
mfreeit(m)
	struct mbuf * m;
{
	register struct mbuf * n;

	if (m == (struct mbuf *)NULL)
		return ((struct mbuf *)NULL);
	n = m->m_next;
	m->m_next = (struct mbuf *)NULL;
	m_freem(m);
	return (n);			 /* return next mbuf */
}

/*
 * m_getcl()
 *	Allocate and return an mbuf which points to an mbuf cluster.
 *
 * It does this in one call/one gate round trip and thus save a gate round trip.
 */

/*ARGSUSED*/
struct mbuf *
m_getcl(canwait, type)
	bool_t	canwait;
	int	type;
{
	register struct mbuf *m;
	spl_t	ms;

	LOCK_MBUF(ms);

	if (mfree == (struct mbuf *)NULL) {
		if (!m_expand() && !m_expand()){	/* tries twice */
			if (canwait == M_DONTWAIT) {
				mbstat.m_drops++;
				UNLOCK_MBUF(ms);
				return ((struct mbuf *)NULL);
			} else 
				while (mbstat.m_mbfree < 1)
					MBUF_WAIT(ms);
		}
	}

	if (canwait == M_WAIT) {
		m = mfree;
		mfree = m->m_next;
		mbstat.m_mtypes[type]++;
		mbstat.m_mbfree--;
	}

	if (mclfree == (struct mbuf *)NULL) {
		if (canwait == M_DONTWAIT) {
			mbstat.m_cldrops++;
			UNLOCK_MBUF(ms);
			return ((struct mbuf *)NULL);
		} else 
			while (mclfree == (struct mbuf *)NULL)
				CLUST_WAIT(ms);
	}

	if (canwait == M_DONTWAIT) {
		m = mfree;
		mfree = m->m_next;
		mbstat.m_mtypes[type]++;
		mbstat.m_mbfree--;
	}
	m->m_next = (struct mbuf *)NULL;
	m->m_type = type;
	m->m_off = (int)mclfree - (int)m;
	m->m_clstart = (caddr_t)mclfree;
	m->m_cltype = MCLT_MCLUST;
	++(((struct mclust *)mclfree)->mcl_refcnt);
	mbstat.m_clfree--;
	mclfree = mclfree->m_next;

	UNLOCK_MBUF(ms);
	return (m);
}

