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
static	char	rcsid[] = "$Header: udp_usrreq.c 2.18 1991/04/30 17:17:40 $";
#endif

/*
 * udp_usrreq.c
 *
 */

/* $Log: udp_usrreq.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../net/if.h"
#include "../net/route.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/udp.h"
#include "../netinet/udp_var.h"

struct	inpcb udb[INP_HASHSZ];
struct	udpstat udpstat;
struct	inhcomm udb_comm;

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */

udp_init()
{
	in_pcbhashinit(udb, &udb_comm);
}

struct	sockaddr_in udp_in = { AF_INET };

udp_input(m0, ifp)
	struct mbuf *m0;
	struct ifnet *ifp;
{
	register struct udpiphdr *ui;
	register struct inpcb *inp;
	register struct mbuf *m;
	int len;
	struct ip ip;
	int	checked = 0;

	/*
	 * Get IP and UDP header together in first mbuf.
	 */

	m = m0;
	if ((m->m_off > MMAXOFF || m->m_len < sizeof (struct udpiphdr)) &&
	    (m = m_pullup(m, sizeof (struct udpiphdr))) == 
				(struct mbuf *)NULL) {

		/*
		 * m_pullup can fail due to failure to allocate mbuf
		 * have to drop packet => best effort fails.
		 */

		udpstat.udps_hdrops++;
		return;
	}
	ui = mtod(m, struct udpiphdr *);
	if (((struct ip *)ui)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ui, (struct mbuf *)NULL);

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */

	len = ntohs((u_short)ui->ui_ulen);
	if (((struct ip *)ui)->ip_len != len) {
		if (len > ((struct ip *)ui)->ip_len) {
			udpstat.udps_badlen++;
			m_freem(m);
			return;
		}

		/*
		 * (struct ip *)ui->ip_len = len;
		 */

		m_adj(m, len - ((struct ip *)ui)->ip_len);
	}

	/*
	 * Checksum extended UDP header and data.
	 *
	 * udpcksum: if given a value other than 0, checksums
	 * are performed - this is a binary config parameter
	 * Note: 4.2 *disables* udp checksumming on input.
	 */

	if (udpcksumi && ui->ui_sum) {	/* checksum input */

		/*
		 * Save a copy of the IP header in case it is
		 * restored for ICMP.
		 */

		ip = *(struct ip*)ui;

		checked++;

		ui->ui_next = ui->ui_prev = 0;
		ui->ui_x1 = 0;
		ui->ui_len = ui->ui_ulen;

		if (ui->ui_sum = in_cksum(m, len + sizeof (struct ip))) {
			udpstat.udps_badsum++;
			m_freem(m);
			return;
		}
	}

	/*
	 * Construct sockaddr format source address before lookup to
	 * minimize critical section - udp_in is used by
	 * sbappendaddr() later if a pcb is found.  Note: m_len and
	 * m_off are also adjusted here to remove udpip header.
	 */

	udp_in.sin_port = ui->ui_sport;
	udp_in.sin_addr = ui->ui_src;
	m->m_len -= sizeof (struct udpiphdr);
	m->m_off += sizeof (struct udpiphdr);

	/*
	 * Locate pcb for datagram.
	 *
	 * in_pcblookup looks up an internet address in the inpcblist headed
	 * by head.  In this case head == &udb which is an inpcb
	 * structure that serves as the head of the recognized internet 
	 * addresses of the udp protocol engine.  in_pcblookup locks the list 
	 * while searching to guard against concurrent changes.
	 * When/if a pcb is found, it's sop_lock is acquired, it's
	 * sop_refcnt count is incremented and the pcb list is unlocked.
	 * If a pcb is not found that is bound to the internet address,
	 * NULL is returned.
	 */

	inp = in_pcblookup(udb,
	    ui->ui_src, ui->ui_sport, ui->ui_dst, ui->ui_dport,
		INPLOOKUP_WILDCARD);

	if (inp == (struct inpcb *)NULL) {

		struct in_addr dest;
		udpstat.udps_noport++;

		/*
		 * don't send ICMP response for broadcast packet
		 */

		if (in_broadcast(ui->ui_dst)){
			m_freem(m);
			return;
		}
		if(checked)
			*(struct ip *)ui = ip;
		dest.s_addr = 0;
		icmp_error((struct ip *)ui, ICMP_UNREACH, ICMP_UNREACH_PORT,
		    ifp, dest);
		return;
	}

	/*
	 * decrement sop_refcnt incremented by pcblookup
	 */

	inp->inp_sopp->sop_refcnt--;

	/*
	 * note bene the pcb is *locked* when lookup finds it.
	 * this also locks the bound socket_peer which mutexes state changes
	 * in the bound socket.  Also note that if udp spins on this lock
	 * because the user happens to be looking at this socket, then no
	 * other network processing goes on - i.e. a user has blocked other
	 * user data from being processed.  This data is queued on the network
	 * interface.  Possible enhancements to this approach include
	 * 1) droping the packet (after all it is "best effort") or, 2) queuing
	 * the packet aside somewhere and trying again later (might be delivered
	 * out of sequence then).
	 */

	/*
	 * check for state change here in case there is no longer a
	 * socket reference.
	 */

	if (inp->inp_socket) { /* still have a socket to talk to */

		/*
		 * Stuff source address and datagram in user buffer.
		 */

		if (sbappendaddr(&inp->inp_socket->so_rcv,
		    (struct sockaddr *)&udp_in, m, (struct mbuf *)NULL)
		     == 0) {
			INP_UNLOCK(inp, SPLNET);
			m_freem(m);
			udpstat.udps_fullsock++;
			return;
		}
		udpstat.udps_delivers++;
		sorwakeup(inp->inp_socket);
	} else {

		/*
		 * socket disappeared apparently in_pcbdetach is trying
		 * to get rid of this (e.g. spinning on udb list lock
		 * to remque it inpcb is gone but need to unlock it so
		 * detach can finish.
		 */

		m_freem(m);
	}

	INP_UNLOCK(inp, SPLNET);
}

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */

udp_notify(inp)
	register struct inpcb *inp;
{
	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
}

udp_ctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	extern u_char inetctlerrmap[];
	struct sockaddr_in *sin;
	int in_rtchange();

	if ((unsigned)cmd > PRC_NCMDS)
		return;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_IMPLINK)
		return;
	sin = (struct sockaddr_in *)sa;
	if (sin->sin_addr.s_addr == INADDR_ANY)
		return;

	switch (cmd) {

	case PRC_QUENCH:
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		in_pcbnotify(udb, &sin->sin_addr, 0, in_rtchange);
		break;

	default:
		if (inetctlerrmap[cmd] == 0)
			return;		/* XXX */
		in_pcbnotify(udb, &sin->sin_addr, (int)inetctlerrmap[cmd],
			udp_notify);
	}
}

udp_output(inp, m0)
	register struct inpcb *inp;
	struct mbuf *m0;
{
	register struct mbuf *m;
	register struct udpiphdr *ui;
	register int len = 0;
	int error;

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP headers.
	 */

	for (m = m0; m; m = m->m_next)
		len += m->m_len;

	/*
	 * N.B. M_DONTWAIT argument is not used (also not used in 4.2)
	 */

	MGET(m, M_DONTWAIT, MT_HEADER);
	if (m == (struct mbuf *)NULL) {
		m_freem(m0);
		return (ENOBUFS);
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */

	m->m_off = MMAXOFF - sizeof (struct udpiphdr);
	m->m_len = sizeof (struct udpiphdr);
	m->m_next = m0;
	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)(len + sizeof (struct udphdr)));
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = ui->ui_len;	/* N.B. net order */

	/*
	 * Stuff checksum and output datagram.
	 */

	ui->ui_sum = 0;
	if (udpcksumo) {
	    if((ui->ui_sum = in_cksum(m, sizeof (struct udpiphdr) + len)) == 0)
			ui->ui_sum = -1;
	}
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;
	((struct ip *)ui)->ip_ttl = udp_ttl;
	error =  ip_output(m, inp->inp_options, &inp->inp_route,
	    inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST));
	if (error == 0)
		udpstat.udps_sends++;
	return(error);
}

/*
 * udp_usrreq is called from user level for a request.  Except for
 * ATTACH, the socket_peer is locked.  For attach, the socket is new and
 * unknown (thus no mutex issue), udp uses in_pcballoc to bind an inpcb
 * to the socket.  This binding includes establishment of a socket_peer
 * structure to be used for mutex between the pr_input engine (i.e.
 * udp_input) and the user (i.e.udp_usrreq).  Therefore, upon entry to
 * udp_usrreq for other than ATTACH, both the socket and inpcb for that
 * socket are locked.
 */
/*ARGSUSED*/
udp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;

	if (req == PRU_CONTROL)
		return (in_control(so, (int)m, (caddr_t)nam,
			(struct ifnet *)rights));

	if (rights && rights->m_len) {
		error = EINVAL;
		goto release;
	}
	if (inp == (struct inpcb *)NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	case PRU_ATTACH:

		if (inp != (struct inpcb *)NULL) {
			error = EINVAL;
			break;
		}

		/*
		 * in_pcballoc gets the pcb resources, links the pcb to
		 * the socket, and sets up a socket_peer structure with
		 * a refcnt of 1.  This socket_peer provides mutex with
		 * the pr_input routines performing network services.
		 * NOTE BENE insque moved to attach routine (here) to
		 * avoid racing with timeout.
		 */

		error = in_pcballoc(so, udb);

		if (error)
			break;

		error = soreserve(so, udp_sendspace, udp_recvspace);

		break;

	case PRU_DETACH:

		in_pcbdetach(inp);
		break;

	case PRU_BIND:
		error = in_pcbbind(inp, nam);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		error = in_pcbconnect(inp, nam);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_ACCEPT:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}

		in_pcbdisconnect(inp);
		so->so_state &= ~SS_ISCONNECTED;                /* XXX */

		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND: {

		/*
		 * must do this differently than a monoprocessor.
		 * Since in_pcblookup does not mutex the pcb's in the
		 * udb list for comparison (too expensive), it is not
		 * appropriate to use that space temporarily to
		 * store an output address.  Use a static structure to pass the
		 * necessary information for udp_output to use.
		 */
		struct inpcb local_inp;

		if (nam) {
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				error = EISCONN;
				break;
			}
			/*
			 * first check to see if we've a local port; if not
			 * we havn't bound yet, so we better before continuing,
			 * otherwise in_pcbconnect will call in_pcbbind, which
			 * will put our local inp into some udb hash line!
			 */

			if (!(inp->inp_lport))
				error = in_pcbbind(inp, (struct mbuf *)0);
			if (error)
				break;

			local_inp = *inp;	/* struct copy */
			error = in_pcbconnect(&local_inp, nam);
			if (error)
				break;
		} else {
			if (inp->inp_faddr.s_addr == INADDR_ANY) {
				error = ENOTCONN;
				break;
			}
			local_inp = *inp;	/* struct copy */
		}
		error = udp_output(&local_inp, m);

		if (nam && local_inp.inp_route.ro_rt)
			RTFREE(local_inp.inp_route.ro_rt);

		m = (struct mbuf *)NULL;
		}
		break;

	case PRU_ABORT:
		in_pcbdetach(inp);
		soisdisconnected(so);
		sofree(so);
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	case PRU_RCVD:
	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error = EOPNOTSUPP;
		break;

	case PRU_SENSE:	
		return(0);

	case PRU_RCVOOB:
		return(EOPNOTSUPP);

	default:
		panic("udp_usrreq");
	}
release:
	if (m != (struct mbuf *)NULL)
		m_freem(m);
	return (error);
}
