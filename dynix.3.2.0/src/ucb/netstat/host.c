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

#ifndef lint
static char rcsid[] = "$Header: host.c 2.0 86/01/28 $";
#endif

#include <sys/types.h>
#include <sys/mbuf.h>

#include <netinet/in.h>
#ifdef notyet
#include <netimp/if_imp.h>
#include <netimp/if_imphost.h>
#endif notyet

#include <netdb.h>

extern	int kmem;
extern 	int nflag;
extern	char *inetname();

/*
 * Print the host tables associated with the ARPANET IMP.
 * Symbolic addresses are shown unless the nflag is given.
 */
hostpr(hostsaddr)
	off_t hostsaddr;
{
	struct mbuf *hosts, mb;
	register struct mbuf *m;
	register struct hmbuf *mh;
	register struct host *hp;
	struct hostent *p;
	char flagbuf[10], *flags;
	int first = 1;

	if (hostsaddr == 0) {
		printf("hosts: symbol not in namelist\n");
		return;
	}
#ifdef notyet
	klseek(kmem, hostsaddr, 0);
	read(kmem, &hosts, sizeof (hosts));
	m = hosts;
	printf("IMP Host Table\n");
	printf("%-5.5s %-15.15s %-4.4s %-9.9s %-4.4s %s\n",
		"Flags", "Host", "Qcnt", "Q Address", "RFNM", "Timer");
	while (m) {
		klseek(kmem, m, 0);
		read(kmem, &mb, sizeof (mb));
		m = &mb;
		mh = mtod(m, struct hmbuf *);
		if (mh->hm_count == 0) {
			m = m->m_next;
			continue;
		}
		for (hp = mh->hm_hosts; hp < mh->hm_hosts + HPMBUF; hp++) {
			if ((hp->h_flags&HF_INUSE) == 0 && hp->h_timer == 0)
				continue;
			flags = flagbuf;
			*flags++ = hp->h_flags&HF_INUSE ? 'A' : 'F';
			if (hp->h_flags&HF_DEAD)
				*flags++ = 'D';
			if (hp->h_flags&HF_UNREACH)
				*flags++ = 'U';
			*flags = '\0';
			printf("%-5.5s %-15.15s %-4d %-9x %-4d %d\n",
				flagbuf,
				inetname(hp->h_addr),
				hp->h_qcnt,
				hp->h_q,
				hp->h_rfnm,
				hp->h_timer);
		}
		m = m->m_next;
	}
#endif notyet
}
