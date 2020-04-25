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
static char rcsid[] = "$Header: 4.form.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#
#include "def.h"
#include "4.def.h"
extern int linechars;
extern int rdfree(), comfree(), labfree(), contfree();
extern int rdstand(), comstand(), labstand(), contstand();
extern int (*rline[])();
extern int (*comment[])();
extern int (*getlabel[])();
extern int (*chkcont[])();
null(c)
char c;
	{return;}



comprint()
	{
	int c, blank, first,count;
	blank = 1;
	first = 1;
	count = 0;
	while ((c = (*comment[inputform])(0) ) || blankline() )
		{
		++count;
		if (c)
			{
			(*comment[inputform])(1);		/* move head past comment signifier */
			blank = blankline();
			/* if (first && !blank)
				OUTSTR("#\n");*/
			prline("#");
			first = 0;
			}
		else
			(*rline[inputform])(null);
		}
	/* if (!blank) 
		OUTSTR("#\n"); */
	return(count);
	}



prcode(linecount,tab)
int linecount, tab;
	{
	int someout;
	someout = FALSE;
	while (linecount)
		{
		if ( (*comment[inputform])(0) )
			{
			linecount -= comprint();
			someout = TRUE;
			continue;
			}
		else if (blankline() )
			(*rline[inputform])(null);
		else if ((*chkcont[inputform])() )
			{
			TABOVER(tab);
			prline("&");
			someout  = TRUE;
			}
		else 
			{if (someout) TABOVER(tab);
			(*getlabel[inputform])(null);
			prline("");
			someout=TRUE;
			}
		--linecount;
		}
	}


charout(c)
char c;
	{
	putc(c,outfd);
	}



prline(str)
char *str;
	{
	fprintf(outfd,"%s",str);
	(*rline[inputform]) (charout);
	putc('\n',outfd);
	}


input2()
	{
	static int c;
	c = inchar();
	if (c == '\n')
		linechars = 0;
	else
		++linechars;
	return(c);
	}


unput2(c)
int c;
	{
	unchar(c);
	--linechars;
	return(c);
	}
