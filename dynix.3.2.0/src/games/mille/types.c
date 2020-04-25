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

/* $Header: types.c 2.0 86/01/28 $ */

# include	"mille.h"

/*
 * @(#)types.c	1.1 (Berkeley) 4/1/82
 */

isrepair(card)
reg CARD	card; {

	return card == C_GAS || card == C_SPARE || card == C_REPAIRS || card == C_INIT;
}

safety(card)
reg CARD	card; {

	switch (card) {
	  case C_EMPTY:
	  case C_GAS:
	  case C_GAS_SAFE:
		return C_GAS_SAFE;
	  case C_FLAT:
	  case C_SPARE:
	  case C_SPARE_SAFE:
		return C_SPARE_SAFE;
	  case C_CRASH:
	  case C_REPAIRS:
	  case C_DRIVE_SAFE:
		return C_DRIVE_SAFE;
	  case C_GO:
	  case C_STOP:
	  case C_RIGHT_WAY:
	  case C_LIMIT:
	  case C_END_LIMIT:
		return C_RIGHT_WAY;
	}
	/* NOTREACHED */
}
