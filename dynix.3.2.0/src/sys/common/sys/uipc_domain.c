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

#undef RAW_ETHER
#define RAW_ETHER

#define AT

#ifndef	lint
static	char	rcsid[] = "$Header: uipc_domain.c 2.3 87/04/10 $";
#endif

/*
 * upic_domain.c
 * 	Struct domain handling routines
 */

/* $Log:	uipc_domain.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"

#include "../machine/gate.h"
#include "../machine/intctl.h"

#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/time.h"
#include "../h/kernel.h"

#define	ADDDOMAIN(x)	{ \
	extern struct domain x/**/domain; \
	x/**/domain.dom_next = domains; \
	domains = &x/**/domain; \
}

/*
 * domaininit() creates a linked list of struct domain
 * and invokes their initializations. 
 */

domaininit()
{
	register struct domain *dp;
	register struct protosw *pr;

#ifndef lint

#ifdef RAW_ETHER
	ADDDOMAIN(rawE);
#endif RAW_ETHER

#ifdef AT		/* Appletalk domain */
	ADDDOMAIN(at);
#endif AT

	ADDDOMAIN(unix);

#ifdef INET
	ADDDOMAIN(inet);
#endif

#endif lint

/*
 * for all domains if(init) init
 *	for each protocol supported by the domain if(init) init
 */
	for (dp = domains; dp; dp = dp->dom_next) {
		if (dp->dom_init)
			(*dp->dom_init)();
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_init)
				(*pr->pr_init)();
	}
	null_init();
	pffasttimo();	/* start the initial fastimer */
	pfslowtimo();	/* start the initial slowtimer */
	return;
}

/*
 * pffindtype finds a protosw from family (AF_XXX) and type
 * (SOCK_XXX).  Family is the same thing as domain in this usage.
 */

struct protosw *
pffindtype(family, type)
	int family, type;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);	/* 0 => not found */
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_type && pr->pr_type == type)
			return (pr);
	return (0);	/* 0 => not found */
}

/*
 * pffindproto finds a protosw from its family (AF_XXX) and a
 * protocol number (e.g. IPPROTO_XXX).
 */

struct protosw *
pffindproto(family, protocol, type)
	int family, protocol, type;
{
	register struct domain *dp;
	register struct protosw *pr;
	struct protosw *maybe = 0;

	if (family == 0)
		return (0);
	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++) {
		if ((pr->pr_protocol == protocol) && (pr->pr_type == type))
			return (pr);

		if (type == SOCK_RAW && pr->pr_type == SOCK_RAW &&
		    pr->pr_protocol == 0 && maybe == (struct protosw *)0)
			maybe = pr;
	}
	return (maybe);
}

/*
 * pfctlinput is a ioctl-like hook that currently is only called to
 * notify that a net interface is down (if_down).
 */

pfctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_ctlinput)
				(*pr->pr_ctlinput)(cmd, sa);
}

/*
 * pfslowtimo() starts all of the protocol slow timers going.
 */

pfslowtimo()
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_slowtimo)
				(*pr->pr_slowtimo)();
	timeout(pfslowtimo, (caddr_t)0, hz/2);
	return;
}

/*
 * pffasttimo() starts all of the protocol fast timers going. 
 */

pffasttimo()
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_fasttimo)
				(*pr->pr_fasttimo)();
	timeout(pffasttimo, (caddr_t)0, hz/5);
	return;
}
