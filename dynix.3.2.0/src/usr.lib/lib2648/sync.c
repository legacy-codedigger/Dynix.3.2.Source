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

/* $Header: sync.c 2.0 86/01/28 $
 *
 * Make the screen & screen mode look like what it's supposed to.
 *
 * There are two basic things to do here, put the _pen
 * in the right place, and make the line drawing mode be right.
 * We don't sync the cursor here, only when there's user input & it's on.
 */

#include "2648.h"

sync()
{
	if (_supx != _penx || _supy != _peny) {
		escseq(ESCP);
		outchar('a');
		motion(_supx, _supy);
	}
	if (_supsmode != _actsmode) {
		escseq(ESCM);
		switch (_actsmode = _supsmode) {
		case MX:
			outchar('3');
			break;
		case MC:
			outchar('1');
			break;
		case MS:
			outchar('2');
			break;
		}
		outchar('a');
	}
}
