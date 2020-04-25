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

#ifdef	NFS

#ifndef	lint
static	char	rcsid[] = "$Header: svc.c 1.5 87/08/05 $";
#endif	lint

/*
 * svc.c
 *	Server-side remote procedure call interface.
 *
 * There are two sets of procedures here.  The xprt routines are
 * for handling transport handles.  The svc routines handle the
 * list of service routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/* $Log:	svc.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/rpc_msg.h"
#include "../rpc/svc.h"
#include "../rpc/svc_auth.h"
#include "../h/time.h"
#include "../h/systm.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

/* head of cached, free authentication parameters */
static caddr_t	rqcred_head;
static lock_t	rqcred_lock;

#define NULL_SVC	((struct svc_callout *)0)
#define	RQCRED_SIZE	400

/*
 * The services list.
 * Each entry represents a set of procedures (an rpc program).
 * The dispatch routine takes request structs and runs the
 * appropriate procedure.
 *
 * svc_callout_lock is used to mutex additions, deletions, and entry lookup
 * from the list.
 */
static lock_t svc_callout_lock;
static struct svc_callout {
	struct svc_callout *sc_next;
	u_long		    sc_prog;
	u_long		    sc_vers;
	void		    (*sc_dispatch)();
} *svc_head;

static struct svc_callout *svc_find();

/*
 * svc_mutexinit
 *	Initialize mutex needs for server side rpc.
 *
 * Called from main().
 */
svc_mutexinit()
{
	init_lock(&svc_callout_lock, G_NFS);
	init_lock(&rqcred_lock, G_NFS);
	svckudp_dupinit();
}


/* ***************  SVCXPRT related stuff **************** */

/*
 * Activate a transport handle.
 */
/*ARGSUSED*/
void
xprt_register(xprt)
	SVCXPRT *xprt;
{
}


/* ********************** CALLOUT list related stuff ************* */

/*
 * Add a service program to the callout list.
 * The dispatch routine will be called when a rpc request for this
 * program number comes in.
 */
/*ARGSUSED*/ 
bool_t
svc_register(xprt, prog, vers, dispatch, protocol)
	SVCXPRT *xprt;
	u_long prog;
	u_long vers;
	void (*dispatch)();
	int protocol;
{
	register struct svc_callout *s;
	struct svc_callout *founds;
	struct svc_callout *prev;
	spl_t	s_ipl;

	/*
	 * Assume need to add new item to list. This is done here
	 * and released if item is already in list because kmem_alloc
	 * may block and locks cannot be held. Items must be inserted into
	 * the list atomically to avoid races with another concurrent
	 * svc_register().
	 */
	s = (struct svc_callout *)mem_alloc(sizeof(struct svc_callout));

	/*
	 * Check to see if entry already in list.
	 * Must lock list so that it doesn't change while traversing.
	 */
	s_ipl = p_lock(&svc_callout_lock, SPLNET);
	if ((founds = svc_find(prog, vers, &prev)) != NULL_SVC) {
		if (founds->sc_dispatch == dispatch) {
			v_lock(&svc_callout_lock, s_ipl);
			mem_free((char *)s, (u_int)sizeof(struct svc_callout));
			return (TRUE);
		}
		v_lock(&svc_callout_lock, s_ipl);
		mem_free((char *)s, (u_int)sizeof(struct svc_callout));
		return (FALSE);
	}

	/*
	 * Entry not found.
	 * Fill in new one and insert into the svc_callout list.
	 */
	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_next = svc_head;
	svc_head = s;
	v_lock(&svc_callout_lock, s_ipl);
	return (TRUE);
}

/*
 * Remove a service program from the callout list.
 */
void
svc_unregister(prog, vers)
	u_long prog;
	u_long vers;
{
	struct svc_callout *prev;
	register struct svc_callout *s;
	spl_t	s_ipl;

	
	s_ipl = p_lock(&svc_callout_lock, SPLNET);
	if ((s = svc_find(prog, vers, &prev)) == NULL_SVC) {
		v_lock(&svc_callout_lock, s_ipl);
		return;
	}

	/*
	 * Found it. Now unlink.
	 */
	if (prev == NULL_SVC) {
		svc_head = s->sc_next;
	} else {
		prev->sc_next = s->sc_next;
	}
	s->sc_next = NULL_SVC;
	v_lock(&svc_callout_lock, s_ipl);

	mem_free((char *)s, (u_int) sizeof(struct svc_callout));
}

/*
 * Search the callout list for a program number, return the callout struct.
 *
 * Called with svc_callout_lock locked at SPLNET.
 */
static struct svc_callout *
svc_find(prog, vers, prev)
	u_long prog;
	u_long vers;
	struct svc_callout **prev;
{
	register struct svc_callout *s, *p;

	p = NULL_SVC;
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if ((s->sc_prog == prog) && (s->sc_vers == vers))
			break;
		p = s;
	}

	*prev = p;
	return (s);
}

/* ******************* REPLY GENERATION ROUTINES  ************ */

/*
 * Send a reply to an rpc request
 */
bool_t
svc_sendreply(xprt, xdr_results, xdr_location)
	register SVCXPRT *xprt;
	xdrproc_t xdr_results;
	caddr_t xdr_location;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY;  
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf; 
	rply.acpted_rply.ar_stat = SUCCESS;
	rply.acpted_rply.ar_results.where = xdr_location;
	rply.acpted_rply.ar_results.proc = xdr_results;
	return (SVC_REPLY(xprt, &rply)); 
}

/*
 * No procedure error reply
 */
void
svcerr_noproc(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROC_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * Can't decode args error reply
 */
void
svcerr_decode(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = GARBAGE_ARGS;
	SVC_REPLY(xprt, &rply); 
}

/*
 * Authentication error reply
 */
void
svcerr_auth(xprt, why)
	SVCXPRT *xprt;
	enum auth_stat why;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_DENIED;
	rply.rjcted_rply.rj_stat = AUTH_ERROR;
	rply.rjcted_rply.rj_why = why;
	SVC_REPLY(xprt, &rply);
}

/*
 * Auth too weak error reply
 */
void
svcerr_weakauth(xprt)
	SVCXPRT *xprt;
{

	svcerr_auth(xprt, AUTH_TOOWEAK);
}

/*
 * Program unavailable error reply
 */
void 
svcerr_noprog(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;  

	rply.rm_direction = REPLY;   
	rply.rm_reply.rp_stat = MSG_ACCEPTED;  
	rply.acpted_rply.ar_verf = xprt->xp_verf;  
	rply.acpted_rply.ar_stat = PROG_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * Program version mismatch error reply
 */
void  
svcerr_progvers(xprt, low_vers, high_vers)
	register SVCXPRT *xprt; 
	u_long low_vers;
	u_long high_vers;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_MISMATCH;
	rply.acpted_rply.ar_vers.low = low_vers;
	rply.acpted_rply.ar_vers.high = high_vers;
	SVC_REPLY(xprt, &rply);
}

/* ******************* SERVER INPUT STUFF ******************* */

/*
 * Get server side input from some transport.
 *
 * Statement of authentication parameters management:
 * This function owns and manages all authentication parameters, specifically
 * the "raw" parameters (msg.rm_call.cb_cred and msg.rm_call.cb_verf) and
 * the "cooked" credentials (rqst->rq_clntcred).  However, this function
 * does not know the structure of the cooked credentials, so it make the
 * following two assumptions: a) the structure is contiguous (no pointers), and
 * b) the structure size does not exceed RQCRED_SIZE bytes. 
 * In all events, all three parameters are freed upon exit from this routine.
 * The storage is trivially managed on the call stack in user land, but
 * is malloc'd in kernel land.
 */
void
svc_getreq(xprt)
	register SVCXPRT *xprt;
{
	register enum xprt_stat stat;
	struct rpc_msg msg;
	int prog_found;
	u_long low_vers;
	u_long high_vers;
	struct svc_req r;
	char *cred_area;  /* too big to allocate on call stack */
	void (*func)();
	spl_t	spl;

	/*
	 * First, allocate the authentication parameters' storage.
	 */
	spl = p_lock(&rqcred_lock, SPLNET);
	if (rqcred_head) {
		cred_area = rqcred_head;
		rqcred_head = *(caddr_t *)rqcred_head;
		v_lock(&rqcred_lock, spl);
	} else {
		v_lock(&rqcred_lock, spl);
		cred_area = mem_alloc(2*MAX_AUTH_BYTES + RQCRED_SIZE);
	}

	msg.rm_call.cb_cred.oa_base = cred_area;
	msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
	r.rq_clntcred = &(cred_area[2*MAX_AUTH_BYTES]);

	/* now receive msgs from xprtprt (support batch calls) */
	do {
		if (SVC_RECV(xprt, &msg)) {

			/* now find the exported program and call it */
			register struct svc_callout *s;
			enum auth_stat why;

			r.rq_xprt = xprt;
			r.rq_prog = msg.rm_call.cb_prog;
			r.rq_vers = msg.rm_call.cb_vers;
			r.rq_proc = msg.rm_call.cb_proc;
			r.rq_cred = msg.rm_call.cb_cred;
			/* first authenticate the message */
			if ((why = _authenticate(&r, &msg)) != AUTH_OK) {
				(void) SVC_FREEARGS(xprt, (xdrproc_t)0,
							(caddr_t)NULL);
				svcerr_auth(xprt, why);
				goto call_done;
			}
			/* now match message with a registered service*/
			prog_found = FALSE;
			low_vers = 0 - 1;
			high_vers = 0;
			spl = p_lock(&svc_callout_lock, SPLNET);
			for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
				if (s->sc_prog == r.rq_prog) {
					/*
					 * found correct program
					 */
					if (s->sc_vers == r.rq_vers) {
						/*
						 * found correct version
						 */
						func = (*s->sc_dispatch);
						v_lock(&svc_callout_lock, spl);
						(*func)(&r, xprt);
						goto call_done;
					}
					prog_found = TRUE;
					if (s->sc_vers < low_vers)
						low_vers = s->sc_vers;
					if (s->sc_vers > high_vers)
						high_vers = s->sc_vers;
				}
			}
			v_lock(&svc_callout_lock, spl);

			/*
			 * if we got here, the program or version
			 * is not served ...
			 * Free arguments and send error reply
			 */
			(void) SVC_FREEARGS(xprt, (xdrproc_t)0, (caddr_t)NULL);
			if (prog_found)
				svcerr_progvers(xprt, low_vers, high_vers);
			else
				svcerr_noprog(xprt);
			/* Fall through to ... */
		}
call_done:
		if ((stat = SVC_STAT(xprt)) == XPRT_DIED) {
			SVC_DESTROY(xprt);
			break;
		}
	} while (stat == XPRT_MOREREQS);

	/*
	 * free authentication parameters' storage
	 */
	spl = p_lock(&rqcred_lock, SPLNET);
	*(caddr_t *)cred_area = rqcred_head;
	rqcred_head = cred_area;
	v_lock(&rqcred_lock, spl);
}

int	Rpccnt;

/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
void
svc_run(xprt)
	SVCXPRT *xprt;
{
	spl_t	s;

	while (TRUE) {
		s = SOLOCK(xprt->xp_sock);
		while (xprt->xp_sock->so_rcv.sb_cc == 0)
			sbwait(&xprt->xp_sock->so_rcv);
		SOUNLOCK(xprt->xp_sock, s);
		svc_getreq(xprt);
		Rpccnt++;
	}
}
#endif	NFS
