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

/* $Header: sh.char.h 1.1 1991/07/26 00:51:00 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sh.char.h	5.4 (Berkeley) 6/6/88
 */

extern unsigned short _cmap[];

#define	_Q	0x001		/* '" */
#define	_Q1	0x002		/* ` */
#define	_SP	0x004		/* space and tab */
#define	_NL	0x008		/* \n */
#define	_META	0x010		/* lex meta characters, sp #'`";&<>()|\t\n */
#define	_GLOB	0x020		/* glob characters, *?{[` */
#define	_ESC	0x040		/* \ */
#define	_DOL	0x080		/* $ */
#define	_DIG	0x100		/* 0-9 */
#define	_LET	0x200		/* a-z, A-Z, _ */

#define	cmap(c, bits)	(_cmap[(unsigned char)(c)] & (bits))

#define	isglob(c)	cmap(c, _GLOB)
#define	isspace(c)	cmap(c, _SP)
#define	isspnl(c)	cmap(c, _SP|_NL)
#define	ismeta(c)	cmap(c, _META)
#define	digit(c)	cmap(c, _DIG)
#define	letter(c)	cmap(c, _LET)
#define	alnum(c)	(digit(c) || letter(c))

#define	LINELEN		128
extern char *linp, linbuf[LINELEN];

#define	CSHPUTCHAR { \
	if (!(ch&QUOTE) && (ch == 0177 || ch < ' ' && ch != '\t' && \
	    ch != '\n')) { \
		*linp++ = '^'; \
		if (ch == 0177) \
			ch = '?'; \
		else \
			ch |= 'A' - 1; \
		if (linp >= &linbuf[sizeof linbuf - 2]) \
			flush(); \
	} \
	ch &= TRIM; \
	*linp++ = ch; \
	if (ch == '\n' || linp >= &linbuf[sizeof(linbuf) - 2]) \
		flush(); \
}
