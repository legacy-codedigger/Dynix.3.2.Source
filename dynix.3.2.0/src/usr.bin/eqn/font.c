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

#ifndef lint
static char rcsid[] = "$Header: font.c 2.0 86/01/28 $";
#endif

# include "e.h"

setfont(ch1) char ch1; {
	/* use number '1', '2', '3' for roman, italic, bold */
	yyval = ft;
	if (ch1 == 'r' || ch1 == 'R')
		ft = ROM;
	else if (ch1 == 'i' || ch1 == 'I')
		ft = ITAL;
	else if (ch1 == 'b' || ch1 == 'B')
		ft = BLD;
	else
		ft = ch1;
	printf(".ft %c\n", ft);
#ifndef NEQN
	if(dbg)printf(".\tsetfont %c %c\n", ch1, ft);
#else NEQN
	if(dbg)printf(".\tsetfont %c\n", ft);
#endif NEQN
}

font(p1, p2) int p1, p2; {
		/* old font in p1, new in ft */
	yyval = p2;
	lfont[yyval] = rfont[yyval] = ft==ITAL ? ITAL : ROM;
	if(dbg)printf(".\tb:fb: S%d <- \\f%c S%d \\f%c b=%d,h=%d,lf=%c,rf=%c\n", 
		yyval, ft, p2, p1, ebase[yyval], eht[yyval], lfont[yyval], rfont[yyval]);
	printf(".ds %d \\f%c\\*(%d\\f%c\n", 
		yyval, ft, p2, p1);
	ft = p1;
	printf(".ft %c\n", ft);
}

fatbox(p) int p; {
	int sh;

	yyval = p;
	sh = ps / 4;
	nrwid(p, ps, p);
	printf(".ds %d \\*(%d\\h'-\\n(%du+%du'\\*(%d\n", p, p, p, sh, p);
	if(dbg)printf(".\tfat %d, sh=%d\n", p, sh);
}

globfont() {
	char temp[20];

	getstr(temp, 20);
	yyval = eqnreg = 0;
	gfont = temp[0];
	switch (gfont) {
	case 'r': case 'R':
		gfont = '1';
		break;
	case 'i': case 'I':
		gfont = '2';
		break;
	case 'b': case 'B':
		gfont = '3';
		break;
	}
	printf(".ft %c\n", gfont);
	ft = gfont;
}
