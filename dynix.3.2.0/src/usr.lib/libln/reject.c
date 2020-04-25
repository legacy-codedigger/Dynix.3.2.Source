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

/* $Header: reject.c 2.0 86/01/28 $ */

# include <stdio.h>
extern struct {int *yyaa, *yybb; int *yystops;} *yylstate [], **yylsp, **yyolsp;
yyreject ()
{
extern FILE *yyout, *yyin;
extern int yyprevious , *yyfnd;
extern char yyextra[];
extern char yytext[];
extern int yyleng;
for( ; yylsp < yyolsp; yylsp++)
	yytext[yyleng++] = yyinput();
if (*yyfnd > 0)
	return(yyracc(*yyfnd++));
while (yylsp-- > yylstate)
	{
	yyunput(yytext[yyleng-1]);
	yytext[--yyleng] = 0;
	if (*yylsp != 0 && (yyfnd= (*yylsp)->yystops) && *yyfnd > 0)
		return(yyracc(*yyfnd++));
	}
if (yytext[0] == 0)
	return(0);
yyoutput(yyprevious = yyinput());
yyleng=0;
return(-1);
}
yyracc(m)
{
yyolsp = yylsp;
if (yyextra[m])
	{
	while (yyback((*yylsp)->yystops, -m) != 1 && yylsp>yylstate)
		{
		yylsp--;
		yyunput(yytext[--yyleng]);
		}
	}
yyprevious = yytext[yyleng-1];
yytext[yyleng] = 0;
return(m);
}
