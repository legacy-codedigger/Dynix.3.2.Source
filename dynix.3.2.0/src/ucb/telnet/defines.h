/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.

/* $Header: defines.h 1.3 91/02/20 $ */

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)defines.h	5.1 (Berkeley) 9/14/90
 */

#define	settimer(x)	clocks.x = clocks.system++

#if	!defined(TN3270)

#define	SetIn3270()

#endif	/* !defined(TN3270) */

#define	NETADD(c)	{ *netoring.supply = c; ring_supplied(&netoring, 1); }
#define	NET2ADD(c1,c2)	{ NETADD(c1); NETADD(c2); }
#define	NETBYTES()	(ring_full_count(&netoring))
#define	NETROOM()	(ring_empty_count(&netoring))

#define	TTYADD(c)	if (!(SYNCHing||flushout)) { \
				*ttyoring.supply = c; \
				ring_supplied(&ttyoring, 1); \
			}
#define	TTYBYTES()	(ring_full_count(&ttyoring))
#define	TTYROOM()	(ring_empty_count(&ttyoring))

/*	Various modes */
#define	MODE_LOCAL_CHARS(m)	((m)&(MODE_EDIT|MODE_TRAPSIG))
#define	MODE_LOCAL_ECHO(m)	((m)&MODE_ECHO)
#define	MODE_COMMAND_LINE(m)	((m)==-1)

#define	CONTROL(x)	((x)&0x1f)		/* CTRL(x) is not portable */
