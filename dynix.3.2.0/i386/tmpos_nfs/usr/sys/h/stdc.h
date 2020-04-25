/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution is only permitted until one year after the first shipment
 * of 4.4BSD by the Regents.  Otherwise, redistribution and use in source and
 * binary forms are permitted provided that: (1) source distributions retain
 * this entire copyright notice and comment, and (2) distributions including
 * binaries display the following acknowledgement:  This product includes
 * software developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with the
 * distribution and in all advertising materials mentioning features or use
 * of this software.  Neither the name of the University nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)stdc.h	7.3 (Berkeley) 6/28/90
 */
/*
 * $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
/* $Header: stdc.h 1.2 1991/09/30 23:08:13 $ */

/*
 * This file is designed to ease the porting from standard C to ANSI C.
 * This has been added to avoid changing too much 4.3 reno code.
 */

#if __STDC__ || __cplusplus
#define	CONCAT(x,y)	x ## y
#define	PROTOTYPE(p)	p
#define	STRING(x)	#x
#else
#define	const
#define	volatile
#define	signed
#define	CONCAT(x,y)	x/**/y
#define	PROTOTYPE(p)	()
#define	STRING(x)	"x"
#endif
