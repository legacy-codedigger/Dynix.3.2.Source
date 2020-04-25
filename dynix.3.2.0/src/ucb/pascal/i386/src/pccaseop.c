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
static char rcsid[] = "$Id: pccaseop.c,v 1.3 88/09/17 03:00:07 ksb Exp $";
#endif lint

#include "whoami.h"
#if defined(PC)
    /*
     *	and the rest of the file
     */
#include "0.h"
#include "tree.h"
#include "objfmt.h"
#include <pcc.h>
#include "pc.h"
#include "tmps.h"
#include "tree_ty.h"

    /*
     *	structure for a case: 
     *	    its constant label, line number (for errors), and location label.
     */
struct ct {
    long	cconst;
    int		cline;
    int		clabel;
};

    /*
     *	the PCC_FORCE operator puts its operand into a register.
     *	these to keep from thinking of it as r0 all over.
     */
#if defined(i386)
#define	FORCENAME	"%eax"
#endif /* i386 */

    /*
     *	given a tree for a case statement, generate code for it.
     *	this computes the expression into a register,
     *	puts down the code for each of the cases,
     *	and then decides how to do the case switching.
     *	tcase	[0]	T_CASE
     *		[1]	lineof "case"
     *		[2]	expression
     *		[3]	list of cased statements:
     *			cstat	[0]	T_CSTAT
     *				[1]	lineof ":"
     *				[2]	list of constant labels
     *				[3]	statement
     */
pccaseop( tcase )
    WHI_CAS *tcase;
{
    struct nl	*exprtype;
    struct nl	*exprnlp;
    struct nl	*rangetype;
    long	low;
    long	high;
    long	exprctype;
    char 	*swlabel;
    char	*endlabel;
    char	*label;
    int		count;
    struct tnode *cstatlp;
    struct tnode *cstatp;
    struct tnode *casep;
    struct ct	*ctab;
    struct ct	*ctp;
    bool	nr;
    long	goc;
    int		casecmp();
    bool	dupcases;

    goc = gocnt;
	/*
	 *  find out the type of the case expression
	 *  even if the expression has errors (exprtype == NIL), continue.
	 */
    line = tcase->line_no;
    codeoff();
    exprtype = rvalue( tcase->expr , NLNIL  , RREQ );
    codeon();
    if ( exprtype != NLNIL ) {
	if ( isnta( exprtype , "bcsi" ) ) {
	    error("Case selectors cannot be %ss" , nameof( exprtype ) );
	    exprtype = NLNIL;
	} else {
	    if ( exprtype -> class != RANGE ) {
		rangetype = exprtype -> type;
	    } else {
		rangetype = exprtype;
	    }
	    if ( rangetype == NLNIL ) {
		exprtype = NLNIL;
	    } else {
		low = rangetype -> range[0];
		high = rangetype -> range[1];
	    }
	}
    }
    if ( exprtype != NLNIL ) {
	    /*
	     *	compute and save the case expression.
	     *	also, put expression into a register
	     *	save its c-type and jump to the code to do the switch.
	     */
	exprctype = p2type( exprtype );
	exprnlp = tmpalloc( (long) (sizeof (long)), nl + T4INT , NOREG );
	putRV((char *) 0 , cbn , exprnlp -> value[ NL_OFFS ] ,
			exprnlp -> extra_flags , PCCT_INT );
	(void) rvalue( tcase->expr , NLNIL , RREQ );
	sconv((int) exprctype, (int) PCCT_INT);
	putop( PCC_ASSIGN , PCCT_INT );
	putop( PCC_FORCE , PCCT_INT );
	putdot( filename , line );
	swlabel = getlab();
	putjbr( (long) swlabel );
    }
	/*
	 *  count the number of cases
	 *  and allocate table for cases, lines, and labels
	 *  default case goes in ctab[0].
	 */
    count = 1;
    for ( cstatlp = tcase->stmnt_list ; cstatlp != TR_NIL ;
		cstatlp = cstatlp->list_node.next ) {
	cstatp = cstatlp->list_node.list;
	if ( cstatp == TR_NIL ) {
	    continue;
	}
	for ( casep = cstatp->c_stmnt.const_list ; casep != TR_NIL ;
			casep = casep->list_node.next ) {
	    count++;
	}
    }
	/*
	 */
    ctab = (struct ct *) malloc( count * sizeof( struct ct ) );
    if ( ctab == (struct ct *) 0 ) {
	error("Ran out of memory (case)");
	pexit( DIED );
    }
	/*
	 *  pick up default label and label for after case statement.
	 */
    ctab[0].clabel = (int) getlab();
    endlabel = getlab();
	/*
	 *  generate code for each case
	 *  filling in ctab for each.
	 *  nr is for error if no case falls out bottom.
	 */
    nr = TRUE;;
    count = 0;
    for ( cstatlp = tcase->stmnt_list ; cstatlp != TR_NIL ;
		cstatlp = cstatlp->list_node.next ) {
	cstatp = cstatlp->list_node.list;
	if ( cstatp == TR_NIL ) {
	    continue;
	}
	line = cstatp->c_stmnt.line_no;
	label = getlab();
	for ( casep = cstatp->c_stmnt.const_list ; casep != TR_NIL ;
			casep = casep->list_node.next ) {
	    gconst( casep->list_node.list );
	    if( exprtype == NLNIL || con.ctype == NIL ) {
		continue;
	    }
	    if ( incompat( con.ctype , exprtype , TR_NIL ) ) {
		cerror("Case label type clashed with case selector expression type");
		continue;
	    }
	    if ( con.crval < low || con.crval > high ) {
		error("Case label out of range");
		continue;
	    }
	    count++;
	    ctab[ count ].cconst = con.crval;
	    ctab[ count ].cline = line;
	    ctab[ count ].clabel = (int) label;
	}
	    /*
	     *	put out the statement
	     */
	(void) putlab( label );
	putcnt();
	level++;
	statement( cstatp->c_stmnt.stmnt );
	nr = (nr && noreach)?TRUE:FALSE;
	noreach = FALSE;
	level--;
	if (gotos[cbn]) {
		ungoto();
	}
	putjbr( (long) endlabel );
    }
    noreach = nr;
	/*
	 *	default action is to call error
	 */
    (void) putlab( (char *) ctab[0].clabel );
    putleaf( PCC_ICON, 0, 0, PCCM_ADDTYPE( PCCTM_FTN | PCCT_INT, PCCTM_PTR ) , "_CASERNG" );
    putRV((char *) 0, cbn, exprnlp->value[ NL_OFFS ], exprnlp->extra_flags, PCCT_INT );
    putop( PCC_CALL , PCCT_INT );
    putdot( filename , line );
	/*
	 *  sort the cases
	 */
    qsort( &ctab[1] , count , sizeof (struct ct) , casecmp );
	/*
	 *  check for duplicates
	 */
    dupcases = FALSE;
    for ( ctp = &ctab[1] ; ctp < &ctab[ count ] ; ctp++ ) {
	if ( ctp[0].cconst == ctp[1].cconst ) {
	    error("Multiply defined label in case, lines %d and %d" ,
		    (char *) ctp[0].cline , (char *) ctp[1].cline );
	    dupcases = TRUE;
	}
    }
    if ( dupcases ) {
	return;
    }
	/*
	 *  choose a switch algorithm and implement it:
	 *	direct switch	>= 1/3 full and >= 4 cases.
	 *	binary switch	not direct switch and > 8 cases.
	 *	ifthenelse	not direct or binary switch.
	 */
    (void) putlab( swlabel );
    if ( ctab[ count ].cconst < (ctab[1].cconst + 3 * count) && count >= 4 ) {
	directsw( ctab , count );
    } else if ( count > 8 ) {
	binarysw( ctab , count );
    } else {
	itesw( ctab , count );
    }
    (void) putlab( endlabel );
    if ( goc != gocnt ) {
	    putcnt();
    }
}

    /*
     *	direct switch
     */
directsw( ctab , count )
    struct ct	*ctab;
    int		count;
{
    int		fromlabel = (int) getlab();
    long	i;
    long	j;

    /*
     *	We can use a table of relative offsets.
     */
    putprintf("\tcmpl\t$%d,%s", 0, ctab[1].cconst, FORCENAME);
    putprintf("\tjl\t%s%d", 0, (int) LABELPREFIX, ctab[0].clabel);
    putprintf("\tcmpl\t$%d,%s", 0, ctab[count].cconst, FORCENAME);
    putprintf("\tjg\t%s%d", 0, (int) LABELPREFIX, ctab[0].clabel);
    /*
     *    execution continues here if value not in range of case.
     */
    if (0 == ctab[1].cconst) {
	/* nothing */;
    } else if (ctab[1].cconst < 0) {
	putprintf("\taddl\t$%d,%s", 0, -ctab[1].cconst, FORCENAME);
    } else {
	putprintf("\tsubl\t$%d,%s", 0, ctab[1].cconst, FORCENAME);
    }
    putprintf("\tmovl\t%s%d(,%s,4),%s", 0, (int) LABELPREFIX, fromlabel,
	FORCENAME, FORCENAME);
    putprintf("\tjmp\t*%s", 0, FORCENAME);
   (void) putlab( (char *) fromlabel );
    i = 1;
    j = ctab[1].cconst;
    while ( i <= count ) {
	if ( j == ctab[ i ].cconst ) {
	    putprintf( "\t.long\t" , 1 );
	    putprintf( PREFIXFORMAT , 0 , (int) LABELPREFIX , ctab[ i ].clabel );
	    i++;
	} else {
	    putprintf( "\t.long\t" , 1 );
	    putprintf( PREFIXFORMAT , 0 , (int) LABELPREFIX , ctab[ 0 ].clabel );
	}
	j++;
    }
}

    /*
     *	binary switch
     *	special case out default label and start recursion.
     */ 
binarysw( ctab , count )
    struct ct	*ctab;
    int		count;
{
    
    bsrecur( ctab[0].clabel , &ctab[0] , count );
}

    /*
     *	recursive log( count ) search.
     */
bsrecur( deflabel , ctab , count )
    int		deflabel;
    struct ct	*ctab;
    int		count;
{
	int	half, gtrlabel;

	if ( count <= 0 ) {
		putjbr((long) deflabel);
		return;
	} 
	if ( count == 1 ) {
#if defined(i386)
		putprintf("\tcmpl\t$%d,%s", 0, ctab[1].cconst, FORCENAME);
		putprintf("\tje\t%s%d", 0, LABELPREFIX, ctab[1].clabel);
		putjbr((long) deflabel);
#endif /* i386 */
		return;
	} 
	half = ( count + 1 ) / 2;
	gtrlabel = (int) getlab();

#if defined(i386)
	putprintf("\tcmpl\t$%d,%s", 0, ctab[half].cconst, FORCENAME);
	putprintf("\tjg\t%s%d", 0, LABELPREFIX, gtrlabel);
	putprintf("\tje\t%s%d", 0, LABELPREFIX, ctab[half].clabel);
#endif /* i386 */
	bsrecur( deflabel , &ctab[0] , half - 1 );
	(void) putlab((char *) gtrlabel);
	bsrecur( deflabel , &ctab[ half ] , count - half );
}

itesw( ctab , count )
    struct ct	*ctab;
    int		count;
{
    int	i;

    for ( i = 1 ; i <= count ; i++ ) {
/* ZZZ: cmp beq */
#if defined(i386)
	    putprintf("\tcmpl\t$%d,%s", 0, ctab[i].cconst, FORCENAME);
	    putprintf("\tje\t%s%d", 0, LABELPREFIX, ctab[i].clabel);
#endif /* i386 */
    }
    putjbr((long) ctab[0].clabel);
    return;
}
int
casecmp( this , that )
    struct ct 	*this;
    struct ct 	*that;
{
    if ( this -> cconst < that -> cconst ) {
	return -1;
    } else if ( this -> cconst > that -> cconst ) {
	return 1;
    } else {
	return 0;
    }
}
#endif PC
