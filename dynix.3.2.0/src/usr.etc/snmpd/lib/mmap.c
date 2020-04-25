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

/* $Header: mmap.c 1.2 1991/08/02 16:12:09 $ */

/* $Log: mmap.c,v $
 *
 *
 */

/* mmap.c:
 *
 * As a quick and easy way to get at things inside of the D3 kernel, we
 * mmap a lot of stuff.  For example, the process of retrieving all the
 * connections implies walking arbitrary mbuf memory.  So we mmap the
 * whole address space and walk it from user space (as opposed to implementing
 * such a feature in the kernel).  Although less reliable than a
 * kernel implementation, this is considerably lower drag, quicker
 * to implement, and keeps the kernel size small.  Besides, snmp is
 * a best effort protocol anyway.
 *
 * The mmap'd regions are managed kind of wierdly due to the system limitation
 * that we have at most X mmap'd regions (3.2: 16).  Since the daemon
 * exceeds that number, we take all the variable from the data and bss
 * areas, and map the area which is collectively that from the lowest
 * addressed variable to the highest addressed variable plus its length.
 * In other words, we mmap one huge region for all the data/bss symbols.
 * Mbufs, on the other hand, are calloc'd during system initialization (from
 * high memory, no less), so we mmap that as a separate region to prevent
 * this process form hogging up to many pte's.
 *
 * Other variable are read from /dev/kmem once at startup and are simply 
 * cached in this process for its duration; these presumably don't 
 * change anyway.
 */

#include "defs.h"

#include <nlist.h>
#include <sys/mman.h>
#include <sys/file.h>

extern	char *sbrk();
extern	char *valloc();
int	pagesize;
char	*unix_file = "/dynix";
char	*kmem_file = "/dev/kmem";
int	km;

/*
 * Globals which are initialized once.
 */
int	arptab_size;
int	tcp_backoff[2];
off_t	mdiff;		/* difference between our VM addresses and kernels */
int	ctl_sock;
int	rthashsize;
int	*ip_ttl;
struct	inpcb *tcb_addrs[INP_HASHSZ];
struct	inpcb *udb_addrs[INP_HASHSZ];
off_t	ifnet;

/*
 * Mbuf memory is not in bss, but in its own address space somewhere in
 * high memory.  So we manage it a separate region
 */
off_t	mblop, mbhip;
struct	mbuf *mlo, *mhi;

/*
 * Globals in data/bss which are memory mapped.  We map one large region,
 * then set these pointers up as offsets into that region
 */
struct	arptab *arptab;
struct  icmpstat *icmpstat;
struct	tcpstat *tcpstat;
struct	ipstat *ipstat;
struct	udpstat *udpstat;
struct	inpcb *tcb;
struct	inpcb *udb;
int	*ipforwarding;
struct	ifqueue	*ipintrq;;
struct	mbuf **rthost;
struct	mbuf **rtnet;


struct	nlist nl[] = {
	{ "_arptab_size" },
#define	ARPTAB_SIZE	0
	{ "_arptab" },
#define	ARPTAB		1
	{ "_icmpstat" },
#define	ICMPSTAT	2
	{ "_tcp_backoff" },
#define	TCP_BACKOFF	3
	{ "_tcpstat" },
#define	TCPSTAT		4
	{ "_tcb" },
#define	TCB		5
	{ "_mblorange" },
#define	MBLORANGE	6
	{ "_mbhirange" },
#define	MBHIRANGE	7
	{ "_udb" },
#define	UDB		8
	{ "_ifnet" },
#define	IFNET		9
	{ "_ipstat" },
#define	IPSTAT		10
	{ "_ipforwarding" },
#define	IPFORWARDING	11
	{ "_ipintrq" },
#define	IPINTRQ		12
	{ "_rthost" },
#define	RTHOST		13
	{ "_rtnet" },
#define	RTNET		14
	{ "_rthashsize" },
#define	RTHASHSIZE	15
	{ "_udpstat" },
#define	UDPSTAT		16
#ifdef	KERN3_2
	{ "_ip_ttl" },
#define	IP_TTL		17
#define	NLIST_MAX	18
#else
#define	NLIST_MAX	17
#endif
	0
};

/* 
 * init_mmanage:
 *
 * Manage the mmap'd address space which covers all of data/bss.  The
 * mm_dontcare field is used to indicate those variables which aren't
 * part of the mmap.  The mm_offset field is used to indicate the
 * symbols's offset in kernel virtual memory, and mm_len is the
 * variable length.  Later this data structure is used to calculate
 * the address range needed to cover all the data/bss symbols.
 */

struct mmanage {
	off_t	mm_offset;
	int	mm_len;
	int	mm_dontcare;
}mm[NLIST_MAX];

init_mmanage()
{
	int i;

	for (i = 0; i < NLIST_MAX; i++)
		mm[i].mm_offset = nl[i].n_value;
	mm[ARPTAB_SIZE].mm_dontcare = 1;
	mm[TCP_BACKOFF].mm_dontcare = 1;
	mm[MBLORANGE].mm_dontcare = 1;
	mm[MBHIRANGE].mm_dontcare = 1;
	mm[RTHASHSIZE].mm_dontcare = 1;
	mm[IFNET].mm_dontcare = 1;

	mm[ARPTAB].mm_len = sizeof(struct arptab) * arptab_size;
	mm[ICMPSTAT].mm_len = sizeof(struct icmpstat);
	mm[TCPSTAT].mm_len = sizeof(tcpstat);
	mm[TCB].mm_len = sizeof(struct inpcb) * INP_HASHSZ;
	mm[UDB].mm_len = sizeof(struct inpcb) * INP_HASHSZ;
	mm[IPSTAT].mm_len = sizeof(struct ipstat);
	mm[IPFORWARDING].mm_len = sizeof(int);
	mm[IPINTRQ].mm_len = sizeof(struct ifqueue);
	mm[RTHOST].mm_len = sizeof(struct mbuf *) * rthashsize;
	mm[RTNET].mm_len = sizeof(struct mbuf *) * rthashsize;
	mm[UDPSTAT].mm_len = sizeof(struct udpstat);

#ifdef	KERN3_2
	mm[IP_TTL].mm_len = sizeof(int);
#endif
}

/*
 * do_mmap:
 *
 * Actually mmap a kernel virtual address into our own process address
 * space.
 */

char*
do_mmap(size, kernaddr)
	int size;
	u_long kernaddr;
{
        off_t	pos;
        int	sz, off;
        char	*va;
	static	times = 0;
	int 	i;

	va = (caddr_t) (((int)sbrk(0) + (pagesize-1)) & ~(pagesize-1));
        pos = (unsigned)kernaddr & ~(pagesize-1);
        off = (unsigned)kernaddr - pos;
        sz = size + off;
        sz = (sz+pagesize-1) & ~(pagesize-1);
	if (mmap(va, sz, PROT_RDWR, MAP_SHARED, km, pos) != 0) {
		syslog(LOG_ERR, "mmap: %m");
		exit(1);
	}
        return((va + off));
}
/*
 * find_region:
 * 
 * find the lowest virtual address out of all symbols we wish to map
 * At the same time, find the highest address we need to mmap
 */
find_region(lp, hp)
	u_long *lp;
	u_long *hp;
{
	int i;
	u_long min = 0xffffffff;
	u_long max = 0;

	for (i = 0; i < NLIST_MAX; i++) {
		if (mm[i].mm_dontcare == 1)
			continue;
		min = MIN(min, mm[i].mm_offset);
		max = MAX(max, mm[i].mm_offset + mm[i].mm_len);
	}

	*lp = min;
	*hp = max;
}

/* 
 * init_mmap:
 *
 * Initialize all mmap's.  This is called from all the get routines, so
 * the daemon startup time is negligible -- the first query takes the
 * hit for setting everything up.  At the same time the mmap's are
 * initialized, those variable which only need be read and not mmap'd
 * are also set up.
 */
init_mmap()
{
	static int initted = 0;
	int error = 0;
	int i, size;
	char *start;
	u_long max, base;

	if (initted)
		return;
	initted = 1;

	if ((km = open(kmem_file, O_RDWR)) < 0) {
		fprintf(stderr, "Can't open kmem file %s\n", kmem_file);
		syslog(LOG_ERR, "open: %m");
		exit(1);
	}

	nlist(unix_file, nl);

	for (i = 0; i < sizeof(nl)/sizeof(struct nlist) - 1; i++) {
		if (nl[i].n_value == 0) {
			syslog(LOG_ERR, "Symbol '%s' not found in %s\n",
				nl[i].n_name, unix_file);
			error = 1;
		}
	}
	if (error)
		exit(1);

	pagesize = getpagesize();
	lseek(km, nl[ARPTAB_SIZE].n_value, 0);
	if (read(km, &arptab_size, sizeof(arptab_size)) 
	    != sizeof(arptab_size)) {
		syslog(LOG_ERR, "read arptab_size: %m");
		exit(1);
	}
	lseek(km, nl[MBLORANGE].n_value, 0);
	if (read(km, &mblop, sizeof(mblop)) != sizeof(mblop)) {
		syslog(LOG_ERR, "read mblop: %m");
		exit(1);
	}
	lseek(km, nl[MBHIRANGE].n_value, 0);
	if (read(km, &mbhip, sizeof(mbhip)) != sizeof(mbhip)) {
		syslog(LOG_ERR, "read mbhip: %m");
		exit(1);
	}
	lseek(km, nl[TCP_BACKOFF].n_value, 0);
	if (read(km, &tcp_backoff[0], sizeof(int)) != sizeof(int)) {
		syslog(LOG_ERR, "read tcp_backoff: %m");
		exit(1);
	}
	lseek(km, nl[TCP_BACKOFF].n_value + sizeof(int) * (TCP_MAXRXTSHIFT - 1),
		0);
	if (read(km, &tcp_backoff[1], sizeof(int)) != sizeof(int)) {
		syslog(LOG_ERR, "read tcp_backoff: %m");
		exit(1);
	}

	lseek(km, nl[RTHASHSIZE].n_value, 0);
	if (read(km, &rthashsize, sizeof(rthashsize)) != sizeof(rthashsize)) {
		syslog(LOG_ERR, "read rthashsize: %m");
		exit(1);
	}
	ifnet = nl[IFNET].n_value;

	size = (int) mbhip - (int) mblop;
	size = (size + (CLBYTES-1)) &~ (CLBYTES-1);
	mlo = (struct mbuf *) do_mmap(size, mblop);
	mhi = (struct mbuf *) ((int)mlo + size);
	mdiff = (off_t) ((caddr_t) mblop - (caddr_t) mlo);

	if ((ctl_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "get interface configuration socket: %m");
		exit(1);
	}

	init_mmanage();
	find_region(&base, &max);

	start = do_mmap(max - base, base);
	init_pointers(base, start);

}

/*
 * init_pcbs
 *
 * Initialize the hash head pointers for the inpcb address lists.  The
 * The inpcbs are doubly threaded, and point back to the head of the
 * list.  So list searches terminate when inp_next == head address.
 * The idea here is that we have a "phony" hash list which only contains
 * the kernel address of the head record, so we can terminate list
 * searches.
 */
init_pcbs(head, off)
	struct inpcb *head[];
	off_t off;
{
	int i;

	for (i = 0; i < INP_HASHSZ; i++) {
		head[i] = (struct inpcb *)off;
		off += sizeof(struct inpcb);
	}
}

/*
 * init_pointers
 *
 * Initialize pointers to mmap'd regions.  Once done, when dereferenced,
 * we're talking directly to kernel virtual memory.
 */
init_pointers(base, start)
	u_long base;	/* lowest virtual address */
	char *start;	/* starting virtual address */

{
	arptab = (struct arptab *)(start + (nl[ARPTAB].n_value - base));
	icmpstat = (struct icmpstat *)(start + (nl[ICMPSTAT].n_value - base));
	tcpstat = (struct tcpstat *)(start + (nl[TCPSTAT].n_value - base));
	ipstat = (struct ipstat *)(start + (nl[IPSTAT].n_value - base));
	udpstat = (struct udpstat *)(start + (nl[UDPSTAT].n_value - base));
	tcb = (struct inpcb *)(start + (nl[TCB].n_value - base));
	udb = (struct inpcb *)(start + (nl[UDB].n_value - base));
	ipforwarding = (int *)(start + (nl[IPFORWARDING].n_value - base));
	ipintrq = (struct ifqueue *)(start + (nl[IPINTRQ].n_value - base));
	rthost = (struct mbuf **)(start + (nl[RTHOST].n_value - base));
	rtnet = (struct mbuf **)(start + (nl[RTNET].n_value - base));
#ifdef	KERN3_2
	ip_ttl = (int *)(start + (nl[IP_TTL].n_value - base));
#endif
}
