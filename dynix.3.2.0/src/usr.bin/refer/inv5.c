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

#ifndef lint
static char rcsid[] = "$Header: inv5.c 2.1 1991/05/20 00:16:18 $";
#endif

#include <stdio.h>

recopy (ft, fb, fa, nhash)
FILE *ft, *fb, *fa;
{
	/* copy fb (old hash items/pointers) to ft (new ones) */
	int n, i, iflong;
	long getl();
	int getw();
	int *hpt_s;
	long *hpt_l;
	long k, lp;
	if (fa==NULL)
	{
		err("No old pointers",0);
		return;
	}
	fread(&n, sizeof(n), 1, fa);
	fread(&iflong, sizeof(iflong), 1, fa);
	if (iflong)
	{
		hpt_l = (long *) calloc(sizeof(*hpt_l), n+1);
		n =fread(hpt_l, sizeof(*hpt_l), n, fa);
	}
	else
	{
		hpt_s =  (int *) calloc(sizeof(*hpt_s), n+1);
		n =fread(hpt_s, sizeof(*hpt_s), n, fa);
	}
	if (n!= nhash)
		fprintf(stderr, "Changing hash value to old %d\n",n);
	fclose(fa);
	for(i=0; i<n; i++)
	{
		if (iflong) {
			lp = hpt_l[i];
			fseek(fb, lp, 0);
			while ( (k= getl(fb) ) != -1)
				fprintf(ft, "%04d %06ld\n",i,k);
		} else {
			lp = hpt_s[i];
			fseek(fb, lp, 0);
			while ( (k= getw(fb) ) != -1)
				fprintf(ft, "%04d %06ld\n",i,k);
		}
	}
	fclose(fb);
	return(n);
}
