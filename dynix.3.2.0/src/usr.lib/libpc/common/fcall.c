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

/* $Header: fcall.c 1.1 89/03/12 $ */
#include "h00vars.h"

FCALL(save, frtn)
char *save;
register struct formalrtn *frtn;
{
	bcopy(&_disply[1], save, frtn->fbn*sizeof(struct display));
	bcopy(&frtn->fdisp[0], &_disply[1], frtn->fbn*sizeof(struct display));
}
