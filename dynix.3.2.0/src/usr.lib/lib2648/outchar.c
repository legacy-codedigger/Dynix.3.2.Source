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

/* $Header: outchar.c 2.0 86/01/28 $ */

#include "2648.h"

outchar(c)
char c;
{
	extern int QUIET;
#ifdef TRACE
	if (trace)
		fprintf(trace, "%s", rdchar(c));
#endif
	if (QUIET)
		return;
	_outcount++;
	putchar(c);

	/* Do 2648 ^E/^F handshake */
	if (_outcount > TBLKSIZ && _on2648) {
#ifdef TRACE
		if (trace)
			fprintf(trace, "ENQ .. ");
#endif
		putchar(ENQ);
		fflush(stdout);
		c = getchar();
		while (c != ACK) {
			if (_pb_front == NULL) {
				_pb_front = _pushback;
				_pb_back = _pb_front - 1;
			}
			*++_pb_back = c;
#ifdef TRACE
			if (trace)
				fprintf(trace, "push back %s, front=%d, back=%d, ", rdchar(c), _pb_front-_pushback, _pb_front-_pushback);
#endif
			c = getchar();
		}
#ifdef TRACE
		if (trace)
			fprintf(trace, "ACK\n");
#endif
		_outcount = 0;
	}
}
