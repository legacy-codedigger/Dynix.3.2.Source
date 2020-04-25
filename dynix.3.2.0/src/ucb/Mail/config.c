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
static char *rcsid = "$Header: config.c 2.2 90/06/12 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)config.c	5.2 (Berkeley) 6/21/85";
#endif not lint

/*
 * This file contains definitions of network data used by Mail
 * when replying.  See also:  configdefs.h and optim.c
 */

/*
 * The subterfuge with CONFIGFILE is to keep cc from seeing the
 * external defintions in configdefs.h.
 */
#define	CONFIGFILE
#include "configdefs.h"

/*
 * Set of network separator characters.
 */
char	*metanet = "!^:%@";

/*
 * Host table of "known" hosts.  See the comment in configdefs.h;
 * not all accessible hosts need be here (fortunately).
 */
struct netmach netmach[] = {
	EMPTY,		EMPTYID,	AN,	/* Filled in dynamically */
	0,		0,		0
};

/*
 * Table of ordered of preferred networks.  You probably won't need
 * to fuss with this unless you add a new network character (foolishly).
 */
struct netorder netorder[] = {
	AN,	'@',
	AN,	'%',
	SN,	':',
	BN,	'!',
	-1,	0
};

/*
 * Table to convert from network separator code in address to network
 * bit map kind.  With this transformation, we can deal with more than
 * one character having the same meaning easily.
 */
struct ntypetab ntypetab[] = {
	'%',	AN,
	'@',	AN,
	':',	SN,
	'!',	BN,
	'^',	BN,
	0,	0
};

struct nkindtab nkindtab[] = {
	AN,	IMPLICIT,
	BN,	EXPLICIT,
	SN,	IMPLICIT,
	0,	0
};
