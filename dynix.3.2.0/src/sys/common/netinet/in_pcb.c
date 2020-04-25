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
static	char	rcsid[] = "$Header: in_pcb.c 2.14 1991/08/15 00:23:00 $";
#endif

/*
 * in_pcb.c
 * Internet domain protocol control block routines
 */

/* $Log: in_pcb.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"	
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in_pcb.h"
#include "../netinet/in_var.h"
#include "../h/protosw.h"

struct	in_addr zeroin_addr;

/*
 * all of the currently known internet addresses are on a linked
 * list of struct inpcb's - this list is mutexed via the inp_lock in the
 * head inpcb.  This lock is in a socket_peer structure contained in
 * the pcb, but there is no associated peer.
 */

/*ARGSUSED*/
in_pcballoc(so, head)
	struct socket *so;
	struct inpcb *head;
{
	struct mbuf *m;
	register struct inpcb *inp;
	register struct socket_peer * sop;

	m = m_getclr(M_DONTWAIT, MT_PCB);
	if (m == (struct mbuf *)NULL)
		return (ENOBUFS);
	inp = mtod(m, struct inpcb *);

	m = m_getclr(M_DONTWAIT, MT_SOPEER);
	if (m == (struct mbuf *)NULL) {
		(void) m_free(dtom(inp));
		return (ENOBUFS);
	}
	sop = mtod(m, struct socket_peer *);
	init_lock(&sop->sop_lock, G_SOCK);

	/*
	 * There are initially two references to the pcb, one from
	 * socket management and one from in_pcb management.
	 *
	 * inp_sopp points to explicit socket_peer not the inp_sop.
 	 * also so->so_sopp -> same socket_peer
 	 */

	sop->sop_refcnt = 2;
	so->so_sopp = (struct socket_peer *) sop;
	inp->inp_sopp = (struct socket_peer *) sop;
	inp->inp_socket = so;	/* socket is bound to pcb */
	inp->inp_head = head;
	so->so_pcb = (caddr_t)inp;
	return (0);
}
	
/*
 * in_pcbbind binds an in_pcb to an internet address.  It is called
 * from in_pcbconnect after checking to see that the address is not
 * already bound or from pr_usrreq.BIND (without checking for a
 * duplicate).  The in_pcb.inp_head's lock is used to mutex the
 * appropriate pcb list.
 *
 * Called with the "inp" locked at SPLNET.
 * Returns with the "inp" locked at SPLNET.
 */

in_pcbbind(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct socket *so = inp->inp_socket;
	register struct inpcb *head;
	struct inpcb *first = inptoinhcomm(inp->inp_head)->inh_root;
	register struct sockaddr_in *sin;
	u_short lport = 0;
	struct socket_peer * saved_sopp;

	if (in_ifaddr == 0)
		return (EADDRNOTAVAIL);

	if (nam != (struct mbuf*)NULL) {	/* user provided name */

		sin = mtod(nam, struct sockaddr_in *);
		if (nam->m_len != sizeof (*sin)) {
			return (EINVAL);
		}
		if (sin->sin_addr.s_addr != INADDR_ANY) {

			int tport = sin->sin_port;

			/*
			 * check to see if we know this guy.
			 */

			sin->sin_port = 0;
			if (ifa_ifwithaddr((struct sockaddr *)sin) == 
						(struct ifaddr *)NULL) {
				return (EADDRNOTAVAIL);
			}
			sin->sin_port = tport;
		}
		lport = sin->sin_port;

		if (lport) { 		/* user-specified port # */

			u_short aport = htons(lport);
			int wild = 0;
	
			if (aport < IPPORT_RESERVED && u.u_uid != 0) {
				return (EACCES);
			}
			if ((so->so_options & SO_REUSEADDR) == 0 &&
		    	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
		     	    (so->so_options & SO_ACCEPTCONN) == 0))
				wild = INPLOOKUP_WILDCARD;
	
			/*
			 * find the cache line for this port
			 */
			head = &first[INP_HASH(lport)];
			/*
			 * open WINDOW to avoid deadlock with timeout routine.
			 * unlock the pcb, then lock the pcb hash line, then 
			 * relock the pcb (using a saved_sopp in case the 
			 * socket and/or pcb disappears during the window).
			 */

			saved_sopp = inp->inp_sopp;
			INP_UNLOCK(inp, SPLNET);	/* WINDOW */
			(void) INP_LOCK(head);		/* lock cache line */
			(void) p_lock(&saved_sopp->sop_lock, SPLNET);

			/*
			 * Verify that this inp is not already bound to some 
			 * address.  Must check after window to avoid races 
			 * with concurrent in_pcbbind().
			 */

			if (inp->inp_lport || 
			    inp->inp_laddr.s_addr != INADDR_ANY) {
				INP_UNLOCK(head, SPLNET);
				return (EINVAL);
			}

			/*
			 * socket and pcb could have disappeared in this window
			 * so check the refcnt.  If called from user, refcnt 
			 * should * be 2.  If called from net, should be 3,
			 * unless the socket and/or pcb are being deleted.
			 */

			if (saved_sopp->sop_refcnt <= 1) {
				/*
				 * they did disappear! - leave it to caller to
				 * clean up - but unlock the list and return
				 * EADDRNOTAVAIL
				 */
				INP_UNLOCK(head, SPLNET);
				return (EADDRNOTAVAIL);
			}

			/*
			 * NOTE the pcb hash line is locked to avoid 
			 * simultaneous binding conflicts.
			 *
			 * in_pcblookup_c is used to scan a locked pcb
			 * list.  Since we've already hashed the head, we
			 * call the common routine, which doesn't hash 
			 * and isn't concerned with locking.
			 * It returns a value if the sin_addr is in
			 * a pcb in the list headed by head.
			 */

			if ((struct inpcb *) in_pcblookup_c(head,
		    	   zeroin_addr, 0, sin->sin_addr, lport, wild)) {
				INP_UNLOCK(head, SPLNET);
				return (EADDRINUSE);
			}

		}	/* end caller specified port processing */

		inp->inp_laddr = sin->sin_addr;

	}	/* end caller provided name */

	/*
	 * NOTE: head pcb list is LOCKED - if specified, ADDR is *not* in
	 * use, keep list locked until binding complete to avoid concurrent
	 * binding conflicts.  If local port is not specified, choose one.
	 * Lock is locked while a free port number is found to assign this
	 * bind request.
	 *
	 * lport == 0 if not specified in nam or nam not specified
	 */

	if (lport == 0) for (;;) {

		/*
		 * look for an available port number starting
		 * with IPPORT_RESERVED
		 *
		 * NOTE: This 4.2 algorithm assumes that a port is
		 * always available, a likely assumption considering there
		 * are up to 64K possible ports.  Also note that
		 * head->inp_lport is updated to the last value assigned
		 * so the port numbers "wrap-around" forever.  Typically,
		 * an available port number is found quickly.  Nonetheless,
		 * if there are 64K ports used up (perhaps pathologically),
		 * this algorithm fails.
		 */

		(void)INPCOMM_LOCK(first);
		if (inptoinhcomm(first)->inh_port++ < IPPORT_RESERVED)
			inptoinhcomm(first)->inh_port = IPPORT_RESERVED;

		lport = htons(inptoinhcomm(first)->inh_port);
		INPCOMM_UNLOCK(first, SPLNET);

		/*
		 * find the cache line for this port
		 */
		head = &first[INP_HASH(lport)];

		/*
		 * open WINDOW to avoid deadlock with timeout routine.
		 * unlock the pcb, then lock the pcb hash line, then 
		 * relock the pcb (using a saved_sopp in case the 
		 * socket and/or pcb disappears during the window).
		 */

		saved_sopp = inp->inp_sopp;
		INP_UNLOCK(inp, SPLNET);	/* WINDOW */
		(void) INP_LOCK(head);		/* lock cache line */
		(void) p_lock(&saved_sopp->sop_lock, SPLNET);

		/*
		 * Verify that this inp is not already bound to some 
		 * address.  Must check after window to avoid races 
		 * with concurrent in_pcbbind().
		 */

		if (inp->inp_lport) {
			INP_UNLOCK(head, SPLNET);
			return (EINVAL);
		}

		/*
		 * socket and pcb could have disappeared in this window
		 * so check the refcnt.  If called from user, refcnt 
		 * should * be 2.  If called from net, should be 3,
		 * unless the socket and/or pcb are being deleted.
		 */

		if (saved_sopp->sop_refcnt <= 1) {
			/*
			 * they did disappear! - leave it to caller to
			 * clean up - but unlock the list and return
			 * EADDRNOTAVAIL
			 */
			INP_UNLOCK(head, SPLNET);
			return (EADDRNOTAVAIL);
		}
		if (in_pcblookup_c(head, zeroin_addr, 0, inp->inp_laddr,
		      lport, INPLOOKUP_WILDCARD) == (struct inpcb *) NULL)
			break;
		else
			INP_UNLOCK(head, SPLNET);

	}

	inp->inp_lport = lport;
	inp->inp_head = head;
	if (inp->inp_next == (struct inpcb *)NULL)
		insque(inp, head);	/* place unbound inpcb into list */
	INP_UNLOCK(head, SPLNET);
	return (0);
}

/*
 * Create a connected circuit by placing both local and foreign
 * addresses in the pcb.
 * Both address and port are specified in argument sin.
 * If a local address for this socket is not bound yet, pick one.
 *
 * called with the socket locked at SPLNET.
 * returns with the socket locked at SPLNET.
 */

in_pcbconnect(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

	if (nam->m_len != sizeof (*sin))
		return (EINVAL);
	return(in_pcbsetaddr(inp, sin));
}

in_pcbsetaddr(inp, sin)
	register struct inpcb *inp;
	register struct sockaddr_in *sin;
{
	struct in_ifaddr *ia;
	struct sockaddr_in *ifaddr;
	int error = 0;
	extern int in_connectzero;

	if (sin->sin_family != AF_INET)
		return (EAFNOSUPPORT);
	if (sin->sin_port == 0)
		return (EADDRNOTAVAIL);
	if (in_connectzero == 0 && sin->sin_addr.s_addr == INADDR_ANY)
		return (EADDRNOTAVAIL);

	if (in_ifaddr) {

		/*
		 * If the destination address is INADDR_ANY,
		 * use the primary local address.
		 * If the supplied address is INADDR_BROADCAST,
		 * find the first interface which supports broadcast and
		 * use the broadcast address for that interface.
		 */

#define	satosin(sa)	((struct sockaddr_in *)(sa))

		if (sin->sin_addr.s_addr == INADDR_ANY)
		    sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;
		else if (sin->sin_addr.s_addr == (u_long)INADDR_BROADCAST) {
			for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
		    			sin->sin_addr = 
					   satosin(&ia->ia_broadaddr)->sin_addr;
					break;
				}
			if (ia == NULL)
				return(EADDRNOTAVAIL);
		}
	}

	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		register struct route *ro;
		struct ifnet *ifp;

		ia = (struct in_ifaddr *)0;

		/* 
		 * If route is known or can be allocated now,
		 * our src addr is taken from the i/f, else punt.
		 */

		ro = &inp->inp_route;
		if (ro->ro_rt &&
		    (satosin(&ro->ro_dst)->sin_addr.s_addr !=
		        sin->sin_addr.s_addr ||
		    inp->inp_socket->so_options & SO_DONTROUTE)) {
			RTFREE(ro->ro_rt);
			ro->ro_rt = (struct rtentry *)0;
		}
		if ((inp->inp_socket->so_options & SO_DONTROUTE) == 0 && /*XXX*/
		    (ro->ro_rt == (struct rtentry *)0 ||
		    ro->ro_rt->rt_ifp == (struct ifnet *)0)) {

			/* No route yet, so try to acquire one */

			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				sin->sin_addr;
			rtalloc(ro);
		}

		/*
		 * If we found a route, use the address
		 * corresponding to the outgoing interface
		 * unless it is the loopback (in case a route
		 * to our address on another net goes to loopback).
		 */

		if (ro->ro_rt && (ifp = ro->ro_rt->rt_ifp) &&
		    (ifp->if_flags & IFF_LOOPBACK) == 0)
			for (ia = in_ifaddr; ia; ia = ia->ia_next)
				if (ia->ia_ifp == ifp)
					break;

		if (ia == 0) {
			int tport = sin->sin_port;
			sin->sin_port = 0;

			ia = (struct in_ifaddr *)
			    ifa_ifwithdstaddr((struct sockaddr *)sin);
			sin->sin_port = tport;

			if (ia == 0)
				ia = in_iaonnetof(in_netof(sin->sin_addr));
			if (ia == 0)
				ia = in_ifaddr;
			if (ia == 0)
				return (EADDRNOTAVAIL);
		}
		ifaddr = (struct sockaddr_in *)&ia->ia_addr;

		if (inp->inp_lport == 0) {
			/*
			 * local port not bound - pick one.
			 * in_pcbbind opens a WINDOW in order
			 * to bind an ASSIGNED port number to this pcb.
			 * It LOCK/UNLOCK's list.
		 	 */
			error =  in_pcbbind(inp, (struct mbuf *)NULL);
			if (error)
				return (error);
		}
		inp->inp_laddr = ifaddr->sin_addr;
	}
	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;
	return (0);
}

#define DODROP			/* for debug analysis of dropped packets */
#undef DODROP			/* for NO analysis of dropped packets */

#ifdef DODROP			/* defined in ../netinet/tcp_input.c */
extern short tcpdrops[64];	/* keep stats on drop codes */
#define DROPIT(x) tcpdrops[(x)]++;
#else
#define DROPIT(x) {;}
#endif DODROP

/*
 * in_pcbdisconnect() - undo a connection by unbinding the foreign
 * address (i.e. store 0), and taking the pcb out of the list
 * (in_pcbdetach).
 */

in_pcbdisconnect(inp)
	struct inpcb *inp;
{
	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

/*
 * in_pcbdetach() - remove a pcb from its list (inp.inp_head).
 */

in_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket *so = inp->inp_socket;
	struct inpcb *head;
	struct socket_peer * saved_sopp;

	/*
	 * In order to remque, the list head must be locked; however, if
	 * pcblookup is currently referencing this pcb, DEADLOCK can occur.
	 * To avoid this open a WINDOW in which pcblookup can acquire the
	 * pcb lock. in_pcblookup() discovers after INP_LOCK that the inp is
	 * disconnected and gives that information back to the network peer
	 * (i.e. nofind).  In the meantime, in_pcbdetach() locks the list
	 * and removes the pcb so it won't be found by pcblookup.  The refcnt
	 * is managed by not decrementing it until there is no longer a kernel
	 * reference.  Nothing can disappear during the window since there is
	 * no socket reference to it and no inp_head.
	 * inp_head is used to indicate to pcblookup that this pcb isn't
	 * really in list even if pcblookup finds it and wins the race to
	 * lock it back.  inp_head == NULL indicates that pcb is being removed.
	 */

	head = inp->inp_head;
	inp->inp_head = (struct inpcb *)NULL;
	saved_sopp = inp->inp_sopp;

	if (head) {	/* not currently being detached */

		/*
		 * since not being detached, must have inp_socket
		 */

		so->so_pcb = (caddr_t)NULL;
		if(inp->inp_options)
			(void) m_free(inp->inp_options);
		if (inp->inp_route.ro_rt) {
			rtfree(inp->inp_route.ro_rt);
			inp->inp_route.ro_rt = (struct rtentry *)NULL;
		}
		sofree(so);
		INP_UNLOCK(inp, SPLNET);	/* open WINDOW */
		(void) INP_LOCK(head);		/*   WINDOW - lock the list */
		(void) p_lock(&saved_sopp->sop_lock, SPLNET);
		if (head->inp_cache == inp)
			head->inp_cache = head;
		if (inp->inp_next != (struct inpcb *)NULL)
			remque(inp);
		INP_UNLOCK(head, SPLNET);
		(void) m_free(dtom(inp));

	} else{	 /* In process of being deleted.  Forgetit */
		DROPIT(52);
	}

	if (--(saved_sopp->sop_refcnt) == 0) {
		DROPIT(51);
		(void) m_free(dtom(saved_sopp));
	}

	/*
	 * N.B. leaves sop locked if it is not released
	 */
}

/*
 * locked version of in_pcbdetach which assumes that the pcb list
 * is locked before calling, and the pcb's socket_peer is also locked.
 * Used by tcp.
 */

lin_pcbdetach(inp)
	struct inpcb *inp;
{
	struct socket		*so = inp->inp_socket;
	struct socket_peer	*saved_sopp = inp->inp_sopp;

	/*
	 * socket_peer is locked so can safely sever the bond to the pcb.
	 * check for simultaneous detach by checking inp_head
	 * if simultaneous detach, then other detach will free inp
	 * so don't do anything.
	 */

	if(inp->inp_head != (struct inpcb *) NULL) {
		so->so_pcb = (caddr_t) NULL;
		sofree(so);
		if (inp->inp_route.ro_rt)
			rtfree(inp->inp_route.ro_rt);
		if (inp->inp_head->inp_cache == inp)
			inp->inp_head->inp_cache = inp->inp_head;
		inp->inp_head = (struct inpcb *) NULL;
		if (inp->inp_next != (struct inpcb *)NULL)
			remque(inp);
		(void) m_free(dtom(inp));
	}

	/*
	 * NOTE: this can be the last reference if called from
	 * tcp_abort->timer_tcp_close of partial connection.
	 */

	if(--(saved_sopp->sop_refcnt) <= 0)
		(void) m_free(dtom(saved_sopp));
}
/*
 * in_setsockaddr() - gets local sockaddr_in from the pcb and puts it
 * into name - used by getsockname() syscall (PRU_SOCKADDR).
 */

in_setsockaddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

/*
 * in_setpeeraddr() - gets foreign sockaddr_in from the pcb and puts it
 * into name - used by getpeername() syscall (PRU_SOCKADDR).
 */

in_setpeeraddr(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_in *sin;
	
	nam->m_len = sizeof (*sin);
	sin = mtod(nam, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  Call the protocol specific
 * routine (if any) to handle each connection.
 */

in_pcbnotify(first, dst, errno, notify)
	struct inpcb *first;
	register struct in_addr *dst;
	int errno, (*notify)();
{
	register struct inpcb *inp, *oinp;
	register struct socket_peer * saved_sopp;
	struct inpcb *head;
	spl_t splevel;
	int i;

	for (i = 0; i < INP_HASHSZ; i++) {
		head = &first[i];
		splevel = INP_LOCK(head);	/* lock this inpcb list */

		/*
		 * scan the appropriate pcb list looking for address matches.
		 * If one is found do the appropriate notify().
		 * NOTE: both the pcb list and pcb socket peer
		 * are locked here in in_pcbnotify, therefore special
		 * attention is appropriate to avoid deadlock at lower
		 * subroutine call levels.
		 */

		for (inp = head->inp_next; inp != head;) {
			(void) INP_LOCK(inp);	/* MUTEX */
			oinp = inp;		/* save inp pointer */

			/*
			 * check for interesting foreign host addr,
			 * head == NULL => pcb currently being removed
			 * socket == NULL => no reference to it 
			 */

			if ((inp->inp_faddr.s_addr != dst->s_addr) ||
			    (inp->inp_head == (struct inpcb *)NULL) ||
			    (inp->inp_socket == (struct socket *)NULL))
			{
				inp = inp->inp_next;
				INP_UNLOCK(oinp, SPLNET);	/* Note oinp */
				continue;
			}

			/*
			 * a match is found - pcb and list are locked
			 */

			if (errno)
				inp->inp_socket->so_error = errno;

			saved_sopp = inp->inp_sopp;	/* MUTEX */
			saved_sopp->sop_refcnt++;	/* avoid sop removal */
			inp = inp->inp_next;
			if(notify)
				(*notify)(oinp);	/* Note oinp */

			/*
			 * manage socket_peer
			 */

			saved_sopp->sop_refcnt--;
			if (saved_sopp->sop_refcnt == 0)
				(void) m_free(dtom(saved_sopp));
			else 
				v_lock(&saved_sopp->sop_lock, SPLNET);
		}
		INP_UNLOCK(head, splevel);
	}
}

/*
 * Check for alternatives when higher level complains
 * about service problems.  For now, invalidate cached
 * routing information.  If the route was created dynamically
 * (by a redirect), time to try a default gateway again.
 */

in_losing(inp)
	struct inpcb *inp;
{
	register struct rtentry *rt;

	if ((rt = inp->inp_route.ro_rt)) {
		if (rt->rt_flags & RTF_DYNAMIC)
			(void) rtrequest((int)SIOCDELRT, rt);
		rtfree(rt);
		inp->inp_route.ro_rt = 0;

		/*
		 * A new route can be allocated
		 * the next time output is attempted.
		 */
	}
}

/*
 * After a routing change, flush old routing
 * and allocate a (hopefully) better one.
 */

in_rtchange(inp)
	register struct inpcb *inp;
{
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt);
		inp->inp_route.ro_rt = 0;

		/*
		 * A new route can be allocated the next time
		 * output is attempted.
		 */
	}
}

/*
 * in_pcblookup looks up an internet address in the inpcblist headed
 * by head.  It locks the list while searching to guard against
 * concurrent changes.  When/if a pcb is found, it is *locked* and
 * it's reference count is incremented then the pcb list is unlocked.
 * If a pcb is not found that is bound to the internet address, NULL
 * is returned.
 *
 * in_pcblookup is called from the pr_input routines to find the
 * bound inpcb and the associated socket.  When locked, a socket_peer
 * is locked, thereby locking out the user from making changes such as
 * deleting the inpcb/socket structures.
 *
 * returns a LOCKED pcb if found.
 */

struct inpcb *
in_pcblookup(head, faddr, fport, laddr, lport, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_short fport, lport;
	int flags;
{
	spl_t splevel;
	struct inpcb *match;

	head = &head[INP_HASH(lport)];
	splevel = INP_LOCK(head);	/* lock the pcb list */

	match = in_pcblookup_c(head, faddr, fport, laddr, lport, flags);

	if (match) {
		(void) INP_LOCK(match);
		if (match->inp_head) {
			match->inp_sopp->sop_refcnt++;
			head->inp_cache = match;
		} else {
			INP_UNLOCK(match, SPLNET);
			match = (struct inpcb *)NULL;
		}
	}
	INP_UNLOCK(head, splevel);

	return(match);			/* if match, incpb is *LOCKED* */
}

struct inpcb *
in_pcblookup_c(head, faddr, fport, laddr, lport, flags)
	struct inpcb *head;
	struct in_addr faddr, laddr;
	u_short fport, lport;
	int flags;
{
	register struct inpcb *inp;
	struct inpcb *match = (struct inpcb *)NULL;
	int matchwild = 3, wildcard;

	/*
	 * check hash line cache
	 */
	
	inp = head->inp_cache;
        if (inp->inp_lport == lport &&
            inp->inp_fport == fport &&
            inp->inp_faddr.s_addr == faddr.s_addr &&
            inp->inp_laddr.s_addr == laddr.s_addr) {
		match = inp;
		return (match);
	} 

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {
		if (inp->inp_lport != lport)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
			    inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard && (flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}

	return (match);
}

in_pcbhashinit(inp, inh)
	struct inpcb inp[];
	struct inhcomm *inh;
{
	int i;

	inh->inh_port = IPPORT_RESERVED;
	inh->inh_root = inp;
	init_lock(&inh->inh_lock, G_PCB);
	for (i = 0; i < INP_HASHSZ; i++) {
		inp[i].inp_next = inp[i].inp_prev = inp[i].inp_cache = &inp[i];
		inp[i].inp_ppcb = (caddr_t) inh;
		inp[i].inp_sopp = (struct socket_peer *) &inp[i].inp_sop;
		inp[i].inp_sop.sop_refcnt = 1;
		init_lock(&inp[i].inp_sop.sop_lock, G_PCB);
	}
}
/*
 * in_pcbins: insert an inp into a hash chain.  Called with inp already
 * locked, inp already filled in (e.g. by tcp creating an embrionic socket)
 */
in_pcbins(inp)
struct inpcb *inp;
{
	struct inpcb *head;

	head = &inp->inp_head[INP_HASH(inp->inp_lport)];

	(void)INP_LOCK(head);
	inp->inp_head = head;
	insque(inp, head);
	INP_UNLOCK(head, SPLNET);
}
