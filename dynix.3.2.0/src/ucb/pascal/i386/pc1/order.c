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
 *
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#if !defined(lint)
static char rcsid[] =
	"$Id: order.c,v 2.8 88/09/02 11:46:23 ksb Exp $";
#endif lint

/*
 *	Berkeley Pascal Compiler	(order.c)
 */

#include "pass2.h"

int maxargs = { -1 };

/* should we delay the INCR or DECR operation p */
deltest( p )
register NODE *p;
{
	p = p->in.left;
	return p->in.op == REG || p->in.op == NAME || p->in.op == OREG;
}

/* ZZ comment */
autoincr(p)
NODE *p;
{
	register NODE *q = p->in.left;

	if( q->in.op == INCR && q->in.left->in.op == REG &&
	    ISPTR(q->in.type) && p->in.type == DECREF(q->in.type) &&
	    tlen(p) == q->in.right->tn.lval )
		return(1);

	return 0;
}

/*
 * is it legal to make an OREG or NAME entry which has an offset of
 * off, (from a register of r), if the * resulting thing had type t
 */
notoff( t, r, off, cp)
TWORD t;
CONSZ off;
char *cp;
{
	return(0);  /* YES */
}

#define MAX(x,y) ((x)<(y)?(y):(x))

/*
 * set the su field in the node to the sethi-ullman number,
 * or local equivalent
 */
sucomp( p )
register NODE *p;
{
	register int o, ty, sul, sur, r;

	o = p->in.op;
	ty = optype(o);

	/* size in regs always 1; BREG treated as 1 (even though 2 physical
	 * registers are used to make one on a ns32000)
	 */
	p->in.su = 1;

	if (ty == LTYPE) {
		if( o == OREG ) {
			r = p->tn.rval;

			if (istreg(r)) {
				++p->in.su;
			}
		}
		if( p->in.su == 1 &&
		   (o != REG || !istreg(p->tn.rval)) &&
		   (p->in.type==INT || p->in.type==UNSIGNED ||
		    p->in.type==FLOAT || p->in.type==DOUBLE ||
		    ISPTR(p->in.type) || ISARY(p->in.type)) )
			p->in.su = 0;
		return;
	}

	if( ty == UTYPE ) {
		switch (o) {
		case UNARY CALL:
		case UNARY STCALL:
			p->in.su = fregs;  /* all regs needed */
			return;
		default:
			p->in.su =  p->in.left->in.su +
				    ((p->in.type == DOUBLE)  ? 1 : 0);
			break;
		}
		return;
	}

	/* If rhs needs n, lhs needs m, regular su computation
	 * sometime we bump the estimate for one side for a temp
	 * while we evaluate the other.
	 */
	sul = p->in.left->in.su;
	sur = p->in.right->in.su;
	switch (o) {
	case ASSIGN:
		/* computed by doing right, then left (if not in mem)
		 * then doing it, we add one to left to hold lvalue
		 */
		++sul;
		p->in.su = MAX(sur,sul);
		return;

	case CALL:
	case STCALL:
		/* in effect, takes all free registers */
		p->in.su = fregs;
		return;

	case STASG:
		/* right, then left */
		++sur;
		if ((p->in.su = MAX(sul, sur)) < fregs)
			p->in.su = fregs;
		return;

	case ANDAND:
	case OROR:
	case QUEST:
	case COLON:
	case COMOP:
		if ((p->in.su = MAX(sul,sur)) < 1)
			p->in.su = 1;
		return;
	default:
		if( asgop(o) ) {
			/* computed by doing right, doing left address,
			 * doing left, op, and store
			 * (how did Pascal gett these ZZZ)
			 */
			++sul;	/* lvalue */
			++sur;	/* rvalue (ksb) */
			p->in.su = MAX(sul,sur);
			return;
		}
		break;
	}

	/* binary op, computed by left, then right, then do op
	 * we increment right for register to hold return value
	 */
	++sur;
	p->in.su = MAX(sul, sur);
	if (p->in.op == DIV || p->in.op == MOD)
		p->in.su += 3;
}

#undef MAX

int radebug = 0;

/*
 * do register allocation	replace this sometime
 */
rallo(p, above)
register NODE *p;
int above;
{
	register int rtemp, ty;

	ty = optype(p->in.op);

	rtemp = RA_NONE;
	if (p->in.type != DOUBLE) {
		switch (p->in.op) {
		case CALL:
		case UNARY CALL:
		case FORCE:
			rtemp = RA_SUGGEST|RA_USE(EAX);
			break;

		case ASSIGN:
		case STASG:
		case STARG:
			break;

		case EQ:
		case NE:
		case GT:
		case GE:
		case LT:
		case LE:
		case NOT:
		case ANDAND:
		case OROR:
			break;
		}
	}
	p->in.rall = rtemp;
	if (ty != LTYPE) {
		rallo(p->in.left, rtemp);
		if (ty == BITYPE) {
			if (p->in.left->in.rall & RA_SUGGEST)
				p->in.right->in.rall = RA_REJECT|RA_USE(RA_REG(p->in.left->in.rall));
			else if (rtemp & RA_SUGGEST)
				p->in.right->in.rall = RA_REJECT|RA_USE(RA_REG(rtemp));
			else
				rallo(p->in.right, RA_NONE);
		}
	} else if (ty == BITYPE) {
		rallo(p->in.right, rtemp);
	}
}

/*
 * nudge an OREG, REG, or NAME node (or even ICON?).
 */
nudge(p, off)
NODE *p;
int off;
{
	switch (p->in.op) {
	case OREG:
	case ICON:
	case NAME:
		p->tn.lval += off;
		break;
	case REG:
		printf("\taddl\t$%d,", off);
		adrput(p);
		printf("\n");
		break;
	default:
		cerror("cannot nudge this node");
		break;
	}
}

/*
 * structure assignment, or a push.
 */
int
setstr(p, cookie)
register NODE *p;
{
	static char *_stkmv[] = { "b", "w", (char *) 0, "l" };
	static char *apc_cpys[3] = {
		"\tmovl\t%RA,%1A\n\tmovl\t%1A,%LA\n",
		"\tmovw\t%RA,%1A\n\tmovw\t%1A,%LA\n",
		"\tmovb\t%RA,%1A\n\tmovb\t%1A,%LA\n"
	};
	register NODE *l, *r;
	register int size;
	auto int oddbytes, reg;
	auto int stkbias;
	auto int sp;
	auto NODE pseudol;			/* hold stack for us	*/

	/* get size and make r be source ADDRESS, left dest
	 */
	size = p->stn.stsize;
	if (p->in.op == STASG) {		/* struct copy		*/
		register NODE *t;
		lvalue(p->in.left);
		l = p->in.left;
		if (M_DONE != order(p->in.right, SH_ORDER)) {
			return M_FAIL;
		}
		r = p->in.right;
		t = talloc();
		nxfer(t, r);
		r->in.op = UNARY MUL;
		r->in.left = t;
	} else if (p->in.op == STARG) {		/* struct to stack	*/
		SETOFF(size, 4);		/* keep stack alligned	*/
		r = p->in.left;
		pseudol.tn.op = OREG;		/* is an lvalue		*/
		pseudol.tn.type = INT;
		pseudol.tn.su = 0;
		pseudol.tn.lval = 0;
		pseudol.tn.rval = ESP;
#if defined(FLEXNAMES)
		pseudol.in.name = "";
#else
		pseudol.in.name[0] = '\000';
#endif
		p->in.op = STASG;		/* for expand's sake	*/
		p->in.left = l = & pseudol;
		p->in.right = r;
		/* below this point we check (l == & pseudol) to
		 * see if we are moving to stack space (ksb)
		 */
	} else {
		cerror("struct assign unknown?");
	}
	lvalue(r);

	/* handle normal sizes, be cool
	 */
	if (size == 1 || size == 2 || size == 4) {
		if (l == & pseudol) {
			(void)allo(p, NEED0);
			if (size != 4) {
				cvttype(p, cookie|SH_AREG|SH_TAREG|SH_EAX);
			}
			printf("\tpushl\t");
			adrput(r);
		} else if ((OREG == r->in.op || NAME == r->in.op) &&
			(OREG == l->in.op || NAME == l->in.op)) {
			(void)allo(p, NEED1(N_AREG));
			printf("\tmov%s\t", _stkmv[size-1]);
			adrput(r);
			printf(",");
			adrput(& resc[0]);
			printf("\n\tmov%s\t", _stkmv[size-1]);
			adrput(& resc[0]);
			printf(",");
			adrput(l);
		} else {
			(void)allo(p, NEED0);
			printf("\tmov%s\t", _stkmv[size-1]);
			adrput(r);
			printf(",");
			adrput(l);
		}
		printf("\n");
	} else {
		if (l == & pseudol) {
			printf("\tsubl\t$%d,%s\n", size, amregs[ESP].acname);
		}
		/* if we did push these regs, how much memory? */
		stkbias = ISBUSY(ESI) + ISBUSY(EDI) + ISBUSY(ECX);
		oddbytes = size & 3;
		size >>= 2;
		/* brute force, quickly
		 */
		if (size <= (stkbias + 5)) {
			sp = -1;
			if (! allo(p, NEED1(N_AREG)))
				return M_FAIL;

			while (size > 0) {
				expand(p, cookie, apc_cpys[0], ASSIGN);
				nudge(l, 4);
				nudge(r, 4);
				--size;
			}
			if (oddbytes & 2) {
				expand(p, cookie, apc_cpys[1], ASSIGN);
				nudge(l, 2);
				nudge(r, 2);
			}
			if (oddbytes & 1) {
				expand(p, cookie, apc_cpys[2], ASSIGN);
			}
		} else {
			(void)allo(p, NEED0);
			sp = spill(ESI, EDI, ECX, -1);

			if (UNARY MUL == r->tn.op) {
				printf("\tmovl\t");
				adrput(r->in.left);
				r = tfree1(r);
				p->in.right = r;
			} else {
				printf("\tleal\t");
				adrput(r);
			}
			printf(",%s\n", amregs[ESI].acname);

			if (l != & pseudol) {
				if (OREG == l->in.op && 0 == l->tn.lval) {
					printf("\tmovl\t%s", amregs[l->tn.rval].acname);
				} else {
					printf("\tleal\t");
					adrput(l);
				}
			} else if (0 == stkbias) {
				printf("\tmovl\t%s", amregs[ESP].acname);
			} else {
				printf("\tleal\t%d(%s)", stkbias, amregs[ESP].acname);
			}
			printf(",%s\n", amregs[EDI].acname);

			printf("\tmovl\t$%d,%s\n", size, amregs[ECX].acname);
			printf("\trep; smovl\n");

			if (oddbytes & 2) {
				printf("\tmovw\t0(%s),%s\n", amregs[ESI].acname, amregs[ECX].acname);
				printf("\tmovw\t%s,0(%s)\n", amregs[ECX].acname, amregs[EDI].acname);
			}
			if (oddbytes & 1) {
				printf("\tmovb\t%d(%s),%s\n", oddbytes & 2, amregs[ESI].acname, amregs[ECX].acname);
				printf("\tmovb\t%s,%d(%s)\n", amregs[ECX].acname, oddbytes & 2, amregs[EDI].acname);
			}
		}
		if (-1 != sp) {
			unspill(sp);
		}
	}
	if (0 != r) {
		recl2(r);
		tfree(r);
	}
	if (0 != l) {
		recl2(l);
		if (l != & pseudol)
			tfree(l);
	}
	p->in.op = ICON;	/* yeah, we did the kids by hand here */
	reclaim(p, RC_NULL, cookie);
	return M_DONE;
}

/*
 * setup for assignment (=)
 *  make the this into am oreg or name addressing mode
 */
int
lvalue(p)
register NODE *p;
{
	register NODE *l;
	register int o = p->in.op;
	auto int oty;

	switch (o) {
	case NAME:
	case OREG:
		break;
	case UNARY MUL:
		l = p->in.left;
		oty = p->in.type;
		(void)order(l, SH_ORDER);
		if (l->in.op == ICON) {
			l->in.op = NAME;
			nxfer(p, l);
			tfree1(l);
		} else if (l->in.op == REG) {
			l->in.op = OREG;
			nxfer(p, l);
			tfree1(l);
		} else {
			order(p, SH_ORDER);
		}
		p->in.type = oty;
		break;
	case REG:
	case ICON:
		/* internal assignment to temp... OK (ksb) */
		break;
	default:
		cerror("cannot form lvalue");
		return 0;
	}
	return 1;
}

/* setup for =ops */
setasop(p)
register NODE *p;
{
	register int rt, ro;

	rt = p->in.right->in.type;
	ro = p->in.right->in.op;

	if (rt == CHAR || rt == SHORT || rt == UCHAR || rt == USHORT ||
	    (ro != REG && ro != ICON && ro != NAME && ro != OREG)) {
		return order(p->in.right, SH_ORDER);
	}
	p = p->in.left;
	if( p->in.op == FLD )
		p = p->in.left;

	switch( p->in.op ) {
	case REG:
	case ICON:
	case NAME:
	case OREG:
		return(0);
	case UNARY MUL:
		return(0);
	}
	cerror( "illegal setasop" );
	/*NOTREACHED*/
}

LABEL crslab = 99999;  /* VAX */

LABEL
getlab()
{
	return crslab--;
}

deflab( l )
LABEL l;
{
	printf( "L%d:\n", l );
}

/*
 * generate code for the arguments
 */
genarg(p)
register NODE *p;
{
	register NODE *pasg;
	register align;
	register size;

	if (p->in.op == STARG) {	/* structure valued argument */
		size = p->stn.stsize;
		align = p->stn.stalign;
		if (p->in.left->in.op == ICON) {
			p = tfree1(p);
		} else {
			/* make it look beautiful... */
			p->in.op = UNARY MUL;
			canon(p);  /* turn it into an oreg */
			if( p->in.op != OREG ) {
				(void)order(p->in.left, SH_ANY_AREG);
				switch(p->in.left->tn.op) {
				case NAME:
				case ICON:
				case OREG:
				case REG:
					break;
				default:
					canon(p);
					order(p, SH_ANY_AREG);
					if (p->in.op != OREG)
						cerror("stuck with starg");
					break;
				}
			}
		}
		pasg = talloc();
		pasg->in.op = STARG;
		pasg->in.rall = 0;
		pasg->stn.stsize = size;
		pasg->stn.stalign = align;
		pasg->in.left = p;
		return order(pasg, SH_ORDER);
	}

	if (M_DONE == order(p, SH_ORDER)) {
		switch (p->in.type) {
		case CHAR:
		case UCHAR:
		case SHORT:
		case USHORT:
			cvttype(p, SH_ORDER);
			/*fallthough*/
		default:
			if (ICON == p->in.op) {
				expand(p, SH_NONE, "\tpushl\t$%A\n", ASSIGN);
			} else {
				expand(p, SH_NONE, "\tpushl\t%A\n", ASSIGN);
			}
			break;
		case DOUBLE:
			if (REG == p->in.op) {
				expand(p, SH_NONE, "\tsubl\t$8,%%esp\n", ASSIGN);
				expand(p, SH_NONE, "\tfstp\t0(%%esp)\n", ASSIGN);
			} else {
				expand(p, SH_NONE, "\tpushl\t4+%A\n", ASSIGN);
				expand(p, SH_NONE, "\tpushl\t%A\n", ASSIGN);
			}
			return M_DONE;
		}
		return M_DONE;
	}
	return M_FAIL;
}

argsize(p)
register NODE *p;
{
	register int t;

	if( p->in.op == CM ) {
		t = argsize( p->in.left );
		p = p->in.right;
	} else {
		t = 0;
	}
	SETOFF(t, 4);
	if (p->in.type == DOUBLE) {
		t += 8;
	} else if (p->in.op == STARG) {
		t += (p->stn.stsize & ~3);  	/* size */
		if (p->stn.stsize & 3)
			t += 4;
	} else {
		t += 4;
	}
	return t;
}
