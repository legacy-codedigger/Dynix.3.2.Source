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
static char rcsid[] = "$Id: lval.c,v 1.1 88/09/02 11:48:08 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree.h"
#include "opcode.h"
#include "objfmt.h"
#include "tree_ty.h"
#if defined(PC)
#include	"pc.h"
#include	<pcc.h>
#endif PC

extern	int flagwas;
/*
 * Lvalue computes the address
 * of a qualified name and
 * leaves it on the stack.
 * for pc, it can be asked for either an lvalue or an rvalue.
 * the semantics are the same, only the code is different.
 */
/*ARGSUSED*/
struct nl *
lvalue(var, modflag , required )
	struct tnode *var; 
	int	modflag;
	int	required;
{
	if (var == TR_NIL) {
		return (NLNIL);
	}
	if (nowexp(var)) {
		return (NLNIL);
	}
	if (var->tag != T_VAR) {
		error("Variable required");	/* Pass mesgs down from pt of call ? */
		return (NLNIL);
	}
#if defined(PC)
		/*
		 *	pc requires a whole different control flow
		 */
	    return pclvalue( var , modflag , required );
#endif PC
}

int lptr(c)
	register struct tnode *c;
{
	register struct tnode *co;

	for (; c != TR_NIL; c = c->list_node.next) {
		co = c->list_node.list;
		if (co == TR_NIL) {
			return (NIL);
		}
		switch (co->tag) {

		case T_PTR:
			return (1);
		case T_ARGL:
			return (0);
		case T_ARY:
		case T_FIELD:
			continue;
		default:
			panic("lptr");
		}
	}
	return (0);
}

/*
 * Arycod does the
 * code generation
 * for subscripting.
 * n is the number of
 * subscripts already seen
 * (CLN 09/13/83)
 */
int arycod(np, el, n)
	struct nl *np;
	struct tnode *el;
	int n;
{
	register struct nl *p, *ap;
	long sub;
	bool constsub;
	extern bool constval();
	int i, d;  /* v, v1;  these aren't used */
	int w;

	p = np;
	if (el == TR_NIL) {
		return (0);
	}
	d = p->value[0];
	for (i = 1; i <= n; i++) {
		p = p->chain;
	}
	/*
	 * Check each subscript
	 */
	for (i = n+1; i <= d; i++) {
		if (el == TR_NIL) {
			return (i-1);
		}
		p = p->chain;
		if (p == NLNIL)
			return (0);
		if ((p->class != CRANGE) &&
			(constsub = constval(el->list_node.list))) {
		    ap = con.ctype;
		    sub = con.crval;
		    if (sub < p->range[0] || sub > p->range[1]) {
			error("Subscript value of %D is out of range", (char *) sub);
			return (0);
		    }
		    sub -= p->range[0];
		} else {
#if defined(PC)
			precheck( p , "_SUBSC" , "_SUBSCZ" );
#endif PC
		    ap = rvalue(el->list_node.list, NLNIL , RREQ );
		    if (ap == NIL) {
			    return (0);
		    }
#if defined(PC)
			postcheck(p, ap);
			sconv(p2type(ap),PCCT_INT);
#endif PC
		}
		if (incompat(ap, p->type, el->list_node.list)) {
			cerror("Array index type incompatible with declared index type");
			if (d != 1) {
				cerror("Error occurred on index number %d", (char *) i);
			}
			return (-1);
		}
		if (p->class == CRANGE) {
			constsub = FALSE;
		} else {
			w = aryconst(np, i);
		}
#if defined(PC)
			/*
			 *	subtract off the lower bound
			 */
		    if (constsub) {
			sub *= w;
			if (sub != 0) {
			    putleaf( PCC_ICON , (int) sub , 0 , PCCT_INT , (char *) 0 );
			    putop(PCC_PLUS, PCCM_ADDTYPE(p2type(np->type), PCCTM_PTR));
			}
			el = el->list_node.next;
			continue;
		    }
		    if (p->class == CRANGE) {
			/*
			 *	if conformant array, subtract off lower bound
			 */
			ap = p->nptr[0];
			putRV(ap->symbol, (ap->nl_block & 037), ap->value[0], 
				ap->extra_flags, p2type( ap ) );
			putop( PCC_MINUS, PCCT_INT );
			/*
			 *	and multiply by the width of the elements
			 */
			ap = p->nptr[2];
			putRV( 0 , (ap->nl_block & 037), ap->value[0], 
				ap->extra_flags, p2type( ap ) );
			putop( PCC_MUL , PCCT_INT );
		    } else {
			if ( p -> range[ 0 ] != 0 ) {
			    putleaf( PCC_ICON , (int) p -> range[0] , 0 , PCCT_INT , (char *) 0 );
			    putop( PCC_MINUS , PCCT_INT );
			}
			    /*
			     *	multiply by the width of the elements
			     */
			if ( w != 1 ) {
			    putleaf( PCC_ICON , w , 0 , PCCT_INT , (char *) 0 );
			    putop( PCC_MUL , PCCT_INT );
			}
		    }
			/*
			 *	and add it to the base address
			 */
		    putop( PCC_PLUS , PCCM_ADDTYPE( p2type( np -> type ) , PCCTM_PTR ) );
		el = el->list_node.next;
#endif PC
	}
	if (el != TR_NIL) {
	    if (np->type->class != ARRAY) {
		do {
			el = el->list_node.next;
			i++;
		} while (el != TR_NIL);
		error("Too many subscripts (%d given, %d required)", (char *) (i-1), (char *) d);
		return (-1);
	    } else {
		return(arycod(np->type, el, d));
	    }
	}
	return (d);
}
