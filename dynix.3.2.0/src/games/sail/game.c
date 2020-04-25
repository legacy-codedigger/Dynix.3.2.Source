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

/* $Header: game.c 2.0 86/01/28 $ */

#ifndef lint
static	char *sccsid = "@(#)game.c	1.3 83/05/20";
#endif

#include "externs.h"

maxturns(shipnum)
int shipnum;
{
	register int turns;
	struct File *ptr;

	turns = specs[scene[game].ship[shipnum].shipnum].ta;
	if ((ptr = scene[game].ship[shipnum].file) -> drift > 1 && turns){
		turns--;
		if(ptr -> FS == 1)
			turns = 0;
		turns |= 0100000;
	}
	return(turns);
}

maxmove(shipnum, dir, fs)
int shipnum, dir, fs;
{
	register int riggone = 0, Move, full, flank = 0;
	struct shipspecs *ptr;

	full = scene[game].ship[shipnum].file -> FS;
	ptr = &specs[scene[game].ship[shipnum].shipnum];
	Move = ptr -> bs;
	if (!ptr -> rig1) riggone++;
	if (!ptr -> rig2) riggone++;
	if (!ptr -> rig3) riggone++;
	if (!ptr -> rig4) riggone++;
	if ((full || fs) && fs != -1){
		flank = 1;
		Move = ptr -> fs;
	}
	if (dir == winddir)
		Move -= 1 + WET[windspeed][ptr -> class-1].B;
	else if (dir == winddir + 2 || dir == winddir - 2 || dir == winddir - 6 || dir == winddir + 6)
		Move -= 1 + WET[windspeed][ptr -> class-1].C;
	else if (dir == winddir + 3 || dir == winddir - 3 || dir == winddir - 5 || dir == winddir + 5)
		Move = (flank ? 2 : 1) - WET[windspeed][ptr -> class-1].D;
	else if (dir == winddir + 4 || dir == winddir - 4)
		Move = 0;
	else 
		Move -= WET[windspeed][ptr -> class-1].A;
	Move -= riggone;
	Move = Move < 0 ? 0 : Move;
	return(Move);
}

