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

/* $Header: readline.c 2.0 86/01/28 $
 *
 * Read a line from the keyboard in the message line.  The line
 * goes into caller provided buffer msg, whos size is maxlen bytes.
 */

#include "2648.h"

readline(prompt, msg, maxlen)
char *prompt;
char *msg;
int maxlen;
{
	register char c;
	register char *cp;
	int oldx, oldy;
	int oldcuron;
	int oldquiet;
	extern int QUIET;

	oldx = _curx; oldy = _cury;
	oldcuron = _cursoron;
	areaclear(4, 4, 4+8, 719);
	setset();
	zoomout();
	curon();
	movecurs(4, 4);
	texton();

	oldquiet = QUIET;
	QUIET = 0;
	outstr(prompt);
	if (oldquiet)
		outstr("\r\n");
	QUIET = oldquiet;

	for (cp=msg; ; cp) {
		fflush(stdout);
		c = getchar();
		switch (c) {
		case '\n':
		case '\r':
		case ESC:
			*cp++ = 0;
			textoff();
			movecurs(oldx, oldy);
			if (oldcuron == 0)
				curoff();
			return;
		case '\b':
			if (--cp >= msg)
				outchar(c);
			else
				cp = msg;
			break;
		default:
			*cp++ = c;
			outstr(rdchar(c));
			if (cp-msg >= maxlen)
				error("line too long");
		}
	}
}
