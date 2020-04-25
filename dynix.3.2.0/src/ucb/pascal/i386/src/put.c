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

#if !defined(lint)
static char rcsid[] = "$Id: put.c,v 1.1 88/09/02 11:48:22 ksb Exp $";
#endif lint

#include "whoami.h"
#include "opcode.h"
#include "0.h"
#include "objfmt.h"
#if defined(PC)
#include	"pc.h"
#include	"align.h"
#else
    short	*obufp	= obuf;
#endif

/*
 * If DEBUG is defined, include the table
 * of the printing opcode names.
 */
#if defined(DEBUG)
#include "OPnames.h"
#endif

/*
 * listnames outputs a list of enumerated type names which
 * can then be selected from to output a TSCAL
 * a pointer to the address in the code of the namelist
 * is kept in value[ NL_ELABEL ].
 */
listnames(ap)

	register struct nl *ap;
{
	struct nl *next;
	register int len;
	register unsigned w;
	register char *strptr;

	if ( !CGENNING )
		/* code is off - do nothing */
		return(NIL);
	if (ap->class != TYPE)
		ap = ap->type;
	if (ap->value[ NL_ELABEL ] != 0) {
		/* the list already exists */
		return( ap -> value[ NL_ELABEL ] );
	}
#if defined(PC)
	    putprintf("	.data", 0);
	    aligndot(A_STRUCT);
	    ap -> value[ NL_ELABEL ] = (int) getlab();
	    (void) putlab((char *) ap -> value[ NL_ELABEL ] );
#endif PC
	/* number of scalars */
	next = ap->type;
	len = next->range[1]-next->range[0]+1;
#if defined(PC)
	    putprintf( "	.word %d" , 0 , len );
#endif PC
	/* offsets of each scalar name */
	len = (len+1)*sizeof(short);
#if defined(PC)
	    putprintf( "	.word %d" , 0 , len );
#endif PC
	next = ap->chain;
	do	{
		for(strptr = next->symbol;  *strptr++;  len++)
			continue;
		len++;
#if defined(PC)
		    putprintf( "	.word %d" , 0 , len );
#endif PC
	} while (next = next->chain);
	/* list of scalar names */
	strptr = getnext(ap, &next);
#if defined(PC)
	    while ( next ) {
		while ( *strptr ) {
		    putprintf( "	.byte	0%o" , 1 , *strptr++ );
		    for ( w = 2 ; ( w <= 8 ) && *strptr ; w ++ ) {
			putprintf( ",0%o" , 1 , *strptr++ );
		    }
		    putprintf( "" , 0 );
		}
		putprintf( "	.byte	0" , 0 );
		strptr = getnext( next , &next );
	    }
	    putprintf( "	.text" , 0 );
#endif PC
	return( ap -> value[ NL_ELABEL ] );
}

char *
getnext(next, new)

	struct nl *next, **new;
{
	if (next != NIL) {
		next = next->chain;
		*new = next;
	}
	if (next == NLNIL)
		return("");
	return(next->symbol);
}

#if !defined(PC)
lenstr(sptr, padding)

	char *sptr;
	int padding;

{
	register int cnt;
	register char *strptr = sptr;

	cnt = padding;
	do	{
		cnt++;
	} while (*strptr++);
	return((++cnt) & ~1);
}
#endif

/*
 * Patch repairs the branch
 * at location loc to come
 * to the current location.
 *	for PC, this puts down the label
 *	and the branch just references that label.
 *	lets here it for two pass assemblers.
 */
patch(loc)
    PTR_DCL loc;
{

#if defined(PC)
	    (void) putlab((char *) loc );
#endif PC
}

/*
 * Getlab - returns the location counter.
 * included here for the eventual code generator.
 *	for PC, thank you!
 */
char *
getlab()
{
#if defined(PC)
	    static long	lastlabel;

	    return ( (char *) ++lastlabel );
#endif PC
}

/*
 * Putlab - lay down a label.
 *	for PC, just print the label name with a colon after it.
 */
char *
putlab(l)
	char *l;
{

#if defined(PC)
	    putprintf(PREFIXFORMAT, 1, LABELPREFIX , l);
	    putprintf(":" , 0 );
#endif PC
	return (l);
}
