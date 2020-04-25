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
static	char	rcsid[] = "$Header: tcp_timer.c 2.7 1991/09/26 21:06:38 $";
#endif

/*
 * tcp_timer.c
 *
 */

/* $Log: tcp_timer.c,v $
 *
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

struct tcpcb* timer_tcp_close();
struct tcpcb* timer_tcp_drop();

int tcp_maxidle;

/*
 * Fast timeout routine for processing delayed acks
 * delayed acks allow the tcp engine to wait in order to "piggy
 * back" an acknowledgement onto a subsequent data packet by
 * having it wait a short while before acking.  When a data
 * packet comes along, we ack.
 */

tcp_fasttimo()
{
	register struct inpcb *inp;
	register struct tcpcb *tp;
	register spl_t splevel;
	register int i;

	/*
	 * Search through tcb's and update active timers.
	 * NOTE: list stays locked for entire scan!!
	 * this blocks users from insque/remque and
	 * tcp_input from looking for an entry.
	 */

	for (i = 0; i < INP_HASHSZ; i++) {
		splevel = INP_LOCK(&tcb[i]);	/* lock the tcb list */
		inp = tcb[i].inp_next;

		for (; inp != &tcb[i]; inp = inp->inp_next) {

		     /*
		      * use a conditional p_lock.  This is cheaper than 
		      * spinning, and is OK since generally, ownership of the 
		      * lock implies state change with respect to the timers.
		      * (This might not * be the case in extreme cases where 
		      * there are many sibling processes sharing the socket 
		      * and colliding on requests).
		      * NOTE BENE: inp_next cannot change while the list is
		      * locked.  This is done around insque/remque.
		      */

		  if (cp_lock(&inp->inp_sopp->sop_lock, SPLNET) != CPLOCKFAIL) {
		     if(inp->inp_head) {	 /* not being detached */

			     if ((tp = (struct tcpcb *)inp->inp_ppcb) &&
			     (tp->t_flags & TF_DELACK)) {

				     tp->t_flags &= ~TF_DELACK;
				     tp->t_flags |= TF_ACKNOW;
				     tcpstat.tcps_delack++;

				     /*
				      * might not be too bad since no additional
				      * resources required and network output
				      * driver is simply behind a queue.
				      */

				     (void) tcp_output(tp);
			     }
		     }
		     INP_UNLOCK(inp, SPLNET);
		  }
		}	/* end for inp */
		INP_UNLOCK(&tcb[i], splevel);
	}
}

/*
 * Tcp protocol timeout routine called every 500 ms.
 * Updates the timers in all active tcb's and
 * causes finite state machine actions if timers expire.
 */

tcp_slowtimo()
{
    register struct inpcb *ip, *ipnxt;
    register struct tcpcb *tp;
    register int i;
    register int j;

    register struct socket_peer *saved_sopp;
    spl_t splevel;

    /*
     * Search through tcb's and update active timers.
     * NOTE: list stays locked for entire scan through a hash line!!
     * this blocks users from insque/remque and
     * tcp_input from looking for an entry.
     */

    tcp_maxidle = tcp_keepcnt * tcp_keepintvl;
    for (j = 0; j < INP_HASHSZ; j++) {
	splevel = INP_LOCK(&tcb[j]);	/* lock the tcb hash line */

	ip = tcb[j].inp_next;

	while (ip != &tcb[j]) {

		/*
		 * use a conditional p_lock.  This is cheaper than spinning,
		 * and is OK since generally, ownership of the lock implies
		 * state change with respect to the timers.  (This might not
		 * be the case in extreme cases where there are many sibling
		 * processes sharing the socket and colliding on requests).
		 * NOTE BENE: inp_next cannot change without the list being
		 * locked.  This is done around insque/remque.
		 */

	   if(cp_lock(&ip->inp_sopp->sop_lock, SPLNET) != CPLOCKFAIL ) {

		/*
		 * note: ipnxt must be valid since the tcb list is LOCKED,
		 * therefore no remque/insque is permitted.
		 */

		ipnxt = ip->inp_next;
		saved_sopp = ip->inp_sopp;

		/*
		 * however, pcb can be in the process of being deleted
		 * *before* the inp_sopp->sop_lock is acquired above, in
		 * which case inp_head is set = NULL;
		 * note that ipnxt is still valid for next pcb since
		 * list LOCKED
		 */

		if(ip->inp_head == (struct inpcb *) NULL){

			/*
			 * nevermind, ip is on its way out
			 */

			goto tpgone;
		}

		/*
		 * this pcb could be in the list *before* a tcb is
		 * bound to it.
		 */

		tp = intotcpcb(ip);
		if (tp == (struct tcpcb *) NULL) { /* no tcb to process? */
			goto tpgone;
		}
		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
				saved_sopp->sop_refcnt++; /* avoid deletion */
				(void) tcp_usrreq(tp->t_inpcb->inp_socket,
				    PRU_SLOWTIMO, (struct mbuf *) NULL,
				    (struct mbuf *)i, (struct mbuf *) NULL);

				/*
				 * if refcnt goes to zero => this slowtimo
				 * caused inp to be deleted, inp_detach has
				 * left the sop around because of the refcnt
				 * so release it.  Do not INP_UNLOCK cause
				 * inp is gone. socket_peer must be released.
				 * if pcb is deleted, then it is remqued and
				 * ipnext->inp_prev is updated.
				 * NOTE: inp_detach does not imply that this
				 * is the last reference to the socket_peer
				 * since socket might still have reference
				 * on time out.  (there may be some
				 * superfluous refcnt'ing here).
				 */

				saved_sopp->sop_refcnt--;
				if(saved_sopp->sop_refcnt == 0) {
					(void) m_free(dtom(saved_sopp));
					goto nextip;
				}

				/*
				 * the pcb can be deleted, but there is still
				 * a reference to the socket_peer (e.g. by
				 * the socket structure waiting for the
				 * disconnect to complete).  It must be
				 * unlocked without using the inp. This is
				 * done at tpgone.  Note that this breaks
				 * out of the for loop that goes through the
				 * timers since there are no more
				 * timers to go through if the pcb is gone.
				 */

				if(ipnxt->inp_prev != ip){
					goto tpgone;
				}
			}
		}

		/*
		 * timers done, tcb not deleted so increment stats
		 */

		tp->t_idle++;
		if (tp->t_rtt)
			tp->t_rtt++;
tpgone:
		v_lock(&saved_sopp->sop_lock, SPLNET);
nextip:
		ip = ipnxt;

	   } else {	 /* cp_lock == CPLOCKFAILS */

		/*
		 * ip->ip_next cannot change while list lock is held!!
		 */

		ip = ip->inp_next;
	   }
	}

	tcp_iss += TCP_ISSINCR/PR_SLOWHZ;		/* increment iss */
	if ((int)tcp_iss < 0)
		tcp_iss = 0;				/* compat 4.2 */
	INP_UNLOCK(&tcb[j], splevel);
    }
}

/*
 * Cancel all timers for TCP tp.
 */

tcp_canceltimers(tp)
	struct tcpcb *tp;
{
	register int i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

/*
 * TCP timer processing.  Called from tcp_usrreq.PRU_SLOWTIMO
 */

struct tcpcb *
tcp_timers(tp, timer)
	register struct tcpcb *tp;
	int timer;
{
	register int rexmt;

	switch (timer) {
	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	case TCPT_2MSL:
		if (tp->t_state != TCPS_TIME_WAIT &&
		    tp->t_idle <= tcp_maxidle)
			tp->t_timer[TCPT_2MSL] = tcp_keepintvl;
		else
			tp = timer_tcp_close(tp);
		break;

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */

	case TCPT_REXMT:
		tp->t_rxtshift++;
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT) {
			tp->t_rxtshift = TCP_MAXRXTSHIFT;
			tcpstat.tcps_timeoutdrop++;
			tp = timer_tcp_drop(tp, ETIMEDOUT);
			break;
		}
		tcpstat.tcps_rexmttimeo++;
		rexmt = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
		rexmt *= tcp_backoff[tp->t_rxtshift];
		TCPT_RANGESET(tp->t_rxtcur, rexmt, TCPTV_MIN, TCPTV_REXMTMAX);
		tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.  Clobber it
		 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
		 * retransmit times until then.
		 */
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
			in_losing(tp->t_inpcb);
			tp->t_rttvar += (tp->t_srtt >> 2);
			tp->t_srtt = 0;
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window, stop the timer.
		 */
		tp->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.  This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.  To get more time between 
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		{
		u_int win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2)
			win = 2;
		tp->snd_cwnd = tp->t_maxseg;
		tp->snd_ssthresh = win * tp->t_maxseg;
		}
		(void) tcp_output(tp);
		break;

	/*
	 * Persistence timer into zero window.
	 * Force a byte to be output, if possible.
	 */

	case TCPT_PERSIST:
		tcpstat.tcps_persisttimeo++;
		tcp_setpersist(tp);
		tp->t_force = 1;
		(void) tcp_output(tp);
		tp->t_force = 0;
		break;

	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 *
	 * note: if keepalive goes off before establishing
	 *       a connection, then ignore it, since the connection attempt
	 *       will abort via the max retries.
	 *       
	 */

	case TCPT_KEEP:
		tcpstat.tcps_keeptimeo++;
		if (tp->t_state < TCPS_ESTABLISHED)
			goto dropit;
		if (tp->t_inpcb->inp_socket->so_options & SO_KEEPALIVE &&
		    tp->t_state <= TCPS_CLOSE_WAIT) {
		    	if (tp->t_idle >= tcp_keepidle + tcp_maxidle)
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent TCP to respond.
			 */
			tcpstat.tcps_keepprobe++;
			/*
			 * The keepalive packet must have nonzero length
			 * to get a 4.2 host to respond.
			 */
			tcp_respond(tp, tp->t_template,
			    tp->rcv_nxt - 1, tp->snd_una - 1, 0);
			tp->t_timer[TCPT_KEEP] = tcp_keepintvl;
		} else
			tp->t_timer[TCPT_KEEP] = tcp_keepidle;
		break;
	dropit:
		tcpstat.tcps_keepdrops++;
		tp = timer_tcp_drop(tp, ETIMEDOUT);
		break;
	}
	return (tp);
}

/*
 * the routines timer_tcp_close, timer_tcp_drop, and lin_pcbdetach
 * have been added to enable timer to do everything
 * with the tcb list locked including *removing* tcb's from the tcb list.
 */

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 *
 *	called with the inpcb socket_peer held
 *	after discarding inpcb, UNLOCKING the socket_peer is inappropriate
 */

#define DODROP			/* for analysis of dropped packets */
#undef DODROP			/* for analysis of dropped packets */
#ifdef DODROP			/* defined in ../netinet/tcp_input.c */
extern short tcpdrops[64];		/* keep stats on drop codes */
#define DROPIT(x) tcpdrops[(x)]++;
#else
#define DROPIT(x) {;}
#endif DODROP

struct tcpcb *
timer_tcp_close(tp)
	register struct tcpcb *tp;
{
	register struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	register struct mbuf *m;
	struct socket * saved_head;

	DROPIT(60);

	t = tp->seg_next;
	while (t != (struct tcpiphdr *)tp) {
		t = (struct tcpiphdr *)t->ti_next;
		m = dtom(t->ti_prev);

		/*
		 * since this tcb no longer exists and the inpcb is locked,
		 * nothing should be looking at this queue so mutex is not
		 * an issue here.
 		 */

		remque(t->ti_prev);
		m_freem(m);
	}

	if (tp->t_template)
		(void) m_free(dtom(tp->t_template));
	inp->inp_ppcb = (caddr_t) NULL;
	soisdisconnected(so);

	if(so->so_head) {

		DROPIT(63);
		saved_head = so->so_head;
		(void) SOLOCK(so->so_head);

		/*
		 * This is a partial connection - either remque it to avoid
		 * simultaneous deletion, or leave it in the accept queue and
		 * let soclose eventually in_pcbdetach.  
		 *
		 * if not accepting connections
		 * then it is either closing or full
		 *      in either case, we will get processed eventually.
		 */
		if(!(so->so_head->so_options & SO_ACCEPTCONN))
			panic("tcp timer remove from closed accept queue");

		if(soqremque(so, 0)) 		/* removed it from so_q0 */
			so->so_sopp->sop_refcnt--;
		/* else on so_q, let accept() (or soclose) remove it */
		DROPIT(62);
		SOUNLOCK(saved_head, SPLNET);
	}

	(void) m_free(dtom(tp));

	/*
	 * note locked version of in_pcbdetach assumes that the
	 * tcb list is locked.
	 */

	lin_pcbdetach(inp);
	tcpstat.tcps_closed++;
	return ((struct tcpcb *) NULL);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */

struct tcpcb *
timer_tcp_drop(tp, errno)
	register struct tcpcb *tp;
	int errno;
{
	struct socket *so = tp->t_inpcb->inp_socket;

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		tcpstat.tcps_drops++;
	} else
		tcpstat.tcps_conndrops++;
	so->so_error = errno;
	return (timer_tcp_close(tp));
}
