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
static	char	rcsid[] = "$Header: af.c 2.2 87/04/10 $";
#endif

/*
 * af.c
 *	Address family routines
 */

/* $Log:	af.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../machine/gate.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/af.h"

/*
 * Address family support routines
 */

int	null_hash(), null_netmatch();
#define	AFNULL \
	{ null_hash,	null_netmatch }

#ifdef INET
extern int inet_hash(), inet_netmatch();
#define	AFINET \
	{ inet_hash,	inet_netmatch }
#else
#define	AFINET	AFNULL
#endif


/*
 * note, there are AF_MAX address families
 *	implementor note, if the value of AF_MAX changes, the number
 *	of afswitch entries must conform.
 */

struct afswitch afswitch[AF_MAX] = {
	AFNULL,	AFNULL,	AFINET,	AFINET,	AFNULL,
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,
	AFNULL,
};

null_init()
{
	register struct afswitch *af;

	for (af = afswitch; af < &afswitch[AF_MAX]; af++)
		if (af->af_hash == (int (*)())NULL) {
			af->af_hash = null_hash;
			af->af_hash = null_hash;
		}
}


/*ARGSUSED*/
null_hash(addr, hp)
	struct sockaddr *addr;
	struct afhash *hp;
{
	hp->afh_nethash = hp->afh_hosthash = 0;
}

/*ARGSUSED*/
null_netmatch(a1, a2)
	struct sockaddr *a1, *a2;
{
	return (0);
}
