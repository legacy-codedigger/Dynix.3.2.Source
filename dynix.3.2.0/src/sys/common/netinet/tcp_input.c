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
static	char	rcsid[] = "$Header: tcp_input.c 2.17 1991/09/26 21:06:30 $";
#endif

/*
 * tcp_input.c
 * 	TCP input routine, follows pages 65-76 of the
 * 	protocol specification dated September, 1981 very closely.
 */

/* $Log: tcp_input.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
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
#include "../netinet/tcp.h"
#include "../netinet/tcp_fsm.h"
#include "../netinet/tcp_seq.h"
#include "../netinet/tcp_timer.h"
#include "../netinet/tcp_var.h"
#include "../netinet/tcpip.h"
#include "../netinet/tcp_debug.h"

int	tcpprintfs = 0;		/* this should be binary-configurable */
int	tcprexmtthresh = 3;

struct	tcpiphdr tcp_saveti;

struct	tcpcb *tcp_newtcpcb();
/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 * Set DELACK for segments received in order, but ack immediately
 * when segments are out of order (so fast retransmit can work).
 */
#define	TCP_REASS(tp, ti, m, so, flags) { \
	if ((ti)->ti_seq == (tp)->rcv_nxt && \
	    (tp)->seg_next == (struct tcpiphdr *)(tp) && \
	    (tp)->t_state == TCPS_ESTABLISHED) { \
		tp->t_flags |= TF_DELACK; \
		(tp)->rcv_nxt += (ti)->ti_len; \
		flags = (ti)->ti_flags & TH_FIN; \
		tcpstat.tcps_rcvpack++;\
		tcpstat.tcps_rcvbyte += (ti)->ti_len;\
		sbappend(&(so)->so_rcv, (m)); \
		sorwakeup(so); \
	} else { \
		(flags) = tcp_reass((tp), (ti)); \
		tp->t_flags |= TF_ACKNOW; \
	} \
}

tcp_reass(tp, ti)
	register struct tcpcb *tp;
	register struct tcpiphdr *ti;
{
	register struct tcpiphdr *q;
	struct socket *so = tp->t_inpcb->inp_socket;
	struct mbuf *m;
	int flags;

	/*
	 * Call with ti==0 after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (ti != (struct tcpiphdr *)NULL) {
		/*
		 * Find a segment which begins after this one does.
		 */

		for (q = tp->seg_next; q != (struct tcpiphdr *)tp;
		     q = (struct tcpiphdr *)q->ti_next)
			if (SEQ_GT(q->ti_seq, ti->ti_seq))
				break;

		/*
		 * If there is a preceding segment, it may provide some of
		 * our data already.  If so, drop the data from the incoming
		 * segment.  If it provides all of our data, drop us.
		 */

		if ((struct tcpiphdr *)q->ti_prev != (struct tcpiphdr *)tp) {
			register int i;
			q = (struct tcpiphdr *)q->ti_prev;

			/* conversion to int (in i) handles seq wraparound */

			i = q->ti_seq + q->ti_len - ti->ti_seq;
			if (i > 0) {
				if (i >= ti->ti_len) {
					tcpstat.tcps_rcvduppack++;
					tcpstat.tcps_rcvdupbyte += ti->ti_len;
					m_freem(dtom(ti));
					return (0);
				}
				m_adj(dtom(ti), i);
				ti->ti_len -= i;
				ti->ti_seq += i;
			}
			q = (struct tcpiphdr *)(q->ti_next);
		}

		tcpstat.tcps_rcvoopack++;
		tcpstat.tcps_rcvoobyte += ti->ti_len;

		/*
		 * While we overlap succeeding segments trim them or,
		 * if they are completely covered, dequeue them.
		 */

		while (q != (struct tcpiphdr *)tp) {
			register int i = (ti->ti_seq + ti->ti_len) - q->ti_seq;
			if (i <= 0)
				break;
			if (i < q->ti_len) {
				q->ti_seq += i;
				q->ti_len -= i;
				m_adj(dtom(q), i);
				break;
			}
			q = (struct tcpiphdr *)q->ti_next;
			m = dtom(q->ti_prev);
			remque(q->ti_prev);
			m_freem(m);
		}

		/*
		 * Stick new segment in its place.
		 */

		insque(ti, q->ti_prev);
	}

	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */

	if (TCPS_HAVERCVDSYN(tp->t_state) == 0)
		return (0);
	ti = tp->seg_next;
	if (ti == (struct tcpiphdr *)tp || ti->ti_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == TCPS_SYN_RECEIVED && ti->ti_len)
		return (0);
	do {
		tp->rcv_nxt += ti->ti_len;
		flags = ti->ti_flags & TH_FIN;
		remque(ti);
		m = dtom(ti);
		ti = (struct tcpiphdr *)ti->ti_next;
		if (so->so_state & SS_CANTRCVMORE)
			m_freem(m);
		else
			sbappend(&so->so_rcv, m);
	} while (ti != (struct tcpiphdr *)tp && ti->ti_seq == tp->rcv_nxt);

	sorwakeup(so);
	return (flags);
}

/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
/*ARGSUSED*/
tcp_input(m0, ifp)
	struct mbuf *m0;
	struct ifnet *ifp;
{
	register struct tcpiphdr *ti;
	struct inpcb *inp;
	register struct mbuf *m;
	struct mbuf *om = 0;
	int len, tlen, off;
	register struct tcpcb *tp = 0;
	register int tiflags;
	struct socket *so;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate;
	struct in_addr laddr;
	int dropsocket = 0;
	int iss = 0;

	int debugit = 0;
	struct socket_peer *saved_sopp = 0;
	struct socket_peer *saved_partial_sopp = 0;
	int error;

	tcpstat.tcps_rcvtotal++;
	/*
	 * Get IP and TCP header together in first mbuf.
	 * Note: IP leaves IP header in first mbuf.
	 */

	m = m0;
	ti = mtod(m, struct tcpiphdr *);
	if (((struct ip *)ti)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ti, (struct mbuf *)0);
	if (m->m_off > MMAXOFF || m->m_len < sizeof (struct tcpiphdr)) {
		if ((m = m_pullup(m, sizeof (struct tcpiphdr))) == 0) {
			tcpstat.tcps_rcvshort++;
			return;
		}
		ti = mtod(m, struct tcpiphdr *);
	}

	/*
	 * Checksum extended TCP header and data.
	 */
	tlen = ((struct ip *)ti)->ip_len;
	len = sizeof (struct ip) + tlen;
	if (tcpcksumi) {

		/*
		 * The binary configurable value of tcpcksumi != 0
		 * means to compute input checksums.
		 * 4.2bsd uses the variable tcpcksum which is
		 * released == 0;
		 */

		ti->ti_next = ti->ti_prev = 0;
		ti->ti_x1 = 0;
		ti->ti_len = (u_short)tlen;
		ti->ti_len = htons((u_short)ti->ti_len);
		if (ti->ti_sum = in_cksum(m, len)) {
			if (tcpprintfs)
				printf("tcp sum: src %x\n", ti->ti_src);
			tcpstat.tcps_rcvbadsum++;
			goto drop;
		}
	}

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.
	 */
	off = ti->ti_off << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		if (tcpprintfs)
			printf("tcp off: src %x off %d\n", ti->ti_src, off);
		tcpstat.tcps_rcvbadoff++;
		goto drop;
	}

	tlen -= off;
	ti->ti_len = tlen;
	if (off > sizeof (struct tcphdr)) {
		if (m->m_len < sizeof(struct ip) + off) {
			if ((m = m_pullup(m, sizeof (struct ip) + off)) == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
			ti = mtod(m, struct tcpiphdr *);
		}
		om = m_get(M_DONTWAIT, MT_DATA);
		if (om == 0)
			goto drop;
		om->m_len = off - sizeof (struct tcphdr);
		{ caddr_t op = mtod(m, caddr_t) + sizeof (struct tcpiphdr);
		  bcopy(op, mtod(om, caddr_t), (unsigned)om->m_len);
		  m->m_len -= om->m_len;
		  bcopy(op+om->m_len, op,
		   (unsigned)(m->m_len-sizeof (struct tcpiphdr)));
		}
	}
	tiflags = ti->ti_flags;

	/*
	 * Convert TCP protocol specific fields to host format.
	 */
	ti->ti_seq = ntohl(ti->ti_seq);
	ti->ti_ack = ntohl(ti->ti_ack);
	ti->ti_win = ntohs(ti->ti_win);
	ti->ti_urp = ntohs(ti->ti_urp);

	/*
	 * Locate pcb for segment.
	 *
	 * in_pcblookup looks up an internet address in the inpcblist headed
	 * by head.  In this case head == &tcb which is an inpcb
	 * structure that serves as the head of the recognized internet 
	 * addresses of the tcp protocol engine.  in_pcblookup locks the list 
	 * while searching to guard against concurrent changes.
	 * When/if a pcb is found, it's sop_lock is acquired, it's
	 * sop_refcnt count is incremented and the pcb list is unlocked.
	 * NOTE the order of the locking is list head then list element.
	 * If a pcb is not found that is bound to the internet address,
	 * NULL is returned.  N.B. pcblookup spins on pcb lock.
	 */
findpcb:
	inp = in_pcblookup(tcb,
		ti->ti_src, ti->ti_sport, ti->ti_dst, ti->ti_dport,
		   INPLOOKUP_WILDCARD);

	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */

	if (inp == 0) 
		goto dropwithreset;

	/*
	 * pcb is found and port is active - inp_sopp is locked
	 *
	 * save pointer to socket_peer for refcnt managment in case
	 * the pcb goes away because of this input packet.
	 */
	saved_sopp = inp->inp_sopp;

	tp = intotcpcb(inp);
	if (tp == 0)
		goto dropwithreset;
	if (tp->t_state == TCPS_CLOSED)
		goto drop;
	so = inp->inp_socket;
	if (so == 0)
		goto dropwithreset;
	if (so->so_options & SO_DEBUG) {
		ostate = tp->t_state;
		tcp_saveti = *ti;
		debugit++;
	}

	/*
	 * The socket_peer is locked.  This mutex's user access to the
	 * bound socket.  The socket that
	 * is locked can be a LISTENing socket in which case a new socket
	 * is created to queue to it.  It is the LISTENing socket that is
	 * locked with the socket_peer of the pcb via in_pcblookup.
	 *
	 * sonewconn is called to create this new connect socket. It
	 * returns a *locked* new socket attached and ready to bind.
	 * The order of the locking is queue entry then so_head.
	 * NOTE: the LISTENing socket is locked, but a window
	 * was opened during which the user could have deleted it.
	 * Make sure the LISTENing socket still exists before fooling
	 * with it again (viz. soqinsque).
	 * (FURTHER NOTE: if the LISTENing socket is deleted during
	 * sonewconn, there is no new connection.)
	 */

	if (so->so_options & SO_ACCEPTCONN) {
		/*
		 * so is a LISTEN socket which is accepting
		 * connections.  These connections are created by
		 * sonewconn and queued to the LISTENing sockets so_q0.
		 *
		 * sonewconn creates a *new* socket structure, makes it look
		 * like the LISTENing socket except for ~SO_ACCEPTCONN.
		 * tcp_usrreq.ATTACH is called to attach a new inpcb.
		 * the socket is bound to an inpcb, but that inpcb is not
		 * a member of any list or hash chain (yet).
		 */

		so = sonewconn(so);
		if (so == 0)
			/*
			 * Failure here indicates either a lack of resources
			 * (e.g. mbufs), or the listen socket went away
			 * (sonewconn opens a window while doing tcp.PRU_ATTACH)
			 * If the listen socket went away we should
			 * really send a RST instead of simply dropping the
			 * packet.  But that makes things more convoluted than
			 * they need to be; the next retransmission
			 * of the SYN will generate the RST for us.
			 */
			goto drop;

		/*
		 * both the new socket and the LISTENing socket are locked
		 * at this point and the LISTENing socket exists.
		 *
		 * inp is now the *new* inpcb bound to the *new* socket 
		 * which is  locked.  Also tp is a *new* tcpcb.
		 * The socket is not known to any user process
		 * until the accept returns but it is queued on the
		 * LISTENing socket referenced by so_head.
		 */

		dropsocket++;
		inp = (struct inpcb *)so->so_pcb;

		saved_partial_sopp = inp->inp_sopp;
		saved_partial_sopp->sop_refcnt++;

		inp->inp_laddr = ti->ti_dst;
		inp->inp_lport = ti->ti_dport;
		inp->inp_options = ip_srcroute();
		/*
		 * At this point we have an inpcb which hasn't been inserted
		 * into any list yet (although it does point back to the
		 * head of the hash list).  Since we've finally given this
		 * pcb an address, put it into its final resting hash chain
		 */
		in_pcbins(inp);
		tp = intotcpcb(inp);
		tp->t_state = TCPS_LISTEN;
		/*
		 * This is bogus if it ever happens, but it can.  If we
		 * ever recieve a packet with both the SYN|FIN bits set,
		 * we will actually honor the FIN later on in tcp_input.
		 * Although BSD systems do the same thing, the MilSpec
		 * indicates that the FIN should (or at least can) be ignored.
		 * Why does this matter??  if we don't, we'll end up with a 
		 * socket on so_q0 in the close_wait state.  If a RST comes 
		 * in before the socket gets moved to so_q, we'll delete
		 * 1 reference to sop in in_pcbdetach, one when we exit this
		 * routine, leaving a sop with no references but a refcnt
		 * of 1.  By ignoring the fin, we stay in syn_rcvd state,
		 * for which if a RST comes in we correctly manage the sop
		 */
		tiflags &= ~TH_FIN;
	} else { /* not a new connect request, check for so_head */

		if (so->so_head) {
			
			/*
			 * this is a partial connection
			 *
			 * the pcb found in in_pcblookup (saved_sopp)
			 * is a partial connection whose refcnt includes
			 * this reference.  Save its sopp for later.
			 */

			SOLOCK(so->so_head);
			saved_partial_sopp = saved_sopp;
			saved_sopp = so->so_head->so_sopp;
			saved_sopp->sop_refcnt++;
			if (!(so->so_head->so_options & SO_ACCEPTCONN)) {
				/*
				 * head is disappearing soclose will
				 * soabort this so
			 	 */
				goto dropwithreset;
			}
		}
	}

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 *
	 * This goes for lookedup socket/pcb or new socket/pcb
	 * both of which are locked if appropriate.
	 *
	 * If it is a partial connection then the so_head is locked
	 * and it is still accepting connections (i.e. not in the
	 * process of being soclosed).
	 */
	tp->t_idle = 0;
	tp->t_timer[TCPT_KEEP] = tcp_keepidle;

	/*
	 * Process options if not in LISTEN state,
	 * else do it below (after getting remote address).
	 */

	if (om && tp->t_state != TCPS_LISTEN) {
		tcp_dooptions(tp, om, ti);
		om = 0;
	}

	/* 
	 * Header prediction: check for the two common cases
	 * of a uni-directional data xfer.  If the packet has
	 * no control flags, is in-sequence, the window didn't
	 * change and we're not retransmitting, it's a
	 * candidate.  If the length is zero and the ack moved
	 * forward, we're the sender side of the xfer.  Just
	 * free the data acked & wake any higher level process
	 * that was blocked waiting for space.  If the length
	 * is non-zero and the ack didn't move, we're the
	 * receiver side.  If we're getting packets in-order
	 * (the reassembly queue is empty), add the data to
	 * the socket buffer and note that we need a delayed ack.
	 */
	if (tp->t_state == TCPS_ESTABLISHED &&
	    (tiflags & (TH_SYN|TH_FIN|TH_RST|TH_URG|TH_ACK)) == TH_ACK &&
	    ti->ti_seq == tp->rcv_nxt &&
	    ti->ti_win && ti->ti_win == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max) {
		if (ti->ti_len == 0) {
			if (SEQ_GT(ti->ti_ack, tp->snd_una) &&
			    SEQ_LEQ(ti->ti_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_wnd) {
				/*
				 * this is a pure ack for outstanding data.
				 */
				if (tp->t_rtt && SEQ_GT(ti->ti_ack,tp->t_rtseq))
					tcp_xmit_timer(tp);
				acked = ti->ti_ack - tp->snd_una;
				tcpstat.tcps_rcvackpack++;
				tcpstat.tcps_rcvackbyte += acked;
				sbdrop(&so->so_snd, acked);
				tp->snd_una = ti->ti_ack;
				m_freem(m);

				/*
				 * If all outstanding data are acked, stop
				 * retransmit timer, otherwise restart timer
				 * using current (possibly backed-off) value.
				 * If process is waiting for space,
				 * wakeup/selwakeup/signal.  If data
				 * are ready to send, let tcp_output
				 * decide between more output or persist.
				 */
				if (tp->snd_una == tp->snd_max)
					tp->t_timer[TCPT_REXMT] = 0;
				else if (tp->t_timer[TCPT_PERSIST] == 0)
					tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;

				if (blocked_sema(
				     &so->so_snd.sb_sbx->sbx_buf_wait) ||
		    		     so->so_snd.sb_sel)
					sowwakeup(so);
				if (so->so_snd.sb_cc)
					(void) tcp_output(tp);
				tcp_drop_locks(saved_sopp, saved_partial_sopp);
				return;
			}
		} else if (ti->ti_ack == tp->snd_una &&
		    tp->seg_next == (struct tcpiphdr *)tp &&
		    ti->ti_len <= sbspace(&so->so_rcv)) {
			/*
			 * this is a pure, in-sequence data packet
			 * with nothing on the reassembly queue and
			 * we have enough buffer space to take it.
			 */
			tp->rcv_nxt += ti->ti_len;
			tcpstat.tcps_rcvpack++;
			tcpstat.tcps_rcvbyte += ti->ti_len;
			/*
			 * Drop TCP and IP headers then add data
			 * to socket buffer
			 */
			m->m_off += sizeof(struct tcpiphdr);
			m->m_len -= sizeof(struct tcpiphdr);
			sbappend(&so->so_rcv, m);
			sorwakeup(so);
			tp->t_flags |= TF_DELACK;
			tcp_drop_locks(saved_sopp, saved_partial_sopp);
			return;
		}
	}

	/*
	 * Drop TCP and IP headers; TCP options were dropped above.
	 */
	m->m_off += sizeof(struct tcpiphdr);
	m->m_len -= sizeof(struct tcpiphdr);

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is the amount of space in the rcv queue,
	 * but not less than advertised window.
	 */
	{ int win;

	win = sbspace(&so->so_rcv);
	if (win < 0)
		win = 0;
	tp->rcv_wnd = MAX(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}

	switch (tp->t_state) {

	/*
	 * If the state is LISTEN then ignore segment if it contains a RST.
	 * If the segment contains an ACK then it is bad and send a RST.
	 * If it does not contain a SYN then it is not interesting; drop it.
	 * If resources cannot be acquired, drop packet.
	 * Don't bother responding if the destination was a broadcast.
	 * Otherwise initialize tp->rcv_nxt, and tp->irs, select an initial
	 * tp->iss, and send a segment:
	 *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
	 * Also initialize tp->snd_nxt to tp->iss+1 and tp->snd_una to tp->iss.
	 * Fill in remote peer address fields if not previously specified.
	 * Enter SYN_RECEIVED state, and process any other fields of this
	 * segment in this state.
	 *
	 * LISTEN state can be entered with *two* locks held -
	 * the lock of the "parent" or Listen socket and the new socket
	 * created to fullfill the connect.
	 *
	 * Both so and so_head need to be locked to avoid simultaneous
	 * modification (e.g. deletion), therefore, if not a new socket
	 * acquire the so_head lock.
	 */
	case TCPS_LISTEN: {
		struct mbuf *am;
		register struct sockaddr_in *sin;

		/*
		 * Normally so is either a newly created
		 * socket, or one that was queued previously and
		 * not yet changed state.  lock the so_head and check it out.
		 * If this is a new socket, then head is already
		 * locked by sonewconn.  If the sonewconn failed, this
		 * is the parent listen socket that is undergoing
		 * deletion.  This is noted by the observation that
		 * the socket is in LISTEN state but not in any so_q.
		 */

		if (!(so->so_head)) {
			/*
			 * head was disappearing before we in_pcblooked it up.
			 * soclose finishes close
			 */
			goto dropwithreset;
		}

		/*
		 * LISTENing socket head is accepting connects
		 * It is locked (saved_sopp) and the partially
		 * connected socket is locked (saved_partial_sopp)
		 */

		if (tiflags & TH_RST)
			goto drop;
		if (tiflags & TH_ACK)
			goto dropwithreset;
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (in_broadcast(ti->ti_dst))
			goto drop;
		am = m_get(M_DONTWAIT, MT_SONAME);
		if (am == NULL)
			goto drop;
		am->m_len = sizeof (struct sockaddr_in);
		sin = mtod(am, struct sockaddr_in *);
		sin->sin_family = AF_INET;
		sin->sin_addr = ti->ti_src;
		sin->sin_port = ti->ti_sport;
		laddr = inp->inp_laddr;
		if (laddr.s_addr == INADDR_ANY)
			inp->inp_laddr = ti->ti_dst;

		/* 
		 * in_pcbconnect verifies that the
		 * connection is legitimate (e.g. this could be a
		 * retry at the connect attempt) and enters the tcb list. 
		 * in_pcbconnect expects the new socket/pcb to be locked
		 * but not the so_head - so unlock the so_head socket
		 * explicitly and check later for deletion.  During
		 * this WINDOW, everything can disappear but the
		 * socket_peers!  Note: that 
		 * the socket_peers have refcnts for these references since
		 * in_pcblookup does refcnt++ when it finds the LISTEN pcb
		 * or it is ++ above when it is locked and
		 * refcnt++ was done for new so after sonewconn.
		 */

		v_lock(&saved_sopp->sop_lock, SPLNET);
		error = in_pcbconnect(inp, am);

		/*
		 * in_pcbconnect returns with inp_sop locked, but inp could
		 * have disappeared.  relock the so_head for state check
		 */

		(void) p_lock(&saved_sopp->sop_lock, SPLNET);
		(void) m_free(am);

		if ((saved_sopp->sop_refcnt == 1) ||

			/*
			 * last reference => head gone => *both* are gone
			 */

		   (saved_partial_sopp->sop_refcnt == 1) ||

			/*
			 * inp disappeared without head going.
			 * Possible if timer does it.
			 */

		   !(so->so_head->so_options & SO_ACCEPTCONN)) {

			/*
			 * so_head is undergoing deletion, let it
			 * soclose this partial connection
			 */

			tcp_drop_locks(saved_sopp, saved_partial_sopp);
			m_freem(m);
			return;
		}

		if (error) {
			inp->inp_laddr = laddr;
			goto drop;
		}
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			tp = tcp_drop(tp, ENOBUFS);
			dropsocket = 0;		/* socket is already gone */
			goto drop;
		}
		if (om) {
			tcp_dooptions(tp, om, ti);
			om = 0;
		}

		if (iss)
			tp->iss = iss;
		else
			tp->iss = tcp_iss;
		tcp_iss += TCP_ISSINCR/2;
		tp->irs = ti->ti_seq;
		tcp_sendseqinit(tp);
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		tp->t_state = TCPS_SYN_RECEIVED;
		tp->t_timer[TCPT_KEEP] = tcp_keepinit;
		dropsocket = 0;		/* committed to socket */
		tcpstat.tcps_accepts++;
		goto trimthenstep6;
		}

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */

	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max)))
			goto dropwithreset;
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK)
				tp = tcp_drop(tp, ECONNREFUSED);
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = ti->ti_ack;
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
		}
		tp->t_timer[TCPT_REXMT] = 0;
		tp->irs = ti->ti_seq;
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			tcpstat.tcps_connects++;
			soisconnected(so);
			tp->t_state = TCPS_ESTABLISHED;
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			(void) tcp_reass(tp, (struct tcpiphdr *)0);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtt)
				tcp_xmit_timer(tp);
		} else
			tp->t_state = TCPS_SYN_RECEIVED;

trimthenstep6:
		/*
		 * Advance ti->ti_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		ti->ti_seq++;
		if (ti->ti_len > tp->rcv_wnd) {
			todrop = ti->ti_len - tp->rcv_wnd;
			m_adj(m, -todrop);
			ti->ti_len = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			tcpstat.tcps_rcvpackafterwin++;
			tcpstat.tcps_rcvbyteafterwin += todrop;
		}
		tp->snd_wl1 = ti->ti_seq - 1;
		tp->rcv_up = ti->ti_seq;
		goto step6;
	}	/* this is the end of the switch --------------------- */

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check that at least some bytes of segment are within 
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */
	todrop = tp->rcv_nxt - ti->ti_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			ti->ti_seq++;
			if (ti->ti_urp > 1) 
				ti->ti_urp--;
			else
				tiflags &= ~TH_URG;
			todrop--;
			if (ti->ti_flags & TH_ACK && 
			    tp->t_state == TCPS_SYN_RECEIVED &&
			    tp->t_flags & TF_ACTOPEN &&
			    tp->rcv_nxt == ti->ti_seq) {
				goto crossingsyn;
			}
		}
		if (todrop > ti->ti_len ||
		    todrop == ti->ti_len && (tiflags&TH_FIN) == 0) {
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += ti->ti_len;
			/*
			 * If segment is just one to the left of the window,
			 * check two special cases:
			 * 1. Don't toss RST in response to 4.2-style keepalive.
			 * 2. If the only thing to drop is a FIN, we can drop
			 *	it, but check the ACK or we will get into FIN
			 *	wars if our FINs crossed (both CLOSING).
			 * In either case, send ACK to resynchronize,
			 * but keep on processing for RST or ACK.
			 */
			if ((tiflags & TH_FIN && todrop == ti->ti_len + 1)
#ifndef	TCP_COMPAT_42
			  || (tiflags & TH_RST && ti->ti_seq == tp->rcv_nxt - 1)
#endif
			   ) {
				todrop = ti->ti_len;
				tiflags &= ~TH_FIN;
				tp->t_flags |= TF_ACKNOW;
			} else
				goto dropafterack;
		} else {
			tcpstat.tcps_rcvpartduppack++;
			tcpstat.tcps_rcvpartdupbyte += todrop;
		}
		m_adj(m, todrop);
		ti->ti_seq += todrop;
		ti->ti_len -= todrop;
		if (ti->ti_urp > todrop)
			ti->ti_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			ti->ti_urp = 0;
		}
	}
crossingsyn:

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && ti->ti_len) {
		tp = tcp_close(tp);
		tcpstat.tcps_rcvafterclose++;
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data
	 * (and PUSH and FIN); if nothing left, just ACK.
	 */
	todrop = (ti->ti_seq+ti->ti_len) - (tp->rcv_nxt+tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat.tcps_rcvpackafterwin++;
		if (todrop >= ti->ti_len) {
			tcpstat.tcps_rcvbyteafterwin += ti->ti_len;
			/*
			 * If a new connection request is received
			 * while in TIME_WAIT, drop the old connection
			 * and start over if the sequence numbers
			 * are above the previous ones.
			 */
			if (tiflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(ti->ti_seq, tp->rcv_nxt)) {
				iss = tp->rcv_nxt + TCP_ISSINCR;
				(void) tcp_close(tp);
				tcp_drop_locks(saved_sopp, saved_partial_sopp);
				goto findpcb;
			}
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments. Continue processing, but
			 * remember to ack. Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && ti->ti_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_rcvwinprobe++;
			} else
				goto dropafterack;
		} else
			tcpstat.tcps_rcvbyteafterwin += todrop;
		m_adj(m, -todrop);
		ti->ti_len -= todrop;
		tiflags &= ~(TH_PUSH|TH_FIN);
	}

	/*
	 * If the RST bit is set examine the state:
	 *    SYN_RECEIVED STATE:
	 *	If passive open, return to LISTEN state.
	 *	If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	 *	Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT STATES
	 *	Close the tcb.
	 */

	/*
	 * if socket has so_head, then *both* are locked
 	 * so_head is still accepting connections.
 	 */

	if (tiflags&TH_RST) switch (tp->t_state) {

	case TCPS_SYN_RECEIVED:
		so->so_error = ECONNREFUSED;
		/*
		 * in dropping the connection, we also blew away the
		 * socket on so_q0.  Since it was created with 2 
		 * references, and in_pcbdetach decremented one of
		 * them, and since there never was a file descriptor
		 * referencing it, we decrement sop_refcnt here
		 * to account for the socket that never was.
		 */
		saved_partial_sopp->sop_refcnt--;
		goto close;

	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
		so->so_error = ECONNRESET;
	close:
		tp->t_state = TCPS_CLOSED;
		tcpstat.tcps_drops++;
		tp = tcp_close(tp);
		goto drop;

	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
		tp = tcp_close(tp);
		goto drop;
	}

	/*
	 * If a SYN is in the window, then this is an
	 * error and we send a RST and drop the connection.
	 */
	if (tiflags & TH_SYN) {
		tp = tcp_drop(tp, ECONNRESET);
		goto dropwithreset;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0)
		goto drop;
	
	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

	/*
	 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	 * ESTABLISHED state and continue processing, otherwise
	 * send a RST.
	 */
	case TCPS_SYN_RECEIVED:

		if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
		    SEQ_GT(ti->ti_ack, tp->snd_max))
			goto dropwithreset;
		tcpstat.tcps_connects++;
		soisconnected(so);
		tp->t_state = TCPS_ESTABLISHED;
		tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
		(void) tcp_reass(tp, (struct tcpiphdr *)0);
		tp->snd_wl1 = ti->ti_seq - 1;

		/*
		 * now, theoretically, anything can happen on the new
		 * connection that can happen on an established connection.  
		 */

		/* fall into ... */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < ti->ti_ack <= tp->snd_max
	 * then advance tp->snd_una to ti->ti_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */

	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:

		if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
			if (ti->ti_len == 0 && ti->ti_win == tp->snd_wnd) {
				tcpstat.tcps_rcvdupack++;
				/*
				 * If we have outstanding data (not a
				 * window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.  If this packet fills the
				 * only hole in the receiver's seq.
				 * space, the next real ack will fully
				 * open our window.  This means we
				 * have to do the usual slow-start to
				 * not overwhelm an intermediate gateway
				 * with a burst of packets.  Leave
				 * here with the congestion window set
				 * to allow 2 packets on the next real
				 * ack and the exp-to-linear thresh
				 * set for half the current window
				 * size (since we know we're losing at
				 * the current window size).
				 */
				if (tp->t_timer[TCPT_REXMT] == 0 ||
				    ti->ti_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;
					u_int win =
					    MIN(tp->snd_wnd, tp->snd_cwnd) / 2 /
						tp->t_maxseg;

					if (win < 2)
						win = 2;
					tp->snd_ssthresh = win * tp->t_maxseg;

					tp->t_timer[TCPT_REXMT] = 0;
					tp->t_rtt = 0;
					tp->snd_nxt = ti->ti_ack;
					tp->snd_cwnd = tp->t_maxseg;
					(void) tcp_output(tp);

					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}
		tp->t_dupacks = 0;
		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			tcpstat.tcps_rcvacktoomuch++;
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		tcpstat.tcps_rcvackpack++;
		tcpstat.tcps_rcvackbyte += acked;

		/*
		 * If transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq)) {
			tcpstat.tcps_rttupdated++;
			if (tp->t_srtt != 0) {
				register short delta;

				/*
				 * srtt is stored as fixed point with 3 bits
				 * after the binary point (i.e., scaled by 8).
				 * The following magic is equivalent
				 * to the smoothing algorithm in rfc793
				 * with an alpha of .875
				 * (srtt = rtt/8 + srtt*7/8 in fixed point).
				 * Adjust t_rtt to origin 0.
				 */
				delta = tp->t_rtt - 1 - (tp->t_srtt >> 3);
				if ((tp->t_srtt += delta) <= 0)
					tp->t_srtt = 1;
				/*
				 * We accumulate a smoothed rtt variance
				 * (actually, a smoothed mean difference),
				 * then set the retransmit timer to smoothed
				 * rtt + 2 times the smoothed variance.
				 * rttvar is stored as fixed point
				 * with 2 bits after the binary point
				 * (scaled by 4).  The following is equivalent
				 * to rfc793 smoothing with an alpha of .75
				 * (rttvar = rttvar*3/4 + |delta| / 4).
				 * This replaces rfc793's wired-in beta.
				 */
				if (delta < 0)
					delta = -delta;
				delta -= (tp->t_rttvar >> 2);
				if ((tp->t_rttvar += delta) <= 0)
					tp->t_rttvar = 1;
			} else {
				/* 
				 * No rtt measurement yet - use the
				 * unsmoothed rtt.  Set the variance
				 * to half the rtt (so our first
				 * retransmit happens at 2*rtt)
				 */
				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
			}
			tp->t_rtt = 0;
			tp->t_rxtshift = 0;
			TCPT_RANGESET(tp->t_rxtcur, 
			    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
			    TCPTV_MIN, TCPTV_REXMTMAX);
		}

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			needoutput = 1;
		} else if (tp->t_timer[TCPT_PERSIST] == 0)
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (maxseg per packet).
		 * Otherwise open linearly (maxseg per window,
		 * or maxseg^2 / cwnd per packet).
		 */
		{
		u_int incr = tp->t_maxseg;

		if (tp->snd_cwnd > tp->snd_ssthresh)
			incr = MAX(incr * incr / tp->snd_cwnd, 1);

		tp->snd_cwnd = MIN(tp->snd_cwnd + incr, TCP_MAXWIN);
		}
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int) (so->so_snd.sb_cc));
			ourfinisacked = 1;
		} else {
			sbdrop(&so->so_snd, acked);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}

		/*
		 * If there is a process waiting or selecting,
		 * this is where user is awakened to transmit data.
		 */
		if (blocked_sema(&so->so_snd.sb_sbx->sbx_buf_wait) ||
		    so->so_snd.sb_sel)
			sowwakeup(so);
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */

		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {

				/*
				 * If we can't receive any more
				 * data, then closing user can proceed.
				 * Starting the timer is contrary to the
				 * specification, but if we don't get a FIN
				 * we'll hang forever.
				 */
				if (so->so_state & SS_CANTRCVMORE) {
					soisdisconnected(so);
					tp->t_timer[TCPT_2MSL] = tcp_maxidle;
				}
				tp->t_state = TCPS_FIN_WAIT_2;
			}
			break;

	 	/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */

		case TCPS_CLOSING:
			if (ourfinisacked) {
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				tp->t_timer[TCPT_2MSL] = tp->t_2mslval;
				soisdisconnected(so);
			}
			break;

		/*
		 * The only thing that can arrive in  LAST_ACK state
		 * is an acknowledgment of our FIN.  If our FIN is now
		 * acknowledged, delete the TCB, enter the closed state
		 * and return.
		 */

		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp = tcp_close(tp);
				goto drop;
			}
			break;

		/*
		 * In TIME_WAIT state the only thing that should arrive
		 * is a retransmission of the remote FIN.  Acknowledge
		 * it and restart the finack timer.
		 */

		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = tp->t_2mslval;
			goto dropafterack;
		}
	}

step6:
	/*
	 * Update window information.
	 * Don't look at window if no ACK: some send garbage on first SYN.
	 */

	if ((tiflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, ti->ti_seq) || tp->snd_wl1 == ti->ti_seq &&
	    (SEQ_LT(tp->snd_wl2, ti->ti_ack) ||
	     tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd)
			tcpstat.tcps_rcvwinupd++;
		tp->snd_wnd = ti->ti_win;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}

	/*
	 * Process segments with URG.
	 */

	if ((tiflags & TH_URG) && ti->ti_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {

		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data (sic 4.2).
		 */
		if (((int) ti->ti_urp + so->so_rcv.sb_cc) > SB_MAX) {
			ti->ti_urp = 0;			/* XXX */
			tiflags &= ~TH_URG;		/* XXX */
			goto dodata;			/* XXX */
		}

		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since
		 * a FIN has been received from the remote side. 
		 * In these states we ignore the URG.
		 *
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section
		 * as the original spec states.
		 */
		if (SEQ_GT(ti->ti_seq+ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			so->so_oobmark = so->so_rcv.sb_cc +
			    (tp->rcv_up - tp->rcv_nxt) - 1;
			if (so->so_oobmark == 0)
				so->so_state |= SS_RCVATMARK;
			sohasoutofband(so);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}

		/*
		 * Remove out of band data so doesn't get presented to user.
		 * This can happen independent of advancing the URG pointer,
		 * but if two URG's are pending at once, some out-of-band
		 * data may creep in... ick.
		 */
		if (ti->ti_urp <= ti->ti_len &&
		    (so->so_options & SO_OOBINLINE) == 0)
			tcp_pulloutofband(so, ti);
	} else
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;

dodata:

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */

	if ((ti->ti_len || (tiflags&TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		TCP_REASS(tp, ti, m, so, tiflags);
		if (tcpnodelack != 0 && (tp->t_flags & TF_DELACK))	
			tp->t_flags |= TF_ACKNOW;
		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 */
		len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
		if (len > tp->max_rcvd)
			tp->max_rcvd = len;
	} else {
		m_freem(m);
		tiflags &= ~TH_FIN;
	}

	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.
	 */

	if (tiflags & TH_FIN) {
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			socantrcvmore(so);
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {

	 	/*
		 * In SYN_RECEIVED and ESTABLISHED STATES
		 * enter the CLOSE_WAIT state.
		 */

		case TCPS_SYN_RECEIVED:
		case TCPS_ESTABLISHED:
			tp->t_state = TCPS_CLOSE_WAIT;
			break;

	 	/*
		 * If still in FIN_WAIT_1 STATE FIN has not been acked so
		 * enter the CLOSING state.
		 */

		case TCPS_FIN_WAIT_1:
			tp->t_state = TCPS_CLOSING;
			break;

	 	/*
		 * In FIN_WAIT_2 state enter the TIME_WAIT state,
		 * starting the time-wait timer, turning off the other 
		 * standard timers.
		 */

		case TCPS_FIN_WAIT_2:
			tp->t_state = TCPS_TIME_WAIT;
			tcp_canceltimers(tp);
			tp->t_timer[TCPT_2MSL] = tp->t_2mslval;
			soisdisconnected(so);
			break;

		/*
		 * In TIME_WAIT state restart the 2 MSL time_wait timer.
		 */

		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = tp->t_2mslval;
			break;
		}
	}

	if (debugit)
		tcp_trace(TA_INPUT, ostate, tp, &tcp_saveti, 0);

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW))
		(void) tcp_output(tp);

	/* 
	 * finished with pcb - release socket_peer lock(s) 
	 */
	tcp_drop_locks(saved_sopp, saved_partial_sopp);
	return;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);

	/* 
	 * finished with pcb - release socket_peer lock(s) 
	 */
	tcp_drop_locks(saved_sopp, saved_partial_sopp);
	return;

dropwithreset:
	if (om) {
		(void) m_free(om);
		om = 0;
	}
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 * Don't bother to respond if destination was broadcast.
	 */
	if ((tiflags & TH_RST) || in_broadcast(ti->ti_dst))
		goto drop;
	if (tiflags & TH_ACK)
		tcp_respond(tp, ti, (tcp_seq)0, ti->ti_ack, TH_RST);
	else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		tcp_respond(tp, ti, ti->ti_seq+ti->ti_len, (tcp_seq)0,
		    TH_RST|TH_ACK);
	}
	if (dropsocket) {
		/* destroy temporarily created socket
		 *
		 * so_head is still accepting connections.
		 * if it is a new socket take it out of the
		 * so_head so_q0 queue and abort it
		 *
		 * newly created socket - backout
		 * by remqueing from so_head's so_q0.
		 */

		(void) soqremque(so, 0);

		/*
		 * unlock head to avoid deadlock with
		 * pcb list management.  Also
		 * dereference sop so soabort gets it.
		 */

		saved_sopp->sop_refcnt--;
		v_lock(&saved_sopp->sop_lock, SPLNET);
		saved_sopp = (struct socket_peer *)NULL;

		/* make tp NULL to avoid tcp_trace */

		tp = (struct tcpcb *)NULL;
		(void) soabort(so);
	}
	tcp_drop_locks(saved_sopp, saved_partial_sopp);
	/* tcp_respond() freed m vi ti */
	return;

drop:
	if (dropsocket) {

		/* destroy temporarily created socket
		 *
		 * so_head is still accepting connections.
		 * if it is a new socket take it out of the
		 * so_head so_q0 queue and abort it
		 *
		 * newly created socket - backout
		 * by remqueing from so_head's so_q0.
		 */

		(void) soqremque(so, 0);

		/*
		 * unlock head to avoid deadlock with
		 * pcb list management.  Also
		 * dereference sop so soabort gets it.
		 */

		saved_sopp->sop_refcnt--;
		v_lock(&saved_sopp->sop_lock, SPLNET);
		saved_sopp = (struct socket_peer *)NULL;

		/* make tp NULL to avoid tcp_trace */

		tp = (struct tcpcb *)NULL;
		(void) soabort(so);
	}
	tcp_drop_locks(saved_sopp, saved_partial_sopp);
	if (om)
		(void) m_free(om);
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_DROP, ostate, tp, &tcp_saveti, 0);
	m_freem(m);
	return;

}

tcp_dooptions(tp, om, ti)
	struct tcpcb *tp;
	struct mbuf *om;
	struct tcpiphdr *ti;
{
	register u_char *cp;
	int opt, optlen, cnt;

	cp = mtod(om, u_char *);
	cnt = om->m_len;
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			break;

		case TCPOPT_MAXSEG:
			if (optlen != 4)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			tp->t_maxseg = *(u_short *)(cp + 2);
			tp->t_maxseg = ntohs((u_short)tp->t_maxseg);
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			break;
		}
	}
	(void) m_free(om);
}

/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */

tcp_pulloutofband(so, ti)
	struct socket *so;
	struct tcpiphdr *ti;
{
	register struct mbuf *m;
	int cnt = ti->ti_urp - 1;
	
	m = dtom(ti);
	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, caddr_t) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			bcopy(cp+1, cp, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == (struct mbuf *)NULL)
			break;
	}
	panic("tcp_pulloutofband");
}

/*
 *  Determine a reasonable value for maxseg size.
 *  If the route is known, use one that can be handled
 *  on the given interface without forcing IP to fragment.
 *  If bigger than an mbuf cluster (MCLBYTES), round down to nearest size
 *  to utilize large mbufs.
 *  If interface pointer is unavailable, or the destination isn't local,
 *  use a conservative size (512 or the default IP max size, but no more
 *  than the mtu of the interface through which we route),
 *  as we can't discover anything about intervening gateways or networks.
 *  We also initialize the congestion/slow start window to be a single
 *  segment if the destination isn't local; this information should
 *  probably all be saved with the routing entry at the transport level.
 *
 *  This is ugly, and doesn't belong at this level, but has to happen somehow.
 */
tcp_mss(tp)
	register struct tcpcb *tp;
{
	struct route *ro;
	struct ifnet *ifp;
	int mss;
	struct inpcb *inp;

	inp = tp->t_inpcb;
	ro = &inp->inp_route;
	if ((ro->ro_rt == (struct rtentry *)0) ||
	    (ifp = ro->ro_rt->rt_ifp) == (struct ifnet *)0) {
		/* No route yet, so try to acquire one */
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				inp->inp_faddr;
			rtalloc(ro);
		}
		if ((ro->ro_rt == 0) || (ifp = ro->ro_rt->rt_ifp) == 0)
			return (TCP_MSS);
	}

	mss = ifp->if_mtu - sizeof(struct tcpiphdr);
	if (mss > MCLBYTES)
		mss = mss / MCLBYTES * MCLBYTES;
	if (in_localaddr(inp->inp_faddr))
		return (mss);

	mss = MIN(mss, TCP_MSS);
	tp->snd_cwnd = mss;
	return (mss);
}

tcp_drop_locks(saved_sopp, saved_partial_sopp)
	struct socket_peer * saved_sopp;
	struct socket_peer * saved_partial_sopp;
{
	if (saved_sopp) {
		if (--(saved_sopp->sop_refcnt) == 0)
			(void) m_free(dtom(saved_sopp));
		else {
			v_lock(&saved_sopp->sop_lock, SPLNET);
		}
	}
	if (saved_partial_sopp) {
		if (--(saved_partial_sopp->sop_refcnt) == 0)
			(void) m_free(dtom(saved_partial_sopp));
		else {
			v_lock(&saved_partial_sopp->sop_lock, SPLNET);
		}
	}
	return;
}

/*
 * Collect new round-trip time estimate
 * and update averages and current timeout.
 */
tcp_xmit_timer(tp)
	register struct tcpcb *tp;
{
	tp->t_srtt = tp->t_rtt << 3;
	tp->t_rttvar = tp->t_rtt << 1;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->t_rtt = 0;
}
