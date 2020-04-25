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
static	char	rcsid[] = "$Header: uipc_pipe.c 2.3 1991/07/03 23:58:51 $";
#endif

/*
 * uipc_pipe.c
 *	Connect sockets to implement a Unix pipe
 */

/* $Log: uipc_pipe.c,v $
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
#include "../h/unpcb.h"

extern int uipc_pipesize;

/*
 * Sneakily connect a pipe from wso to rso.
 * attaches unix domain pcb's to two sockets 
 */
piconnect(wso, rso)
	struct socket *wso, *rso;
{
	register struct unpcb *unp = sotounpcb(wso);
	register struct unpcb *unp2 = sotounpcb(rso);

	/*
	 * Note that LOCKING is not required here since sockets (i.e. pipe)
	 *	not known yet.
	 */
	unp->unp_conn = unp2;
	unp2->unp_conn = unp;
	unp->unp_csop = 1;	/* now share a common socket peer */ 
	unp2->unp_csop = 1;

	wso->so_snd.sb_hiwat = uipc_pipesize;
	wso->so_snd.sb_mbmax = 2*uipc_pipesize;
	wso->so_state |= SS_ISCONNECTED|SS_CANTRCVMORE;

	rso->so_rcv.sb_hiwat = 0;
	rso->so_rcv.sb_mbmax = 0;
	rso->so_state |= SS_ISCONNECTED|SS_CANTSENDMORE;

	/*** now set up peer lock **/

	(void) m_free(dtom(rso->so_sopp));

	rso->so_sopp = wso->so_sopp;

	rso->so_sopp->sop_refcnt = 4;	/* 4 PEERS for Pipes **/

	/*
	 * Now both rso and wso are locked by same lock to avoid peer update
	 *	deadlock.
	 */
	return (1);
}
