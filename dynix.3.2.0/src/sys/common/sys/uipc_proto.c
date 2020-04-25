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

#ifndef	lint
static	char	rcsid[] = "$Header: uipc_proto.c 2.1 87/04/10 $";
#endif

/*
 * uipc_proto.c
 *	Unix domain protosw
 */

/* $Log:	uipc_proto.c,v $
 */

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"

/*
 * Definitions of protocols supported in the UNIX domain.
 */

int	uipc_usrreq();
int	uipc_init();

int	raw_init(),raw_usrreq(),raw_input(),raw_ctlinput();

extern	struct domain unixdomain;		/* or at least forward */

struct protosw unixsw[] = {
{ SOCK_STREAM,	&unixdomain,	0,	PR_CONNREQUIRED|PR_WANTRCVD|PR_RIGHTS,
  0,		0,		0,		0,
  uipc_usrreq,
  0,		0,		0,		0,
},
{ SOCK_DGRAM,	&unixdomain,	0,		PR_ATOMIC|PR_ADDR|PR_RIGHTS,
  0,		0,		0,		0,
  uipc_usrreq,
  uipc_init,		0,		0,		0,
},
{ 0,		0,		0,		0,
  raw_input,	0,		raw_ctlinput,	0,
  raw_usrreq,
  raw_init,	0,		0,		0,
}
};

/*
 * n.b. passing rights not supported in DYNIX - e.g. unp_externalized
 * and unp_dispose.
 */

#ifdef notyet
int	unp_externalize(), unp_dispose();
#endif notyet

struct domain unixdomain =
#ifdef notyet
    { AF_UNIX, "unix", 0, unp_externalize, unp_dispose,
#endif notyet
    { AF_UNIX, "unix", 0, 0, 0,
      unixsw, &unixsw[sizeof(unixsw)/sizeof(unixsw[0])] };
