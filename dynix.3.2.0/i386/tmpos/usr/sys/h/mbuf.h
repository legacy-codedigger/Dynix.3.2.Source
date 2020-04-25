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
#ifndef _SYS_MBUF_INCLUDED
#define _SYS_MBUF_INCLUDED

/*
 * $Header: mbuf.h 2.12 1991/09/11 01:03:29 $
 *
 * mbuf.h
 *	Memory-buffer definitions.  Mostly used for network code.
 */

/* $Log: mbuf.h,v $
 *
 *
 */

/*
 * Constants related to memory allocator.
 */

#define	MSIZE		128			/* size of an mbuf */
#define	MMINOFF		12			/* mbuf header length */
#define	MTAIL		4
#define	MMAXOFF		(MSIZE-MTAIL)		/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF-MTAIL)	/* mbuf data length */

#define	MCLBYTES	sizeof(struct mclust)	/* Size of a cluster */
#define MCLEN		1514			/* Data portion of cluster */

/*
 * Macros for type conversion
 */

/*
 * address in mbuf to mbuf head
 */

#define	dtom(x)		((struct mbuf *)((int)x & ~(MSIZE-1)))

/*
 * mbuf head, to typed data
 */

#define	mtod(x,t)	((t)((int)(x) + (x)->m_off))

struct mbuf {
	struct	mbuf *m_next;		/* next buffer in chain */
	u_long	m_off;			/* offset of data */
	short	m_len;			/* amount of data in this mbuf */
	u_char	m_flags;		/* mbuf flags */
	u_char	m_type;			/* mbuf type (0 == free) */
	union {
		u_char	mun_dat[MLEN];	/* data storage */
		struct {
			short	mun_cltype;
			int	(*mun_clfun)();
			int	mun_clarg;
			int	(*mun_clswp)();
			caddr_t	 mun_clstart;	/* ptr to 1st byte of cluster */
		} mun_cl;
	      } m_un;
	struct	mbuf *m_act;		/* link in higher-level mbuf list */
};
#define	m_dat			m_un.mun_dat
#define	m_cltype		m_un.mun_cl.mun_cltype
#define	m_clfun			m_un.mun_cl.mun_clfun
#define	m_clarg			m_un.mun_cl.mun_clarg
#define	m_clswp			m_un.mun_cl.mun_clswp	
#define	m_clstart		m_un.mun_cl.mun_clstart

struct	mclust	{			/* cluster mbuf */
	char	mcl_data[MCLEN];	/* enough for one ether packet */
	char	mcl_reserved[4];	/* ifp passing */
	char	mcl_pad[16];		/* reserved?  could be used as data */
	u_short	mcl_refcnt;		/* reference count */
};

/*
 * mbuf types
 */

#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_ZOMBIE	9	/* zombie proc status */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */
#define MT_SOPEER	12	/* socket peer buffer */
#define	MT_IFADDR	13	/* interface address */
#define MT_RIGHTS	14	/* access rights */ 
#define MT_MAX		15	/* highest MT_ + 1 */

#define	MF_BROADCAST	0x1	/* input packet was link level broadcast */

/*
 * mbuf cluster types
 */

#define	MCLT_MCLUST	1	/* standard bsd mbuf clusters */
#define	MCLT_KHEAP	2	/* kmem (heap) mbuf clusters */

/*
 * flags to m_get
 */

#define	M_DONTWAIT	0
#define	M_WAIT		1

/*
 * flags to m_pgalloc
 */

#define	MPG_MBUFS	0		/* put new mbufs on free list */
#define	MPG_CLUSTERS	1		/* put new clusters on free list */
#define	MPG_SPACE	2		/* don't free; caller wants space */

/*
 * length to m_copy to copy all
 */

#define	M_COPYALL	1000000000

/*
 * m_pullup pulls up additional length if convenient;
 * should be enough to hold headers of second-level and higher protocols. 
 */

#define	MPULL_EXTRA	32

struct	mbuf	*mfreeit();

#define	MGET(m, i, t) \
	(m) = m_getm(i, t, 1)	/* m_getm *one* mbuf */

#define	MFREE(m, n) \
	{ if ((m) == (struct mbuf *)NULL)\
		n = (struct mbuf *)NULL;\
	  else {\
		n = (m)->m_next;\
		(m)->m_next = (struct mbuf *)NULL;\
		m_freem(m);\
	  }\
	}

#define	m_get(canwait,type)	m_getm(canwait,type,1)
#define	m_free(m)		mfreeit(m)
#define	m_getclr(canwait,type)	m_getclrm(canwait,type,1)

#define	M_HASCL(m)	((m)->m_off >= MSIZE)

/*
 * Mbuf statistics.
 */

struct mbstat {
	u_int	m_mbufs;	/* mbufs obtained from page pool */
	u_int	m_mbfree;	/* mbufs on our free list */
	u_int	m_clusters;	/* clusters obtained from page pool */
	u_int	m_clfree;	/* free clusters */
	u_int	m_drops;	/* times failed to find space */
	u_int	m_cldrops;	/* times failed to alloc cluster */
	u_int	m_waits;	/* times waited for a mbuf */
	u_int	m_clwaits;	/* times waited for a cluster */
	short	m_mtypes[256];	/* type specific mbuf allocations */
};

#ifdef	KERNEL
/*
 * mbinitbufs and mbinitcl are binary-configurable
 * globals initialized in ../conf/conf_net.c.
 */

extern	int		mbinitbufs;
extern	int		mbinitcl;
extern	struct	mbuf 	*mbutl;		/* virtual address of net free mem */
extern	struct	pte 	*Mbmap;		/* page tables to map Netutl */
extern	struct	mbstat 	mbstat;
extern	int		nmbclusters;
extern	struct	mbuf 	*mfree, *mclfree;

struct	mbuf		*m_copy(),*m_pullup();
struct	mbuf		*m_getm(), *m_getclrm();
struct	mbuf		*m_getcl();
bool_t			m_expandorwait();
#endif	KERNEL
#endif	/* _SYS_MBUF_INCLUDED */
