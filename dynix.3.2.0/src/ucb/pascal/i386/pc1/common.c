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
	"$Id: common.c,v 2.8 88/09/02 11:46:06 ksb Exp $";
#endif lint

/*
 *	Berkeley Pascal Compiler	(common.c)
 */

#include "pass2.h"

#undef BUFSTDERR

#if !defined(EXIT)
#define EXIT exit
#endif

int nerrors = 0;  /* number of errors */

extern unsigned int offsz;

unsigned caloff() {
	register i;
	unsigned int temp;
	unsigned int off;

	temp = 1;
	i = 0;
	do {
		temp <<= 1;
		++i;
	} while( temp != 0 );
	off = 1 << (i-1);
	return (off);
}

NODE *lastfree;  /* pointer to last free node; (for allocator) */

/* VARARGS1 */
uerror( s, a ) char *s; { /* nonfatal error message */

	/* the routine where is different for pass 1 and pass 2; */
	/* it tells where the error took place 			 */

	++nerrors;
	where('u');
	fprintf( stderr, s, a );
	fprintf( stderr, "\n" );
#if defined(BUFSTDERR)
	fflush(stderr);
#endif
	if( nerrors > 30 ) cerror( "too many errors");
}

/* compiler error: die */
/* VARARGS1 */
cerror(s, a)
char *s;
int a;
{
	fflush(stdout);
	where('c');
	/* give the compiler the benefit of the doubt */
	if( nerrors && nerrors <= 30 ) {
		fprintf( stderr, "cannot recover from earlier errors: goodbye!\n" );
	} else {
		fprintf(stderr, "compiler error: ");
		_doprnt(s, & a, stderr);
		fputc('\n', stderr);
	}
	fflush(stderr);
	EXIT(1);
}

int Wflag = 0; /* Non-zero means do not print warnings */

/* warning */
/* VARARGS1 */
werror( s, a, b)
char *s;
{
	if(Wflag) return;
	where('w');
	fprintf( stderr, "warning: " );
	fprintf( stderr, s, a, b );
	fprintf( stderr, "\n" );
#if defined(BUFSTDERR)
	fflush(stderr);
#endif
}

static NODE *ntfree;
/* initialize expression tree search */
tinit()
{
	register NODE *p;
	register NODE *lastnode;

	lastnode = &node[TREESZ-1];
	for( p = ntfree = node; p < lastnode; ++p) {
		p->in.op = FREE;
		p->in.left = p+1;
	}
	lastnode->in.op = FREE;
	lastnode->in.left = (NODE *)0;
	lastfree = node;
}

/* #define TNEXT(p) (p== &node[TREESZ-1]?node:p+1) */

/* LINEAR SEARCH !!!! ZZZZZZZZ */
NODE *
talloc()
{
	register NODE *p;

	if ((NODE *)0 != ntfree) {
		p = ntfree;
		ntfree = p->in.left;
#if defined(DEBUG)
		if (FREE != p->in.op)
			cerror( "corrupt tree space!" );
#endif
		return p;
	}
	cerror( "out of tree space; simplify expression" );
	/* NOTREACHED */
}

/* ensure that all nodes have been freed */
tcheck()
{
	register NODE *p;
	register NODE *lastnode;

	lastnode = &node[TREESZ-1];
#if defined(KBUG)
	if( !nerrors  && 0 != kdebug) {
		for( p=node; p <= lastnode; ++p ) {
			if( p->in.op != FREE )
				fprintf(kdebug, "wasted space: %x, op = %d\n", p, p->in.op);
		}
	}
#endif
	tinit();
#if defined(FLEXNAMES)
	freetstr();
#endif
}

/* ZZZ comment */
walkf( t, f )
register NODE *t;
int (*f)();
{
	register int opty;

	opty = optype(t->in.op);

	if (opty != LTYPE)
		walkf( t->in.left, f );
	if (opty == BITYPE)
		walkf( t->in.right, f );
	(*f)(t);
}

/*
 * called on every node in a tree to free it
 * return the left pointer we use to store links
 */
NODE *
tfree1(p)
register NODE *p;
{
	register NODE *q;

#if defined(KBUG)
	if((NODE *)0 == p)
		cerror("freeing nil tree!");
	if (p == & resc[0] || p == &resc[1] || p == &resc[2])
		cerror("oops!  tfree of static tree");
#endif /* if this happens we are toast anyway (ksb) */
	q = p->in.left;
	p->in.op = FREE;
	p->in.left = ntfree;
	ntfree = p;
	return q;
}

/* free the tree p
 * dive right, tfree1 it, cycle left
 */
tfree(p)
register NODE *p;
{
	if (p->in.op != FREE) {
		for (;;) { switch( optype( p->in.op ) ) {
		case BITYPE:
			tfree(p->in.right);
			/* fall through */
		case UTYPE:
			p = tfree1(p);
			continue;
		default:
			tfree1(p);
			break;
		} break; }
	}
}

fwalk( t, f, down )
register NODE *t;
int (*f)();
int down;
{
	auto int down1, down2;

	for (;;) {
		down1 = down2 = 0;

		(*f)( t, down, &down1, &down2 );

		switch( optype( t->in.op ) ) {
		case BITYPE:
			fwalk( t->in.left, f, down1 );
			t = t->in.right;
			down = down2;
			continue;

		case UTYPE:
			t = t->in.left;
			down = down1;
			continue;
		default:
			break;
		}
		break;
	}
}

int dope[DSIZE];
char *opst[DSIZE];

struct dopest {
	int dopeop;
	char opst[8];
	int dopeval;
};

struct dopest indope[] = {
	NAME, "NAME", LTYPE,
	STRING, "STRING", LTYPE,
	REG, "REG", LTYPE,
	OREG, "OREG", LTYPE,
	ICON, "ICON", LTYPE,
	FCON, "FCON", LTYPE,
	DCON, "DCON", LTYPE,
	CCODES, "CCODES", LTYPE,
	UNARY MINUS, "U-", UTYPE,
	UNARY MUL, "U*", UTYPE,
	UNARY AND, "U&", UTYPE,
	UNARY CALL, "UCALL", UTYPE|CALLFLG,
	UNARY FORTCALL, "UFCALL", UTYPE|CALLFLG,
	NOT, "!", UTYPE|LOGFLG,
	COMPL, "~", UTYPE,
	FORCE, "FORCE", UTYPE,
	INIT, "INIT", UTYPE,
	SCONV, "SCONV", UTYPE,
	PCONV, "PCONV", UTYPE,
	PLUS, "+", BITYPE|FLOFLG|SIMPFLG|COMMFLG,
	ASG PLUS, "+=", BITYPE|ASGFLG|ASGOPFLG|FLOFLG|SIMPFLG|COMMFLG,
	MINUS, "-", BITYPE|FLOFLG|SIMPFLG,
	ASG MINUS, "-=", BITYPE|FLOFLG|SIMPFLG|ASGFLG|ASGOPFLG,
	MUL, "*", BITYPE|FLOFLG|MULFLG,
	ASG MUL, "*=", BITYPE|FLOFLG|MULFLG|ASGFLG|ASGOPFLG,
	AND, "&", BITYPE|SIMPFLG|COMMFLG,
	ASG AND, "&=", BITYPE|SIMPFLG|COMMFLG|ASGFLG|ASGOPFLG,
	QUEST, "?", BITYPE,
	COLON, ":", BITYPE,
	ANDAND, "&&", BITYPE|LOGFLG,
	OROR, "||", BITYPE|LOGFLG,
	CM, ",", BITYPE,
	COMOP, ",OP", BITYPE,
	ASSIGN, "=", BITYPE|ASGFLG,
	DIV, "/", BITYPE|FLOFLG|MULFLG|DIVFLG,
	ASG DIV, "/=", BITYPE|FLOFLG|MULFLG|DIVFLG|ASGFLG|ASGOPFLG,
	MOD, "%", BITYPE|DIVFLG,
	ASG MOD, "%=", BITYPE|DIVFLG|ASGFLG|ASGOPFLG,
	LS, "<<", BITYPE|SHFFLG,
	ASG LS, "<<=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG,
	RS, ">>", BITYPE|SHFFLG,
	ASG RS, ">>=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG,
	OR, "|", BITYPE|COMMFLG|SIMPFLG,
	ASG OR, "|=", BITYPE|COMMFLG|SIMPFLG|ASGFLG|ASGOPFLG,
	ER, "^", BITYPE|COMMFLG|SIMPFLG,
	ASG ER, "^=", BITYPE|COMMFLG|SIMPFLG|ASGFLG|ASGOPFLG,
	INCR, "++", BITYPE|ASGFLG,
	DECR, "--", BITYPE|ASGFLG,
	STREF, "->", BITYPE,
	CALL, "CALL", BITYPE|CALLFLG,
	FORTCALL, "FCALL", BITYPE|CALLFLG,
	EQ, "==", BITYPE|LOGFLG,
	NE, "!=", BITYPE|LOGFLG,
	LE, "<=", BITYPE|LOGFLG,
	LT, "<", BITYPE|LOGFLG,
	GE, ">", BITYPE|LOGFLG,
	GT, ">", BITYPE|LOGFLG,
	UGT, "UGT", BITYPE|LOGFLG,
	UGE, "UGE", BITYPE|LOGFLG,
	ULT, "ULT", BITYPE|LOGFLG,
	ULE, "ULE", BITYPE|LOGFLG,
#if defined(ARS)
	ARS, "A>>", BITYPE,
#endif
	TYPE, "TYPE", LTYPE,
	LB, "[", BITYPE,
	CBRANCH, "CBRANCH", BITYPE,
	FLD, "FLD", UTYPE,
	PMCONV, "PMCONV", BITYPE,
	PVCONV, "PVCONV", BITYPE,
	RETURN, "RETURN", BITYPE|ASGFLG|ASGOPFLG,
	CAST, "CAST", BITYPE|ASGFLG|ASGOPFLG,
	GOTO, "GOTO", UTYPE,
	STASG, "STASG", BITYPE|ASGFLG,
	STARG, "STARG", UTYPE,
	STCALL, "STCALL", BITYPE|CALLFLG,
	UNARY STCALL, "USTCALL", UTYPE|CALLFLG,
	-1,	"",	0
};

mkdope() {
	register struct dopest *q;

	for( q = indope; q->dopeop >= 0; ++q ) {
		dope[q->dopeop] = q->dopeval;
		opst[q->dopeop] = q->opst;
	}
}

#if !defined(BUG4)
/*
 * output a nice description of the type of t
 */
tprint( t )
TWORD t;
{
	static char * tnames[] = {
		"undef",
		"farg",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"uchar",
		"ushort",
		"unsigned",
		"ulong",
		"?", "?"
	};

	for( ;; t = DECREF(t) ) {

		if( ISPTR(t) ) printf( "PTR " );
		else if( ISFTN(t) ) printf( "FTN " );
		else if( ISARY(t) ) printf( "ARY " );
		else {
			printf( "%s", tnames[t] );
			return;
		}
	}
}
#endif

#if defined(FLEXNAMES)
#define	NTSTRBUF	40
#define	TSTRSZ		2048
char	itstrbuf[TSTRSZ];
char	*tstrbuf[NTSTRBUF] = { itstrbuf };
char	**curtstr = tstrbuf;
int	tstrused;

char *
tstr(cp)
register char *cp;
{
	register int i = strlen(cp);
	register char *dp;
	extern char *malloc(), *strcpy();

	if (tstrused + i >= TSTRSZ) {
		if (++curtstr >= &tstrbuf[NTSTRBUF])
			cerror("out of temporary string space");
		tstrused = 0;
		if (*curtstr == 0) {
			dp = (char *)malloc(TSTRSZ);
			if (dp == 0)
				cerror("out of memory (tstr)");
			*curtstr = dp;
		}
	}
	strcpy(dp = *curtstr+tstrused, cp);
	tstrused += i + 1;
	return (dp);
}
#endif
