#if !defined(lint)
static char rcsid[] = "$Id: local2.c,v 2.9 88/10/02 21:17:49 ksb Exp $";
#endif lint

/*
 *	Berkeley Pascal Compiler	(local2.c)
 *
 *	Spring of 1986
 *
 *	James S. Schoner		Kevin S. Braunsdorf
 *
 *	Purdue University Computing Center
 *	West Lafayette, Indiana  47907
 */

#include "pass2.h"
#include "ctype.h"

int ftlab1, ftlab2;

#define BITMASK(n) ((1L<<n)-1)

where(c)
{
	fprintf( stderr, "%s, line %d: ", filename, lineno );
	if (c != 'w') {
		printf("\n	.ABORT\n");
		fflush(stdout);
	}
}

	/* identify line l and file fn */
lineid( l, fn )
char *fn;
{
	printf( "#	line %d, file %s\n", l, fn );
}

extern int fsparea;		/* float spill area (allo.c)		*/
/*
 * at the end of block level 2 we output the stack adjust (jss & ksb)
 */
eobl2()
{
	auto OFFSZ spoff;	/* offset from stack pointer */

	spoff = maxoff;
	if (spoff >= AUTOINIT)
		spoff -= AUTOINIT;
	spoff /= SZCHAR;
	SETOFF(spoff, 4);
#if defined(FLEXNAMES)
	printf( "\t.set\tLF%d,%ld\n", ftnno, spoff+fsparea);
#else
	printf( "\t.set\t.F%d,%ld\n", ftnno, spoff+fsparea);
#endif
	maxargs = -1;
}

struct hoptab {
	int opmask;
	char * opstring;
};

struct hoptab ioptab[] = {
	ASG PLUS, "add",
	ASG MINUS, "sub",
	ASG MUL, "mul",
	ASG DIV, "div",
	ASG MOD, "rem",
	ASG OR, "or",
	ASG ER,	"xor",
	ASG AND, "bic",
	PLUS,	"add",
	MINUS,	"sub",
	MUL,	"mul",
	DIV,	"div",
	MOD,	"rem",
	OR,	"or",
	ER,	"xor",
	AND,	"bic",
	-1, ""
};

/* output the appropriate string from the above table
 */
hopcode( f, o )
{
	register struct hoptab *q;

	if ((f == 'l' || f == 'w' || f == 'b') && (o == DIV || o == ASG DIV )) {
		printf ("quo%c", f);
	} else {
		for( q = ioptab;  q->opmask>=0; ++q ) {
			if( q->opmask == o ) {
				printf( "%s%c", q->opstring, f );
				return;
			}
		}
		cerror( "no hoptab for %s", opst[o] );
	}
}

tlen(p)
NODE *p;
{
	switch(p->in.type) {
	case CHAR:
	case UCHAR:
		return(1);
	case SHORT:
	case USHORT:
		return(2);
	case DOUBLE:
		return(8);
	default:
		return(4);
	}
}

mixtypes(p, q)
NODE *p, *q;
{
	register TWORD tp, tq;

	tp = p->in.type;
	tq = q->in.type;
	return (tp==DOUBLE) != (tq==DOUBLE);
}

/*
 * collapsible(dest, src) -- if a conversion with a register destination
 *	can be accomplished in one instruction, return the type of src
 *	that will do the job correctly; otherwise return 0.  Note that
 *	a register must always end up having type TY_INT or TY_UNSIGNED.
 */
int
collapsible(dest, src)
NODE *dest, *src;
{
	int st = src->in.type;
	int dt = dest->in.type;
	int newt = 0;

	/*
	 * Are there side effects of evaluating src?
	 * If the derived type will not be the same size as src,
	 * we have to use two steps.
	 */
	if (tlen(src) > tlen(dest)) {
		if (0 != (tshape(src) & SH_TAREG))
			return (0);
		if (src->in.op == OREG && R2TEST(src->tn.rval))
			return (0);
	}

	/*
	 * Can we get an object of dest's type by punning src?
	 * Praises be to great Cthulhu for little-endian machines...
	 */
	if (st == TY_CHAR && dt == TY_USHORT)
		/*
		 * Special case -- we must sign-extend to 16 bits.
		 */
		return (0);

	if (tlen(src) < tlen(dest))
		newt = st;
	else
		newt = dt;
	return (newt);
}

rmove( rt, rs, t )
TWORD t;
{
	printf( "\tmov%c\t%s,%s\n", !Oflag ? (t==TY_DOUBLE ? 'Z' : 'l') : (t==TY_DOUBLE ? 'Z' : 'l'), amregs[rs].acname, amregs[rt].acname );
}

struct respref respref[] = {
	SH_TAREG|SH_TBREG,
	SH_TAREG|SH_TBREG,
	SH_AREG|SH_BREG,
	SH_AREG|SH_BREG|SH_OAREG|SH_TAREG|SH_NAME|SH_ANY_CON,
	SH_ANY_TEMP,
	SH_ANY_TEMP,
	SH_NONE,
	SH_NONE,
	SH_ANY_TEMP,
	SH_TAREG|SH_AREG|SH_TBREG|SH_BREG|SH_OAREG,
	0,
	0 
};

/* set up temporary registers */
setregs()
{
	/* tbl- 3 free scalar and 3 free flpt regs on Balance */
	fregs = 3;
}

/*
 * return the register the function return value is in
 */
callreg(p)
NODE *p;
{
	if ((p->in.type == DOUBLE)) { 
		return FP2;
	}
	return EAX;
}

/*ARGSUSED*/ /*ZZZ*/
shltype(o, p)
register NODE *p;
{
	switch (o) {
	case REG:
	case NAME:
	case ICON:
	case OREG:
		return 1;
	case UNARY MUL:
		break;
		/* return shumul(p->in.left); */
	default:
		break;
	}
	return 0;
}

flshape( p )
register NODE *p;
{
	return( p->in.op == REG || p->in.op == NAME || p->in.op == ICON ||
		(p->in.op == OREG && (!R2TEST(p->tn.rval) || tlen(p) == 1)) );
}

#if 0
shtemp( p )
register NODE *p;
{
	if( p->in.op == STARG )
		p = p->in.left;
	return( p->in.op==NAME || p->in.op ==ICON || p->in.op == OREG || (p->in.op==UNARY MUL && shumul(p->in.left)) );
}

shumul( p )
register NODE *p;
{
	extern int xdebug;

	if (xdebug) {
		printf("\nshumul:op=%d,lop=%d,rop=%d", p->in.op, p->in.left->in.op, p->in.right->in.op);
		printf(" prname=%s,plty=%d, prlval=%D\n", p->in.right->in.name, p->in.left->in.type, p->in.right->tn.lval);
	}
#if 0
	o = p->in.op;
	if ((o == NAME && (p->in.name[0] != '_') && (p->tn.lval == 0)) ||
	    (o == OREG && !R2TEST(p->tn.rval) && (p->tn.rval > LASTRV)) ||
	     o == ICON)
		return( STARNM );
#endif

	return( 0 );
}
#endif

adrcon( val )
CONSZ val;
{
	printf( CONFMT, val );
}

conput(p)
register NODE *p;
{
	switch( p->in.op ) {
	case ICON:
		acon( p );
		return;
	case REG:
		printf( "%s", amregs[p->tn.rval].acname );
		return;
	default:
		cerror( "conput: illegal usage" );
	}
}

/* output an address, with offsets, from p */
adrput(p)
register NODE *p;
{
	register int r;

	if( p->in.op == FLD ) {
		p = p->in.left;
	}
	switch( p->in.op ) {
	case NAME:
		acon( p );
		return;
	case ICON:
		/* addressable value of the constant */
		acon( p );
		return;
	case REG:
		printf("%s", amregs[p->tn.rval].acname );
		return;
	case OREG:
		r = p->tn.rval;
		if (R2TEST(r)) {	/* double indexing */
			register int flags;

			flags = R2UPK3(r);
			if(flags & 1)
				printf("0(");
			if( p->tn.lval != 0 || p->in.name[0] != '\0' ) {
				acon(p);
				if (R2UPK1(r) != 100)
					printf("(%s)", amregs[R2UPK1(r)].acname);
			} else if( R2UPK1(r) != 100) {
				printf( "0(%s)", amregs[R2UPK1(r)].acname );
			}
			if (flags & 1)
				printf(")");
			printf("[%s:%c]", amregs[R2UPK2(r)].acname, "bwlq"[flags>>1]);
			return;
		}
		if (r == EBP) {  /* in the argument region */
			if( p->tn.lval == 0 || p->in.name[0] != '\0' )
				werror( "bad arg temp" );
			printf(CONFMT, p->tn.lval);
			printf("(%%ebp)");
			return;
		}
		if( p->tn.lval != 0 || p->in.name[0] != '\0') {
			acon( p );
			if (p->in.name[0] == '\0')
				printf( "(%s)", amregs[p->tn.rval].acname );
			else
				printf( "[%s:b]", amregs[p->tn.rval].acname );
		} else {
			printf("0(%s)", amregs[p->tn.rval].acname);
		}
		return;
	case UNARY MUL:
		/* STARNM found */
#if 0
		if (0 != (tshape(p)&STARNM)) {
			if (p->in.left->in.op == ICON) {
				adrput(p->in.left);
			} else {
				printf("0(");
				adrput(p->in.left);
				printf(")");
			}
		} else {	/* SH_TARREG - really auto inc or dec */
			cerror("auto inc or dec not supported in Pascal");
		}
#endif
		/*return;*/
	default:
		cerror("illegal address");
		return;
	}
}

/* print out a constant */
acon( p )
register NODE *p;
{

	if (ICON == p->in.op) {
		if (p->tn.rval == SETCON) {
			printf("LC%d", p->tn.lval);
		} else if (p->in.name[0] == '_' || p->in.name[0] == 'L') {
#if !defined(FLEXNAMES)
			printf("%.8s+", p->in.name);
#else
			printf("%s+", p->in.name);
#endif
			printf(CONFMT, p->tn.lval);
		} else {
			printf(CONFMT, p->tn.lval);
		}
	} else if (p->tn.lval == 0) {
#if !defined(FLEXNAMES)
		printf("%.8s", p->in.name);
#else
		printf("%s", p->in.name);
#endif
	} else if (p->in.name[0] == '_' || p->in.name[0] == 'L') {
#if !defined(FLEXNAMES)
		printf("%.8s+", p->in.name);
#else
		printf("%s+", p->in.name);
#endif
		printf(CONFMT, p->tn.lval);
	} else {
#if !defined(FLEXNAMES)
		printf("%d", p->tn.lval);
#else
		printf("%d", p->tn.lval);
#endif
	}
}

int gc_numbytes;

/* generate the call given by p
 */
MATCH
gencall(p, cookie)
register NODE *p;
{
	register NODE *p1, *r;
	register int temp, temp1;
	register int m;
	auto int spcookie, fcdata;
	auto int busy, shape;

	/* this code clears eax for us if it is busy. */
	if (p->in.type == DOUBLE) {
		r = 0;
		shape = SH_TBREG;
	} else {
		if (ISBUSY(EAX)) {
			printf("\tpushl\t%s\n", amregs[EAX].acname);
			r = amregs[EAX].pNOown;
			busy = amregs[EAX].ibusy;
			rfree(EAX, INT);
		} else {
			r = (NODE *)0;
		}
		shape = SH_TAREG;
	}
	/* drop float needs some auto space in our frame */
	(void) dropflt();
	spcookie = spill(EBX, ECX, EDX, shape == SH_TAREG ? -1 : EAX, -1);

	temp = 0 != p->in.right ? argsize(p->in.right) : 0;

	/* set aside room for structure return */
	if( p->in.op == STCALL || p->in.op == UNARY STCALL ) {
		if (p->stn.stsize > temp)
			temp1 = p->stn.stsize;
		else
			temp1 = temp;
	}
	if( temp > maxargs )
		maxargs = temp;
	SETOFF(temp1, 4);

	/* make temp node, put offset in (?), and generate args
	 */
	if (0 != (p1 = p->in.right)) {
		while(p1->in.op == CM) {
			if (M_DONE != genarg(p1->in.right))
				return M_FAIL;
			recl2(p1->in.right);
			tfree(p1->in.right);
			p1 = tfree1(p1);
		}
		if (M_DONE != genarg(p1))
			return M_FAIL;
		recl2(p1);
		tfree(p1);
		p->in.right = 0;
	}

	p1 = p->in.left;
	if( p1->in.op != ICON && p1->in.op != REG && p1->in.op != NAME ) {
		if (p1->in.op != OREG || R2TEST(p1->tn.rval)) {
			/* spill at least one A reg here?? */
			m = drive_match(p1, SH_TAREG|SH_AREG|SH_NAME);
			if (m != M_DONE)	/* fail for function name */
				return M_FAIL;
			/* put it back here */
		}
	}
	/* set up gc_numbytes so reference to %Q works */
	gc_numbytes = temp;

	p->in.op = UNARY CALL;
	m = drive_match(p, shape);	/* allocate EXA|FP0 for us */

	if (m != M_DONE) {
		cerror("cannot call?");
	}

	if (0 != temp) {
		printf("\taddl\t$%d,%s\n", temp, amregs[ESP].acname);
	}
	if (-1 != spcookie)
		unspill(spcookie);
	/* match will restore float for us */
	if ((NODE *)0 != r) {
		register int reg;

		reg = freereg(r, N__A);
		r->tn.rval = reg;
		amregs[reg].pNOown = r;
		amregs[reg].ibusy = busy;
		printf("\tpopl\t%s\n", amregs[reg].acname);
	}

	return M_DONE;
}

char *
ccbranches[] = {
	"\tje\tL%d\n",
	"\tjne\tL%d\n",
	"\tjle\tL%d\n",
	"\tjl\tL%d\n",
	"\tjge\tL%d\n",
	"\tjg\tL%d\n",
	"\tjbe\tL%d\n",
	"\tjb\tL%d\n",
	"\tjae\tL%d\n",
	"\tja\tL%d\n",
};

/*   printf conditional and unconditional branches */
cbgen(o, lab, mode)
LABEL lab;
{
	if (o == 0) {
		printf("\tjmp\tL%d\n", lab);
#if defined(DEBUG)
	} else if (o > UGT) {
		cerror( "bad conditional branch: %s", opst[o] );
#endif
	} else {
		printf(ccbranches[o-EQ], lab);
	}
}

int     negrel[] = {
	NE, EQ, GT, GE, LT, LE, UGT, UGE, ULT, ULE
};				       /* negatives of relationals */

/* 
 * evaluate p for truth value, and branch to true or false
 * accordingly: label <0 means fall through 
 */
cbranch(p, true, false)
NODE * p;
LABEL true, false;
{
	register int o;
	auto LABEL lab, flab, tlab;

	lab = -1;

	switch (o = p->in.op) {
	case ANDAND: 
		lab = false < 0 ? getlab() : false;
		cbranch(p->in.left, (LABEL)-1, lab);
		cbranch(p->in.right, true, false);
		if (false < 0)
			deflab(lab);
		break;
	case OROR: 
		lab = true < 0 ? getlab() : true;
		cbranch(p->in.left, lab, (LABEL)-1);
		cbranch(p->in.right, true, false);
		if (true < 0)
			deflab(lab);
		break;
	case NOT: 
		cbranch(p->in.left, false, true);
		break;
	case COMOP: 
		(void) order(p->in.left, SH_FOREFF);
		tfree(p->in.left);
		cbranch(p->in.right, true, false);
		break;
	case QUEST: 
		flab = false < 0 ? getlab () : false;
		tlab = true < 0 ? getlab () : true;
		lab = getlab();
		cbranch(p->in.left, (LABEL)-1, lab);
		cbranch(p->in.right->in.left, tlab, flab);
		deflab(lab);
		cbranch(p->in.right->in.right, true, false);
		if (true < 0)
			deflab(tlab);
		if (false < 0)
			deflab(flab);
		break;
	case ICON: 
		if (p->in.type != FLOAT && p->in.type != DOUBLE && p->tn.rval != SETCON) {
			if (p->tn.lval || p->in.name[0]) {

				/* addresses of C objects are never 0 */
				if (true >= 0)
					cbgen(0, true, 'I');
			} else if (false >= 0) {
				cbgen(0, false, 'I');
			}
			break;
		}
	default: 
		/* get condition codes */
		(void) order(p, SH_ORDER);
		/* fall though */
		if (0) {
	case ULE: 
	case ULT: 
	case UGE: 
	case UGT: 
	case EQ: 
	case NE: 
	case LE: 
	case LT: 
	case GE: 
	case GT: 
			if (true < 0) {
				o = p->in.op = negrel[o - EQ];
				true = false;
				false = -1;
			}
			p->bn.label = true;
			(void) order(p, SH_ANY_AREG);
		}
		expand(p, SH_ANY_AREG, "\tcmpb\t$0,%A\n", o);
		if (true >= 0) {
			cbgen(NE, true, 'I');
			if (false >= 0)
				cbgen(0, false, 'I');
		} else if (false >= 0) {
			cbgen(EQ, false, 'I');
		}
		recl2(p);
		break;
	}
	tfree(p);
}

/*
 * covert from one type to another
 * This includes all promotions, will demote scalers, but will not
 * demote floats.
 */
do_sconv(p, cookie)
register NODE *p;
{
	register char *pc = "";
	register NODE *l;
	int ltype, restype;
	int need = NEED0;
	int rw = RC_LEFT;

	if (M_DONE != order(p->in.left, SH_ORDER))
		cerror("help in do_sconv");

	l = p->in.left;
	ltype = mtype(l->in.type);
	restype = mtype(p->in.type);

	if (ltype == restype) {
		/*
		 * we shouldn't even be in this routine
		 */
		pc = "\t# conversion to same type?\n";

	} else if (p->in.type != DOUBLE) {
		if (l->in.type == DOUBLE) {
			uerror("implicit convertion from double to scalar");
		}
		/*
		 * scalar to scalar conversion
		 */
		switch (l->in.op) {
		case ICON:
			break;

		case REG:
			/*
			 * since we know that TREGs are already zero or
			 * sign extended to long, we don't have to
			 * generate anything when the destination type is
			 * any long, or when the in three special cases
			 * that involve converting chars to short
			 */
			if ((TY_ANY_LONG & restype) ||
			((TY_CHAR & ltype) && (TY_SHORT & restype)) ||
			((TY_UCHAR & ltype) && (TY_ANY_WORD & restype))) {
				break;
			}
			/*
			 * otherwise, we convert based only on
			 * the result type
			 */
			l->in.type = p->in.type;
			if (istreg(l->tn.rval)) {
				pc = "\t%LP\t%LA,%LA\n";
				break;
			}
			need = NEED1(N_AREG);
			rw = RC_KEEP1;
			pc = "\t%LP\t%LA,%1A\n";
			break;

		case OREG:
		case NAME:
			/*
			 * We have memory -- we only need to generate a
			 * conversion if it's really a promotion; the
			 * byte order of the machine lets us get away
			 * with reading a longer type as a shorter one.  We
			 * don't special case signedness conversions here.
			 */
			if (((TY_ANY_BYTE & ltype) && ((TY_ANY_WORD|TY_ANY_LONG) & restype)) ||
			((TY_ANY_WORD & ltype) && (TY_ANY_LONG & restype))) {
				need = NEED1(N_AREG);
				rw = RC_KEEP1;
				pc = "\t%LP\t%LA,%1A\n";
				break;
			}
			break;
		}

	} else {
		/*
		 * result type is double
		 */
		switch (l->in.op) {
		case ICON:
			if (TY_ANY_SIGNED & ltype) {
				/*
				 * signed scalar constant
				 */
				need = NEED1(N_BREG);
				rw = RC_KEEP1;
				pc = "\tmovl\t$%LA%L&,_.cvt\n\tfildl\t_.cvt\n";
				break;
			}
			/*
			 * unsigned scalar constant
			 */
			need = NEED2(N_AREG,N_BREG);
			rw = RC_KEEP2;
			pc = "\tmovl\t$%LA,%1A\n\tmovl\t%1A,_.cvt\n\tfildl\t_.cvt\n\ttstl\t0x80000000,%1A\n\tjz\t0f\n\tfadd\t_.bias.ui2lf\n0:\n";
			break;

		case REG:
			need = NEED1(N_BREG);
			rw = RC_KEEP1;
			if (TY_ANY_SIGNED & ltype) {
				/*
				 * signed AREG
				 */
				pc = "\tmovl\t%LA,_.cvt\n\tfildl\t_.cvt\n";
				break;
			}
			/*
			 * unsigned AREG
			 */
			pc = "\tmovl\t%LA,_.cvt\n\tfildl\t_.cvt\n\ttstl\t0x80000000,%LA\n\tjz\t0f\n\tfadd\t_.bias.ui2lf\n0:\n";
			break;

		case NAME:
		case OREG:
			if (TY_ANY_SIGNED & ltype) {
				/*
				 * signed scalar
				 */
				if (TY_CHAR & ltype) {
					need = NEED2(N_AREG,N_BREG);
					rw = RC_KEEP2;
					pc = "\tmovsbl\t%LA,%1A\n\tmovl\t%1A,_.cvt\n\tfildl\t_.cvt\n";
					break;
				}
				need = NEED1(N_BREG);
				rw = RC_KEEP1;
				pc = "\tfild%LW\t%LA\n";
				break;
			}
			/*
			 * unsigned scalar
			 */
			need = NEED2(N_AREG,N_BREG);
			rw = RC_KEEP2;
			pc = "\t%LP\t%LA,%1A\n\tmovl\t%1A,_.cvt\n\tfildl\t_.cvt\n\ttstl\t0x80000000,%1A\n\tjz\t0f\n\tfadd\t_.bias.ui2lf\n0:\n";
		}
	}
	allo(p, need);
	expand(p, cookie, pc, SCONV);
	return reclaim(p, rw, cookie);
}

/* ``It's obvious'' */
main(argc, argv)
char **argv;
{
	return mainp2(argc, argv);
}
