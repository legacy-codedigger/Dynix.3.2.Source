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

/* $Header: defines.h 1.2 89/07/31 $ */

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)defines.h	1.5 (Berkeley) 3/8/88
 */

#define	settimer(x)	clocks.x = clocks.system++

#if	!defined(TN3270)

#define	ExitString(s,r)	{ fprintf(stderr, s); exit(r); }
#define	Exit(x)			exit(x)
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
#define	MODE_LINE(m)	(modelist[m].modetype & LINE)
#define	MODE_LOCAL_CHARS(m)	(modelist[m].modetype &  LOCAL_CHARS)
#define	MODE_LOCAL_ECHO(m)	(modelist[m].modetype & LOCAL_ECHO)
#define	MODE_COMMAND_LINE(m)	(modelist[m].modetype & COMMAND_LINE)

#define	LOCAL_CHARS	0x01		/* Characters processed locally */
#define	LINE		0x02		/* Line-by-line mode of operation */
#define	LOCAL_ECHO	0x04		/* Echoing locally */
#define	COMMAND_LINE	0x08		/* Command line mode */
