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

/* $Header: escseq.c 2.0 86/01/28 $
 *
 * escseq: get us out of any escape sequence we are in the middle of
 * and put us into the requested kind of escape sequence.
 */

#include "2648.h"

escseq(mode)
int mode;
{
	if (mode == _escmode)
		return;
	/* Get out of previous mode */
	switch (_escmode) {
	case NONE:
		break;
	case ESCD:
		if (mode == TEXT) {
			outchar('s');
			_escmode = mode;
			return;
		}
	case ESCP:
	case ESCM:
		outchar('Z');	/* no-op */
		break;
	case TEXT:
		outstr("\33*dT");
		break;
	}
	/* Get into new mode */
	switch (_escmode = mode) {
	case NONE:
		break;
	case ESCD:
		outstr("\33*d");
		break;
	case ESCP:
		outstr("\33*p");
		break;
	case ESCM:
		outstr("\33*m");
		break;
	case TEXT:
		outstr("\33*dS");
		break;
	}
}
