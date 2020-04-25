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
static	char	rcsid[] = "$Header: svc_kudp.c 1.5 91/03/11 $";
#endif	lint

/*
 * svc_kudp.c,
 *	Server side for UDP/IP based RPC in the kernel.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* $Log:	svc_kudp.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../rpc/svc_dup.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

#define rpc_buffer(xprt) ((xprt)->xp_p1)

/*
 * Routines exported through ops vector.
 */
static bool_t		svckudp_recv();
static bool_t		svckudp_send();
static enum xprt_stat	svckudp_stat();
static bool_t		svckudp_getargs();
static bool_t		svckudp_freeargs();
static void		svckudp_destroy();

/*
 * Server transport operations vector.
 */
struct xp_ops svckudp_op = {
	svckudp_recv,		/* Get requests */
	svckudp_stat,		/* Return status */
	svckudp_getargs,	/* Deserialize arguments */
	svckudp_send,		/* Send reply */
	svckudp_freeargs,	/* Free argument data space */
	svckudp_destroy		/* Destroy transport handle */
};


struct mbuf	*ku_recvfrom();
void		xdrmbuf_init();

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct udp_data {
	sema_t	ud_sema;			/* see svckudp_send */
	u_long 	ud_xid;				/* id */
	struct	mbuf *ud_inmbuf;		/* input mbuf chain */
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
};

/*
 * Server statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;

/*
 * Create a transport record.
 * The transport record, output buffer, and private data structure
 * are allocated.  The output buffer is serialized into using xdrmem.
 * There is one transport record per user process which implements a
 * set of services.
 */
SVCXPRT *
svckudp_create(sock, port)
	struct socket	*sock;
	u_short		port;
{
	register SVCXPRT	 *xprt;
	register struct udp_data *ud;

#ifdef	RPCDEBUG
	rpc_debug(4, "svckudp_create so = %x, port = %d\n", sock, port);
#endif	RPCDEBUG

	xprt = (SVCXPRT *)kmem_alloc((u_int)sizeof(SVCXPRT));
	rpc_buffer(xprt) = (caddr_t)kmem_alloc((u_int)UDPMSGSIZE);
	ud = (struct udp_data *)kmem_alloc((u_int)sizeof(struct udp_data));
	bzero((caddr_t)ud, sizeof(*ud));
	init_sema(&ud->ud_sema, 1, 0, G_NFS);
	xprt->xp_addrlen = 0;
	xprt->xp_p2 = (caddr_t)ud;
	xprt->xp_verf.oa_base = ud->ud_verfbody;
	xprt->xp_ops = &svckudp_op;
	xprt->xp_port = port;
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
}
 
/*
 * Destroy a transport record.
 * Frees the space allocated for a transport record.
 */
static void
svckudp_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

#ifdef	RPCDEBUG
	rpc_debug(4, "usr_destroy %x\n", xprt);
#endif	RPCDEBUG

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	kmem_free((caddr_t)ud, (u_int)sizeof(struct udp_data));
	kmem_free((caddr_t)rpc_buffer(xprt), (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)xprt, (u_int)sizeof(SVCXPRT));
}

/*
 * Receive rpc requests.
 * Pulls a request in off the socket, checks if the packet is intact,
 * and deserializes the call packet.
 */
static bool_t
svckudp_recv(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg	 *msg;
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR		 *xdrs = &(ud->ud_xdrin);
	register struct mbuf	 *m;
	spl_t s;

#ifdef	RPCDEBUG
	rpc_debug(4, "svckudp_recv %x\n", xprt);
#endif	RPCDEBUG

	rsstat.rscalls++;

	s = SOLOCK(xprt->xp_sock);
	m = ku_recvfrom(xprt->xp_sock, &(xprt->xp_raddr));
	(void) SOUNLOCK(xprt->xp_sock, s);

	if (m == NULL) {
		rsstat.rsnullrecv++;
		return (FALSE);
	}

	if (m->m_len < 4*sizeof(u_long)) {
		rsstat.rsbadlen++;
		goto bad;
	}
	xdrmbuf_init(&ud->ud_xdrin, m, XDR_DECODE);
	if (! xdr_callmsg(xdrs, msg)) {
		rsstat.rsxdrcall++;
		goto bad;
	}
	ud->ud_xid = msg->rm_xid;
	ud->ud_inmbuf = m;

#ifdef	RPCDEBUG
	rpc_debug(5, "svckudp_recv done\n");
#endif	RPCDEBUG

	return (TRUE);

bad:
	m_freem(m);
	ud->ud_inmbuf = NULL;
	rsstat.rsbadcalls++;
	return (FALSE);
}

/*
 * Send rpc reply.
 * Serialize the reply packet into the output buffer then
 * call ku_sendto to make an mbuf out of it and send it.
 */
/* ARGSUSED */
static bool_t
svckudp_send(xprt, msg)
	register SVCXPRT *xprt; 
	struct rpc_msg	 *msg; 
{
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR *xdrs = &(ud->ud_xdrout);
	register int slen;
	register int stat = FALSE;
	struct mbuf *m, *mclgetx();

#ifdef	RPCDEBUG
	rpc_debug(4, "svckudp_send %x\n", xprt);
#endif	RPCDEBUG
	/*
	 * If previous output in progress, wait here.
	 */
	p_sema(&ud->ud_sema, PZERO-2);
	m = mclgetx((int (*)())v_sema, (int)&ud->ud_sema, rpc_buffer(xprt),
			UDPMSGSIZE, M_DONTWAIT);
	if (m == (struct mbuf *)NULL) {
		v_sema(&ud->ud_sema);
		return (stat);
	}

	xdrmbuf_init(&ud->ud_xdrout, m, XDR_ENCODE);
	msg->rm_xid = ud->ud_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if (m->m_next == 0) {		/* XXX */
			m->m_len = slen;
		}
		if (!ku_sendto_mbuf(xprt->xp_sock, m, &xprt->xp_raddr))
			stat = TRUE;
	} else {
		printf("svckudp_send: xdr_replymsg failed\n");
		m_freem(m);
	}
	/*
	 * This is completely disgusting.  If public is set it is
	 * a pointer to a structure whose first field is the address
	 * of the function to free that structure and any related
	 * stuff.  (see rrokfree in nfs_xdr.c).
	 */
	if (xdrs->x_public) {
		(**((int (**)())xdrs->x_public))(xdrs->x_public, stat);
	}

#ifdef	RPCDEBUG
	rpc_debug(5, "svckudp_send done\n");
#endif	RPCDEBUG

	return (stat);
}

/*
 * Return transport status.
 */
/*ARGSUSED*/
static enum xprt_stat
svckudp_stat(xprt)
	SVCXPRT *xprt;
{
	return (XPRT_IDLE); 
}

/*
 * Deserialize arguments.
 */
static bool_t
svckudp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT   *xprt;
	xdrproc_t xdr_args;
	caddr_t	  args_ptr;
{
	return ((*xdr_args)(&(((struct udp_data *)(xprt->xp_p2))->ud_xdrin), args_ptr));
}

static bool_t
svckudp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT   *xprt;
	xdrproc_t xdr_args;
	caddr_t	  args_ptr;
{
	register XDR *xdrs = &(((struct udp_data *)(xprt->xp_p2))->ud_xdrin);
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

	if (ud->ud_inmbuf) {
		m_freem(ud->ud_inmbuf);
	}
	ud->ud_inmbuf = (struct mbuf *)NULL;
	if (args_ptr) {
		xdrs->x_op = XDR_FREE;
		return ((*xdr_args)(xdrs, args_ptr));
	} else {
		return (TRUE);
	}
}

/*
 * The dup cacheing routines below provide a cache of non-failure
 * transaction id's.  rpc service routines can use this to detect
 * retransmissions and re-send a non-failure response.
 *
 * maxdupreqs is the number of cached items. It should be adjusted in
 * conf_nfs.c to the service load so that there is likely to be a response
 * entry when the first retransmission comes in.
 */

extern int	maxdupreqs;
extern struct dupreq	drtable[];
extern int	drhashsz;
extern struct dupreq	*drhashtbl[];

#define	DUPREQSZ	(sizeof(struct dupreq) - 2*sizeof(caddr_t))
#define	XIDHASH(xid)	((xid) & (drhashsz-1))
#define	DRHASH(dr)	XIDHASH((dr)->dr_xid)
#define	REQTOXID(req)	((struct udp_data *)((req)->rq_xprt->xp_p2))->ud_xid

static int	ndupreqs;
static int	dupreqs;
static int	dupchecks;
static lock_t	dup_lock;

/*
 * drmru points to the head of a circular linked list in lru order.
 * drmru->dr_next == drlru
 */
static struct dupreq *drmru;

/*
 * svckudp_dupinit
 *	Initialize mutex for dup cache and initialize drtable.
 * Called from svc_mutexinit().
 */
svckudp_dupinit()
{
	init_lock(&dup_lock, G_NFS);
}

svckudp_dupsave(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;
	spl_t spl;

	spl = p_lock(&dup_lock, SPLNET);
	if (ndupreqs < maxdupreqs) {
		dr = &drtable[ndupreqs];
		ndupreqs++;
		if (drmru) {
			dr->dr_next = drmru->dr_next;
			drmru->dr_next = dr;
		} else {
			dr->dr_next = dr;
		}
	} else {
		dr = drmru->dr_next;
		unhash(dr);
	}
	drmru = dr;

	dr->dr_xid = REQTOXID(req);
	dr->dr_prog = req->rq_prog;
	dr->dr_vers = req->rq_vers;
	dr->dr_proc = req->rq_proc;
	dr->dr_addr = req->rq_xprt->xp_raddr;
	dr->dr_chain = drhashtbl[DRHASH(dr)];
	drhashtbl[DRHASH(dr)] = dr;
	v_lock(&dup_lock, spl);
}

svckudp_dup(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;
	u_long	xid;
	spl_t	spl;
	 
	xid = REQTOXID(req);

	spl = p_lock(&dup_lock, SPLNET);
	dupchecks++;
	dr = drhashtbl[XIDHASH(xid)]; 
	while (dr != NULL) { 
		if (dr->dr_xid != xid ||
		    dr->dr_prog != req->rq_prog ||
		    dr->dr_vers != req->rq_vers ||
		    dr->dr_proc != req->rq_proc ||
		    bcmp((caddr_t)&dr->dr_addr,
		     (caddr_t)&req->rq_xprt->xp_raddr,
		     sizeof(dr->dr_addr)) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			dupreqs++;
			v_lock(&dup_lock, spl);
			return (1);
		}
	}
	v_lock(&dup_lock, spl);
	return (0);
}

/*
 * Called with dup_lock locked at SPLNET.
 */
static
unhash(dr)
	struct dupreq *dr;
{
	register struct dupreq *drt;
	struct dupreq *drtprev = NULL;
	 
	drt = drhashtbl[DRHASH(dr)]; 
	while (drt != NULL) { 
		if (drt == dr) { 
			if (drtprev == NULL) {
				drhashtbl[DRHASH(dr)] = drt->dr_chain;
			} else {
				drtprev->dr_chain = drt->dr_chain;
			}
			return; 
		}	
		drtprev = drt;
		drt = drt->dr_chain;
	}	
}
#endif	NFS
