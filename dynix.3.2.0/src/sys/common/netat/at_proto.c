
/*
 * $Copyright:	$
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
static	char	rcsid[] = "$Header: at_proto.c 1.4 87/04/10 $";
#endif

/*
 * $Log:	at_proto.c,v $
 */

#define	AT	/* APPLETALK */

#include "../h/param.h"
#include "../h/socket.h"

#ifdef sequent
#include "../h/socketvar.h"
#endif sequent

#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"
#include "../net/if.h"
#include "../netat/atalk.h"
#include "../netat/katalk.h"

#ifdef AT	/* appletalk */

/*
 * Definitions of protocols supported in the appletalk domain.
 */

int	ddp_usrreq(), ddp_init();

extern	struct domain atdomain;

struct protosw atsw[] = {

{ SOCK_DGRAM,	&atdomain,	PF_APPLETALK, PR_ATOMIC|PR_ADDR,
  	0,	0,		0,		0,
  ddp_usrreq,
  ddp_init,	0,		0,		0,
},
};

struct domain atdomain =
    { AF_APPLETALK, "appletalk", 0, 0, 0, 
      atsw, &atsw[sizeof(atsw)/sizeof(atsw[0])] };

#endif AT
