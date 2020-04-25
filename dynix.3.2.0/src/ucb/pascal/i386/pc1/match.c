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
static char rcsid[] =
	"$Id: match.c,v 2.14 88/12/01 10:46:00 ksb Exp $";
#endif lint

/*
 *	Berkeley Pascal Compiler	(match.c)
 */

#include <ctype.h>
#include "pass2.h"

#if defined(WCARD1)
#if defined(WCARD2)
#define NOINDIRECT
#endif
#endif

extern int vdebug;
int fldsz, fldshf;

/*
 * return all the shapes this node can be
 */
int
tshape(p)
NODE *p;
{
	register int o;

	o = p->in.op;

	/*
	 * we check for special constant shapes
	 */
	if (o == ICON) {
		if (p->tn.rval == SETCON)		/* ZZZ fold */
			return SH_QCON|SH_BCON|SH_WCON|SH_LCON;
		if (p->tn.lval >= -8 && p->tn.lval <= 7)
			return SH_QCON|SH_BCON|SH_WCON|SH_LCON;
		if (p->tn.lval >= -128 && p->tn.lval <= 127)
			return SH_BCON|SH_WCON|SH_LCON;
		if (p->tn.lval >= -32768 && p->tn.lval <= 32767)
			return SH_WCON|SH_LCON;
		return SH_LCON;
	}

	switch (o) {
	case NAME:
		return SH_NAME;
	case FLD:
		if( !flshape( p->in.left ) )
			return(0);

		/* it is a FIELD shape; make side-effects */

		o = p->tn.rval;
		fldsz = UPKFSZ(o);
#if defined(RTOLBYTES)
		fldshf = UPKFOFF(o);
#else
		fldshf = SZINT - fldsz - UPKFOFF(o);
#endif
		return SH_FLD;
	case CCODES:
		cerror("codes?");
	case REG:
		return amregs[p->tn.rval].istatus;
	case OREG:
		return SH_OAREG;
	case UNARY MUL:
		/* return 0 for force match to recduce us*/
		return 0;
	case STARG:
		return tshape(p->in.left);
	default:
		break;
	}
	return 0;
}

/*
 * shape transmute -- change a shape into something close to what we
 * want, a shape request of 0 indicates we don't know, just try something
 * that make sense for this node (like moving to a temp register, or
 * something).
 */
MATCH
sxmute(p, cookie)
NODE *p;
int cookie;
{
	register int reg;
	register NODE *q, *r;

	if (cookie & SH_TAREG|SH_AREG) {
		q = talloc();
		r = talloc();
		nxfer(q, p);

		p->in.op = ASSIGN;
		p->in.su = 1;
		p->in.right = q;
		p->in.left = r;

		r->in.op = REG;
		r->in.type = q->in.type;
		r->tn.rval = freereg(r, N__A);
		r->tn.lval = 0;

#if defined(FLEXNAMES)
		p->in.name = "";
		r->in.name = "";
#else
		p->in.name[0] = '\0';
		r->in.name[0] = '\0';
#endif
		reg = order(p, cookie);
		tfree1(r);	/* drive match reclaimed */
		tfree1(q);
		return reg;
	}
	if (cookie & SH_EAX) {
		forcereg(p, EAX);
		return M_DONE;
	}
#if defined(KBUG)
	cerror("sxmute: no mana left to change shapes");
#endif
	return M_FAIL;
}


int tdebug = 0;

/* take a pass0 type.. convert to a match type				(ksb)
 */
int
mtype(t)
TWORD t;
{
	if (t == UNDEF) {
		t = INT;
	} else if (ISPTR(t) || ISFTN(t)) {
		return TY_POINT;
	} else if (ISARY(t)) {
		return TY_STRUCT;
	}

	switch (t) {
	case CHAR:
		return TY_CHAR;
	case SHORT:
		return TY_SHORT;
	case STRTY:
	case UNIONTY:
		return TY_STRUCT;
	case INT:
		return TY_INT;
	case UNSIGNED:
		return TY_UNSIGNED;
	case USHORT:
		return TY_USHORT;
	case UCHAR:
		return TY_UCHAR;
	case ULONG:
		return TY_ULONG;
	case LONG:
		return TY_LONG;
	case DOUBLE:
		return TY_DOUBLE;
	case FLOAT:	/* no way */
	default:
		break;
	}
	cerror("what type, huh?");
	return 0;
}

/*
 * type transmute -- change the type of something into a larger		(ksb)
 * type.
 */
MATCH
cvttype(p, cookie)
NODE *p;
int cookie;
{
	register NODE *q, *r;
	auto TWORD t;
	auto MATCH ret;

	t = p->in.type == USHORT || p->in.type == UCHAR ? UNSIGNED : INT;

	if (p->in.op == ICON) {
		p->in.type = t;
		return M_DONE;
	}
	q = talloc();
	r = talloc();
	nxfer(q, p);

	p->in.op = SCONV;
	p->in.su = 1;
	p->in.type = t;
	p->in.right = r;
	p->in.left = q;

	r->in.op = ICON;
	r->in.su = 0;
	r->in.type = t;
	r->tn.rval = 0;
	r->tn.lval = 0;

#if defined(FLEXNAMES)
	p->in.name = "";
	r->in.name = "";
#else
	p->in.name[0] = '\0';
	r->in.name[0] = '\0';
#endif
	ret = order(p, cookie);

	tfree1(r);	/* drive match reclaimed */
	tfree1(q);
	p->in.type = t;
	return ret;
}

/* look for situations where we can turn * into OREG	(better -- ksb)
 */
MATCH
um2oreg(p)
register NODE * p;
{
	register NODE *q;
	register NODE *ql, *qr;
	register int r;
	register char *cp;
	auto CONSZ temp;

	if (p->in.op != UNARY MUL) {
		return M_FAIL;
	}
	q = p->in.left;
	if (q->in.op == REG) {
		temp = q->tn.lval;	/* 0 */
		r = q->tn.rval;
		cp = q->in.name;
	} else if (q->in.op != PLUS && q->in.op != MINUS) {
		return M_FAIL;
	} else {
		ql = q->in.left;
		qr = q->in.right;
#if 0 	/* a bad idea, not enough registers to do this -- ksb */
		if (qr->in.op == ICON && ql->in.op != REQ) {
			(void) order(ql, SH_TAREG|SH_AREG);
		}
#endif
		if (qr->in.op != ICON || ql->in.op != REG) {
			return M_FAIL;
		}
		temp = qr->tn.lval;
		if (q->in.op == MINUS) {
			temp = -temp;
		}
		r = ql->tn.rval;
		temp += ql->tn.lval;
		cp = qr->in.name;
		if (*cp && (q->in.op == MINUS || *ql->in.name)) {
			return M_FAIL;
		}
		if (!*cp)
			cp = ql->in.name;
	}

	if (notoff(p->in.type, r, temp, cp)) {
		return M_FAIL;
	}
	p->in.op = OREG;
	p->tn.rval = r;
	p->tn.lval = temp;
#if defined(FLEXNAMES)
	p->in.name = cp;
#else
	{
		register int i;
		for (i = 0; i < NCHNAM; ++i)
			p->in.name[i] = *cp++;
	}
#endif
	xferown(q, r, p);
	tfree(q);	/*  xferown gave reg to p -- no recl2 call here */
	return M_DONE;
}

/*
 * find an apropos shape to transmute a resource into for it to be the
 * (0 = left, 1= right) hand side of an opcode for p
 */
sapropos(p, cookie, bSide, q)
NODE *p;
int cookie, bSide;
struct optab *q;
{

	register int sleft, sright;
	register int aleft, aright;

	if ((struct optab *)0 == q) {
		return SH_AREG|SH_TAREG|SH_NAME|SH_OAREG;
	}
	aleft = aright = ~0;
	for(sleft = sright = 0; q->op != FREE; ++q ) {
		sleft |= q->lshape;
		sright |= q->rshape;
		aleft &= q->lshape;
		aright &= q->rshape;
	}
	if (0 != (bSide ? aright : aleft))
		return (bSide ? aright : aleft);
	return bSide ? sright : sleft;
}

/*
 * determine *forced* order of evaluation and su numbers for this
 * subtree, call drive_match to match the tree when ever order is
 * not important, and the operator is in table.c.  Call other special
 * purpose routines to handle other special cases.
 */
MATCH
order(p, cook)
NODE *p;
{
	register int cookie;
	register int o;
	auto NODE *p1, *p2;
	auto long m;

	while (FREE != (o = p->in.op)) {
		cookie = cook;
		rcount();
		canon(p);
		rallo(p, p->in.rall);

		m = optype(o);
		p1 = p->in.left;
		if (m == BITYPE)
			p2 = p->in.right;
		else
			p2 = NIL;

		switch (o) {
		case UNARY MUL: 
			/* 
			 * if arguments are passed in register, care must be
			 * taken that reclaim not throw away the register
			 * which now has the result... 
			 */
			if (cook & SH_FOREFF) {
				/* do almost nothing */
				p = p->in.left;
				continue;
			}
			/* fall through */
			if (0) {
		case ASSIGN: 
				lvalue(p->in.left);
			}
		case FORCE: 
		default: 
			/* fall into
			 * look for op in table
			 */
			for (;;) {
				m = drive_match(p, cookie);
				switch (m) {
				case M_DONE:
					goto cleanup;
				case M_FAIL:
					if (!(cookie = nextcook(p, cookie)))
						break;
					continue;
				}
				break;
			}
			break;

		case CBRANCH: 
			m = p2->tn.lval;
			cbranch(p1, (LABEL) -1, m);
			tfree1(p2);
			tfree1(p);	/* cbranch frees and reclaims p1 */
			return M_DONE;

		case UNARY STCALL: 
		case STCALL: 
		case UNARY CALL: 
		case CALL: 
			if (M_DONE == gencall(p, cookie))
				goto cleanup;
			break;

		case STASG: 
		case STARG:
			if (! setstr(p, cookie))
				break;
			/*fallthrough*/
		case NAME: 
		case OREG:
		case REG:
		case ICON:
	cleanup: 
			/* if it is not yet in the right state, put it there */
			if (cook & SH_FOREFF) {
				reclaim(p, RC_NULL, SH_NONE);
				return M_DONE;
			}
			if (0 != (tshape(p)&cook))
				return M_DONE;

			/* we are in bad shape, try one last chance */
			return lastchance(p, cook);

		case INIT: 
			uerror("illegal initialization");
			return M_FAIL;
		case COMOP:
			order(p1, SH_FOREFF);
			tfree(p1);
			order(p2, cook);
			nxfer(p, p2);
			tfree1(p2);
			continue;
		}
		break;
	}
	if (p->in.op == FREE) {
		return M_DONE;
	}
	cerror("no table entry for op %s", opst[p->in.op]);
	return M_FAIL;
}

/* we have failed to match p with cookie; try another
 */
nextcook(p, cookie)
NODE *p;
{
	if (0 == (cookie&SH_TAREG) || 0 == (cookie&SH_TBREG))
		return SH_TAREG|SH_TBREG;
	if (0 == (cookie&SH_ANY_TEMP) && asgop(p->in.op))
		return SH_ORDER;
	return 0;
}

/* forget it!  (here we should try to push a register, go on and
 * pop it before we return) (using any_not_owed())
 */
MATCH
lastchance(p, cook)
NODE *p;
{
	return M_FAIL;
}

/*
 * return the pointer to the left or right side of p, or p itself,
 * depending on the optype of p 
 */
NODE *
getlr(p, c)
NODE *p;
{
	switch( c ) {
	case '1':
		return &resc[0];
	case '2':
		return &resc[1];
	case '3':
		return &resc[2];
	case 'L':
		if (optype(p->in.op) == LTYPE)
			cerror("getlr: left of node with no left side");
		return p->in.left;
	case 'R':
		if (optype(p->in.op) != BITYPE)
			cerror("getlr: right of node with no right side");
		return p->in.right;
	default:
		break;
	}
	cerror("getlr: bad key %c", c);
	/* NOTREACHED */
}

/*
 * look for match in the tables and generate code if found.	(ksb)
 * (much cleaner now!)  (match_op is the real op, if a table
 * entry is overloaded like all the compares are (mapped to EQ))
 *
 * M_DONE  - we did it, p is now an addressing mode of the result
 * M_MLEFT - p->in.left needs to be matched first -- try again after that
 * M_MRIGHT- p->in.right needs to be matched first -- try again after that
 * M_SLEFT- we think that the left shape should be changed
 * M_SRIGHT- we think that the right shape should be changed
 * M_TLEFT - we think the left type needs converting (to right)
 * M_TRIGHT - we think the right type needs converting (to left)
 * M_FAIL  - we failed, no instructions matched at all :-{ quit for us
 */
static MATCH
match(p, cookie, match_op, q)
NODE *p;
int cookie, match_op;
register struct optab *q;
{
	register NODE *pl, *pr;
	auto int sl, sr;
	auto int tl, tr;
	auto int faill = 0, failr = 0;
	auto int m;

	rcount();

	/* if we have left side to match, get all the data on it
	 */
	m = 0;
	if (SH_NONE != q->lshape) {
#if defined(KBUG)
		pl = getlr(p, 'L');
#else
		pl = p->in.left;
#endif
		sl = tshape(pl);
		if (0 == sl) {		/* no recognized shape, bag it	*/
			m = M_MLEFT;
		} else {
			tl = mtype(pl->in.type);
			if  (tl & (TY_CHAR|TY_UCHAR|TY_SHORT|TY_USHORT)) {
				switch (p->in.op) {
				case ASSIGN:
				case SCONV:
					break;
				default:
					return M_TLEFT;
				}
			}
		}
	} else {
		pl = 0;
		sl = 0;
	}

	/* if we have right side to match, get all the data on it too
	 */
	if (SH_NONE != q->rshape) {
#if defined(KBUG)
		pr = getlr(p, 'R');
#else
		pr = p->in.right;
#endif
		sr = tshape(pr);
		if (0 == sr) {		/* no recognized shape, bag it	*/
			m |= M_MRIGHT;
		} else {
			tr = mtype(pr->in.type);
			if  (tr & (TY_CHAR|TY_UCHAR|TY_SHORT|TY_USHORT))
				return M_TRIGHT;
		}
	} else {
		sr = 0;
		pr = 0;
	}

	/*
	 * this allows the caller (drive_match) to select the child that
	 * is best to allocate first according to the su numbers.
	 */
	if (m != 0) {
		return m;
	}

	/* scan the list of instructions we could generate,
	 * reject those we cannot use for one reason or another.
	 */
	for(; q->op != FREE; ++q ) {
		if (q->op != p->in.op) {
			cerror("wrong op in table list!? (can't happen)");
			continue;
		}

		/* not the shape we want to generate
		 */
		if (0 == (q->nshape & cookie)) {
			continue;
		}

		/* see if left child matches */
		if (0 != pl) {
			if (0 == (tl & q->ltype)) {
				continue;
			}
			if (0 == (sl & q->lshape)) {
				++faill;
				continue;
			}
		}

		/* see if right child matches */
		if (0 != pr) {
			if (0 == (tr & q->rtype)) {
				continue;
			}
			if (0 == (sr & q->rshape)) {
				++failr;
				continue;
			}
		}

		/* (last chance) if can't find recources try next
		 */
		if( !allo(p, q->needs)) {
			continue;
		}

		if ((sl & SH_TBREG) || (sr & SH_TBREG)) {
			(void)restflt((0 != (sl & SH_TBREG)) + (0 != (sr & SH_TBREG)));
		}


		/* generate code marco expanded and stuff
		 */
		expand(p, cookie, q->cstring, match_op);

		/* and reclain resources -- change p into new node
		 */
		reclaim(p, q->rewrite, cookie);

		return M_DONE;
	}
	/* :-( we failed to find a match we could generate
	 */
#if defined(KBUG)
	if (NULL != kdebug) {
		fprintf(kdebug, "match: failed (left: p%08x, s%04x(%d) t%04x; right p%08x, s%04x(%d) t%04x)\n", pl, sl, faill, tl, pr, sr, failr, tr);
	}
#endif
	return faill > failr ? M_SLEFT : failr == 0 ? M_FAIL : M_SRIGHT;
}

/*
 * do a ?: expression in Pascal, Jeeze I didn't know we could get here...
 * get 2 lables for false case (flab) and exit (elab)
 */
void
DoQuest(p, cookie)
NODE *p;
int cookie;
{
	auto LABEL flab, elab;
	auto int frnum, badrnum;
	register NODE *pr;

	flab = getlab();
	elab = getlab();
	cbranch(p->in.left, -1, flab);		/* free'd		*/
	pr = p->in.right;

	/* true case */
	order(pr->in.left, SH_ANY_REG);
	if (0 == (tshape(pr->in.left) & SH_ANY_REG)) {
		if (M_FAIL == sxmute(pr->in.left, SH_ANY_AREG))
panic:
			cerror("quest colon panic");
	}
	frnum = pr->in.left->tn.rval;
	recl2(pr->in.left);
	cbgen(0, elab, 0);

	/* false case */
	deflab(flab);
	order(pr->in.right, SH_ANY_REG);
	if (0 == (tshape(pr->in.right) & SH_ANY_REG)) {
		if (M_FAIL == sxmute(pr->in.right, SH_ANY_AREG))
			goto panic;
	}
	badrnum = pr->in.right->tn.rval;
	if (isbreg(frnum)) {
		if (!isbreg(badrnum)) {
			/* one side real, one side not? */
			goto panic;
		}
		/* done -- top of stack */
	} else if (frnum != badrnum) {
		/* here we must change register to the one the then case had */
		if (0 != amregs[frnum].ibusy) {
			/* drive_match allocated 2 regs? then reg busy too! */
			goto panic;
		}
		rmove(frnum, badrnum, TY_INT);
		amregs[frnum].pNOown = amregs[badrnum].pNOown;
		amregs[frnum].ibusy = amregs[badrnum].ibusy;
		amregs[badrnum].pNOown =  (NODE *)0;
		amregs[badrnum].ibusy = 0;
		pr->in.right->tn.rval = frnum;
	}
	ncopy(p, pr->in.right);
	xferown(pr->in.right, frnum, p);

	/* exit code */
	deflab(elab);
	tfree1(pr->in.right);
	tfree1(pr);
}

/*
 * call match post order to travers this tree				(ksb)
 */
MATCH
drive_match(p, cookie)
register NODE *p;
{
	register int match_op;
	register struct optab *q;
	register int sl;

	switch (match_op = p->in.op) {
	case STCALL:
	case CALL:
		(void)gencall(p, cookie);	/* turn it into UNARY CALL */
		return order(p, cookie);
	case SCONV:
		do_sconv(p, cookie);
		return order(p, cookie);
	case UNARY MUL:
		if (M_DONE == um2oreg(p)) {
			/* warm fuzzy feeling (saved lots of code)!
			 * but we still need to satisfy the cookie...
			 */
			break;
		}
		(void)order(p->in.left, SH_ORDER);
		sl = tshape(p->in.left);
		if (0 != (sl & (SH_TAREG|SH_ANY_CON))) {
			allo(p, NEED0);
			expand(p, cookie, "%LO", UNARY MUL);
			reclaim(p, RC_LEFT, cookie);
		} else if (DOUBLE == p->in.type) {
			switch (p->in.left->in.op) {
			case OREG:
			case NAME:
				allo(p, NEED1(N_AREG));
				expand(p, cookie, "\tmovl\t%LA,%1A\n", UNARY MUL);
				reclaim(p, RC_KEEP1, cookie);
				p->in.op = OREG;
				break;
			default:
				cerror("oops");
			}
		} else if (0 != (sl & SH_ANY_CON)) {
			allo(p, NEED1(N_AREG));
			expand(p, cookie, "\t%P\t%LA,%1A\n", UNARY MUL);
			reclaim(p, RC_KEEP1, cookie);
		} else {
			allo(p, NEED1(N_AREG));
			expand(p, cookie, "\tmovl\t%LA,%1A\n\t%1O\n", UNARY MUL);
			reclaim(p, RC_KEEP1, cookie);
		}
		break;

	case LE:	/* map floating point ops to unsigned		*/
	case GE:
	case GT:
	case LT:
		if (DOUBLE == p->in.left->tn.type) {
			match_op += ULE - LE;
		}
	case UGE:	/* map all relational ops to ==			*/
	case ULE:	/* let expand fix them using match_op		*/
	case UGT:
	case ULT:
	case EQ:
	case NE:
		p->in.op = EQ;
		/* fall through */
	case UNARY CALL:
	default:
		q = table[p->in.op];
		if (0 == q)
			cerror("no translation for opcode %d (yet?)", p->in.op);

		for (;;) { switch (match(p, cookie, match_op, q)) {
		case M_DONE:
			break;
		case M_MRIGHT|M_MLEFT:
			if (p->in.left->in.su > p->in.right->in.su) {
				p->in.rall |= RA_LFIRST;
		case M_MLEFT:
				switch (order(p->in.left, SH_TAREG|SH_TBREG|SH_NAME|SH_ANY_CON|SH_OAREG)) {
				case M_DONE:
					continue;
				default:
					return M_FAIL;
				}
			}
			p->in.rall |= RA_RFIRST;
		case M_MRIGHT:
			switch (order(p->in.right, SH_TAREG|SH_TBREG|SH_NAME|SH_ANY_CON|SH_OAREG)) {
			case M_DONE:
				continue;
			default:
				return M_FAIL;
			}
		case M_SLEFT:
			if (M_DONE == sxmute(p->in.left, sapropos(p, cookie, 0, q)))
				continue;
			return M_FAIL;
		case M_SRIGHT:
			if (M_DONE == sxmute(p->in.right, sapropos(p, cookie, 1, q)))
				continue;
			return M_FAIL;
		case M_TLEFT:
			if (M_DONE == cvttype(p->in.left, SH_TAREG|SH_TBREG))
				continue;
			return M_FAIL;
		case M_TRIGHT:
			if (M_DONE == cvttype(p->in.right, SH_TAREG|SH_TBREG))
				continue;
			return M_FAIL;
		case M_FAIL:
			return M_FAIL;
		} break; }
		break;
	case QUEST:
		DoQuest(p, cookie);
		break;
	case REG:
	case NAME:
	case OREG:
	case ICON:
		break;
	}

	if (0 != (cookie & SH_FOREFF) || 0 != (cookie & tshape(p)))
		return M_DONE;

	/* last chance, can we transmute it for the caller?
	 */
	return sxmute(p, cookie);
}

/* print out the constant to and with to mask out the legal bits for this type
 */
static void
camask(t)
TWORD t;
{
	switch (t) {
	case DOUBLE:
		break;
	case LONG:
	case ULONG:
	case INT:
	case UNSIGNED:
		break;
	case SHORT:
	case USHORT:
		printf("&0xffff");
		break;
	case CHAR:
	case UCHAR:
		printf("&0xff");
		break;
	default:
		if ( !ISPTR(t) )
	case FLOAT:
			cerror("camask -- bad type");
		break;
	}
}

/* print out the machine opcode postfix for the type */
static void
prtype(n)
NODE *n;
{
	switch (n->in.type) {
	case DOUBLE:
		printf("f");
		break;
	case LONG:
	case ULONG:
	case INT:
	case UNSIGNED:
		printf("l");
		break;
	case SHORT:
	case USHORT:
		printf("w");
		break;
	case CHAR:
	case UCHAR:
		printf("b");
		break;
	default:
		if ( !ISPTR( n->in.type ) )
	case FLOAT:
			cerror("prtype -- bad type");
		printf("l");
		break;
	}
}

/*
 * generate code by interpreting string in the table entry
 * we move zzzcode into this routine for speed (ksb)
 */
expand(k, cookie, cp, match_op)
NODE *k;
register char *cp;
int cookie, match_op;
{
	extern int gc_numbytes;
	register char ch;
	register NODE *tp;		/* make AL into %LA		*/
#if defined(DEBUG)
	extern int xdebug;
	auto CONSZ val;
#else
	register CONSZ val;
#endif

#if 0
	rtyflg = 0;
#endif

	while ('\000' != (ch = *cp++)) {
		/* this is the usual case... zoom */
		if ('%' != ch) {
			PUTCHAR(ch);
			continue;
		}

		tp = k;
		for (;;) { switch(*cp++) {
		default:
			cerror("unknown expand/zzz code `%c'!", cp[-1]);
			break;
		case '%':		/* simple, need a percent	*/
			PUTCHAR('%');
			break;
		case '1':		/* fall down #1			*/
			tp = &resc[0];
			continue;
		case '2':		/* fall down #2			*/
			tp = &resc[1];
			continue;
		case '3':		/* fall down #3			*/
			tp = &resc[2];
			continue;
		case 'L':		/* fall down left		*/
			if (optype(tp->in.op) == LTYPE)
				cerror("no left side in expand");
			tp = tp->in.left;
			continue;
		case 'R':		/* fall down right		*/
			if (optype(tp->in.op) != BITYPE)
				cerror("no right side in expand");
			tp = tp->in.right;
			continue;
		case '&':		/* constant to and with in fold	*/
			camask(tp->in.type);
			break;
		case 'A':		/* addressing mode		*/
			adrput(tp);
			break;
		case 'C':		/* for constant value only	*/
			conput(tp);
			break;
		/*zzz A*/
		case 'D':		/* data movment			*/
			cerror("old style data move call setstr");
		case 'E':		/* address + fold, or eat to \n	*/
			/* we will keep resc[0], a ICON lable... */
			break;
		case 'F':		/* this line deleted if SH_FOREFF*/
			if (cookie & SH_FOREFF) {
gobble:
				while (*cp++ != '\n')
					/* skip */;
			}
			break;
		/*zzz G (used to do only right side) */
		case 'G':		/* address of ???		*/
			if (((tp->in.name[0] == '_') && 0 == (tshape(tp)&SH_ANY_CON)) || 
			     (tp->in.name[0] == '\0' && ((tp->tn.lval > 16777215) ||
			     (tp->tn.lval < 0)))) {
				printf ("movl\t");
			} else {
				printf("addr\t");
				if (tp->in.name[0] == '\0')
					printf("@");
			}
			break;
		case 'H':		/* field shift			*/
			printf("%d", fldshf);
			break;
		case 'I':		/* an instruction		*/
			cerror("insput");
			break;
		case 'J':		/* ~K				*/
			switch (match_op) {
			case GT:
				printf("l");
				break;
			case GE:
				printf("le");
				break;
			case LT:
				printf("g");
				break;
			case LE:
				printf("ge");
				break;
			case UGT:
				printf("b");
				break;
			case UGE:
				printf("be");
				break;
			case ULT:
				printf("a");
				break;
			case ULE:
				printf("ae");
				break;
			case EQ:
				printf("e");
				break;
			case NE:
				printf("ne");
				break;
			default:
				cerror("illegal match op");
				break;
			}
			break;
		case 'K': 		/* logical ops			*/
			switch (match_op) {
			case GT:
				printf("g");
				break;
			case GE:
				printf("ge");
				break;
			case LT:
				printf("l");
				break;
			case LE:
				printf("le");
				break;
			case UGT:
				printf("a");
				break;
			case UGE:
				printf("ae");
				break;
			case ULT:
				printf("b");
				break;
			case ULE:
				printf("be");
				break;
			case EQ:
				printf("e");
				break;
			case NE:
				printf("ne");
				break;
			default:
				cerror("illegal match op");
				break;
			}
			break;
		case 'M':		/* field mask			*/
		case 'N':		/* complement of field mask	*/
			val = 1;
			val <<= fldsz;
			--val;
			val <<= fldshf;
			adrcon(ch == 'M' ? val : ~val);
			break;
		case 'O':		/* cvt REG to 0(OREG)		*/
#if defined(KBUG)
			if ((tp->tn.op != REG && tp->tn.op != ICON) && 0 != kdebug)
				fprintf(kdebug, "not a reg node!\n");
#endif
			tp->tn.type = k->tn.type;
			tp->tn.op = tp->tn.op == REG ? OREG : NAME;
			/* tp->tn.lval = 0; */
			break;
		case 'P':		/* mov any			*/
			{
				static char *apcCvt[]= {
					0, 0, "sb", "sw",
					"", "", 0, 0,
					0, 0, "", "",
					"zb", "zw", "", ""
				};
				register char *pc;
				if (ISPTR(tp->in.type))
					pc = apcCvt[INT];
				else
					pc = apcCvt[tp->in.type & BASETYPE];
				if ((char *)0 == pc)
					cerror("no extend for that!");
				printf("mov%sl", pc);
			}
			break;
		case 'Q':		/* num words pushed on stack	*/
			printf("\tsubl\t$%d,%%esp\n", gc_numbytes);
			break;
		case 'S':		/* field size			*/
			printf("%d", fldsz);
			break;
		case 'T':		/* output special label field	*/
			printf("%d", tp->bn.label);
			break;
		case 'U':		/* this node is a .set constant	*/
			printf("%d", tp->tn.lval);
			break;
		case 'V':		/* INCR and DECR, SH_FOREFF	*/
			if (tp->in.right->tn.lval < 8 || (tp->in.right->tn.lval == 8 && tp->in.op == DECR)) {
				printf("\taddq");
				prtype(tp->in.left);
				printf("\t%d,", (tp->in.op == INCR ? tp->in.right->tn.lval : -tp->in.right->tn.lval));
				adrput(tp->in.left);
			} else {
				printf("\t%s", (tp->in.op == INCR ? "add" : "sub") );
				prtype(tp->in.left);
				printf("\t");
				adrput(tp->in.right);
				printf(",");
				adrput(tp->in.left);
			}
			break;
		case 'W':		/* call to prtype		*/
			prtype(tp);
			break;
		case 'B':		/* swap tos and nos sometimes	*/
			if (0 != (tp->in.rall & RA_LFIRST)) {
				puts("\tfxch");
			}
			break;
		case 'X':		/* 'r' if reversed fsub or fdiv	*/
			if (0 != (tp->in.rall & RA_LFIRST)) {
				putchar('r');
#if defined(KBUG)
			} else if (0 == (tp->in.rall & RA_RFIRST)) {
				cerror("%%X macro inaprop.");
#endif
			} else {
				/* do nothing */;
			}
			break;
		case 'Y':		/* register type		*/
			cerror("don't use %Y");
			break;
		case 'Z':		/* complement mask for bic	*/
			printf("%ld", ~tp->tn.lval);
			break;
		} break; }
	}
}

#if defined(MULTILEVEL)

union mltemplate {
	struct ml_head {
		int tag;    		    /* identifies class of tree */
		int subtag; 		    /* subclass of tree */
		union mltemplate *nexthead; /* linked by mlinit() */
	} mlhead;
	struct ml_node {
		int op; 	/* either an operator or op description */
		int nshape;     /* shape of node */
		/*
		 * both op and nshape must match the node.
		 * where the work is to be done entirely by
		 * op, nshape can be SANY, visa versa, op can
		 * be OPANY.
		 */
		int ntype; 	/* type descriptor from mfile2 */
	} mlnode;
};

#define MLSZ 30

extern union mltemplate mltree[];
int mlstack[MLSZ];
int *mlsp; 	/* pointing into mlstack */
NODE *ststack[MLSZ];
NODE **stp; 	/* pointing into ststack */

mlinit()
{
	union mltemplate **lastlink;
	register union mltemplate *n;
	register mlop;

	lastlink = &(mltree[0].nexthead);
	n = &mltree[0];
	for( ; (n++)->mlhead.tag != 0;
		*lastlink = ++n, lastlink = &(n->mlhead.nexthead) ) {
	 	/*
	 	 * Wander through a tree with a stack, finding
	 	 * its structure so the next header can be located.
	 	 */
		mlsp = mlstack;

		for( ;; ++n ) {
			if( (mlop = n->mlnode.op) < OPSIMP ) {
				switch( optype(mlop) ) {
				default:
					cerror("(1)unknown opcode: %o",mlop);
				case BITYPE:
					goto binary;
				case UTYPE:
					break;
				case LTYPE:
					goto leaf;
				}
			} else {
				if( mamask[mlop-OPSIMP] &
					(SIMPFLG|COMMFLG|MULFLG|DIVFLG|LOGFLG|FLOFLG|SHFFLG) ) {
			binary:
					*mlsp++ = BITYPE;
				} else if( ! (mamask[mlop-OPSIMP] & UTYPE) ) { /* includes OPANY */
			leaf:
					if( mlsp == mlstack )
						goto tree_end;
					if ( *--mlsp != BITYPE )
						cerror("(1)bad multi-level tree descriptor around mltree[%d]",
						n-mltree);
				}
			}
		}
	tree_end: /* n points to final leaf */
		;
	}
#if !defined(BUG3)
	if( vdebug > 3 ){
		printf("mltree={\n");
		for( n= &(mltree[0]); n->mlhead.tag != 0; ++n)
			printf("%o: %d, %d, %o,\n",n,
				n->mlhead.tag,n->mlhead.subtag,n->mlhead.nexthead);
		printf("\t}\n");
	}
#endif
}

/*
 * Does subtree match a multi-level tree with tag "target"?
 * Return zero on failure, non-zero subtag on success
 * (or MDONE if there is a zero subtag field).
 */
mlmatch( subtree, target, subtarget )
NODE *subtree;
int target, subtarget;
{
	auto union mltemplate *head; 	/* current template header */
	register union mltemplate *n;   /* node being matched */
	auto NODE *st; 			/* subtree being matched */
	register int mlop;

	for( head = &(mltree[0]); head->mlhead.tag != 0; head=head->mlhead.nexthead) {
#if !defined(BUG3)
		if( vdebug > 1 ) printf("mlmatch head(%o) tag(%d)\n", head->mlhead.tag);
#endif
		if( head->mlhead.tag != target ) continue;
		if( subtarget && head->mlhead.subtag != subtarget) continue;
#if !defined(BUG3)
		if( vdebug ) printf("mlmatch for %d\n",target);
#endif

		/* potential for match */

		n = head + 1;
		st = subtree;
		stp = ststack;
		mlsp = mlstack;
		/*
		 * Compare n->op, ->nshape, ->ntype to
		 * the subtree node st
		 */
		for( ;; ++n ) { /* for each node in multi-level template */
			/* opmatch */
			if( n->op < OPSIMP ) {
				if( st->op != n->op )break;
			} else {
				register opmtemp;

				opmtemp=mamask[n->op-OPSIMP]
				if (optemp&SPFLG) {
					if (st->op!=NAME && st->op!=ICON && st->op!=OREG && ! shltype(st->op,st))
						break;
				} else if ((dope[st->op]&(opmtemp|ASGFLG))!=opmtemp)
					break;
			}

			/* check shape and type */

			if( 0 == (tshape(st) & n->mlnode.nshape) ) break;
#if 0
			broken due to change in mtype (AKA t-type)
			if( 0 !=  (mtype( st->type) & n->mlnode.ntype ) ) break;
#endif

			/* that node matched, let's try another */
			/* must advance both st and n and halt at right time */

			if( (mlop = n->mlnode.op) < OPSIMP ) {
				switch( optype(mlop) ) {
				default:
					cerror("(2)unknown opcode: %o",mlop);
				case BITYPE:
					goto binary;
				case UTYPE:
					st = st->left;
					break;
				case LTYPE:
					goto leaf;
				}
			} else {
				if( mamask[mlop - OPSIMP] &
					(SIMPFLG|COMMFLG|MULFLG|DIVFLG|LOGFLG|FLOFLG|SHFFLG) ) {
				binary:
					*mlsp++ = BITYPE;
					*stp++ = st;
					st = st->left;
				} else if( ! (mamask[mlop-OPSIMP] & UTYPE) ) { /* includes OPANY */

				leaf:
					if( mlsp == mlstack )
						goto matched;
					else if ( *--mlsp != BITYPE )
						cerror("(2)bad multi-level tree descriptor around mltree[%d]",
						n-mltree);
					st = (*--stp)->right;
				} else /* UNARY */ st = st->left;
			}
			continue;

		matched:
			/* complete multi-level match successful */
#if !defined(BUG3)
			if( vdebug ) printf("mlmatch() success\n");
#endif
			if( head->mlhead.subtag == 0 )
				return( MDONE );
#if !defined(BUG3)
			if( vdebug ) printf("\treturns %d\n",
				head->mlhead.subtag );
#endif
			return( head->mlhead.subtag );
		}
	}
	return( 0 );
}
#endif
