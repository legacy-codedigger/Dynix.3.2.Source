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
	"$Id: allo.c,v 2.9 88/12/01 10:45:10 ksb Exp $";
#endif /* lint */

/*
 *	Berkeley Pascal Compiler 	(allo.c)
 */

#include "pass2.h"

NODE	resc[3];
static int	maxa = -1, mina = 0, maxb = -1, minb = 0;

RSTATS amregs[REGSZ] = {
	{"%eax",	0,	SH_AREG|SH_TAREG|SH_COND|SH_EAX,0,	0},
	{"%ebx",	0,	SH_AREG|SH_TAREG|SH_COND,	0,	0},
	{"%ecx",	0,	SH_AREG|SH_TAREG|SH_COND,	0,	0},
	{"%edx",	0,	SH_AREG|SH_TAREG|SH_COND,	0,	0},
	{"%esi",	0,	SH_AREG|SH_TAREG,		0,	0},
	{"%edi",	0,	SH_AREG|SH_TAREG,		0,	0},
	{"%ebp",	PBUSY,	SH_AREG,			0,	0},
	{"%esp",	PBUSY,	SH_AREG,			0,	0},
	{"%sp(0)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(1)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(2)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(3)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(4)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(5)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(6)",	0,	SH_BREG|SH_TBREG,		0,	0},
	{"%sp(7)",	0,	SH_BREG|SH_TBREG,		0,	0}
};

#if !defined(ALLO0)
/*
 * free everything
 */
allo0()
{
	register int i;

	maxa = maxb = -1;
	mina = minb = 0;

	for (i = 0; i < REGSZ; ++i) {
		amregs[i].ibusy = 0;
		if (amregs[i].istatus & SH_TAREG) {
			if (maxa<0)
				mina = i;
			maxa = i;
		}
		if (amregs[i].istatus & SH_TBREG) {
			if (maxb<0)
				minb = i;
			maxb = i;
		}
	}
}
#endif /* alloc 0 */

#if !defined(ALLO)
/*
 * q is a code rule, allcate resources for q to execute with NODE p
 */
int
allo(p, n)
NODE *p;
register int n;
{
	register int cur, i, j, r;
	auto int retval;

	retval = 1;
	for (i = 0; i < 3 && retval != 0; ++i) {
		cur = n & N_MASK;
		r = cur >> N_CSHIFT;		/* zero fill shift!	*/
		n >>= N_SHIFT;
#if defined(FLEXNAMES)
		resc[i].in.name = "";
#else
		resc[i].in.name[0] = '\0';
#endif
		switch (cur & N__MASK) {
		case N__S:		/* a particulat register	*/
			resc[i].in.op = REG;
			resc[i].tn.lval = 0;
			resc[i].tn.type = isbreg(r) ? DOUBLE : INT;
			/* needed(!) and allocated ... we have a problem */
			if (! usable(& resc[i], cur, r)) {
				forcereg(& resc[i], r);
			}
			resc[i].tn.rval = r;
			break;
		case N__A:
		case N__C:		/* some a register subset	*/
			resc[i].in.op = REG;
			resc[i].tn.type = p->in.type;
			resc[i].tn.rval = freereg(& resc[i], cur);
			resc[i].tn.lval = 0;
			break;
		case N__B:		/* a register of a given type	*/
			resc[i].in.op = REG;
			resc[i].tn.type = DOUBLE;
			resc[i].tn.rval = getfp(& resc[i], cur);
			resc[i].tn.lval = 0;
			break;
		case N__T:		/* some stack space		*/
			resc[i].in.op = OREG;
			resc[i].tn.rval = TMPREG;
			switch (p->in.op) {
			case STCALL:
			case STARG:
			case UNARY STCALL:
			case STASG:
				r = (SZCHAR*p->stn.stsize + (SZINT-1))/SZINT;
				break;
			default:
				break;
			}

			j = freetemp(r);
			resc[i].tn.lval = BITOOR(j);
			break;
		case N__R:		/* need  rewrite only		*/
			cerror("rewrite!");
			break;
		case N__L:		/* a unique new set label	*/
			resc[i].tn.op = ICON;
			resc[i].tn.type = p->in.type;
			resc[i].tn.su = 0;
			resc[i].tn.lval = getlab();
			resc[i].tn.rval = SETCON;
			break;
		case N__F:
			resc[i].tn.op = FREE;
			break;
		}
	}

	return retval;
}
#endif /* alloc o */

#define newSP()	((SPILL *)malloc(sizeof(SPILL)))
extern char *malloc();

/* the floating spill area fills upward in memory, it is below the locals
 * and has a size of fsparea bytes.  The first number pushed will be
 * spilled to the lowest save area position when the compiler needs
 * the MAX_FSTACK floating point register.
 * Values will be unspilled at a low warter mart (MIN_FSPILL)
 */
#define MAX_FSTACK	8	/* max flt. accel. regs (power of 2!)	*/
#define MIN_FSPILL	3	/* start reloading stack at 3 regs	*/
int fdepth = 0;			/* `87 stack depth			*/
int fsparea = 0;		/* max spill area on i386 stack at LF%d	*/
int fspcur = 0;			/* current spill offset			*/
int factive = 0;		/* active float regs			*/

/*
 * dump register fpn, stack element sto, to offset fpo
 */
fdump(fpn, sto, fpo)
int fpn, sto, fpo;
{
	register SPILL *pSPThis;
	if (sto == 0) {
		printf("\tfstpl\t%d-LF%d(%%ebp)\n", fpo, ftnno);
	} else {
		printf("\tfxch\t%%sp(%d)\n\tfstl\t%d-LF%d(%%ebp)\n", sto, fpo, ftnno);
		printf("\tfxch\t%%sp(%d)\n\tffree\t%%sp(%d)\n", sto, sto);
	}
	pSPThis = newSP();
	pSPThis->islevel = -1;
	pSPThis->isbusy = amregs[fpn].ibusy;
	pSPThis->pNOsown = amregs[fpn].pNOown;
	pSPThis->pSPnext = amregs[fpn].pSPfirst;
	amregs[fpn].pSPfirst = pSPThis;
	amregs[fpn].pNOown = 0;
	amregs[fpn].ibusy = 0;
}

/*
 * go through the fp registers circularly, spill them as we run out.
 */
getfp(p, cur)
NODE *p;
int cur;
{
	register int fpn;

	fpn = (factive++ & (MAX_FSTACK - 1)) + FP0;
	if (factive > MAX_FSTACK) {
		fdump(fpn, 7, fspcur);
		fspcur += 8;
		if (fsparea < fspcur)
			fsparea = fspcur;
	} else {
		++fdepth;
	}

	amregs[fpn].ibusy |= TBUSY;
	amregs[fpn].pNOown = p;
	return fpn;
}

/*
 * we need all the stack space on the `87 stack to a function call
 * (-1 indicated do not restore any, we did not have to drop any)
 */
int
dropflt()
{
	register int nregs, i, o;

	if (fdepth == 0)
		return -1;
	nregs = fdepth;
	fspcur += nregs * 8;
	if (fsparea < fspcur)
		fsparea = fspcur;
	o = fspcur;
	for (i = 0; i < nregs; ++i) {
		o -= 8;
		fdump((o >> 3)+FP0, 0, o);
	}
	fdepth = 0;
	return nregs;
}

/*
 * restore floats from float spill area
 */
restflt(fnregs)
{
	register int i;
	register SPILL *pSP;
	auto int r;

#if defined(KBUG)
	if (factive < fnregs)
		cerror("not enought doubles to load!");
#endif

	if (fdepth >= fnregs)
		return;

	if (fdepth == 0) {
		fspcur -= fnregs << 3;
		for (i = 0; i < fnregs; ++i) {
			r = fspcur >> 8 + i;
			r = (r & 7) + FP0;
			pSP = amregs[r].pSPfirst;
			amregs[r].pSPfirst = pSP->pSPnext;
			amregs[r].ibusy = pSP->isbusy;
			amregs[r].pNOown = pSP->pNOsown;
			amregs[r].pSPfirst = pSP->pSPnext;
			free(pSP);
			printf("\tfldl\t%d-LF%d(%%ebp)\n", fspcur+(i<<3), ftnno);
		}
	} else {
		for (i = fdepth; i < fnregs; ++i) {
			fspcur -= 8;
			r = fspcur >> 3;
			r = (r & 7) + FP0;
			printf("\tfldl\t%d-LF%d(%%ebp)\n", fspcur, ftnno);
			if (7 == i)
				printf("\tfincstp\n");
			else
				printf("\tfstp\t%%st(%d)\n", i+1);
			pSP = amregs[r].pSPfirst;
			amregs[r].pSPfirst = pSP->pSPnext;
			amregs[r].ibusy = pSP->isbusy;
			amregs[r].pNOown = pSP->pNOsown;
			amregs[r].pSPfirst = pSP->pSPnext;
			free(pSP);
		}
	}
	fdepth = fnregs;
}

/*
 * we have to have *this* register, no other will do.  So generate code
 * to move this value in it to some other place (register) and give it
 * to us.
 * force register r into a free register (f) for node amreg[r].pNOown
 */
forcereg(p, r)
NODE *p;
{
	register int f, b;
	register NODE *pr;

	pr = amregs[r].pNOown;
	b =  amregs[r].ibusy;
	f = freereg(pr, (amregs[r].istatus & (SH_TAREG|SH_AREG)) ? N__A : N__B);
	rmove(f, r, pr->in.type);
	pr->tn.rval = f;
	amregs[f].ibusy = b;
	xferown(pr, r, p);
}

extern unsigned int offsz;

/*
 * Allocate k integers worth of temp space.
 * We also make the convention that if the 
 * number of words is more than 1, it must
 * be aligned for storing doubles...
 */
long
freetemp(k)
{ 
#if defined(BACKTEMP)
	tmpoff += k*SZINT;
	if (k > 1) {
		SETOFF(tmpoff, ALDOUBLE);
	}
	if (tmpoff > maxoff)
		maxoff = tmpoff;
	if (tmpoff >= offsz)
		cerror("stack overflow");
	if (tmpoff-baseoff > maxtemp)
		maxtemp = tmpoff-baseoff;
	return -tmpoff;
#else
	register int t;

	if (k > 1) {
		SETOFF(tmpoff, ALDOUBLE);
	}

	t = tmpoff;
	tmpoff += k*SZINT;
	if (tmpoff > maxoff)
		maxoff = tmpoff;
	if (tmpoff >= offsz)
		cerror("stack overflow");
	if (tmpoff-baseoff > maxtemp)
		maxtemp = tmpoff-baseoff;
	return(t);
#endif
}

/*
 * any register of type A not owned by any node in arglist
 */
any_not_owned(a)
NODE *a;
{
	register NODE **pp;
	register int j;

	for(j=mina; j<=maxa; ++j) {
		if (0 == (amregs[j].istatus & SH_TAREG)) {
			continue;
		}
		for (pp = & a; *pp != (NODE *)0; ++pp) {
			if (amregs[j].pNOown == *pp)
				goto next;
		}
		return j;
	next:
		;
	}
	return -1;
}

/*
 * Allocate a register to fit need n, p is dest node
 */
freereg(p, n)
NODE *p;
{
	register int j, f;

#if defined(KBUG)
	if ((n & N__MASK) == N__B) {
		cerror("no you don't");
	}
#endif
	f = (p->in.rall & RA_REJECT) ? RA_REG(p->in.rall) : -1;
	for (j = mina; j <= maxa; ++j) {
		if (0 == (amregs[j].istatus & SH_TAREG)) {
			continue;
		}
		if (j != f && usable(p, n, j))
			return j;
	}
	/* use the one we should avoid anyway */
	if (f != -1 && 0 != (amregs[j].istatus & SH_TAREG)) {
		if (usable(p, n, f))
			return f;
	}
	cerror("out of registers!");
	exit(1);
	/*NOTREACHED*/
}

#if !defined(USABLE)
/*
 * decide if register r is usable in tree p to satisfy need n
 */
usable(p, n, r)
NODE *p;
{
#if defined(KBUG)
	if (!istreg(r))
		cerror("usable asked about nontemp register");
#endif

	if (0 != amregs[r].ibusy)
		return 0;	/* used to call share it for no reason */

	switch (N__MASK & n) {
	case N__A:
		if (isbreg(r)) {
			return 0;
		}
		break;
	case N__B:
		if (!isbreg(r)) {
			return 0;
		}
	case N__C:
		if (0 == (amregs[r].istatus & SH_COND)) {
			return 0;
		}
		break;
	case N__S:
		break;
	}

	amregs[r].ibusy |= TBUSY;
	amregs[r].pNOown = p;
	return(1);
}
#endif

/*
 * release registers back to free pool
 */
recl2(p)
register NODE *p;
{
	register r = p->tn.rval;
	register int op = p->in.op;

	switch (op) {
	case FREE:
		return;
	case REG:
		rfree(r, p->in.type);
		break;
	case OREG:
		rfree(r, PTR|INT);
		break;
	case ICON:
	case NAME:
		/* nothing to do here */
		break;
	default:
		cerror("recl2 got a bad (unmatched?) node");
	}
}

/*
 * spill registers to the stack for a call only if they are busy
 * spill takes a list of regs to spill, term with a -1.
 *	spill(EAX, EDX, -1);
 */
static int sp_level = 0;
/* VARARGS */
int
spill(r)
int r;
{
	register int *pr;
	register int j;
	auto SPILL  *pSPThis;
	auto int i;

	for (i = 0; i < REGSZ; ++i) {
		for (pr = & r; -1 != (j = *pr); ++pr) {
			if (j != i) {		/* try next */
				continue;
			}
			if (! ISBUSY(j)) {	/* no value, no spill */
				break;
			}
			printf("\tpushl\t%s\n", amregs[j].acname);
			pSPThis = newSP();
			pSPThis->islevel = sp_level;
			pSPThis->isbusy = amregs[j].ibusy;
			pSPThis->pNOsown = amregs[j].pNOown;
			pSPThis->pSPnext = amregs[j].pSPfirst;
			amregs[j].pSPfirst = pSPThis;
			amregs[j].pNOown = 0;
			amregs[j].ibusy = 0;
			break;
		}
	}
	return sp_level++;
}

/*
 * undo what a spill does
 */
unspill(l)
int l;
{
	register int i;
	register SPILL *pSPThis;

	if (l != --sp_level)
		cerror("overlapping stack spills");
	for (i = REGSZ; i-- > 0; ) {
		if (nilSP == (pSPThis = amregs[i].pSPfirst) || pSPThis->islevel != l)
			continue;
		printf("\tpopl\t%s\n", amregs[i].acname);
		amregs[i].pSPfirst = pSPThis->pSPnext;
		amregs[i].ibusy = pSPThis->isbusy;
		amregs[i].pNOown = pSPThis->pNOsown;
		amregs[i].pSPfirst = pSPThis->pSPnext;
		free(pSPThis);
	}
}

/*
 * this register might be a temporary, if so really allocate it
 * we need to keep it through the next expand call.
 */
keepreg(r, t, p)
int r;
TWORD t;
NODE *p;
{
	if (EBP == r) {
		return;
	} else if (ISTBUSY(r)) {
		amregs[r].ibusy &= ~TBUSY;
		amregs[r].ibusy |= PBUSY;
	} else if (! ISBUSY(r) || amregs[r].pNOown != p) {
		cerror("cannot keep what you don't have...");
	}
}

/*
 * this resource is what we want to keep
 */
keep(p)
register NODE *p;
{
	register int r = p->tn.rval;

	switch (p->in.op) {
	case FREE:
		cerror("keep: keep a free node?");
		break;
	case REG:
		if (r >= REGSZ) {
			/* fall through */;
		} else {
			keepreg(r, p->in.type, p);
			break;
		}
	case OREG:
		if (R2TEST(r)) {
			if (R2UPK1(r) != 100)	/* ZZZ 100 ? */
				keepreg(R2UPK1(r), PTR+INT, p);
			keepreg(R2UPK2(r), INT, p);
		} else {
			keepreg(r, PTR+INT, p);
		}
		break;
	case ICON:
	case NAME:
		/* nothing to do here */
		break;
	default:
		cerror("keep got a bad node");
		break;
	}
}

int rdebug = 0;

#if !defined(RFREE)
/*
 * Mark register r free, if it is legal to do so (t is the type)
 */
rfree(r, t)
int r;
TWORD t;
{
#if !defined(BUG3)
	if (rdebug) {
		printf("rfree(%s), size %d\n", amregs[r].acname, 1);
	}
#endif

	if (EBP == r || ESP == r)
		return;
	if (! ISBUSY(r)) {
		cerror("register overfreed");
	}
	if (isbreg(r)) {
		--fdepth;
		--factive;
	}
	amregs[r].ibusy &= ~(TBUSY|PBUSY);

#if defined(KBUG)
	if (0 != kdebug) switch (amregs[r].pNOown->in.op) {
	case REG:
	case OREG:
		break;
	default:
		fprintf(kdebug, "register free'd from nonregister node %s\n", amregs[r].acname);
		break;
	}
#endif
}
#endif

#if !defined(RBUSY)
/*
 * Mark register r busy
 * t is the type 
 */
rbusy(r, t, pNOOwn)
TWORD t;
NODE *pNOOwn;
{
#if !defined(BUG3)
	if (rdebug) {
		printf("rbusy(%s), size %d\n", amregs[r].acname, 1);
	}
#endif

	if (EBP == r)			/* all sha:e this, of course */
		return;
	if (! istreg(r)) {
		cerror("non temp reg with TBUSY set?");
	}
	amregs[r].ibusy |= TBUSY;
	amregs[r].pNOown = pNOOwn;
}
#endif

#if !defined(BUG3)
/*
 * print rewriting rule
 */
rwprint(rw)
{ 
	register i, flag;
	static char *rwnames[] = {
		"RLEFT",
		"RRIGHT",
		"RESC1",
		"RESC2",
		"RESC3",
		(char *)0,
	};

	if (rw == RC_NULL) {
		printf("RC_NULL");
		return;
	}
	if (rw == RC_NOP) {
		printf("RC_NOP");
		return;
	}

	flag = 0;
	for(i=0; rwnames[i]; ++i) {
		if (rw & (1<<i)) {
			if (flag) printf("|");
			++flag;
			printf(rwnames[i]);
		}
	}
}
#endif

/*
 * get back stuff allo built
 */
reclaim(p, rw, cookie)
NODE *p;
{
	register NODE **qq;
	register NODE *q;
	auto NODE *recres[6];
	register int opty;

	if (rw == RC_NOP || (p->in.op==FREE && rw==RC_NULL))  /* do nothing */
		return;

	opty = optype(p->in.op);

	/* RC_POP is for the floating stack, chaing it into
	 * either an RC_LEFT or RC_RIGHT depend on reg nums
	 */
	if (RC_POP == rw) {
		if (p->in.left->tn.rval < p->in.right->tn.rval) {
			if (p->in.left->tn.rval+1 == p->in.right->tn.rval) {
				rw = RC_LEFT;
			} else {
				rw = RC_RIGHT;
			}
		} if (p->in.left->tn.rval == p->in.right->tn.rval+1) {
			rw = RC_RIGHT;
		} else {
			rw = RC_LEFT;
		}
	}
	/* locate results */
	qq = recres;
	if (opty != LTYPE) {
		if (0 == (rw & RC_LEFT)) {
			recl2(p->in.left);
			tfree1(p->in.left);
		} else {
			*qq++ = p->in.left;
		}
	}
	if (opty == BITYPE) {
		if (0 == (rw & RC_RIGHT)) {
			recl2(p->in.right);
			tfree1(p->in.right);
		} else {
			*qq++ = p->in.right;
		}
	}
	if (0 == (rw&RC_KEEP1)) {
		recl2(& resc[0]);
		resc[0].tn.op = FREE;
	} else {
		*qq++ = & resc[0];
	}
	if (0 == (rw&RC_KEEP2)) {
		recl2(& resc[1]);
		resc[1].tn.op = FREE;
	} else {
		*qq++ = & resc[1];
	}
	if (0 == (rw&RC_KEEP3)) {
		recl2(& resc[2]);
		resc[2].tn.op = FREE;
	} else {
		*qq++ = & resc[2];
	}

	/* totally clobber, leaving nothing */
	if (rw == RC_NULL || qq == recres) {
		switch (p->in.op) {
		case REG:
		case OREG:
			recl2(p);
			break;
		default:
			break;
		}
		tfree(p);
		return;
	}

	if (& recres[1] != qq) {
		cerror("more than one thing to keep in recliam!");
	}

	q = recres[0];
	keep(q);

	if (callop(p->in.op)) {	/* check that all scratch regs are free     */
		callchk(p);     /* ordinarily, this is the same as allchk() */
	}

	if (p->in.op == STARG) { /* STARGs are still STARGS  humm? ZZZ */
		p = p->in.left;
	}
	nxfer(p, q);
	/* don't free resc nodes, they are static for all time
	 */
	if (0 == ((RC_KEEP1|RC_KEEP2|RC_KEEP3) & rw))
		tfree1(q);
	else
		q->tn.op = FREE;
}

/*
 * transfer the contents of q to p, including ownerships
 */
nxfer(p, q)
NODE *p, *q;
{
	p->in.op = q->in.op;
	p->in.type = q->in.type;  /* to make multi-register allocations work */
	p->tn.lval = q->tn.lval;
	p->tn.rval = q->tn.rval;
#if defined(FLEXNAMES)
	p->in.name = q->in.name;
#else
	{ register int i;
	for(i=0; i<NCHNAM; ++i)
		p->in.name[i] = q->in.name[i];
	}
#endif

	switch(p->in.op) {
	case REG:
	case OREG:
		xferown(q, q->tn.rval, p);
		break;
	case ICON:
	case NAME:
		/* was a label... he gets to keep it. */
		break;
	default:
		cerror("reclaim what?");
		break;
	}
}

/*
 * tranfer ownership of a register to a new node
 */
int
xferown(pOwn, iReg, pTo)
NODE *pOwn, *pTo;
int iReg;
{
	if (EBP == iReg)
		return;
	if (amregs[iReg].pNOown != pOwn) {
		cerror("not owner of a register to give it away!");
	}
	amregs[iReg].pNOown = pTo;
}

/*
 * Copy the contents of p into q without any feeling for the contents.
 * This code assume that copying rval and lval does the job; in 
 * general, it might be necessary to special case the operator types.
 */
ncopy(q, p)
NODE *p, *q;
{
	q->in.op = p->in.op;
	q->in.rall = p->in.rall;
	q->in.type = p->in.type;
	q->tn.lval = p->tn.lval;
	q->tn.rval = p->tn.rval;
#if defined(FLEXNAMES)
	q->in.name = p->in.name;
#else
	{
		register int i;
		for(i=0; i<NCHNAM; ++i) {
			q->in.name[i] = p->in.name[i];
		}
	}
#endif

}

/*
 * make a fresh copy of p
 */
NODE *
tcopy(p)
register NODE *p;
{
	register NODE *q;
	register int r;

	ncopy(q=talloc(), p);

	r = p->tn.rval;
	if (p->in.op == REG)
		rbusy(r, p->in.type, p);
	else if (p->in.op == OREG) {
		if (R2TEST(r)) {
			if (R2UPK1(r) != 100)	/* ZZZ 100 ? */
				rbusy(R2UPK1(r), PTR+INT, p);
			rbusy(R2UPK2(r), INT, p);
		} else {
			rbusy(r, PTR+INT, p);
		}
	}
	switch(optype(q->in.op)) {
	case BITYPE:
		q->in.right = tcopy(p->in.right);
	case UTYPE:
		q->in.left = tcopy(p->in.left);
	default: /* ZZZ ksb added default */
		break;
	}
	return(q);
}

/*
 * check to ensure that all register are free, or really allocated
 * not just temp. allocated for this expand.
 */
allchk()
{
	register int i;

	for (i = 0; i < REGSZ; ++i) {
		if (ISTBUSY(i)) {
			cerror("register allocation error %s", amregs[i].acname);
		}
	}
}
