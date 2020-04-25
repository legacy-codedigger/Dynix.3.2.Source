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

/* $Header: dkbad.c 2.0 86/01/28 $ */

#ifndef NOBADSECT
#include <sys/param.h>
#include <sys/buf.h>
#include <mbad/dkbad.h>

/*
 * Search the bad sector table looking for
 * the specified sector.  Return index if found.
 * Return -1 if not found.
 */

isbad(dkbad, cyl, trk, sec)
	register struct dkbad *dkbad;
{
	register int i;
	register long blk, bblk;
	register union bt_bad *bt;
	register int maxentries;

	maxentries = DK_NBAD_N * dkbad->bt_lastb + DK_NBAD_0;
	blk = ((long)cyl << 16) + (trk << 8) + sec;
	bt = dkbad->bt_bad;
	i = 0;
	while(i < maxentries) {
#ifndef BOOTXX
		if(bt[i].bt_cyl == DK_INVAL) {
			i += DK_NBAD_N;
			continue;
		}
#endif
		bblk = ((long)bt[i].bt_cyl << 16) + bt[i].bt_trksec;
		if (blk == bblk)
			return (i);
		if (blk < bblk || bblk < 0)
			break;
		i++;
	}
	return (-1);
}
#endif
