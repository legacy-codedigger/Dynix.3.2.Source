#include "whoami.h"
#include "0.h"
#include "tree.h"
#include "opcode.h"
#include "objfmt.h"
#if defined(PC)
#include <pcc.h>
#endif PC
#include "tree_ty.h"

/*
 * stkrval Rvalue - an expression, and coerce it to be a stack quantity.
 *
 * Contype is the type that the caller would prefer, nand is important
 * if constant sets or constant strings are involved, the latter
 * because of string padding.
 */
/*
 * for the obj version, this is a copy of rvalue hacked to use fancy new
 * push-onto-stack-and-convert opcodes.
 * for the pc version, i just call rvalue and convert if i have to,
 * based on the return type of rvalue.
 */
struct nl *
stkrval(r, contype , required )
	register struct tnode *r;
	struct nl *contype;
	long	required;
{
	register struct nl *p;
	register struct nl *q;
	register char *cp, *cp1;
	register int c, w;
	struct tnode *pt;
	long l;
	union
	{
		double pdouble;
		long   plong[2];
	}f;

	if (r == TR_NIL)
		return (NLNIL);
	if (nowexp(r))
		return (NLNIL);
	/*
	 * The root of the tree tells us what sort of expression we have.
	 */
	switch (r->tag) {

	/*
	 * The constant nil
	 */
	case T_NIL:
#if defined(PC)
		    putleaf( PCC_ICON , 0 , 0 , PCCT_INT , (char *) 0 );
#endif PC
		return (nl+TNIL);

	case T_FCALL:
	case T_VAR:
		p = lookup(r->var_node.cptr);
		if (p == NLNIL || p->class == BADUSE)
			return (NLNIL);
		switch (p->class) {
		case VAR:
			/*
			 * if a variable is
			 * qualified then get
			 * the rvalue by a
			 * stklval and an ind.
			 */
			if (r->var_node.qual != TR_NIL)
				goto ind;
			q = p->type;
			if (q == NLNIL)
				return (NLNIL);
			if (classify(q) == TSTR)
				return(stklval(r, NOFLAGS));
#if defined(PC)
			    q = rvalue( r , contype , (int) required );
			    if (isa(q, "sbci")) {
				sconv(p2type(q),PCCT_INT);
			    }
			    return q;
#endif PC

		case WITHPTR:
		case REF:
			/*
			 * A stklval for these
			 * is actually what one
			 * might consider a rvalue.
			 */
ind:
			q = stklval(r, NOFLAGS);
			if (q == NLNIL)
				return (NLNIL);
			if (classify(q) == TSTR)
				return(q);
#if defined(PC)
			    if ( required == RREQ ) {
				putop( PCCOM_UNARY PCC_MUL , p2type( q ) );
				if (isa(q,"sbci")) {
				    sconv(p2type(q),PCCT_INT);
				}
			    }
			    return q;
#endif PC

		case CONST:
			if (r->var_node.qual != TR_NIL) {
				error("%s is a constant and cannot be qualified", r->var_node.cptr);
				return (NLNIL);
			}
			q = p->type;
			if (q == NLNIL)
				return (NLNIL);
			if (q == nl+TSTR) {
				/*
				 * Find the size of the string
				 * constant if needed.
				 */
				cp = (char *) p->ptr[0];
cstrng:
				cp1 = cp;
				for (c = 0; *cp++; c++)
					continue;
				w = c;
				if (contype != NIL && !opt('s')) {
					if (width(contype) < c && classify(contype) == TSTR) {
						error("Constant string too long");
						return (NLNIL);
					}
					w = width(contype);
				}
#if defined(PC)
				    putCONG( cp1 , w , LREQ );
#endif PC
				/*
				 * Define the string temporarily
				 * so later people can know its
				 * width.
				 * cleaned out by stat.
				 */
				q = defnl((char *) 0, STR, NLNIL, w);
				q->type = q;
				return (q);
			}
			if (q == nl+T1CHAR) {
#if defined(PC)
				putleaf(PCC_ICON, p -> value[0], 0, PCCT_INT, 
						(char *) 0);
#endif PC
			    return(q);
			}
			/*
			 * Every other kind of constant here
			 */
#if defined(PC)
			    q = rvalue( r , contype , (int) required );
			    if (isa(q,"sbci")) {
				sconv(p2type(q),PCCT_INT);
			    }
			    return q;
#endif PC

		case FUNC:
		case FFUNC:
			/*
			 * Function call
			 */
			pt = r->var_node.qual;
			if (pt != TR_NIL) {
				switch (pt->list_node.list->tag) {
				case T_PTR:
				case T_ARGL:
				case T_ARY:
				case T_FIELD:
					error("Can't qualify a function result value");
					return (NLNIL);
				}
			}
#if defined(PC)
			    p = pcfunccod( r );
			    if (isa(p,"sbci")) {
				sconv(p2type(p),PCCT_INT);
			    }
#endif PC
			return (p);

		case TYPE:
			error("Type names (e.g. %s) allowed only in declarations", p->symbol);
			return (NLNIL);

		case PROC:
		case FPROC:
			error("Procedure %s found where expression required", p->symbol);
			return (NLNIL);
		default:
			panic("stkrvid");
		}
	case T_PLUS:
	case T_MINUS:
	case T_NOT:
	case T_AND:
	case T_OR:
	case T_DIVD:
	case T_MULT:
	case T_SUB:
	case T_ADD:
	case T_MOD:
	case T_DIV:
	case T_EQ:
	case T_NE:
	case T_GE:
	case T_LE:
	case T_GT:
	case T_LT:
	case T_IN:
		p = rvalue(r, contype , (int) required );
#if defined(PC)
		    if (isa(p,"sbci")) {
			sconv(p2type(p),PCCT_INT);
		    }
#endif PC
		return (p);
	case T_CSET:
		p = rvalue(r, contype , (int) required );
		return (p);
	default:
		if (r->const_node.cptr == (char *) NIL)
			return (NLNIL);
		switch (r->tag) {
		default:
			panic("stkrval3");

		/*
		 * An octal number
		 */
		case T_BINT:
			f.pdouble = a8tol(r->const_node.cptr);
			goto conint;
	
		/*
		 * A decimal number
		 */
		case T_INT:
			f.pdouble = atof(r->const_node.cptr);
conint:
			if (f.pdouble > MAXINT || f.pdouble < MININT) {
				error("Constant too large for this implementation");
				return (NLNIL);
			}
			l = f.pdouble;
			if (bytes(l, l) <= 2) {
#if defined(PC)
				putleaf( PCC_ICON , (short) l , 0 , PCCT_INT , 
						(char *) 0 );
#endif PC
				return(nl+T4INT);
			}
#if defined(PC)
			    putleaf( PCC_ICON , (int) l , 0 , PCCT_INT , (char *) 0 );
#endif PC
			return (nl+T4INT);
	
		/*
		 * A floating point number
		 */
		case T_FINT:
#if defined(PC)
			    putCON8( atof( r->const_node.cptr ) );
#endif PC
			return (nl+TDOUBLE);
	
		/*
		 * Constant strings.  Note that constant characters
		 * are constant strings of length one; there is
		 * no constant string of length one.
		 */
		case T_STRNG:
			cp = r->const_node.cptr;
			if (cp[1] == 0) {
#if defined(PC)
				    putleaf( PCC_ICON , cp[0] , 0 , PCCT_INT , 
						(char *) 0 );
#endif PC
				return(nl+T1CHAR);
			}
			goto cstrng;
		}
	
	}
}
