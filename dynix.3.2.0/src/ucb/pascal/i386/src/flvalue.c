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
static char rcsid[] = "$Id: flvalue.c,v 1.1 88/09/02 11:48:03 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree.h"
#include "opcode.h"
#include "objfmt.h"
#include "tree_ty.h"
#if defined(PC)
#include "pc.h"
#include <pcc.h>
#endif PC
#include "tmps.h"

    /*
     *	flvalue generates the code to either pass on a formal routine,
     *	or construct the structure which is the environment for passing.
     *	it tells the difference by looking at the tree it's given.
     */
struct nl *
flvalue( r , formalp )
    struct tnode *r; 	/* T_VAR */
    struct nl	*formalp;
    {
	struct nl	*p;
	struct nl	*tempnlp;
	char		*typename;
#if defined(PC)
	char		extname[ BUFSIZ ];
#endif PC

	if ( r == TR_NIL ) {
	    return NLNIL;
	}
	typename = formalp -> class == FFUNC ? "function":"procedure";
	if ( r->tag != T_VAR ) {
	    error("Expression given, %s required for %s parameter %s" ,
		    typename , typename , formalp -> symbol );
	    return NLNIL;
	}
	p = lookup(r->var_node.cptr);
	if (p == NLNIL) {
	    return NLNIL;
	}
	switch ( p -> class ) {
	    case FFUNC:
	    case FPROC:
		    if ( r->var_node.qual != TR_NIL ) {
			error("Formal %s %s cannot be qualified" ,
				typename , p -> symbol );
			return NLNIL;
		    }
#if defined(PC)
			putRV( p -> symbol , bn , p -> value[ NL_OFFS ] , 
				p -> extra_flags ,
				p2type( p ) );
#endif PC
		    return p;
	    case FUNC:
	    case PROC:
		    if ( r->var_node.qual != TR_NIL ) {
			error("%s %s cannot be qualified" , typename ,
				p -> symbol );
			return NLNIL;
		    }
		    if (bn == 0) {
			error("Built-in %s %s cannot be passed as a parameter" ,
				typename , p -> symbol );
			return NLNIL;
		    }
			/*
			 *	allocate space for the thunk (sic ksb)
			 */
		    tempnlp = tmpalloc((long) (sizeof(display)), NLNIL, NOREG);
#if defined(PC)
			putleaf( PCC_ICON , 0 , 0 ,
			    PCCM_ADDTYPE( PCCTM_PTR , PCCM_ADDTYPE( PCCTM_FTN , PCCTM_PTR|PCCT_STRTY ) ) ,
			    "_FSAV" );
			sprintf( extname , "%s" , FORMALPREFIX );
			sextname( &extname[ strlen( extname ) ] ,
				    p -> symbol , bn );
			putleaf( PCC_ICON , 0 , 0 , p2type( p ) , extname );
			putleaf( PCC_ICON , bn , 0 , PCCT_INT , (char *) 0 );
			putop( PCC_CM , PCCT_INT );
			putLV( (char *) 0 , cbn , tempnlp -> value[NL_OFFS] ,
				tempnlp -> extra_flags , PCCT_STRTY );
			putop( PCC_CM , PCCT_INT );
			putop( PCC_CALL , PCCTM_PTR | PCCT_STRTY );
#endif PC
		    return p;
	    default:
		    error("Variable given, %s required for %s parameter %s" ,
			    typename , typename , formalp -> symbol );
		    return NLNIL;
	}
    }
