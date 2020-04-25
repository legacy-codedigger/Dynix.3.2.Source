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

/* $Header: random.c 1.1 89/03/12 $ */
#include "h00vars.h"

/*
 * return a 'random' number in the range [0..1]
 *
 * This random routine comes straight from the 4.3BSD Berkeley
 * Pascal User's Manual PS1:4-49.  Successive seeds are generated
 * as (seed * a + c) mod m.  The new random number is a normalization
 * of the seed to the range 0.0 to 1.0; a=62,605 c=113,218,009
 * m=536,870,912.  The initial seed is 7,774,755.
 * 
 * This function assumes at least 31bit long and unsigned long
 * integers.
 */
/*ARGSUSED*/
double
RANDOM(dDum)
double dDum;
{
	register double yf;

	yf = (double)(unsigned long int)_seed * 62605.0 + 113218009.0;
	/*
	 * The following casts simulate trunc(3m).  The expression
	 * yf / 536870912.0 is always between 0 and 500,841 (assuming
	 * the worst case of someone having just set the seed to -1).
	 * We depend on the casts rounding non-negative doubles toward zero.
	 */
	yf -= (double)(long int)(yf / 536870912.0) * 536870912.0;
	_seed = (long int)yf;
	return (double)_seed / 536870911.0;
}
