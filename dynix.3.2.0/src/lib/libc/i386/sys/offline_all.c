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

/* $Header: offline_all.c 2.0 86/01/28 $ */

#include <sys/tmp_ctl.h>

offline_all()
{
	unsigned int first;	/* first on-line processor */
	unsigned int n_engine;	/* number of on-line processors */
	unsigned int i, j;

	n_engine = tmp_ctl(TMP_NENG, 0);

	for (first = 0; first < n_engine; first++)
		if (tmp_ctl(TMP_QUERY, first) == TMP_ENG_ONLINE)
			break; /* don't off-line first on-line processor */

	for (i = first+1; i < n_engine; i++)
		tmp_ctl(TMP_OFFLINE, i); /* off-line the rest */

	for (j = i = 0; i < n_engine; i++)
		if (tmp_ctl(TMP_QUERY, i) == TMP_ENG_ONLINE)
			j++; /* check that ONLY 1 is on-line */

	return ( j == 1 ); /* if we found more than one on-line return 0 */
}
