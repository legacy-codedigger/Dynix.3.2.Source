%{
#ifndef lint
static char rcsid[] = "$Header: scan.l 2.2 90/07/20 $";
static char sccsid[] = "@(#)scan.l	5.6 (Berkeley) 6/18/88";
#endif

#include "y.tab.h"
#include "htable.h"
%}

BLANK	[ \t]
DIGIT	[0-9]
ALPHA	[A-Za-z]
ANUM	[0-9A-Za-z]
NAMECHR	[0-9A-Za-z./-]

%%
"NET"		{
			yylval.number = KW_NET;
			return (KEYWORD);
		}

"GATEWAY"	{
			yylval.number = KW_GATEWAY;
			return (KEYWORD);
		}

"HOST"		{
			yylval.number = KW_HOST;
			return (KEYWORD);
		}

{ALPHA}{NAMECHR}*{ANUM}	{
			yylval.namelist = newname(yytext);
			return (NAME);
		}

{ALPHA}		{
			yylval.namelist = newname(yytext);
			return (NAME);
		}

{DIGIT}+{ALPHA}{NAMECHR}* {
			fprintf(stderr, "Warning: nonstandard name \"%s\"\n",
				yytext);
			yylval.namelist = newname(yytext);
			return (NAME);
		}

{DIGIT}+	{
			yylval.number = atoi(yytext);
			return (NUMBER);
		}

"."		return ('.');
":"		return (':');
","		return (',');
"/"		return ('/');
";".*		;
"\n"{BLANK}+	;
{BLANK}+	;
"\n"		return (END);
.		fprintf(stderr, "Illegal char: '%s'\n", yytext);

%%

yywrap()
{
	return (1);
}
