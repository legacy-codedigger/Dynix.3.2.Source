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
static	char rcsid[] = "$Header: lookup.c 2.0 86/01/28 $";
static	char *sccsid = "@(#)lookup.c	1.3 (Berkeley) 11/2/81";
#endif lint

/*
 * lookup.c
 */

#include "gprof.h"

    /*
     *	look up an address in a sorted-by-address namelist
     *	    this deals with misses by mapping them to the next lower 
     *	    entry point.
     */
nltype *
nllookup( address )
    unsigned long	address;
{
    register long	low;
    register long	middle;
    register long	high;
#   ifdef DEBUG
	register int	probes;

	probes = 0;
#   endif DEBUG
    for ( low = 0 , high = nname - 1 ; low != high ; ) {
#	ifdef DEBUG
	    probes += 1;
#	endif DEBUG
	middle = ( high + low ) >> 1;
	if ( nl[ middle ].value <= address && nl[ middle+1 ].value > address ) {
#	    ifdef DEBUG
		if ( debug & LOOKUPDEBUG ) {
		    printf( "[nllookup] %d (%d) probes\n" , probes , nname-1 );
		}
#	    endif DEBUG
	    return &nl[ middle ];
	}
	if ( nl[ middle ].value > address ) {
	    high = middle;
	} else {
	    low = middle + 1;
	}
    }
    fprintf( stderr , "[nllookup] binary search fails???\n" );
    return 0;
}

arctype *
arclookup( parentp , childp )
    nltype	*parentp;
    nltype	*childp;
{
    arctype	*arcp;

    if ( parentp == 0 || childp == 0 ) {
	fprintf( stderr, "[arclookup] parentp == 0 || childp == 0\n" );
	return 0;
    }
#   ifdef DEBUG
	if ( debug & LOOKUPDEBUG ) {
	    printf( "[arclookup] parent %s child %s\n" ,
		    parentp -> name , childp -> name );
	}
#   endif DEBUG
    for ( arcp = parentp -> children ; arcp ; arcp = arcp -> arc_childlist ) {
#	ifdef DEBUG
	    if ( debug & LOOKUPDEBUG ) {
		printf( "[arclookup]\t arc_parent %s arc_child %s\n" ,
			arcp -> arc_parentp -> name ,
			arcp -> arc_childp -> name );
	    }
#	endif DEBUG
	if ( arcp -> arc_childp == childp ) {
	    return arcp;
	}
    }
    return 0;
}
