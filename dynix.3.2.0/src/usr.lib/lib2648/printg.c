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

/* $Header: printg.c 2.0 86/01/28 $ */

#include "2648.h"

printg()
{
	int oldvid = _video;
	int c, c2;

	if (oldvid==INVERSE)
		togvid();
	sync();
	escseq(NONE);
	outstr("\33&p4d5u0C");
	outchar('\21');	/* test handshaking fix */

	/*
	 * The terminal sometimes sends back S<cr> or F<cr>.
	 * Ignore them.
	 */
	fflush(stdout);
	c = getchar();
	if (c=='F' || c=='S') {
		c2 = getchar();
		if (c2 != '\r' && c2 != '\n')
			ungetc(c2, stdin);
	} else {
		ungetc(c, stdin);
	}

	if (oldvid==INVERSE)
		togvid();
}
