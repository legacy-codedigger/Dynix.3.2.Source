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
static char rcsid[] = "$Id: fend.c,v 1.2 88/09/14 16:21:55 aho Exp $";
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

#include "tree_ty.h"

struct	nl *Fp;
int	pnumcnt;
/*
 * Funcend is called to finish a block by generating
 * the code for the statements.  It then looks for unresolved declarations
 * of labels, procedures and functions,  and cleans up the name list.
 * For the program, it checks the semantics of the program statement (yuchh).
 */
funcend(fp, bundle, endline)
struct nl *fp;
struct tnode *bundle;
int endline;
{
	register struct nl *p;
	register int i, b;
	auto int inp, out;
	auto struct tnode *blk;
	auto bool chkref;
	auto struct nl *iop;
	auto char *cp;
	extern int cntstat;
#if defined(PC)
	auto struct entry_exit_cookie	eecookie;
#else
	auto int var;
#endif PC

	cntstat = 0;
	/* yyoutline();
	 */
	if (program != NIL)
		line = program->value[3];
	blk = bundle->stmnt_blck.stmnt_list;
	if (fp == NIL) {
		cbn--;
#if defined(PTREE)
		nesting--;
#endif /* PTREE */
		return;
	}
#if defined(PC)
	/*
	 * put out the procedure entry code
	 */
	eecookie.nlp = fp;
	if (PROG == fp->class) {
		/*
		 * If there is a label declaration in the main routine
		 * then there may be a non-local goto to it that does
		 * not appear in this module. We have to assume that
		 * such a reference may occur and generate code to
		 * prepare for it.
		 */
		if ( parts[ cbn ] & LPRT ) {
			parts[ cbn ] |= ( NONLOCALVAR | NONLOCALGOTO );
		}
		codeformain();
		ftnno = fp->value[NL_ENTLOC];
		prog_prologue(&eecookie);
		stabline(bundle->stmnt_blck.line_no);
		stabfunc(fp, "program", bundle->stmnt_blck.line_no, 0L);
	} else {
		ftnno = fp->value[NL_ENTLOC];
		fp_prologue(&eecookie);
		stabline(bundle->stmnt_blck.line_no);
		stabfunc(fp, fp->symbol, bundle->stmnt_blck.line_no, (long)(cbn - 1));
		for ( p = fp->chain ; p != NIL ; p = p->chain ) {
			stabparam( p, p->value[ NL_OFFS ], (int) lwidth(p->type));
		}
		/*
		 * stab the function variable
		 */
		if (FUNC == fp->class) {
			p = fp->ptr[ NL_FVAR ];
			stablvar(p, p->value[ NL_OFFS ], (int) lwidth(p->type));
		}
		/*
		 * stab local variables
		 * rummage down hash chain links.
		 */
		for (i = 0 ; i <= 077 ; ++i) {
			for (p = disptab[ i ] ; p != NIL ; p = p->nl_next) {
				if (( p->nl_block & 037 ) != cbn) {
					break;
				}
				/*
				 *	stab locals (not parameters)
				 */
				if ( p->symbol != NIL ) {
					if ( p->class == VAR && p->value[ NL_OFFS ] < 0 ) {
						stablvar( p, p->value[ NL_OFFS ],
						(int) lwidth( p->type ) );
					} else if ( p->class == CONST ) {
						stabconst( p );
					}
				}
			}
		}
	}
	stablbrac( cbn );
	/*
	 * ask second pass to allocate known locals
	 */
	putlbracket(ftnno, &sizes[cbn]);
	fp_entrycode(&eecookie);
#endif PC
	if ( monflg ) {
		if ( fp->value[ NL_CNTR ] != 0 ) {
			inccnt( fp->value [ NL_CNTR ] );
		}
		inccnt( bodycnts[ fp->nl_block & 037 ] );
	}
	if (fp->class == PROG) {
		/*
		 * The glorious buffers option.
		 *          0 = don't buffer output
		 *          1 = line buffer output
		 *          2 = 512 byte buffer output
		 */
#if defined(PC)
		if ( opt( 'b' ) != 1 ) {
			putleaf(PCC_ICON, 0, 0, PCCM_ADDTYPE(PCCTM_FTN|PCCT_INT, PCCTM_PTR), "_BUFF");
			putleaf(PCC_ICON, opt( 'b' ), 0, PCCT_INT, (char *) 0 );
			putop( PCC_CALL, PCCT_INT );
			putdot( filename, line );
		}
#endif PC
		inp = 0;
		out = 0;
		for (p = fp->chain; p != NIL; p = p->chain) {
			if (pstrcmp(p->symbol, input->symbol) == 0) {
				inp++;
				continue;
			}
			if (pstrcmp(p->symbol, output->symbol) == 0) {
				out++;
				continue;
			}
			iop = lookup1(p->symbol);
			if (iop == NIL || bn != cbn) {
				error("File %s listed in program statement but not declared", p->symbol);
				continue;
			}
			if (iop->class != VAR) {
				error("File %s listed in program statement but declared as a %s", p->symbol, classes[iop->class]);
				continue;
			}
			if (iop->type == NIL)
				continue;
			if (iop->type->class != FILET) {
				error("File %s listed in program statement but defined as %s", p->symbol, nameof(iop->type));
				continue;
			}
#if defined(PC)
			putleaf( PCC_ICON, 0, 0, PCCM_ADDTYPE( PCCTM_FTN | PCCT_INT, PCCTM_PTR ), "_DEFNAME" );
			putLV( p->symbol, bn, iop->value[NL_OFFS], iop->extra_flags, p2type( iop ) );
			putCONG( p->symbol, strlen( p->symbol ), LREQ );
			putop( PCC_CM, PCCT_INT );
			putleaf( PCC_ICON, strlen( p->symbol ), 0, PCCT_INT, (char *) 0 );
			putop( PCC_CM, PCCT_INT );
			putleaf( PCC_ICON, text(iop->type) ? 0 : width(iop->type->type), 0, PCCT_INT, (char *) 0 );
			putop( PCC_CM, PCCT_INT );
			putop( PCC_CALL, PCCT_INT );
			putdot( filename, line );
#endif PC
		}
	}
	/*
	 * Process the prog/proc/func body
	 */
	noreach = FALSE;
	line = bundle->stmnt_blck.line_no;
	statlist(blk);

#if defined(PTREE)
	pDEF( PorFHeader[ nesting -- ] ).PorFBody =  tCopy( blk );
#endif PTREE

#if defined(PC)
	if ( fp->class == PROG && monflg ) {
		putleaf( PCC_ICON, 0, 0, PCCM_ADDTYPE( PCCTM_FTN | PCCT_INT, PCCTM_PTR ), "_PMFLUSH" );
		putleaf( PCC_ICON, cnts, 0, PCCT_INT, (char *) 0 );
		putleaf( PCC_ICON, pfcnt, 0, PCCT_INT, (char *) 0 );
		putop( PCC_CM, PCCT_INT );
		putLV( PCPCOUNT, 0, 0, NGLOBAL, PCCT_INT );
		putop( PCC_CM, PCCT_INT );
		putop( PCC_CALL, PCCT_INT );
		putdot( filename, line );
	}
#endif PC
	/*
	 * Clean up the symbol table displays and check for unresolves
	 */
	line = endline;
	if (fp->class == PROG && inp == 0 && (input->nl_flags & (NUSED|NMOD)) != 0) {
		recovered();
		error("Input is used but not defined in the program statement");
	}
	if (fp->class == PROG && out == 0 && (output->nl_flags & (NUSED|NMOD)) != 0) {
		recovered();
		error("Output is used but not defined in the program statement");
	}
	b = cbn;
	Fp = fp;
	chkref = (syneflg == errcnt[cbn] && opt('w') == 0)?TRUE:FALSE;
	for (i = 0; i <= 077; i++) {
		for (p = disptab[i]; p != NIL && (p->nl_block & 037) == b; p = p->nl_next) {
			/*
			 * Check for variables defined
			 * but not referenced 
			 */
			if (chkref && p->symbol != NIL) {
				switch (p->class) {
				case FIELD:
				/* If the corresponding record is unused,
				 * we shouldn't complain about the fields.
				 */
				default:
					if ((p->nl_flags & (NUSED|NMOD)) == 0) {
						warning();
						nerror("%s %s is neither used nor set", classes[p->class], p->symbol);
						break;
					}
					/*
					 * If a var parameter is either
					 * modified or used that is enough.
					 */
					if (p->class == REF)
						continue;
#if defined(PC)
					if (((p->nl_flags & NUSED) == 0) && ((p->extra_flags & NEXTERN) == 0)) {
						warning();
						nerror("%s %s is never used", classes[p->class], p->symbol);
						break;
					}
#endif PC
					if ((p->nl_flags & NMOD) == 0) {
						warning();
						nerror("%s %s is used but never set", classes[p->class], p->symbol);
						break;
					}
				case LABEL:
				case FVAR:
				case BADUSE:
					break;
				}
			}
			switch (p->class) {
			case BADUSE:
				cp = "s";
				/* This used to say ud_next
				 * that is not a member of nl so
				 * i changed it to nl_next,
				 * which may be wrong
				 */
				if (p->chain->nl_next == NIL)
					cp++;
				eholdnl();
				if (p->value[NL_KINDS] & ISUNDEF)
					nerror("%s undefined on line%s", p->symbol, cp);
				else
					nerror("%s improperly used on line%s", p->symbol, cp);
				pnumcnt = 10;
				pnums((struct udinfo *) p->chain);
				pchr('\n');
				break;

			case FUNC:
			case PROC:
#if defined(PC)
				if ((p->nl_flags & NFORWD) && ((p->extra_flags & NEXTERN) == 0))
					nerror("Unresolved forward declaration of %s %s", classes[p->class], p->symbol);
#endif PC
				break;

			case LABEL:
				if (p->nl_flags & NFORWD)
					nerror("label %s was declared but not defined", p->symbol);
				break;
			case FVAR:
				if ((p->nl_flags & NMOD) == 0)
					nerror("No assignment to the function variable");
				break;
			}
		}
		/*
		 * Pop this symbol table slot
		 */
		disptab[i] = p;
	}

#if defined(PC)
	fp_exitcode(&eecookie);
	stabrbrac(cbn);
	putrbracket(ftnno);
	fp_epilogue(&eecookie);
	if (fp->class != PROG) {
		fp_formalentry(&eecookie);
	}
	/* declare pcp counters, if any
	 */
	if ( monflg && fp->class == PROG ) {
		putprintf( "\t.data", 0 );
		aligndot(PCCT_INT);
		putprintf( "\t.comm\t", 1 );
		putprintf( PCPCOUNT, 1 );
		putprintf( ",%d", 0, ( cnts + 1 ) * sizeof (long) );
		putprintf( "\t.text", 0 );
	}
#endif PC
#if defined(DEBUG)
	dumpnl(fp->ptr[2], (int) fp->symbol);
#endif

	/*
	 * Restore the (virtual) name list position
	 */
	nlfree(fp->ptr[2]);
	/*
	 * Proc/func has been resolved
	 */
	fp->nl_flags &= ~NFORWD;
	/*
	 * Patch the beg of the proc/func to the proper variable size
	 */
	if (Fp == NIL)
		elineon();
	cbn--;
	if (inpflist(fp->symbol)) {
		opop('l');
	}
}

#if defined(PC)
/*
 * construct the long name of a function based on it's static nesting.
 * into a caller-supplied buffer (that should be about BUFSIZ big).
 */
sextname( buffer, name, level )
char	*buffer;
char	*name;
int	level;
{
	register char *starthere;
	register int i;

	starthere = &buffer[0];
	for ( i = 1 ; i < level ; i++ ) {
		sprintf( starthere, EXTFORMAT, enclosing[ i ] );
		starthere += strlen( enclosing[ i ] ) + 1;
	}
	sprintf( starthere, EXTFORMAT, name );
	starthere += strlen( name ) + 1;
	if ( starthere >= &buffer[ BUFSIZ ] ) {
		panic( "sextname" );
	}
}

#if defined(i386)

/* code for main constant inits and start up
 */
codeformain()
{
	putprintf("\t.text", 0 );
	putprintf("\t.align\t2\n\t.globl\t_main", 0 );
	putprintf("_main:", 0 );
	putprintf("\tpushl\t%%ebp", 0 );
	putprintf("\tmovl\t%%esp,%%ebp", 0 );
	if ( opt ( 't' ) ) {
		putprintf("\tpushl\t$1", 0 );
	} else {
		putprintf("\tpushl\t$0", 0 );
	}
	putprintf("\tcall\t_PCSTART", 0 );
	putprintf("\tpopl\t%%ecx", 0 );

	putprintf("\tmovl\t8(%%ebp),%%eax", 0 );
	putprintf("\tmovl\t%%eax,__argc", 0 );
	putprintf("\tmovl\t12(%%ebp),%%eax", 0 );
	putprintf("\tmovl\t%%eax,__argv", 0 );

	putprintf("\tcall\t_program", 0 );
	putprintf("\tpushl\t$0", 0 );
	putprintf("\tcall\t_PCEXIT", 0 );
	putprintf("\tleave", 0 );
	putprintf("\tret", 0 );
}

/*
 * prologue for the program.
 * different because it doesn't have formal entry point (not any more)
 */
prog_prologue(eecookiep)
struct entry_exit_cookie	*eecookiep;
{
	putprintf("\t.text", 0 );
	putprintf("\t.globl\t_program", 0 );
	putprintf("_program:", 0 );
	putprintf("\tpushl\t%%ebp", 0);
	putprintf("\tmovl\t%%esp,%%ebp", 0);
	putprintf("\tsubl\t$%s%d,%%esp", 0, FRAME_SIZE_LABEL, ftnno);

	putprintf( "\tpushl\t%%esi", 0 );
	putprintf( "\tpushl\t%%edi", 0 );
	putprintf( "\tpushl\t%%ebx", 0 );
}

/*
 * any functions prologue
 */
fp_prologue(eecookiep)
struct entry_exit_cookie *eecookiep;
{
	sextname( eecookiep->extname, eecookiep->nlp->symbol, cbn - 1 );
	putprintf("\t.text", 0 );
	putprintf("\t.globl\t%s", 0, eecookiep->extname );
	putprintf("%s:", 0, eecookiep->extname);

	putprintf("\tpushl\t%%ebp", 0);
	putprintf("\tmovl\t%%esp,%%ebp", 0);

	putprintf("\tsubl\t$%s%d,%%esp", 0, FRAME_SIZE_LABEL, ftnno);
	putprintf("\tpushl\t%%esi", 0);
	putprintf("\tpushl\t%%edi", 0);
	putprintf("\tpushl\t%%ebx", 0);
}

/*
 * Code before any user code. Or code that is machine dependent.
 */
fp_entrycode(eecookiep)
struct entry_exit_cookie	*eecookiep;
{
	int ftnno = eecookiep->nlp->value[NL_ENTLOC];
	int setjmp0 = (int) getlab();
	/*
	 * top of code;  destination of jump from formal entry code.
	 */
	if ( profflag ) {
		int	proflabel = (int) getlab();
		/*
		 * call mcount for profiling
		 */
		putprintf( "\tmovl	$", 1 );
		putprintf( PREFIXFORMAT, 1, (int) LABELPREFIX, proflabel );
		putprintf( ",%%eax", 0 );
		putprintf( "\tcall\tmcount", 0 );
		putprintf( "\t.data", 0 );
		(void) putlab( (char *) proflabel );
		putprintf( "\t.long\t0", 0 );
		putprintf( "\t.text", 0 );
	}
	/*
	 *	if there are nested procedures that access our variables
	 *	we must save the display.
	 */
	if ( parts[ cbn ] & NONLOCALVAR ) {
		/*
		 * save old display 
		 */
		putprintf( "\tmovl\t%s+%d,%s\n\tmovl\t%s,%d(%%ebp)", 0, DISPLAYNAME, cbn*sizeof(savedfp_t), "%eax", "%eax", DSAVEOFFSET);
		/*
		 * set up new display by saving %ebp in appropriate
		 * slot in display structure.
		 */
		putprintf( "\tmovl\t%%ebp,%s+%d", 0, DISPLAYNAME, cbn*sizeof(savedfp_t));
	}
	/*
	 * zero local variables if checking (-C) is on
	 * by calling blkclr( bytes of locals, starting local address ); ZZ: backwards here (dsg)
	 */
	if ( opt( 't' ) && ( -sizes[ cbn ].om_max ) > DPOFF1 ) {
		putleaf( PCC_ICON, 0, 0, PCCM_ADDTYPE( PCCTM_FTN | PCCT_INT, PCCTM_PTR ), "_bzero" );
		putLV((char *) 0, cbn, (int) sizes[ cbn ].om_max, NLOCAL, PCCT_CHAR );
		putleaf( PCC_ICON,  (int) (( -sizes[ cbn ].om_max ) - DPOFF1) , 0, PCCT_INT,(char *) 0 );
		putop( PCC_CM, PCCT_INT );
		putop( PCC_CALL, PCCT_INT );
		putdot( filename, line );
	}
	/*
	 * set up goto vector if non-local goto to this frame
	 */
	if ( parts[ cbn ] & NONLOCALGOTO ) {
		/*
		 * on non-local goto, setjmp returns with address to
		 * be branched to.
		 */
		putleaf( PCC_ICON, 0, 0, PCCM_ADDTYPE( PCCTM_FTN | PCCT_INT, PCCTM_PTR ), "_setjmp" );
		putLV( (char *) 0, cbn, GOTOENVOFFSET, NLOCAL, PCCTM_PTR|PCCT_STRTY );
		putop( PCC_CALL, PCCT_INT );
		putdot( filename, line );
		putprintf("\ttestl\t%s,%s", 0, "%eax", "%eax");
		putprintf("\tje\t%s%d", 0, LABELPREFIX, setjmp0);
		putprintf( "\tjmp\t*%s", 0, "%eax");
		putdot( filename, line );
		(void) putlab(setjmp0);
	}
}

/*
 *	if there were file variables declared at this level
 *	call PCLOSE( ap ) to clean them up.
 */
fp_exitcode(eecookiep)
struct entry_exit_cookie	*eecookiep;
{
	if ( dfiles[ cbn ] ) {
		putleaf( PCC_ICON, 0, 0, PCCM_ADDTYPE( PCCTM_FTN | PCCT_INT, PCCTM_PTR ), "_PCLOSE" );
		putleaf( PCC_REG, 0, P2FP, PCCM_ADDTYPE( PCCT_CHAR, PCCTM_PTR ), (char *) 0 );
		putop( PCC_CALL, PCCT_INT );
		putdot( filename, line );
	}
	/*
	 * if this is a function, the function variable is the return value.
	 * if it's a scalar valued function, return scalar,
	 * else, return a pointer to the structure value.
	 */
	if ( eecookiep->nlp->class == FUNC ) {
		struct nl *fvar = eecookiep->nlp->ptr[ NL_FVAR ];
		long fvartype = p2type( fvar->type );
		long label;
		char labelname[ BUFSIZ ];

		switch ( classify( fvar->type ) ) {
		case TBOOL:
		case TCHAR:
		case TINT:
		case TSCAL:
		case TDOUBLE:
		case TPTR:
			putRV(fvar->symbol, (fvar->nl_block) & 037, fvar->value[NL_OFFS], fvar->extra_flags, (int)fvartype);
			putop( PCC_FORCE, (int) fvartype );
			break;
		default:
			label = (int) getlab();
			putprintf( "\t.data", 0 );
			aligndot(A_STRUCT);
			sprintf( labelname, PREFIXFORMAT, LABELPREFIX, label );
			putprintf( "\t.lcomm	%s,%d", 0, labelname, lwidth(fvar->type));
			putprintf( "\t.text", 0 );
			putleaf( PCC_NAME, 0, 0, (int) fvartype, labelname );
			putLV( fvar->symbol, ( fvar->nl_block ) & 037, fvar->value[ NL_OFFS ], fvar->extra_flags, (int) fvartype ); putstrop( PCC_STASG, (int) PCCM_ADDTYPE(fvartype, PCCTM_PTR), (int) lwidth( fvar->type ), align( fvar->type ) );
			putdot( filename, line );
			putleaf( PCC_ICON, 0, 0, (int) PCCM_ADDTYPE(fvartype, PCCTM_PTR), labelname );
			putop( PCC_FORCE, (int) PCCM_ADDTYPE(fvartype, PCCTM_PTR) );
			break;
		}
		putdot( filename, line );
	}
	/*
	 *	if there are nested procedures we must save the display.
	 */
	if ( parts[ cbn ] & NONLOCALVAR ) {
		/*
		 * restore old display entry from save area
		 */
		putprintf( "\tmovl\t%s+%d,%s\n\tmovl\t%s,%d(%%ebp)", 0, DISPLAYNAME, cbn*sizeof(savedfp_t), "%eax", "%eax", DSAVEOFFSET);
	}
	putprintf( "\tpopl\t%%ebx", 0 );
	putprintf( "\tpopl\t%%edi", 0 );
	putprintf( "\tpopl\t%%esi", 0 );
}

fp_epilogue(eecookiep)
struct entry_exit_cookie	*eecookiep;
{
	stabline(line);
	putprintf("\tleave", 0);
	putprintf("\tret", 0 );
}

fp_formalentry(eecookiep)
struct entry_exit_cookie	*eecookiep;
{
	;
}
#endif /* i386	*/
#endif /* PC	*/
