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
static char rcsid[] = "$Id: fort.c,v 2.8 88/09/02 11:46:09 ksb Exp $";
#endif lint

/*
 *	Berkeley Pascal Compiler	(fort.c)
 */

#include "pass2.h"
#include "fort.h"

/* masks for unpacking longs */

#if !defined(FOP)
#define FOP(x) ((int)((x)&0377))
#endif

#if !defined(VAL)
#define VAL(x) ((int)(((x)>>8)&0377))
#endif

#if !defined(REST)
#define REST(x) ((int)(((x)>>16)&0177777))
#endif

FILE *lrd;  /* for default reading routines */

#if !defined(NOLNREAD)
#if defined(FLEXNAMES)
char *
lnread()
{
	auto char buf[BUFSIZ];
	register char *cp = buf;
	register char *limit = &buf[BUFSIZ];

	for (;;) {
		if (fread(cp, sizeof (long), 1, lrd) !=  1)
			cerror("intermediate file read error");
		cp += sizeof (long);
		if (cp[-1] == 0)
			break;
		if (cp >= limit)
			cerror("lnread overran string buffer");
	}
	return tstr(buf);
}
#endif
#endif NOLNREAD

#if !defined(NOLREAD)
long
lread()
{
	static long x;

	if( fread( (char *) &x, 4, 1, lrd ) <= 0 )
		cerror( "intermediate file read error" );
	return x;
}
#endif

#if !defined(NOLOPEN)
/* if null, opens the standard input */
lopen(s)
char *s;
{
	if (0 == s || '\000' == *s) {
		lrd = stdin;
	} else if (NULL == (lrd = fopen( s, "r" ))) {
		cerror( "cannot open intermediate file %s", s );
	}
}
#endif

#if !defined(NOLCREAD)
lcread( cp, n )
char *cp;
{
	if( n > 0 ) {
		if( fread( cp, 4, n, lrd ) != n )
			cerror( "intermediate file read error" );
	}
}
#endif

#if !defined(NOLCCOPY)
lccopy( n )
register n;
{
	register int i;
	static char fbuf[BUFSIZ];

	if( n <= 0 ) {
		return;
	}
	if( n > BUFSIZ/4 )
		cerror( "lccopy asked to copy too much" );
	if( fread( fbuf, 4, n, lrd ) != n )
		cerror( "intermediate file read error" );
	for( i=4*n; fbuf[i-1] == '\0' && i>0; --i ) {
		/* VOID */
	}
	if( i ) {
		if( fwrite( fbuf, 1, i, stdout ) != i )
			cerror( "output file error" );
	}
}
#endif

/* stack for reading nodes in postfix form */

#define NSTACKSZ 250

NODE *fstack[NSTACKSZ];
NODE **fsp;  /* points to next free position on the stack */

unsigned int offsz;
unsigned int caloff();

mainp2( argc, argv )
char **argv;
{
	int files;
	register long x;
	register NODE *p;

	offsz = caloff();
	files = p2init( argc, argv );
	tinit();

	if (files) {
		while( files < argc && argv[files][0] == '-' ) {
			++files;
		}
		if( files > argc )
			return( nerrors );
		lopen( argv[files] );
	} else {
		lopen( "" );
	}

	fsp = fstack;

	for(;;) {
		/* read nodes, and go to work... */
		x = lread();
		switch( (int)FOP(x) ) {  /* switch on opcode */

		case 0:
#if defined(KBUG)
			if (0 != kdebug)
				fprintf(kdebug, "null opcode ignored\n" );
#endif
			continue;
		case FTEXT:
			lccopy( VAL(x) );
			putc('\n', stdout);
			continue;
		case FLBRAC:
			tmpoff = baseoff = lread();
			maxtreg = VAL(x);
			/* maxoff at end of ftn is max of autos and
			 * temps over all blocks in the function
			 */
			if( ftnno != REST(x) ) { /* beginning of function */
				maxoff = baseoff;
				ftnno = REST(x);
				maxtemp = 0;
			} else if (baseoff > maxoff) {
				maxoff = baseoff;
			}
			setregs();
			continue;
		case FRBRAC:
			SETOFF(maxoff, ALSTACK);
			eobl2();
			continue;
		case FEOF:
			return nerrors;
		case FSWITCH:
			uerror( "switch not yet done" );
			for( x=VAL(x); x>0; --x ) lread();
			continue;
		case ICON:
			p = talloc();
			p->in.op = ICON;
			p->in.type = REST(x);
			p->tn.rval = 0;
			p->tn.lval = lread();
			if( VAL(x) ) {
#if !defined(FLEXNAMES)
				lcread( p->in.name, 2 );
#else
				p->in.name = lnread();
#endif
			} else {
#if !defined(FLEXNAMES)
				p->in.name[0] = '\0';
#else
				p->in.name = "";
#endif
			}

		bump:
			p->in.su = 0;
			p->in.rall = 0;
			*fsp++ = p;
			if( fsp >= &fstack[NSTACKSZ] ) 
				uerror( "expression depth exceeded" );
			continue;
		case NAME:
			p = talloc();
			p->in.op = NAME;
			p->in.type = REST(x);
			p->tn.rval = 0;
			if( VAL(x) ) p->tn.lval = lread();
			else p->tn.lval = 0;
#if !defined(FLEXNAMES)
			lcread( p->in.name, 2 );
#else
			p->in.name = lnread();
#endif
			goto bump;
		case OREG:
			p = talloc();
			p->in.op = OREG;
			p->in.type = REST(x);
			p->tn.rval = VAL(x);
			p->tn.lval = lread();
#if !defined(FLEXNAMES)
			lcread( p->in.name, 2 );
#else
			p->in.name = lnread();
#endif
			goto bump;
		case REG:
			p = talloc();
			p->in.op = REG;
			p->in.type = REST(x);
			p->tn.rval = VAL(x);
			p->tn.lval = 0;
#if !defined(FLEXNAMES)
			p->in.name[0] = '\000';
#else
			p->in.name = "";
#endif
			rbusy(p->tn.rval, p->in.type, p);
			goto bump;
		case FEXPR:		/* turn the crank */
			lineno = REST(x);
			if (VAL(x))
				lcread( filename, VAL(x) );
			if (fsp == fstack) /* filename only */
				continue; 
			if( --fsp != fstack )
				cerror("expression poorly formed");
			if( lflag )
				lineid( lineno, filename );
			tmpoff = baseoff;
			p = fstack[0];
#if defined(KBUG)
			if( edebug )
				fwalk( p, eprint, 0 );
#endif
			nrecur = 0;
			canon(p);
			rallo(p, RA_NONE);
			(void) order(p, SH_FOREFF);
			reclaim(p, RC_NULL, 0);
			tfree1(p);
			allchk();
			tcheck();
			continue;
		case FLABEL:
			if( VAL(x) ) {
				tlabel();
			} else {
				label( (int) REST(x) );
			}
			continue;
		case GOTO:
			if (VAL(x)) {  /* unconditional branch */
				cbgen(0, (LABEL) REST(x), 'I');
				continue;
			}
			/* otherwise, treat as unary */
			goto def;
		case STASG:
		case STARG:
		case STCALL:
		case UNARY STCALL:
			/*
			 * size and alignment come from next long words
			 */
			p = talloc();
			p->stn.stsize = lread();
			p->stn.stalign = lread();
			goto defa;
		default:
		def:
			p = talloc();
		defa:
			p->in.op = FOP(x);
			p->in.type = REST(x);

			switch( optype( p->in.op ) ) {
			case BITYPE:
				p->in.right = *--fsp;
				p->in.left = *--fsp;
				goto bump;
			case UTYPE:
				p->in.left = *--fsp;
				p->tn.rval = 0;
				goto bump;
			case LTYPE:
				uerror( "illegal leaf node: %d", p->in.op );
				exit( 1 );
			}
		}
	}
}
