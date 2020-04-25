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

/* $Header: allow.c 2.0 86/01/28 $ */

static char sccsid[] = "	allow.c	4.1	82/05/11	";

#include "back.h"

movallow ()  {

	register int	i, m, iold;
	int		r;

	if (d0)
		swap;
	m = (D0 == D1? 4: 2);
	for (i = 0; i < 4; i++)
		p[i] = bar;
	i = iold = 0;
	while (i < m)  {
		if (*offptr == 15)
			break;
		h[i] = 0;
		if (board[bar])  {
			if (i == 1 || m == 4)
				g[i] = bar+cturn*D1;
			else
				g[i] = bar+cturn*D0;
			if (r = makmove(i))  {
				if (d0 || m == 4)
					break;
				swap;
				movback (i);
				if (i > iold)
					iold = i;
				for (i = 0; i < 4; i++)
					p[i] = bar;
				i = 0;
			} else
				i++;
			continue;
		}
		if ((p[i] += cturn) == home)  {
			if (i > iold)
				iold = i;
			if (m == 2 && i)  {
				movback(i);
				p[i--] = bar;
				if (p[i] != bar)
					continue;
				else
					break;
			}
			if (d0 || m == 4)
				break;
			swap;
			movback (i);
			for (i = 0; i < 4; i++)
				p[i] = bar;
			i = 0;
			continue;
		}
		if (i == 1 || m == 4)
			g[i] = p[i]+cturn*D1;
		else
			g[i] = p[i]+cturn*D0;
		if (g[i]*cturn > home)  {
			if (*offptr >= 0)
				g[i] = home;
			else
				continue;
		}
		if (board[p[i]]*cturn > 0 && (r = makmove(i)) == 0)
			i++;
	}
	movback (i);
	return (iold > i? iold: i);
}
