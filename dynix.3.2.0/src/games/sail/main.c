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

/* $Header: main.c 2.0 86/01/28 $ */

#ifndef lint
static	char *sccsid = "@(#)main.c	1.1 83/03/17";
#endif
#include "externs.h"

#define distance(x,y) (abs(x) >= abs(y) ? abs(x) + abs(y)/2 : abs(y) + abs(x)/2)

range(fromship, toship)
register int fromship, toship;
{
    int bow1r, bow1c, bow2r, bow2c;
    int stern1r, stern1c, stern2c, stern2r;
    register int bb, bs, sb, ss, result;

    if (fromship > scene[game].vessels
	    || toship > scene[game].vessels) /* just in case */
	return(30000);
    if (!pos[toship].dir)
	return(30000);
    stern1r = bow1r = pos[fromship].row;
    stern1c = bow1c = pos[fromship].col;
    stern2r = bow2r = pos[toship].row;
    stern2c = bow2c = pos[toship].col;
    result = bb = distance((bow2r - bow1r), (bow2c - bow1c));
    if (bb < 5)
	{
	drdc(&stern2r, &stern2c, pos[toship].dir);
	drdc(&stern1r, &stern1c, pos[fromship].dir);
	bs = distance((bow2r - stern1r) ,(bow2c - stern1c));
	sb = distance((bow1r - stern2r) ,(bow1c - stern2c));
	ss = distance((stern2r - stern1r) ,(stern2c - stern1c));
	result = min(bb, min(bs, min(sb, ss)));
	}
    return(result);
}

drdc(dr, dc, dir)
register int *dr, *dc;
int dir;
{
    switch (dir)
	{
	case 1:
	    (*dr)++;
	    break;
	case 2:
	    (*dr)++;
	    (*dc)--;
	    break;
	case 3:
	    (*dc)--;
	    break;
	case 4:
	    (*dr)--;
	    (*dc)--;
	    break;
	case 5:
	    (*dr)--;
	    break;
	case 6:
	    (*dr)--;
	    (*dc)++;
	    break;
	case 7:
	    (*dc)++;
	    break;
	case 8:
	    (*dr)++;
	    (*dc)++;
	    break;
	}
}
