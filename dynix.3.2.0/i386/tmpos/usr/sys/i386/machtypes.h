/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 *	@(#)machtypes.h	7.4 (Berkeley) 6/25/90
 */

/* $Copyright: $
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

/*
 * $Header: machtypes.h 1.2 1991/09/30 22:31:22 $
 */

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

/*
 * Types which are fundamental to the implementation and may appear in
 * more than one standard header are defined here.  Standard headers
 * then use:
 *	#ifdef	_SIZE_T_
 *	typedef	_SIZE_T_ size_t;
 *	#undef	_SIZE_T_
 *	#endif
 *
 * Thanks, ANSI!
 */
#define	_CLOCK_T_	unsigned long		/* clock() */
#define	_PTRDIFF_T_	int			/* ptr1 - ptr2 */
#define	_SIZE_T_	int			/* sizeof() */
#define	_TIME_T_	long			/* time() */
#define	_VA_LIST_	char *			/* va_list */
#define	_WCHAR_T_	unsigned short		/* wchar_t */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
typedef struct _physadr {
	int r[1];
} *physadr;
typedef struct label_t {
	int     lt_esp;
	int     lt_ebp;
	int     lt_eip;
	int     lt_ebx;
	int     lt_esi;
	int     lt_edi;
} label_t;
#endif
#endif	/* _MACHTYPES_H_ */
