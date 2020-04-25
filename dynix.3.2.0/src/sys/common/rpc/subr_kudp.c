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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: subr_kudp.c 1.8 91/03/11 $";
#endif	lint

/*
 * subr_kudp.c
 *	Subroutines to do UDP/IP sendto and recvfrom in the kernel.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* $Log:	subr_kudp.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../h/errno.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"

#include "../machine/intctl.h"

/*
 * General kernel udp stuff.
 * The routines below are used by both the client and the server side
 * rpc code.
 */

/*
 * Kernel recvfrom.
 * Pull address mbuf and data mbuf chain off socket receive queue.
 *
 * Called with socket locked (SOLOCK) at SPLNET.
 * Return with socket locked (SOLOCK) at SPLNET.
 */
struct mbuf *
ku_recvfrom(so, from)
	register struct socket *so;
	struct sockaddr_in *from;
{
	register struct mbuf	*m;
	register struct mbuf	*m0;
	int		len = 0;

#ifdef	RPCDEBUG
	rpc_debug(4, "urrecvfrom so=%X\n", so);
#endif	RPCDEBUG

	m = so->so_rcv.sb_mb;
	if (m == NULL) {
		return (m);
	}

	*from = *mtod(m, struct sockaddr_in *);
	sbfree(&so->so_rcv, m);
	so->so_rcv.sb_mb = m->m_act;	/* next record */
	MFREE(m, m0);			/* free the SONAME mbuf */
	if (m0 == NULL) {
		printf("cku_recvfrom: no body!\n");
		return (m0);
	}

	/*
	 * Walk down mbuf chain till 
	 * end of chain freeing socket buffer space as we go.
	 */

	for (m = m0; m; m = m->m_next) {
		sbfree(&so->so_rcv, m);
		len += m->m_len;
	}

	if (len > UDPMSGSIZE) {
		printf("ku_recvfrom: len = %d\n", len);
	}

#ifdef	RPCDEBUG
	rpc_debug(4, "urrecvfrom %d from %X\n", len, from->sin_addr.s_addr);
#endif	RPCDEBUG

	return (m0);
}

/*
 * Instrumentation.
 *	Note these values are approximations. These are not important enough
 *	to motivate mutex.
 */
int Sendtries;
int Sendok;
#define	PERFSTAT
#ifdef	PERFSTAT
static int Sendfastok;
static int Sendfastbad;
static int Sendslowok;
static int Sendslowbad;
#endif	PERFSTAT

/*
 * Kernel sendto.
 * Set addr and send off via UDP.
 * Use ku_fastsend if possible.
 */
int
ku_sendto_mbuf(so, m, addr)
	struct socket *so;
	struct mbuf *m;
	struct sockaddr_in *addr;
{
	register struct inpcb *inp = sotoinpcb(so);
	int error;
	spl_t s;
	struct inpcb local_inp;

#ifdef	RPCDEBUG
	rpc_debug(4, "ku_sendto_mbuf %X\n", addr->sin_addr.s_addr);
#endif	RPCDEBUG

	s = splnet();
	Sendtries++;
	error = ku_fastsend(so, m, addr);
	splx(s);
	if (!error) {
		Sendok++;
#ifdef	PERFSTAT
		Sendfastok++;
#endif	PERFSTAT
		return (0);
	}
#ifdef	PERFSTAT
	Sendfastbad++;
#endif	PERFSTAT

	/*
	 * If ku_fastsend returns -2, then we can try to send m the
	 * slow way.  else m was freed and we return ENOBUFS.
	 */
	if (error != -2) {
#ifdef	RPCDEBUG
		rpc_debug(3, "ku_sendto_mbuf: fastsend failed\n");
#endif	RPCDEBUG
		return (ENOBUFS);
	}

	/*
	 * Try the slow way.
	 * Lock the socket since in_pcbsetaddr() expects it to be locked.
	 */
	s = SOLOCK(so);
	/*
	 * first check to see if we've a local port; if not
	 * we havn't bound yet, so we better before continuing,
	 * otherwise in_pcbconnect will call in_pcbbind, which
	 * will put our local inp into some udb hash line!
	 */

	if (!(inp->inp_lport))
		error = in_pcbbind(inp, (struct mbuf *)0);
	if (error) {
		SOUNLOCK(so, s);
#ifdef	PERFSTAT
		Sendslowbad++;
#endif	PERFSTAT
		printf("in_pcbbind failed %d\n", error);
		m_freem(m);
		return (error);
	}

	local_inp = *inp;	/* struct copy */
	error = in_pcbsetaddr(&local_inp, addr);
	if (error) {
		SOUNLOCK(so, s);
		if (local_inp.inp_route.ro_rt)
			RTFREE(local_inp.inp_route.ro_rt);
#ifdef	PERFSTAT
		Sendslowbad++;
#endif	PERFSTAT
		printf("pcbsetaddr failed %d\n", error);
		m_freem(m);
		return (error);
	}
	SOUNLOCK(so, s);

	error = udp_output(&local_inp, m);

	if (local_inp.inp_route.ro_rt)
		RTFREE(local_inp.inp_route.ro_rt);

#ifdef	RPCDEBUG
	rpc_debug(4, "ku_sendto returning %d\n", error);
#endif	RPCDEBUG

	if (!error) {
		Sendok++;
#ifdef	PERFSTAT
		Sendslowok++;
#endif	PERFSTAT
	}
#ifdef	PERFSTAT
	else
		Sendslowbad++;
#endif	PERFSTAT
	return (error);
}

#ifdef	RPCDEBUG
int rpcdebug = 2;

/*VARARGS2*/
rpc_debug(level, str, a1, a2, a3, a4, a5, a6, a7, a8, a9)
        int level;
        char *str;
        int a1, a2, a3, a4, a5, a6, a7, a8, a9;
{

        if (level <= rpcdebug)
                printf(str, a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
#endif	RPCDEBUG
#endif	NFS
