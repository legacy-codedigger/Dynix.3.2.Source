/* $Copyright:	$
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
static	char	rcsid[] = "$Header: tcp_subr.c 2.10 91/03/11 $";
#endif

/*
 * tcp_subr.c
 *
 */

/* $Log:	tcp_subr.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"

#include "../net/route.h"
#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#include "../netinet/ip_var.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/tcp.h"
#include "../netinet/tcp_fsm.h"
#include "../netinet/tcp_seq.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"

/*
 * Tcp initialization
 */

lock_t tcp_dblock;

/*
 * NOTE: tcp_iss and tcpsenderrors are global variables that are
 * modified without mutual exclusion.  tcp_iss is used as the initial
 * sequence number for a virtual circuit.  It needn't be other than
 * random so mutex is not an issue.  tcpsenderrors is used to count how
 * many tcp_output errors tcp_usrreq.PRU_SEND encounters.
 */

tcp_seq	tcp_iss;		/* tcp initial send seq # */
struct inhcomm tcb_comm;	/* inpcb hash head common structure */

tcp_init()
{
	tcp_iss = 1;		/* wrong (sic 4.2) */
	in_pcbhashinit(tcb, &tcb_comm);
	init_lock(&tcp_dblock, G_PCB);

	return(0);
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
struct tcpiphdr *
tcp_template(tp)
	struct tcpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register struct mbuf *m;
	register struct tcpiphdr *n;

	if ((n = tp->t_template) == (struct tcpiphdr *) NULL) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == (struct mbuf *) NULL)
			return ((struct tcpiphdr *) NULL);
		m->m_off = MMAXOFF - sizeof (struct tcpiphdr);
		m->m_len = sizeof (struct tcpiphdr);
		n = mtod(m, struct tcpiphdr *);
	}
	n->ti_next = n->ti_prev = 0;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof (struct tcpiphdr) - sizeof (struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return (n);
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If flags==0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */

tcp_respond(tp, ti, ack, seq, flags)
	struct tcpcb *tp;
	register struct tcpiphdr *ti;
	tcp_seq ack, seq;
	int flags;
{
	struct mbuf *m;
	int win = 0, tlen;
	struct route *ro = (struct route *) NULL;

	if (tp) {
		win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
		if(win < 0)			/* XXX */
			win = 0;
		ro = &tp->t_inpcb->inp_route;
	}
	if (flags == 0) {
		m = m_get(M_DONTWAIT, MT_HEADER);
		if (m == (struct mbuf *) NULL)
			return;
		m->m_len = sizeof (struct tcpiphdr) + 1;
		*mtod(m, struct tcpiphdr *) = *ti;
		ti = mtod(m, struct tcpiphdr *);
		flags = TH_ACK;
		tlen = 1;
	} else {
		m = dtom(ti);
		m_freem(m->m_next);
		m->m_next = (struct mbuf *) NULL;
		m->m_off = (int)ti - (int)m;
		m->m_len = sizeof (struct tcpiphdr);

#define xchg(a,b,type) { type t; t=a; a=b; b=t; }

		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, u_long);
		xchg(ti->ti_dport, ti->ti_sport, u_short);

#undef xchg

		tlen = 0;
	}
	ti->ti_next = ti->ti_prev = 0;
	ti->ti_x1 = 0;
	ti->ti_len = htons((u_short)(sizeof (struct tcphdr) + tlen));
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof (struct tcphdr) >> 2;
	ti->ti_flags = flags;
	ti->ti_win = htons((u_short)win);
	ti->ti_urp = 0;

	/*
	 * zero the sum store *before* including it in the
	 * in_cksum calculation.
	 */

	ti->ti_sum = 0;

	if(tcpcksumo) {
		ti->ti_sum = in_cksum(m, sizeof (struct tcpiphdr) + tlen);
	}
	((struct ip *)ti)->ip_len = sizeof (struct tcpiphdr) + tlen;
	((struct ip *)ti)->ip_ttl = tcp_ttl;
	(void) ip_output(m, (struct mbuf *) NULL, ro, 0);
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */

struct tcpcb *
tcp_newtcpcb(inp)
	struct inpcb *inp;
{
	struct mbuf *m = m_getclr(M_DONTWAIT, MT_PCB);
	register struct tcpcb *tp;

	if (m == (struct mbuf *) NULL)
		return ((struct tcpcb *) NULL);
	tp = mtod(m, struct tcpcb *);
	tp->seg_next = tp->seg_prev = (struct tcpiphdr *)tp;
	tp->t_maxseg = TCP_MSS;
	tp->t_flags = 0;		/* sends options! */
	tp->t_inpcb = inp;
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = TCPTV_SRTTDFLT << 2;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = TCP_MAXWIN;
	tp->snd_ssthresh = TCP_MAXWIN;
	tp->t_2mslval = 2 * TCPTV_MSL;
	inp->inp_ppcb = (caddr_t)tp;
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */

struct tcpcb *
tcp_drop(tp, errno)
	register struct tcpcb *tp;
	int errno;
{
	struct socket *so = tp->t_inpcb->inp_socket;

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		tcpstat.tcps_drops++;
	} else {
		tcpstat.tcps_conndrops++;
	}
	so->so_error = errno;
	return (tcp_close(tp));
}

struct tcpcb* timer_tcp_close();

#define DODROP			/* for analysis of dropped packets */
#undef DODROP			/* for analysis of dropped packets */
#ifdef DODROP			/* defined in ../netinet/tcp_input.c */
extern short tcpdrops[64];		/* keep stats on drop codes */
#define DROPIT(x) tcpdrops[(x)]++;
#else
#define DROPIT(x) {;}
#endif DODROP

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 *
 *	called with the inpcb socket_peer held
 *	after discarding inpcb, UNLOCKING the socket_peer is inappropriate
 */
struct tcpcb *
tcp_close(tp)
	register struct tcpcb *tp;
{
	register struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	register struct mbuf *m;
	struct socket_peer * saved_sopp = (struct socket_peer *) NULL;

	t = tp->seg_next;
	while (t != (struct tcpiphdr *)tp) {
		t = (struct tcpiphdr *)t->ti_next;
		m = dtom(t->ti_prev);
		remque(t->ti_prev);
		m_freem(m);
	}
	if (tp->t_template)
		(void) m_free(dtom(tp->t_template));

	soisdisconnected(so);

	if(so->so_head) {

		/*
		 * This is a partial connection - either remque it to avoid
		 * simultaneous deletion, or leave it in the accept queue and
		 * let soclose eventually in_pcbdetach.  If so_head, tcp_input
		 * has locked *both* of the sockets - therefore must unlock
		 * the head before in_pcbdetach or risk deadlock.
		 */

		saved_sopp = so->so_head->so_sopp;

		if(!(so->so_head->so_options & SO_ACCEPTCONN)) {

			/*
			 * so_head is no longer accepting connections
			 * so soclose will soabort this socket.
			 */

			DROPIT(45);
			return ((struct tcpcb *) NULL);
		}

		/*
		 * still accepting connections
		 */

		if(tp->t_state < TCPS_ESTABLISHED) { /* not off of so_q0 yet */
			(void) soqremque(so, 0);
			DROPIT(46);
		}else{
			/*
			 * must be on so_q so let soclose eventually
			 * soabort it.  Should not see this tp again.
			 */

			DROPIT(47);
		}

		v_lock(&saved_sopp->sop_lock, SPLNET);
	}

	/*
	 * can now release tcpcb
	 */

	(void) m_free(dtom(tp));
	inp->inp_ppcb = (caddr_t) NULL;
	in_pcbdetach(inp);

	/*
	 * if this was a partial connection, re_lock the so_head
	 * so tcp_input can release it - this may be superfluous.
	 */

	if(saved_sopp) {
		DROPIT(43);
		(void) p_lock(&saved_sopp->sop_lock, SPLNET);
	}
	tcpstat.tcps_closed++;
	return ((struct tcpcb *) NULL);
}

/*
 * tcp_drain() - not much to this routine, eh?
 */

tcp_drain()
{

}

/*
 * Notify a tcp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
tcp_notify(inp)
	register struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;

	(void) vall_sema(so->so_conn_semap);
	sorwakeup(so);
	sowwakeup(so);
}

tcp_ctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	extern u_char inetctlerrmap[];
	struct sockaddr_in *sin;
	int tcp_quench(), in_rtchange();

	if ((unsigned)cmd > PRC_NCMDS)
		return;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_IMPLINK)
		return;
	sin = (struct sockaddr_in *)sa;
	if (sin->sin_addr.s_addr == INADDR_ANY)
		return;

	switch (cmd) {

	case PRC_QUENCH:
		in_pcbnotify(tcb, &sin->sin_addr, 0, tcp_quench);
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		in_pcbnotify(tcb, &sin->sin_addr, 0, in_rtchange);
		break;

	default:
		if (inetctlerrmap[cmd] == 0)
			return;		/* XXX */
		in_pcbnotify(tcb, &sin->sin_addr, (int)inetctlerrmap[cmd],
			tcp_notify);
	}
}

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
tcp_quench(inp)
	struct inpcb *inp;
{
	struct tcpcb *tp = intotcpcb(inp);

	if (tp)
		tp->snd_cwnd = tp->t_maxseg;
}
