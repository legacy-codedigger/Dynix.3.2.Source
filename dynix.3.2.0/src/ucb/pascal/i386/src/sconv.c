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
static char rcsid[] = "$Id: sconv.c,v 1.1 88/09/02 11:48:25 ksb Exp $";
#endif lint

    /*
     *	functions to help pi put out
     *	polish postfix binary portable c compiler intermediate code
     *	thereby becoming the portable pascal compiler
     */

#include "whoami.h"

#if defined(PC)

#include "0.h"
#include <pcc.h>

/*
 * this routine enforces ``the usual arithmetic conversions''
 * all integral operands are converted to ints.
 * if either operand is a double, both are made to be double.
 * this routine takes struct nl *'s for the types,
 * and returns both the struct nl * and the p2type for the result.
 */
tuac(thistype, thattype, resulttypep, resultp2typep)
struct nl *thistype;
struct nl *thattype;
struct nl **resulttypep;
int *resultp2typep;
{
	int thisp2type = p2type(thistype);
	int thatp2type = p2type(thattype);

	*resulttypep = thistype;
	*resultp2typep = thisp2type;
	/*
	 * should only be passed scalars
	 */
	if (isnta(thistype,"sbcid") || isnta(thattype,"sbcid")) {
		return;
	}
	if (thisp2type == PCCT_CHAR || thisp2type == PCCT_SHORT) {
		*resultp2typep = PCCT_INT;
		*resulttypep = nl + T4INT;
	}
	if (*resultp2typep == PCCT_INT && thatp2type == PCCT_DOUBLE) {
		*resultp2typep = PCCT_DOUBLE;
		*resulttypep = nl + TDOUBLE;
	}
	sconv(thisp2type, *resultp2typep);
}
    
/*
 * this routine will emit sconv operators when it thinks they are needed.
 * this is code generator specific, rather than machine-specific.
 * this routine takes p2types for arguments, not struct nl *'s.
 */
#if defined(i386)
/*
 * the code generator is very good, this routine is extremely boring.
 */
sconv(fromp2type, top2type)
int fromp2type;
int top2type;
{
	switch (top2type) {
	case PCCT_CHAR:
	case PCCT_SHORT:
	case PCCT_INT:
		switch (fromp2type) {
		case PCCT_CHAR:
		case PCCT_SHORT:
		case PCCT_INT:
		case PCCT_DOUBLE:
			return;	/* pass1 knows how to do these */
		default:
			return;
		}
	case PCCT_DOUBLE:
		switch (fromp2type) {
		case PCCT_CHAR:
		case PCCT_SHORT:
		case PCCT_INT:
			putop(PCC_SCONV, PCCT_DOUBLE);
			return;
		case PCCT_DOUBLE:
			return;
		default:
			return;
		}
	default:
		return;
	}
}
#endif /* i386 */
#endif PC