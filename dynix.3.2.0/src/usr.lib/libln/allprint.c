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

/* $Header: allprint.c 2.0 86/01/28 $ */

# include <stdio.h>
allprint(c)
  char c; {
	extern FILE *yyout;
	switch(c){
		case '\n':
			fprintf(yyout,"\\n");
			break;
		case '\t':
			fprintf(yyout,"\\t");
			break;
		case '\b':
			fprintf(yyout,"\\b");
			break;
		case ' ':
			fprintf(yyout,"\\\bb");
			break;
		default:
			if(!printable(c))
				fprintf(yyout,"\\%-3o",c);
			else 
				putc(c,yyout);
			break;
		}
	return;
	}
sprint(s)
  char *s; {
	while(*s)
		allprint(*s++);
	return;
	}
printable(c)
  int c;
	{
	return(040 < c && c < 0177);
	}
