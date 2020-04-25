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

#ifndef	lint
static	char	rcsid[] = "$Header: subr_prf.c 2.6 90/12/13 $";
#endif

/*
 * subr_prf.c
 *	Various output subroutines.
 */

/* $Log:	subr_prf.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/tty.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

extern lock_t prf_lock;		/* Printf lock */
extern short upyet;

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=2<BITTWO,BITONE>
 */

#ifndef KLINT
/*VARARGS1*/
printf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	spl_t	s;

	if (!upyet)
		prf(fmt, &x1, 0);
	else {
		s = p_lock(&prf_lock, SPLHI);
		prf(fmt, &x1, 0);
		v_lock(&prf_lock, s);
	}
}
#endif /*KLINT*/

/*
 * uprintf()
 *	Print on current users terminal.
 *
 * Guarantees not to sleep (so can be called by interrupt routines)
 * and now does watermark checking - (so verbose messages may be dropped).
 */

#ifndef KLINT
/*VARARGS1*/
uprintf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	if (u.u_ttyp && ttycheckoutq(u.u_ttyp, 0)) 
		prf(fmt, &x1, 2);
}
#endif /*KLINT*/

prf(fmt, adx, touser)
	register char *fmt;
	register u_int *adx;
{
	register int b, c, i;
	char *s;
	int any;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		putchar(c, touser);
	}
again:
	c = *fmt++;
	/* THIS CODE IS VAX DEPENDENT IN HANDLING %l? AND %c */
	switch (c) {

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		printn((u_long)*adx, b, touser);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				putchar(c, touser);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((u_long)b, *s++, touser);
		any = 0;
		if (b) {
			putchar('<', touser);
			while (i = *s++) {
				if (b & (1 << (i-1))) {
					if (any)
						putchar(',', touser);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c, touser);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				putchar('>', touser);
		}
		break;

	case 's':
		s = (char *)*adx;
		while (c = *s++)
			putchar(c, touser);
		break;

	case '%':
		putchar('%', touser);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */

printn(n, b, touser)
	u_long n;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-', touser);
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp, touser);
	while (cp > prbuf);
}

/*
 * Warn that a system table is full.
 */

tablefull(tab)
	char *tab;
{
	printf("%s: table is full\n", tab);
	/*
	 *+ The specified kernel table is full. The table may be configurable
	 *+ and so may be increased.
	 */
}

/*
 * Print a character on console or users terminal.
 */

/*ARGSUSED*/
putchar(c, touser)
	register int c;
{
	if (touser) {
		(void)tputchar(c, u.u_ttyp);
		return;
	}
	if (c == 0)
		return;
	cnputc(c);
}
