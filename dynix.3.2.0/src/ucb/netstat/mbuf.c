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

#ifndef lint
static char rcsid[] = "$Header: mbuf.c 2.5 1991/09/11 16:16:30 $";
#endif

#include <stdio.h>
#include <sys/param.h>
#include <sys/mbuf.h>

#define	YES	1
typedef int bool;

struct	mbstat mbstat;
extern	int kmem;

static struct mbtypes {
	int	mt_type;
	char	*mt_name;
} mbtypes[] = {
	{ MT_DATA,	"data" },
	{ MT_HEADER,	"packet headers" },
	{ MT_SOCKET,	"socket structures" },
	{ MT_SOPEER,	"socket peers" },
	{ MT_PCB,	"protocol control blocks" },
	{ MT_RTABLE,	"routing table entries" },
	{ MT_HTABLE,	"IMP host table entries" },
	{ MT_ATABLE,	"address resolution tables" },
	{ MT_FTABLE,	"fragment reassembly queue headers" },
	{ MT_SONAME,	"socket names and addresses" },
	{ MT_ZOMBIE,	"zombie process information" },
	{ MT_SOOPTS,	"socket options" },
	{ MT_IFADDR,	"interface addresses" }, 
	{ 0, 0 }
};

int nmbtypes = sizeof(mbstat.m_mtypes) / sizeof(short);
bool seen[256];			/* "have we seen this type yet?" */

/*
 * Print mbuf statistics.
 */
mbpr(mbaddr)
	off_t mbaddr;
{
	register int totmem, totfree, totmbufs;
	register struct mbtypes *mp;
	register int i;

	if (nmbtypes != 256) {
		fprintf(stderr, "unexpected change to mbstat; check source\n");
		return;
	}

	if (mbaddr == 0) {
		printf("mbstat: symbol not in namelist\n");
		return;
	}
	klseek(kmem, mbaddr, 0);
	if (read(kmem, &mbstat, sizeof (mbstat)) != sizeof (mbstat)) {
		printf("mbstat: bad read\n");
		return;
	}
	printf("%d/%d mbufs in use:\n",
		mbstat.m_mbufs - mbstat.m_mbfree, mbstat.m_mbufs);
	totmbufs = 0;
	for (mp = mbtypes; mp->mt_name; mp++)
		if (mbstat.m_mtypes[mp->mt_type]) {
			seen[mp->mt_type] = YES;
			printf("\t%d mbufs allocated to %s\n",
				mbstat.m_mtypes[mp->mt_type], mp->mt_name);
			totmbufs += mbstat.m_mtypes[mp->mt_type];
		}

	seen[MT_FREE] = YES;
	for (i = 0; i < nmbtypes; i++)
		if (!seen[i] && mbstat.m_mtypes[i]) {
			printf("\t%d mbufs allocated to <mbuf type %d>\n",
			    mbstat.m_mtypes[i], i);
			totmbufs += mbstat.m_mtypes[i];
		}

	if (totmbufs != mbstat.m_mbufs - mbstat.m_mbfree)
		printf("*** %d mbufs missing ***\n",
			(mbstat.m_mbufs - mbstat.m_mbfree) - totmbufs);
	printf("%d/%d mapped pages in use\n",
		mbstat.m_clusters - mbstat.m_clfree, mbstat.m_clusters);
	totmem = mbstat.m_mbufs * MSIZE + mbstat.m_clusters * MCLBYTES;
	totfree = mbstat.m_mbfree * MSIZE + mbstat.m_clfree * MCLBYTES;
	printf("%d Kbytes allocated to network (%d%% in use)\n",
		totmem / 1024, (totmem - totfree) * 100 / totmem);
	printf("%d requests for memory denied\n", mbstat.m_drops);
	printf("%d requests for clusters denied\n", mbstat.m_cldrops);
	printf("%d requests for memory which waited\n", mbstat.m_waits);
	printf("%d requests for clusters which waited\n", mbstat.m_clwaits);
}
