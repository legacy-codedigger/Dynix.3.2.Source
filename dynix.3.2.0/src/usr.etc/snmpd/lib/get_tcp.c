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

#ident	"$Header: get_tcp.c 1.1 1991/07/31 00:03:36 $"

/*
 * get_tcp.c
 *	Get tcp stats and tcp connection table.
 */

/* $Log: get_tcp.c,v $
 *
 */
#include "defs.h"

#define TCPSTATES
#include <netinet/tcp_fsm.h>

int snmp_tcp_states_tab[] = {
	1,2,3,4,5,8,6,10,9,7,11
};

extern int errno;

int
compare_tcp_conn(entry1, entry2)
	struct mib_tcpConnEntry_struct *entry1, *entry2;
{
	if ((u_long)(ntohl(entry1->tcpConnLocalAddress)) >
	    (u_long)(ntohl(entry2->tcpConnLocalAddress)))
		return(1);
	else if ((u_long)(ntohl(entry1->tcpConnLocalAddress)) <
	    (u_long)(ntohl(entry2->tcpConnLocalAddress)))
		return(-1);

	if (entry1->tcpConnLocalPort > entry2->tcpConnLocalPort)
		return(1);
	else if (entry1->tcpConnLocalPort < entry2->tcpConnLocalPort)
		return(-1);

	if ((u_long)(ntohl(entry1->tcpConnRemAddress)) >
	    (u_long)(ntohl(entry2->tcpConnRemAddress)))
		return(1);
	else if ((u_long)(ntohl(entry1->tcpConnRemAddress)) <
	    (u_long)(ntohl(entry2->tcpConnRemAddress)))
		return(-1);

	if (entry1->tcpConnRemPort > entry2->tcpConnRemPort)
		return(1);
	else if (entry1->tcpConnRemPort < entry2->tcpConnRemPort)
		return(-1);

	return(0);
}

/*
 * Get tcp connections 
 */
get_tcp_pcb(mib_tcpConnEntry)
	struct mib_tcpConnEntry_struct **mib_tcpConnEntry;
{
	struct mib_tcpConnEntry_struct *tcpConnEntry;
	int nentries, count, i, j;
	struct inpcb *inp;
	struct tcpcb *tp;

	init_mmap();		/* never know who's going to be the first */

	nentries = count_pcbs(tcb, tcb_addrs, 0);
	nentries += 10;		/* slop for connections that may come while
					 * we're walking the table */
	count = nentries;

	if (*mib_tcpConnEntry)
		free((char *)(*mib_tcpConnEntry));

	tcpConnEntry = *mib_tcpConnEntry = (struct mib_tcpConnEntry_struct *)
		malloc(nentries * sizeof(struct mib_tcpConnEntry_struct));
	if (tcpConnEntry == NULL)
		return(-1);

	for (i = 0; i < INP_HASHSZ && count > 0; i++) {
		inp = tcb[i].inp_next;
		if (inp == tcb_addrs[i])
			continue;
		do {
			inp = mbuf_unmap(inp, struct inpcb *);
			if (inp < (struct inpcb *) mlo || 
			    inp > (struct inpcb *) mhi)
				break;
			tp = intotcpcb(inp);
			tp = mbuf_unmap(tp, struct tcpcb *);
			if (tp < (struct tcpcb *) mlo || 
			    tp > (struct tcpcb *) mhi)
				break;
			count--;
			tcpConnEntry->tcpConnState = snmp_tcp_states_tab[tp->t_state];
			tcpConnEntry->tcpConnLocalAddress = inp->inp_laddr.s_addr;
			tcpConnEntry->tcpConnLocalPort = ntohs(inp->inp_lport);
			tcpConnEntry->tcpConnRemAddress = inp->inp_faddr.s_addr;
			tcpConnEntry->tcpConnRemPort = ntohs(inp->inp_fport);

			/* build the objid extension for this entry */
			j=0;
			tcpConnEntry->objid[j++] = (oid)(tcpConnEntry->tcpConnLocalAddress & 0xff);
			tcpConnEntry->objid[j++] = (oid)((tcpConnEntry->tcpConnLocalAddress & 0xff00) >> 8);
			tcpConnEntry->objid[j++] = (oid)((tcpConnEntry->tcpConnLocalAddress & 0xff0000) >> 16);
			tcpConnEntry->objid[j++] = (oid)((tcpConnEntry->tcpConnLocalAddress & 0xff000000) >> 24);
			tcpConnEntry->objid[j++] = (oid)tcpConnEntry->tcpConnLocalPort;
			tcpConnEntry->objid[j++] = (oid)(tcpConnEntry->tcpConnRemAddress & 0xff);
			tcpConnEntry->objid[j++] = (oid)((tcpConnEntry->tcpConnRemAddress & 0xff00) >> 8);
			tcpConnEntry->objid[j++] = (oid)((tcpConnEntry->tcpConnRemAddress & 0xff0000) >> 16);
			tcpConnEntry->objid[j++] = (oid)((tcpConnEntry->tcpConnRemAddress & 0xff000000) >> 24);
			tcpConnEntry->objid[j++] = (oid)tcpConnEntry->tcpConnRemPort;
			inp = inp->inp_next;
			tcpConnEntry++;
		} 
		while (inp != tcb_addrs[i]);
	}

	nentries -= count;
	qsort((char *)(*mib_tcpConnEntry), nentries, sizeof(struct mib_tcpConnEntry_struct), compare_tcp_conn);

	return(nentries);
}


get_tcp_mib_stats(mib_tcp)
	struct mib_tcp_struct *mib_tcp;
{

	init_mmap();		/* never know who's going to be the first */

#ifdef	KERN3_2
	mib_tcp->tcpActiveOpens = tcpstat->tcps_connattempt;
	mib_tcp->tcpPassiveOpens = tcpstat->tcps_accepts;
	mib_tcp->tcpAttemptFails = tcpstat->tcps_conndrops;
	mib_tcp->tcpEstabResets = tcpstat->tcps_drops;
	mib_tcp->tcpInSegs = tcpstat->tcps_rcvtotal;
	mib_tcp->tcpOutSegs = tcpstat->tcps_sndtotal;
	mib_tcp->tcpRetransSegs = tcpstat->tcps_sndrexmitpack;
	mib_tcp->tcpInErrs = tcpstat->tcps_rcvbadsum + tcpstat->tcps_rcvbadoff +
	    tcpstat->tcps_rcvshort + tcpstat->tcps_rcvduppack +
	    tcpstat->tcps_rcvafterclose + tcpstat->tcps_rcvdupack;
	mib_tcp->tcpOutRsts = tcpstat->tcps_sndrst;
#else
	bzero(mib_tcp, sizeof(*mib_tcp));
#endif
	mib_tcp->tcpRtoAlgorithm = MIB_TCPRTOALG_VANJ;
	mib_tcp->tcpRtoMin = tcp_backoff[0];
	mib_tcp->tcpRtoMax = tcp_backoff[1];
	mib_tcp->tcpMaxConn = -1;	/* dynamic */
	mib_tcp->tcpCurrEstab = count_pcbs(tcb, tcb_addrs, 1);

	return(0);
}

count_pcbs(head, addrs, estab)
	struct inpcb *head;
	struct inpcb *addrs[];
	int estab;
{
	int i, count = 0;
	struct inpcb *inp;
	struct tcpcb *tp;

	for (i = 0; i < INP_HASHSZ; i++) {
		inp = head[i].inp_next;
		if (inp == addrs[i])
			continue;
		do {
			inp = mbuf_unmap(inp, struct inpcb *);
			if (inp < (struct inpcb *) mlo || 
			    inp > (struct inpcb *) mhi)
				break;
			if (estab) {
				tp = mbuf_unmap(intotcpcb(inp), struct tcpcb *);
				if (tp < (struct tcpcb *) mlo || 
			    	    tp > (struct tcpcb *) mhi)
					break;
				if (tp->t_state == TCPS_ESTABLISHED || 
				    tp->t_state == TCPS_CLOSE_WAIT)
					count++;
			} 
			else
				count++;
			inp = inp->inp_next;
		} 
		while (inp != addrs[i]);
	}
	return(count);
}
