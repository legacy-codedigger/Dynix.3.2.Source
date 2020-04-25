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

#ident	"$Header: get_at.c 1.1 1991/07/31 00:02:11 $"
/*LINTLIBRARY*/

/* $Log: get_at.c,v $
 *
 */

#include "defs.h"

/*
 * table of interfaces supported by this machine
 */
extern struct ifTable IfTable[];
extern int if_num_entries;
extern struct mib_ipAddrEntry_struct *mib_ipAddrEntry;
extern int mib_ipAddrEntry_nentries;
extern char ipdev[];

int
compare_atEnt(entry1, entry2)
	struct mib_atEntry_struct *entry1, *entry2;
{
	if (entry1->atIfIndex > entry2->atIfIndex)
		return(1);
	else if (entry1->atIfIndex < entry2->atIfIndex)
		return(-1);

	if ((u_long)(ntohl(entry1->atNetAddress)) >
	    (u_long)(ntohl(entry2->atNetAddress)))
		return(1);
	else if ((u_long)(ntohl(entry1->atNetAddress)) <
	    (u_long)(ntohl(entry2->atNetAddress)))
		return(-1);

	return(0);
}

get_at(mib_atEntry)
	struct mib_atEntry_struct **mib_atEntry;
{
	struct mib_atEntry_struct *atEntry;
	struct arptab *at, *atp;
	int nentries, i, j, k, l, needfree;
	int fl = 0, size;
	struct mib_ipAddrEntry_struct *ipAddrEntry;
	struct ifTable *ifTablep;
	int count;

	init_mmap();		/* never know who's going to be the first */

	atp = arptab;
	count = nentries = arptab_size;

	if (*mib_atEntry)
		free((char *)(*mib_atEntry));

	atEntry = *mib_atEntry = (struct mib_atEntry_struct *)
		malloc(nentries * sizeof(struct mib_atEntry_struct));

	if (atEntry == NULL)
		return(-1);

	for (i=0; i < nentries; i++, atp++) {
		if (atp->at_flags == 0)
			continue;	/* empty entry */
		count--;
		memcpy(atEntry->atPhysAddress, atp->at_enaddr, 6);
		atEntry->PhysAddrLen = 6;
		atEntry->atNetAddress = atp->at_iaddr.s_addr;

		atEntry->atIfIndex = 0;

		/*
		 * In order to find the index into the interface table, we 
		 * must first see if it has been built. Then search thru it 
		 * for an interface name that matches the name returned by 
		 * if_config. note: there does not have to be an entry in 
		 * the interface table, as not all entries in the 
		 * interface table have to be configured with ip.
		 */
		ifTablep = IfTable;
		if (if_num_entries == 0)
			if_num_entries = fill_ifTable(ifTablep);

		/* get ip config info */
		if (mib_ipAddrEntry_nentries == 0)
			mib_ipAddrEntry_nentries = get_ipAddr(&mib_ipAddrEntry);

		/* check the subnet mask for each if entry */
		ipAddrEntry = mib_ipAddrEntry;
		for (l=0; l<mib_ipAddrEntry_nentries; l++, ipAddrEntry++){
		    if ((ipAddrEntry->ipAdEntAddr & ipAddrEntry->ipAdEntNetMask)
		       == (atp->at_iaddr.s_addr & ipAddrEntry->ipAdEntNetMask)){
			atEntry->atIfIndex = ipAddrEntry->ipAdEntIfIndex;
			break;
		    }
		}

		/* build the objid extension for this entry */
		j=0;

		atEntry->objid[j++] = (oid)(atEntry->atIfIndex);
		atEntry->objid[j++] = 1; /* yes .. this is always a 1! */
		atEntry->objid[j++] = (oid)(atEntry->atNetAddress & 0xff);
		atEntry->objid[j++] = (oid)((atEntry->atNetAddress & 0xff00) >> 8);
		atEntry->objid[j++] = (oid)((atEntry->atNetAddress & 0xff0000) >> 16);
		atEntry->objid[j++] = (oid)((atEntry->atNetAddress & 0xff000000) >> 24);
		atEntry++;
	}

	nentries -= count;
	qsort((char *)(*mib_atEntry), nentries, sizeof(struct mib_atEntry_struct), compare_atEnt);

	return(nentries);
}
