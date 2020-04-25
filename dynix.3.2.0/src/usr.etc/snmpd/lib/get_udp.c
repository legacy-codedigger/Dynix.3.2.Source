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

#ident	"$Header: get_udp.c 1.1 1991/07/31 00:03:44 $"

/*
 * get_udp.c
 *	Get udp stats and udp connection table.
 */

/* $Log: get_udp.c,v $
 *
 */

#include "defs.h"

int
compare_udp_conn(entry1, entry2)
	struct mib_udpEntry_struct *entry1, *entry2;
{
	if ((u_long)(ntohl(entry1->udpLocalAddress)) >
	    (u_long)(ntohl(entry2->udpLocalAddress)))
		return(1);
	else if ((u_long)(ntohl(entry1->udpLocalAddress)) <
	    (u_long)(ntohl(entry2->udpLocalAddress)))
		return(-1);

	if (entry1->udpLocalPort > entry2->udpLocalPort)
		return(1);
	else if (entry1->udpLocalPort < entry2->udpLocalPort)
		return(-1);

	return(0);
}


/*
 * Print a summary of connections related to an Internet
 * protocol.  For UDP, also give state of connection.
 * Listening processes (aflag) are suppressed unless the
 * -a (all) flag is specified.
 */
get_udp_pcb(mib_udpEntry)
	struct mib_udpEntry_struct **mib_udpEntry;
{

	struct mib_udpEntry_struct *udpEntry;
	int nentries, count, i, j;
	struct inpcb *inp;

	init_mmap();		/* never know who's going to be the first */

	nentries = count_pcbs(udb, udb_addrs, 0);
	nentries += 10;		/* slop */
	count = nentries;

	if (*mib_udpEntry)
		free((char *)(*mib_udpEntry));

	udpEntry = *mib_udpEntry = (struct mib_udpEntry_struct *)
		malloc(nentries * sizeof(struct mib_udpEntry_struct));
	if (udpEntry == NULL)
		return(-1);

	for (i = 0; i < INP_HASHSZ && count > 0; i++) {
		inp = udb[i].inp_next;
		if (inp == udb_addrs[i])
			continue;
		do {
			inp = mbuf_unmap(inp, struct inpcb *);
			if (inp < (struct inpcb *) mlo || 
			    inp > (struct inpcb *) mhi)
				break;
			count--;
			udpEntry->udpLocalAddress = inp->inp_laddr.s_addr;
			udpEntry->udpLocalPort = ntohs(inp->inp_lport);
			/* build the objid extension for this entry */
			j=0;
			udpEntry->objid[j++] = 
			    (oid)(udpEntry->udpLocalAddress & 0xff);
			udpEntry->objid[j++] =
			    (oid)((udpEntry->udpLocalAddress & 0xff00) >> 8);
			udpEntry->objid[j++] =
			    (oid)((udpEntry->udpLocalAddress & 0xff0000) >> 16);
			udpEntry->objid[j++] =
			    (oid)((udpEntry->udpLocalAddress & 0xff000000)>>24);
			udpEntry->objid[j++] = (oid)udpEntry->udpLocalPort;
			inp = inp->inp_next;
			udpEntry++;
		} 
		while (inp != udb_addrs[i]);
	}

	nentries -= count;
	qsort((char *)(*mib_udpEntry), nentries, sizeof(struct mib_udpEntry_struct), compare_udp_conn);

	return(nentries);
}


get_udp_stats(mib_udp)
	struct mib_udp_struct *mib_udp;
{
	init_mmap();		/* never know who's going to be the first */

	mib_udp->udpInErrors = udpstat->udps_hdrops + udpstat->udps_badsum + 
		udpstat->udps_badlen + udpstat->udps_fullsock;

#ifdef	KERN3_2
	mib_udp->udpInDatagrams = udpstat->udps_delivers;
	mib_udp->udpNoPorts = udpstat->udps_noport;
	mib_udp->udpOutDatagrams = 0;
#else
	mib_udp->udpInDatagrams = 0;	/* have to support something! */
	mib_udp->udpNoPorts = 0;
	mib_udp->udpOutDatagrams = 0;
#endif

	return(0);
}
