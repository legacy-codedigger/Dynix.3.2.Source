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
#include <sys/types.h>
#include <fcntl.h>

#if !defined(lint)
static char     rcsid[] =
	"$Id: reader.c,v 2.8 88/09/02 11:46:28 ksb Exp $";
#endif lint

/*
 *	Berkeley Pascal Compiler	(reader.c)
 */

#include "pass2.h"

/* some storage declarations */

NODE node[TREESZ];
char    filename[100] = "";	       /* the name of the file */
int     ftnno;			       /* number of current function */
int     lineno;

int     nrecur;
int     lflag;
int     Oflag = 0;
extern int      Wflag;
int     edebug = 0;
int     xdebug = 0;
int     udebug = 0;
int     vdebug = 0;
#if defined(KBUG)
FILE	*kdebug = 0;		/* file to write ksb's error mesages to	*/
#endif

/* offset for first temporary in bits for current block */
OFFSZ tmpoff;
/* maximum temporary offset over all blocks in current ftn, in bits */
OFFSZ maxoff;

int     maxtreg;

OFFSZ baseoff = 0;
OFFSZ maxtemp = 0;

/*
 * set the values of the pass 2 arguments and parameters
 */
p2init (argc, argv)
char **argv;
{
	register int c;
	register char *cp;
	register int  files;

#if defined(KBUG)
	if (-1 != fcntl(3, F_GETFD, 0)) { /* is ksb on channel 3? */
		kdebug = fdopen(3, "w");
		fprintf(kdebug, "hi!\n");
	}
#endif

	allo0();	/* free all regs */
	files = 0;

	for (c = 1; c < argc; ++c) {
		if (*(cp = argv[c]) != '-') {
			files = c;	/* assumed to be a filename */
			continue;
		}
		while (*++cp) {
			switch (*cp) {

			case 'X': /* pass1 flags */
				while (*++cp) {/* VOID */
				}
				--cp;
				break;
			case 'l': /* linenos */
				++lflag;
				break;
			case 'e': /* expressions */
				++edebug;
				break;
			case 'o': /* orders */
				++odebug;
				break;
			case 'r': /* register allocation */
				++rdebug;
				break;
			case 'a': /* rallo */
				++radebug;
				break;
			case 'v': 
				++vdebug;
				break;
			case 't': /* mtype calls */
				++tdebug;
				break;
			case 'u': /* Sethi-Ullman testing */
				++udebug;
				break;
			case 'x': /* general machine-dependent debugging flag */
				++xdebug;
				break;
			case 'w': 
			case 'W': /* shut up warnings */
				++Wflag;
				break;
			case 'O': /* optimizing */
				++Oflag;
				break;
			default: 
				cerror("bad option: %c", *cp);
			}
		}
	}
	mkdope();
	opinit();	/* set up table.c */
#if defined(MULTILEVEL)
	/* also initialize multi-level tree links */
	mlinit();
#endif
	if (0 == files && isatty(0)) {
		files = 1;
		argv[1] = "pc.pc0";
	}
	return(files);
}

#if !defined(BUG4)
/* print a nice-looking description of cookie */
prcook (cookie)
{
	printf("SH_NULL");
	if (cookie & SH_FOREFF) printf("|SH_FOREFF");
	if (cookie & SH_AREG) printf("|SH_AREG");
	if (cookie & SH_TAREG) printf("|SH_TAREG");
	if (cookie & SH_BREG) printf("|SH_BREG");
	if (cookie & SH_TBREG) printf("|SH_TBREG");
	if (cookie & SH_NAME) printf("|SH_NAME");
	if (cookie & SH_FLD) printf("|SH_FLD");
	if (cookie & SH_QCON) printf("|SH_QCON");
	if (cookie & SH_BCON) printf("|SH_BCON");
	if (cookie & SH_WCON) printf("|SH_WCON");
	if (cookie & SH_LCON) printf("|SH_LCON");
	if (cookie & SH_OAREG) printf("|SH_OAREG");
	if (cookie & SH_OFP) printf("|SH_OFP");
}
#endif

int     odebug = 0;
int     callflag;
int     fregs;


/*
 * we are looking for a subtree to factor out and store
 */
mkadrs(p)
register NODE *p;
{
	register int o;

	o = p->in.op;

	if (asgop(o)) {
		if( p->in.left->in.su >= p->in.right->in.su ) {
			if( p->in.left->in.op == UNARY MUL ) {
				(void)order(p->in.left->in.left, SH_ORDER);
			} else if( p->in.left->in.op == FLD &&
				   p->in.left->in.left->in.op == UNARY MUL ) {
				(void)order(p->in.left->in.left->in.left, SH_ORDER);
			} else {  /* should be only structure assignment */
				(void)order(p->in.left, SH_ORDER);
			}
		} else {
			(void)order(p->in.right, SH_ORDER);
		}
	} else if( p->in.left->in.su > p->in.right->in.su ) {
		(void)order(p->in.left, SH_ORDER);
	} else {
		(void)order(p->in.right, SH_ORDER);
	}
}

/* find a subtree of p which should be stored
 */
store(p)
register NODE *p;
{
	register int  o, ty;

	o = p->in.op;
	ty = optype (o);

	if (ty == LTYPE)
		return;

	switch (o) {
	case UNARY CALL: 
	case UNARY FORTCALL: 
	case UNARY STCALL: 
		++callflag;
		break;
	case UNARY MUL: 
		if (M_DONE == um2oreg(p))
			return;
		break;
	case CALL: 
	case FORTCALL: 
	case STCALL: 
		store (p->in.left);
		stoarg (p->in.right, o);
		++callflag;
		return;
	case COMOP: 
		markcall (p->in.right);
		if (p->in.right->in.su > fregs) {
			(void)order(p, SH_ORDER);
		}
		store(p->in.left);
		return;
	case ANDAND: 
	case OROR: 
	case QUEST: 
		markcall(p->in.right);
		if (p->in.right->in.su > fregs) {
			(void)order(p, SH_ORDER);
		}
	case CBRANCH:
	case NOT: 
		/* prevent complicated LHS expressions from being stored
		 */
		constore(p->in.left);
		return;
	}
	if (ty == UTYPE) {
		store(p->in.left);
		return;
	}
	/* try to diminish registers by pulling off a large subtree
	 */
	if (p->in.su > fregs) {
		mkadrs(p);
	}
	store(p->in.right);
	store(p->in.left);
}

/* 
 * store conditional expressions
 * the point is to avoid storing expressions in conditional
 * context, since the evaluation order is predetermined
 */
constore(p)
register NODE *p;
{
	switch (p->in.op) {
	case ANDAND: 
	case OROR: 
	case QUEST: 
		markcall(p->in.right);
		/*falling*/
	case NOT: 
		constore(p->in.left);
		return;
	}
	store(p);
}

/* mark off calls below the current node
 */
markcall(p)
register   NODE * p;
{
	for (;;) {
		switch (p->in.op) {
		case UNARY CALL: 
		case UNARY STCALL: 
		case UNARY FORTCALL: 
		case CALL: 
		case STCALL: 
		case FORTCALL: 
			++callflag;
			return;
		}
		switch (optype (p->in.op)) {
		case BITYPE: 
			markcall (p->in.right);
			/*falling*/
		case UTYPE: 
			p = p->in.left;
			/* eliminate recursion */
			continue;
		case LTYPE: 
			return;
		}
		break;
	}
}

/* arrange to store the args
 */
stoarg(p, calltype)
register   NODE * p;
{

	if (p->in.op == CM) {
		stoarg (p->in.left, calltype);
		p = p->in.right;
	}
	if (calltype == CALL) {
		STOARG(p);
	} else if (calltype == STCALL) {
		STOSTARG(p);
	} else {
		STOFARG(p);
	}
	callflag = 0;
	store (p);
#if !defined(NESTCALLS)
	if (callflag) {/* prevent two calls from being active at once  */
		(void)order(p, SH_ORDER);
	}
#endif
}

/* count recursions -- too many and we know we are stuck
 */
rcount ()
{
	if (++nrecur > NRECUR) {
		cerror("expression causes compiler loop: try simplifying");
	}
}

#if !defined(BUG4)
eprint (p, down, a, b)
NODE * p;
int    *a, *b;
{
	*a = *b = down + 1;
	while (down >= 2) {
		printf ("\t");
		down -= 2;
	}
	if (down--)
		printf ("    ");
	printf ("%o) %s", p, opst[p->in.op]);
	switch (p->in.op) {/* special cases */
	case REG: 
		printf (" %s", amregs[p->tn.rval].acname);
		break;
	case ICON: 
	case NAME: 
	case OREG: 
		printf (" ");
		adrput (p);
		break;
	case STCALL: 
	case UNARY STCALL: 
	case STARG: 
	case STASG: 
		printf (" size=%d", p->stn.stsize);
		printf (" align=%d", p->stn.stalign);
		break;
	}
	printf (", ");
	tprint (p->in.type);
	printf (", ");
	if (p->in.rall != 0) {
		printf("reg_pref %d", p->in.rall);
	}
	printf (", SU= %d\n", p->in.su);
}
#endif

/* put p in canonical form
 */
canon (p)
NODE * p;
{
	extern int oreg2(), sucomp();

#if defined(MYCANON)
	MYCANON(p);		/* your own canonicalization routine(s) */
#endif
	walkf(p, sucomp);	/* do the Sethi-Ullman computation	*/
}
