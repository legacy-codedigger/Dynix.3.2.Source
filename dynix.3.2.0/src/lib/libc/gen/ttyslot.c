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

/* $Header: ttyslot.c 2.0 86/01/28 $
 *
 * Return the number of the slot in the utmp file
 * corresponding to the current user: try for file 0, 1, 2.
 * Definition is the line number in the /etc/ttys file.
 */


char	*ttyname();
char	*getttys();
char	*rindex();
static	char	ttys[]	= "/etc/ttys";
static	char	*_b, *_p;
static	int	_c;

#define	NULL	0

ttyslot() {
	register char *tp, *p;
	register s, tf;
	char b[1024];

	if ((tp = ttyname(0)) == NULL &&
	    (tp = ttyname(1)) == NULL &&
	    (tp = ttyname(2)) == NULL)
		return(0);
	if ((p = rindex(tp, '/')) == NULL)
		p = tp;
	else
		p++;
	if ((tf = open(ttys, 0)) < 0)
		return(0);
	_p = _b = b;
	_c = 0;
	s = 0;
	while (tp = getttys(tf)) {
		s++;
		if (strcmp(p, tp)==0) {
			close(tf);
			return(s);
		}
	}
	close(tf);
	return(0);
}

static char *
getttys(f) {
	static char line[32];
	register char *lp;

	lp = line;
	for (;;) {
		if (--_c < 0)
			if ((_c = read(f, _p = _b, 1024)) <= 0)
				return(NULL);
			else
				--_c;
		*lp = *_p++;
		if (*lp =='\n') {
			*lp = '\0';
			return(line + 2);
		}
		if (lp >= &line[sizeof line])
			return(line + 2);
		lp++;
	}
}
