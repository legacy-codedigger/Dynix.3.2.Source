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
static char rcsid[] = "$Id: fdec.c,v 1.1 88/09/02 11:47:59 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"
#include "tree.h"
#include "opcode.h"
#include "objfmt.h"
#include "align.h"
#include "tmps.h"

/*
 * this array keeps the pxp counters associated with
 * functions and procedures, so that they can be output
 * when their bodies are encountered
 */
int	bodycnts[ DSPLYSZ ];

#if defined(PC)
#include "pc.h"
#include <pcc.h>
#endif PC

funcfwd(fp)
	struct nl *fp;
{

	    /*
	     *	save the counter for this function
	     */
	if ( monflg ) {
	    fp -> value[ NL_CNTR ] = bodycnts[ cbn ];
	}
}

/*
 * Funcext marks the procedure or
 * function external in the symbol
 * table. Funcext should only be
 * called if PC, and is an error
 * otherwise.
 */

struct nl *
funcext(fp)
	struct nl *fp;
{

#if defined(PC)
	    /*
	     *	save the counter for this function
	     */
	if ( monflg ) {
	    fp -> value[ NL_CNTR ] = bodycnts[ cbn ];
	}
 	if (opt('s')) {
		standard();
		error("External procedures and functions are not standard");
	} else {
		if (cbn == 1) {
			fp->extra_flags |= NEXTERN;
			stabefunc( fp -> symbol , fp -> class , line );
		}
		else
			error("External procedures and functions can only be declared at the outermost level.");
	}
#endif PC

	return(fp);
}

/*
 * Funcbody is called
 * when the actual (resolved)
 * declaration of a procedure is
 * encountered. It puts the names
 * of the (function) and parameters
 * into the symbol table.
 */
struct nl *
funcbody(fp)
	struct nl *fp;
{
	register struct nl *q;

	cbn++;
	if (cbn >= DSPLYSZ) {
		error("Too many levels of function/procedure nesting");
		pexit(ERRS);
	}
	tmpinit(cbn);
	gotos[cbn] = NIL;
	errcnt[cbn] = syneflg;
	parts[ cbn ] = NIL;
	dfiles[ cbn ] = FALSE;
	if (fp == NIL)
		return (NIL);
	/*
	 * Save the virtual name
	 * list stack pointer so
	 * the space can be freed
	 * later (funcend).
	 */
	fp->ptr[2] = nlp;
	if (fp->class != PROG) {
		for (q = fp->chain; q != NIL; q = q->chain) {
			(void) enter(q);
#if defined(PC)
			    q -> extra_flags |= NPARAM;
#endif PC
		}
	}
	if (fp->class == FUNC) {
		/*
		 * For functions, enter the fvar
		 */
		(void) enter(fp->ptr[NL_FVAR]);
#if defined(PC)
		    q = fp -> ptr[ NL_FVAR ];
		    if (q -> type != NIL ) {
			sizes[cbn].curtmps.om_off = q -> value[NL_OFFS];
			sizes[cbn].om_max = q -> value[NL_OFFS];
		    }
#endif PC
	}
#if defined(PTREE)
		/*
		 *	pick up the pointer to porf declaration
		 */
	    PorFHeader[ ++nesting ] = fp -> inTree;
#endif PTREE
	return (fp);
}

/*
 * Segend is called to check for
 * unresolved variables, funcs and
 * procs, and deliver unresolved and
 * baduse error diagnostics at the
 * end of a routine segment (a separately
 * compiled segment that is not the 
 * main program) for PC. This
 * routine should only be called
 * by PC (not standard).
 */
 segend()
 {
#if defined(PC)
	register struct nl *p;
	register int i,b;
	char *cp;

	if ( monflg ) {
	    error("Only the module containing the \"program\" statement");
	    cerror("can be profiled with ``pxp''.\n");
	}
	if (opt('s')) {
		standard();
		error("Separately compiled routine segments are not standard.");
	} else {
		b = cbn;
		for (i=0; i<077; i++) {
			for (p = disptab[i]; p != NIL && (p->nl_block & 037) == b; p = p->nl_next) {
			switch (p->class) {
				case BADUSE:
					cp = "s";
					if (((struct udinfo *) (p->chain))->ud_next == NIL)
						cp++;
					eholdnl();
					if (p->value[NL_KINDS] & ISUNDEF)
						nerror("%s undefined on line%s", p->symbol, cp);
					else
						nerror("%s improperly used on line%s", p->symbol, cp);
					pnumcnt = 10;
					pnums((struct udinfo *) (p->chain));
					pchr('\n');
					break;
				
				case FUNC:
				case PROC:
					if ((p->nl_flags & NFORWD) &&
					    ((p->extra_flags & NEXTERN) == 0))
						nerror("Unresolved forward declaration of %s %s", classes[p->class], p->symbol);
					break;

				case FVAR:
					if (((p->nl_flags & NMOD) == 0) &&
					    ((p->chain->extra_flags & NEXTERN) == 0))
						nerror("No assignment to the function variable");
					break;
			    }
			   }
			   disptab[i] = p;
		    }
	}
#endif PC

}


/*
 * Level1 does level one processing for
 * separately compiled routine segments
 */
level1()
{

#if defined(PC)
	    if (opt('s')) {
		    standard();
		    error("Missing program statement");
	    }
#endif PC

	cbn++;
	tmpinit(cbn);
	gotos[cbn] = NIL;
	errcnt[cbn] = syneflg;
	parts[ cbn ] = NIL;
	dfiles[ cbn ] = FALSE;
	progseen = TRUE;
}



pnums(p)
	struct udinfo *p;
{

	if (p->ud_next != NIL)
		pnums(p->ud_next);
	if (pnumcnt == 0) {
		printf("\n\t");
		pnumcnt = 20;
	}
	pnumcnt--;
	printf(" %d", p->ud_line);
}

/*VARARGS*/
nerror(a1, a2, a3)
    char *a1,*a2,*a3;
{

	if (Fp != NIL) {
		yySsync();
		if (opt('l'))
			yyoutline();
		yysetfile(filename);
		printf("In %s %s:\n", classes[Fp->class], Fp->symbol);
		Fp = NIL;
		elineoff();
	}
	error(a1, a2, a3);
}
