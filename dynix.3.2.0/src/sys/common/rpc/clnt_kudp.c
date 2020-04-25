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
static	char	rcsid[] = "$Header: clnt_kudp.c 1.11 91/03/13 $";
#endif	lint

/*
 * clnt_kudp.c
 *	Implements a kernel UPD/IP based, client side RPC.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* $Log:	clnt_kudp.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/mbuf.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_pcb.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

struct	mbuf	*ku_recvfrom();
static	int	ckuwakeup();

static enum clnt_stat	clntkudp_callit();
static void		clntkudp_abort();
static void		clntkudp_error();
static bool_t		clntkudp_freeres();
static void		clntkudp_destroy();

void	xdrmbuf_init();

/*
 * Operations vector for UDP/IP based RPC
 */
static struct clnt_ops udp_ops = {
	clntkudp_callit,	/* do rpc call */
	clntkudp_abort,		/* abort call */
	clntkudp_error,		/* return error status */
	clntkudp_freeres,	/* free results */
	clntkudp_destroy	/* destroy rpc handle */
};

/*
 * Private data per rpc handle.  This structure is allocated by
 * clntkudp_create, and freed by cku_destroy.
 */
struct cku_private {
	u_int			cku_flags;	/* see below */
	lock_t			cku_lock;	/* access to cku_bufsema */
	sema_t			cku_bufsema;	/* mutex access to cku_outbuf */
	CLIENT			cku_client;	/* client handle */
	int			cku_retrys;	/* request retrys */
	ulong			cku_xid;	/* transaction id */
	struct socket		*cku_sock;	/* open udp socket */
	struct sockaddr_in	cku_addr;	/* remote address */
	struct rpc_err		cku_err;	/* error status */
	XDR			cku_outxdr;	/* xdr routine for output */
	XDR			cku_inxdr;	/* xdr routine for input */
	u_int			cku_outpos;	/* position of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct mbuf		*cku_inmbuf;	/* input mbuf */
	struct ucred		*cku_cred;	/* credentials */
};

struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
} rcstat;


#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/* cku_flags */
#define	CKU_TIMEDOUT	0x001

static int	clntkudpxid;	/* transaction id used by all clients */
static lock_t	xid_lock;	/* lock access to clntkudpxid */

/*
 * Initialize client rpc mutex and clntkudpxid
 */
clnt_init()
{
	init_lock(&xid_lock, G_NFS);
	clntkudpxid = time.tv_usec;
}

static
noop()
{
}

/*
 * buffree
 *	Free output buffer resource.
 * Use lock to access semaphore to maintain consistency with
 * clntkudp_callit() usage. Since maximum number of waiters is 1,
 * wake the waiter (if one exists) then mark buffer as available.
 */
static
buffree(p)
	struct cku_private *p;
{
	spl_t	s_ipl;

	s_ipl = p_lock(&p->cku_lock, SPLIMP);
	if (blocked_sema(&p->cku_bufsema))
		v_sema(&p->cku_bufsema);
	sema_count(&p->cku_bufsema) = 1;
	v_lock(&p->cku_lock, s_ipl);
}

/*
 * cku_bufwakeup
 *	Timeout routine while waiting for output buffer resource.
 * Wake up process on "cku_bufsema" in case of deadlock in loop-back mode.
 * See clntkudp_callit().
 */
static
cku_bufwakeup(p)
	struct cku_private *p;
{
	spl_t	s_ipl;

	s_ipl = p_lock(&p->cku_lock, SPLIMP);
	/*
	 * Essentially a cv_sema().
	 */
	if (blocked_sema(&p->cku_bufsema))
		v_sema(&p->cku_bufsema);
	v_lock(&p->cku_lock, s_ipl);
}

/*
 * Create an rpc handle for a udp rpc connection.
 * Allocates space for the handle structure and the private data, and
 * opens a socket.  Note sockets and handles are one to one.
 */
CLIENT *
clntkudp_create(addr, pgm, vers, retrys, cred)
	struct sockaddr_in *addr;
	u_long pgm;
	u_long vers;
	int retrys;
	struct ucred *cred;
{
	register CLIENT *h;
	register struct cku_private *p;
	int error = 0;
	struct mbuf *m, *mclgetx();
	struct rpc_msg call_msg;

#ifdef	RPCDEBUG
	rpc_debug(4, "clntkudp_create(%X, %d, %d, %d\n",
	    addr->sin_addr.s_addr, pgm, vers, retrys);
#endif	RPCDEBUG

	p = (struct cku_private *)kmem_alloc((u_int)sizeof(*p));
	bzero((caddr_t)p, sizeof(*p));
	init_lock(&p->cku_lock, G_NFS);
	init_sema(&p->cku_bufsema, 1, 0, G_NFS);
	h = ptoh(p);

	/* handle */
	h->cl_ops = &udp_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/* call message, just used to pre-serialize below */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/* private */
	clntkudp_init(h, addr, retrys, cred);
	p->cku_outbuf = (char *)kmem_alloc((u_int)UDPMSGSIZE);
	m = mclgetx(noop, 0, p->cku_outbuf, UDPMSGSIZE, M_DONTWAIT);
	if (m == (struct mbuf *)NULL)
		goto bad;
	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);

	/* pre-serialize call message header */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {
		printf("clntkudp_create - Fatal header serialization error.\n");
		(void) m_freem(m);
		goto bad;
	}
	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	(void) m_free(m);

	/* open udp socket */
	error = socreate(AF_INET, &p->cku_sock, SOCK_DGRAM, IPPROTO_UDP);
	if (error) {
		printf("clntkudp_create: socket creation problem, %d\n", error);
		goto bad;
	}
	if (error = bindresvport(p->cku_sock)) {
		printf("clntkudp_create: socket bind problem, %d\n", error);
		goto bad;
	}
	return (h);

bad:
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p, (u_int)sizeof(struct cku_private));

#ifdef	RPCDEBUG
	rpc_debug(4, "create failed\n");
#endif	RPCDEBUG

	return ((CLIENT *)NULL);
}

clntkudp_init(h, addr, retrys, cred)
	CLIENT	*h;
	struct sockaddr_in *addr;
	int	retrys;
	struct ucred	*cred;
{
	register struct cku_private *p = htop(h);

	p->cku_retrys = retrys;
	p->cku_addr = *addr;
	p->cku_cred = cred;
	p->cku_xid = 0;
}

/*
 * Time out back off function. tim is in hz
 */
extern int clntkudp_maxtimo;

#define MAXTIMO	(clntkudp_maxtimo * hz)
#define backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

/*
 * Call remote procedure.
 * Most of the work of rpc is done here.  We serialize what is left
 * of the header (some was pre-serialized in the handle), serialize
 * the arguments, and send it off.  We wait for a reply or a time out.
 * Timeout causes an immediate return, other packet problems may cause
 * a retry on the receive.  When a good packet is received we deserialize
 * it, and check verification.  A bad reply code will cause one retry
 * with full (longhand) credentials.
 *
 * Currently only called IFF the caller has the client handle. So no
 * serialization is necessary in clntkudp_callit(). If RPC will be used
 * by other kernel services that may share client handles then serialization
 * will be necessary within clntkudp_callit(). This was done in SUN 3.0
 * via the CKU_BUSY, CKU_WANTED flags and when/if needed would be a
 * semaphore in Dynix.
 */
static enum clnt_stat 
clntkudp_callit(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait)
	register CLIENT	*h;
	u_long		procnum;
	xdrproc_t	xdr_args;
	caddr_t		argsp;
	xdrproc_t	xdr_results;
	caddr_t		resultsp;
	struct timeval	wait;
{
	register struct cku_private *p = htop(h);
	register XDR		*xdrs;
	register struct socket	*so = p->cku_sock;
	int			rtries;
	int			stries = p->cku_retrys;
	spl_t			s;
	struct ucred		*tmpcred;
	struct mbuf		*m;
	int timohz;
	u_long xid;
	struct sockaddr_in	from;
	struct rpc_msg		reply_msg;
	extern int		clntkudp_recvtries;	/* Times to retry */

#ifdef	RPCDEBUG
	rpc_debug(4, "cku_callit\n");
#endif	RPCDEBUG

	rcstat.rccalls++;

	/*
	 * Set credentials into the u structure
	 */
	tmpcred = u.u_cred;
	u.u_cred = p->cku_cred;

	if (p->cku_xid == 0) {
		s = p_lock(&xid_lock, SPLHI);
		p->cku_xid = clntkudpxid++;
		v_lock(&xid_lock, s);
	}
	xid = p->cku_xid;

	/*
	 * This is dumb but easy: keep the time out in units of hz
	 * so it is easy to call timeout and modify the value.
	 */
	timohz = wait.tv_sec * hz + (wait.tv_usec * hz) / 1000000;

call_again:

	/*
	 * Wait until buffer gets freed then make a MCLT_KHEAP mbuf point at it
	 * The buffree routine does the v_sema when the mbuf gets freed.
	 */
	s = p_lock(&p->cku_lock, SPLIMP);
	while (!sema_avail(&p->cku_bufsema)) {
		/*
		 * This is a kludge to avoid deadlock in the case of a
		 * loop-back call. The client can block waiting for the
		 * server to free the mbuf while the server is blocked
		 * waiting for the client to free the reply mbuf. Avoid this
		 * by flushing the input queue every once in a while while
		 * we are waiting.
		 */
		timeout(cku_bufwakeup, (caddr_t)p, hz);
		p_sema_v_lock(&p->cku_bufsema, PZERO-3, &p->cku_lock, s);
		untimeout(cku_bufwakeup, (caddr_t)p);
		s = SOLOCK(so);
		sbflush(&so->so_rcv);
		SOUNLOCK(so, s);
		s = p_lock(&p->cku_lock, SPLIMP);
	}
	sema_count(&p->cku_bufsema) = 0;
	v_lock(&p->cku_lock, s);

	m = mclgetx(buffree, (int)p, p->cku_outbuf, UDPMSGSIZE, M_DONTWAIT);
	if (m == (struct mbuf *)NULL) {
		p->cku_err.re_status = RPC_SYSTEMERROR;
		p->cku_err.re_errno = ENOBUFS;
		buffree(p);
		goto done;
	}

	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	(*(u_long *)(p->cku_outbuf)) = xid;

	xdrmbuf_init(&p->cku_outxdr, m, XDR_ENCODE);
	xdrs = &p->cku_outxdr;
	XDR_SETPOS(xdrs, p->cku_outpos);

	/*
	 * Serialize dynamic stuff into the output buffer.
	 */
	if ((! XDR_PUTLONG(xdrs, (long *)&procnum)) ||
	    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
	    (! (*xdr_args)(xdrs, argsp))) {
		p->cku_err.re_status = RPC_CANTENCODEARGS;
		p->cku_err.re_errno = EIO;
		(void) m_freem(m);
		goto done;
	}

	if (m->m_next == 0) {		/* XXX */
		m->m_len = XDR_GETPOS(&(p->cku_outxdr));
	}
	if (p->cku_err.re_errno =
	    ku_sendto_mbuf(so, m, &p->cku_addr)) {
		p->cku_err.re_status = RPC_CANTSEND;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = resultsp;
	reply_msg.acpted_rply.ar_results.proc = xdr_results;

	for (rtries = clntkudp_recvtries; rtries; rtries--) {
		s = SOLOCK(so);
		while (so->so_rcv.sb_cc == 0) {
			/*
			 * Set timeout then wait for input or timeout
			 */
#ifdef	RPCDEBUG
			rpc_debug(3, "callit: waiting %d\n", timohz);
#endif	RPCDEBUG
			timeout(ckuwakeup, (caddr_t)p, timohz);
			/*
			 * Similar to sbwait() but no signals are allowed.
			 */
			p_sema_v_lock(&so->so_rcv.sb_sbx->sbx_buf_wait, PRIBIO,
					&(so->so_sopp->sop_lock), s);
			untimeout(ckuwakeup, (caddr_t)p);

			s = SOLOCK(so);
			if (p->cku_flags & CKU_TIMEDOUT) {
				p->cku_flags &= ~CKU_TIMEDOUT;
				SOUNLOCK(so, s);
				p->cku_err.re_status = RPC_TIMEDOUT;
				p->cku_err.re_errno = ETIMEDOUT;
				rcstat.rctimeouts++;
				goto done;
			}
		}

		if (so->so_error) {
			so->so_error = 0;
			SOUNLOCK(so, s);
			continue;
		}

		p->cku_inmbuf = ku_recvfrom(so, &from);
		SOUNLOCK(so, s);
		if (p->cku_inmbuf == NULL) {
			continue;
		}
		p->cku_inbuf = mtod(p->cku_inmbuf, char *);

		if (p->cku_inmbuf->m_len < sizeof(u_long)) {
			m_freem(p->cku_inmbuf);
			continue;
		}
		/*
		 * If reply transaction id matches id sent
		 * we have a good packet.
		 */
		if (*((u_long *)(p->cku_inbuf))
		    != *((u_long *)(p->cku_outbuf))) {
			rcstat.rcbadxids++;
			m_freem(p->cku_inmbuf);
			continue;
		}
		/*
		 * Flush the rest of the stuff on the input queue
		 * for the socket.
		 */
		s = SOLOCK(so);
		sbflush(&so->so_rcv);
		SOUNLOCK(so, s);
		break;
	} 

	if (rtries == 0) {
		p->cku_err.re_status = RPC_CANTRECV;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmbuf_init(xdrs, p->cku_inmbuf, XDR_DECODE);

	/*
	 * Decode and validate the response.
	 */
	if (xdr_replymsg(xdrs, &reply_msg)) {
		_seterr_reply(&reply_msg, &(p->cku_err));

		if (p->cku_err.re_status == RPC_SUCCESS) {
			/*
			 * Reply is good, check auth.
			 */
			if (! AUTH_VALIDATE(h->cl_auth,
			    &reply_msg.acpted_rply.ar_verf)) {
				p->cku_err.re_status = RPC_AUTHERROR;
				p->cku_err.re_why = AUTH_INVALIDRESP;
			}
			if (reply_msg.acpted_rply.ar_verf.oa_base != NULL) {
				/* free auth handle */
				xdrs->x_op = XDR_FREE;
				(void)xdr_opaque_auth(xdrs,
				    &(reply_msg.acpted_rply.ar_verf));
			} 
		}
	} else {
		p->cku_err.re_status = RPC_CANTDECODERES;
		p->cku_err.re_errno = EIO;
	}
	m_freem(p->cku_inmbuf);

#ifdef	RPCDEBUG
	rpc_debug(4, "cku_callit done\n");
#endif	RPCDEBUG
done:
	if ((p->cku_err.re_status != RPC_SUCCESS) &&
	    (p->cku_err.re_status != RPC_CANTENCODEARGS) &&
	    (--stries > 0)) {
		rcstat.rcretrans++;
		timohz = backoff(timohz);
		if (p->cku_err.re_status == RPC_SYSTEMERROR ||
		    p->cku_err.re_status == RPC_CANTSEND) {
			/*
			 * Errors due to lack of resources, wait a bit
			 * and try again.
			 */
			p_sema(&lbolt, PZERO-4);
		}
		goto call_again;
	}
	u.u_cred = tmpcred;
	if (p->cku_err.re_status != RPC_SUCCESS) {
		rcstat.rcbadcalls++;
	}
	return (p->cku_err.re_status);
}

/*
 * Wake up client waiting for a reply.
 */
static
ckuwakeup(p)
	register struct cku_private *p;
{
	spl_t	s;

#ifdef	RPCDEBUG
	rpc_debug(4, "cku_timeout\n");
#endif	RPCDEBUG

	s = SOLOCK(p->cku_sock);
	p->cku_flags |= CKU_TIMEDOUT;
	sorwakeup(p->cku_sock);
	SOUNLOCK(p->cku_sock, s);
}

/*
 * Return error info on this handle.
 */
static void
clntkudp_error(h, err)
	CLIENT	*h;
	struct rpc_err *err;
{
	register struct cku_private *p = htop(h);

	*err = p->cku_err;
}

static bool_t
clntkudp_freeres(cl, xdr_res, res_ptr)
	CLIENT	*cl;
	xdrproc_t xdr_res;
	caddr_t	res_ptr;
{
	register struct cku_private *p = (struct cku_private *)cl->cl_private;
	register XDR *xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

static void 
clntkudp_abort()
{
}

/*
 * Destroy rpc handle.
 * Frees the space used for output buffer, private data, and handle
 * structure, and closes the socket for this handle.
 */
static void
clntkudp_destroy(h)
	CLIENT *h;
{
	register struct cku_private *p = htop(h);

#ifdef	RPCDEBUG
	rpc_debug(4, "cku_destroy %x\n", h);
#endif	RPCDEBUG

	(void) soclose(p->cku_sock);
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p, (u_int)sizeof(*p));
}

/*
 * try to bind to a reserved port
 */
static
bindresvport(so)
	struct socket *so;
{
	struct sockaddr_in *sin;
	struct mbuf *m;
	u_short i;
	int error;
	struct ucred *savcred;

#define	MAX_PRIV	(IPPORT_RESERVED-1)
#define	MIN_PRIV	(IPPORT_RESERVED/2)

	m = m_get(M_DONTWAIT, MT_SONAME);
	while (m == (struct mbuf *)NULL) {
		if (!m_expandorwait()) {
			printf("bindresvport: couldn't alloc mbuf.\n");
			return (ENOBUFS);
		}
		m = m_get(M_DONTWAIT, MT_SONAME);
	}

	sin = mtod(m, struct sockaddr_in *);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	m->m_len = sizeof(struct sockaddr_in);

	savcred = u.u_cred;
	u.u_cred = crdup(u.u_cred);
	u.u_uid = 0;
	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);
		error = sobind(so, m);
	}
	crfree(u.u_cred);
	u.u_cred = savcred;

	(void) m_freem(m);
	return (error);
}
#endif	NFS
