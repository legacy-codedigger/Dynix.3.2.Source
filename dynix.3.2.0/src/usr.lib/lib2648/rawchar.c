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

/* $Header: rawchar.c 2.0 86/01/28 $
 *
 * get a character from the terminal, with no line buffering.
 */

#include "2648.h"

rawchar()
{
	char c;

	sync();
	escseq(NONE);
	fflush(stdout);
	if (_pb_front && _on2648) {
		c = *_pb_front++;
#ifdef TRACE
		if (trace)
			fprintf(trace, "%s from queue, front=%d, back=%d\n", rdchar(c), _pb_front-_pushback, _pb_back-_pushback);
#endif
		if (_pb_front > _pb_back) {
			_pb_front = _pb_back = NULL;
#ifdef TRACE
			if (trace)
				fprintf(trace, "reset pushback to null\n");
#endif
		}
		return (c);
	}
	_outcount = 0;
	c = getchar();
#ifdef TRACE
	if (trace)
		fprintf(trace, "rawchar '%s'\n", rdchar(c));
#endif
	return (c);
}
