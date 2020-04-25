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
static	char	rcsid[] = "$Header: emon_filter.c 2.3 87/04/11 $";
#endif

/*
 * $Log:	emon_filter.c,v $
 */

#include "emon.h"

init_filters()
{
	bkeep = OFF;		/* default - don't keep broadcasts */
	keepall = OFF;		/* default - don't keep all */
	keeparp = OFF;		/* default - don't keep arp */
	keeptrail = OFF;	/* default - don't keep trail */
	keeptrap = OFF;		/* default - don't keep trap */
	keepbogus = OFF;	/* default - don't keep bogus */
	keepxns = OFF;		/* default - don't keep xns */
	keepat = OFF;		/* default - don't keep at */
	return;
}

do_do_keepall()
{
	if(keepall == OFF) {
		keepall = ON;
		printf("keepall now ON - keep all packets\n");
	}else{
		keepall = OFF;
		printf("keepall now OFF - do not keep all\n");
	}
	return;
}

do_do_keeptrail()
{
	if(keeptrail == OFF) {
		keeptrail = ON;
		printf("keeptrail now ON - keep trail packets\n");
	}else{
		keeptrail = OFF;
		printf("keeptrail now OFF - do not keep based on trail\n");
	}
	return;
}

do_do_keeparp()
{
	if (keeparp == OFF) {
		keeparp = ON;
		printf("keeparp now ON - keep arp packets\n");
	}else{
		keeparp = OFF;
		printf("keeparp now OFF - do not keep based on arp\n");
	}
	return;
}

do_do_keepat()
{
	if (keepat == OFF) {
		keepat = ON;
		printf("keepat now ON - keep at packets\n");
	}else{
		keepat = OFF;
		printf("keepat now OFF - do not keep based on at\n");
	}
	return;
}

do_do_keepxns()
{
	if (keepxns == OFF) {
		keepxns = ON;
		printf("keepxns now ON - keep xns packets\n");
	}else{
		keepxns = OFF;
		printf("keepxns now OFF - do not keep based on xns\n");
	}
	return;
}

do_do_keepwho()
{
	if(keepall) printf ("keepall ON, keeping all packets\n");
		else printf("keepall OFF\n");
	if(keeptrail) printf ("keeptrail ON, keeping trailer packets\n");
		else printf("keeptrail OFF\n");
	if(keeptrap) printf ("keeptrap ON, keeping traper packets\n");
		else printf("keeptrap OFF\n");
	if(keeparp) printf ("keeparp ON, keeping arp packets\n");
		else printf("keeparp OFF\n");
	if(keepbogus) printf ("keepbogus ON, keeping bogus packets\n");
		else printf("keepbogus OFF\n");
	if(bkeep) printf ("bkeep ON, keeping broadcast packets\n");
		else printf("bkeep OFF\n");
	if(keepxns) printf ("keepxns ON, keeping xns packets\n");
		else printf("keepxns OFF\n");
	if(keepat) printf ("keepat ON, keeping at packets\n");
		else printf("keepat OFF\n");
	return;
}

do_do_keepbogus()
{
	if(keepbogus == OFF) {
		keepbogus = ON;
		printf("keepbogus now ON - keep bogus packets\n");
	}else{
		keepbogus = OFF;
		printf("keepbogus now OFF - do not keep based on bogus\n");
	}
	return;
}

do_do_keeptrap()
{
	if (keeptrap == OFF) {
		keeptrap = ON;
		printf("keeptrap now ON - keep trap packets\n");
	}else{
		keeptrap = OFF;
		printf("keeptrap now OFF - do not keep based on trap\n");
	}
	return;
}

