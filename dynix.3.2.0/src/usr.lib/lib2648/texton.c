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

/* $Header: texton.c 2.0 86/01/28 $ */

#include "2648.h"

texton()
{
	sync();
	escseq(TEXT);
}

textoff()
{
	sync();

	/*
	 * The following is needed because going into text mode
	 * leaves the pen where the cursor last was.
	 */
	_penx = -40; _peny = 40;
	escseq(ESCP);
	outchar('a');
	motion(_supx, _supy);
	_penx = _supx; _peny = _supy;
}
