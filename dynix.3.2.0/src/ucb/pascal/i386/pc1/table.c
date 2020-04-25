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
	"$Id: table.c,v 2.20 88/12/08 15:44:18 ksb Exp $";
#endif lint

/*
 *	Almost Berkeley Pascal Compiler	(table.c)
 */
#include "pass2.h"

char acHelpOp[] = "Help, I am out of this opcode!";
#define NOMORE	{FREE, FREE, FREE, FREE, FREE, FREE, FREE, FREE, acHelpOp}

/*
 * table of templates for tree matching
 */

/*
 * Here, the right operand is only used for type matching
 * information.
 */
static OPCODE pconvOp[] = {
	{PCONV,
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY,			TY_POINT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA,%1A\n"},

	{PCONV,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE|TY_ANY_WORD,
		SH_ANY,			TY_POINT,
		NEED0,
		RC_LEFT,
			"\tmovz%LWl\t%LA,%LA\n"},

	{PCONV,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_BYTE|TY_ANY_WORD,
		SH_ANY,			TY_POINT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%LWl\t%LA,%1A\n"},

	{PCONV,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_INT|TY_UNSIGNED,
		SH_ANY,			TY_POINT,
		NEED0,
		RC_LEFT,
			"\t# convert int/unsigned to pointer (%LA)\n"},

	{PCONV,
		SH_TAREG,
		SH_ANY_ADST,		TY_INT|TY_UNSIGNED,
		SH_ANY,			TY_POINT,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmovl\t%LA,%1A\n"},

	NOMORE
};

static OPCODE initOp[] = {
	{INIT,	/*ZZ: %C bright enough for this?*/
		SH_FOREFF,
		SH_LCON,		TY_DOUBLE,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_NOP,
			"\t.double\t%C\n"},

	{INIT,
		SH_FOREFF,
		SH_LCON,		TY_ANY_LONG,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_NOP,
			"\t.long\t%C\n"},

	{INIT,
		SH_FOREFF,
		SH_WCON,		TY_ANY,
		SH_NONE,		TY_SHORT|TY_USHORT,
		NEED0,
		RC_NOP,
			"\t.word\t%C\n"},

	{INIT,
		SH_FOREFF,
		SH_BCON,		TY_ANY,
		SH_ANY,			TY_CHAR|TY_UCHAR,
		NEED0,
		RC_NOP,
			"\t.byte\t%C\n"},

	NOMORE
};

static OPCODE gotoOp[] = {
	{GOTO,
		SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_NOP,
			"\tjmp\t%LA\n"},

	{GOTO,
		SH_FOREFF,
		SH_OAREG|SH_ANY_AREG,	TY_ANY_LONG,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_NOP,
			"\tjmp\t*%LA\n"},

	NOMORE
};

static OPCODE ucallOp[] = {
	{UNARY CALL,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tcall\t%LA\n"},

	{UNARY CALL,
		SH_TBREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tcall\t%LA\n"},

	{UNARY CALL,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tcall\t*%LA\n"},

	{UNARY CALL,
		SH_TBREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_BREG), RC_KEEP1,
			"\tcall\t*%LA\n"},

	NOMORE
};

static OPCODE callOp[] = {
	{CALL,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tcall\t%LA\n"},

	{CALL,
		SH_TBREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tcall\t%LA\n"},

	{CALL,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tcall\t*%LA\n"},

	{CALL,
		SH_TBREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_BREG), RC_KEEP1,
			"\tcall\t*%LA\n"},

	NOMORE
};

static OPCODE forceOp[] = {
	{FORCE,
		SH_ANY_AREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tmovl\t%LA,%%eax\n"},

	{FORCE,
		SH_ANY_AREG|SH_FOREFF,
		SH_EAX,			TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_NONE,		0,
		NEED0,
		RC_LEFT,
			""},

	{FORCE,
		SH_ANY_AREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tmovl\t$%LA,%%eax\n"},

	{FORCE,
		SH_ANY_AREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_NONE,		0,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tmovl\t%LA,%%eax\n"},

	{FORCE,
		SH_ANY_BREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_NONE,		0,
		NEED0,
		RC_LEFT,
			""},

	{FORCE,
		SH_ANY_BREG|SH_FOREFF,
		SH_ANY_BDST,		TY_DOUBLE,
		SH_NONE,		0,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n"},


	NOMORE
};

static OPCODE assignOp[] = {
/*
 * this code meets the invariants
 * 	(1) the return type is only guarenteed to be yours if it is TAREG
 *	(2) we fill registers we allocate or keep to the full shape
 *
 * It does not do type promotions other than what is required to meet
 * the above.
 */
	{ASSIGN,
		SH_ANY_AREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tmov%LW\t$%RA%L&,%LA\n"},

	{ASSIGN,
		SH_ANY_AREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_CHAR|TY_SHORT,
		NEED0,
		RC_LEFT,
			"\tmovs%RWl\t%RA,%LA\n"},

	{ASSIGN,
		SH_ANY_AREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_UCHAR|TY_USHORT,
		NEED0,
		RC_LEFT,
			"\tmovz%RWl\t%RA,%LA\n"},

	{ASSIGN,
		SH_ANY_AREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tmovl\t%RA,%LA\n"},

	{ASSIGN,
		SH_ANY_AREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_FIXED,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tmov%LW\t$%RA%L&,%LA\n"},

	{ASSIGN,
		SH_TAREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_FIXED,
		SH_TAREG,		TY_ANY_FIXED,
		NEED0,
		RC_LEFT,
			"\tmov%LW\t%RA,%LA\n"},

	{ASSIGN,
		SH_ANY_AREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_FIXED,
		SH_ANY_AREG,		TY_ANY_FIXED,
		NEED0,
		RC_LEFT,
			"\tmov%LW\t%RA,%LA\n"},

	{ASSIGN,
		SH_TAREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_CHAR|TY_SHORT,
		NEED0,
		RC_LEFT,
			"\t%RP\t%RA,%LA\n"},

	{ASSIGN,
		SH_TAREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_CHAR|TY_SHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\t%RP\t%RA,%1A\n\tmov%LW\t%1A,%LA\n"},

	{ASSIGN,
		SH_TAREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_UCHAR|TY_USHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%LWl\t%RA,%1A\n\tmov%LW\t%1A,%LA\n"},

	{ASSIGN,
		SH_TAREG|SH_FOREFF,
		SH_ANY_MEM,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%RA,%1A\n\tmov%LW\t%1A,%LA\n"},

	/* floating point	*/
	{ASSIGN,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_LEFT,
			"%!"},

	{ASSIGN,
		SH_TBREG,
		SH_TBREG,		TY_DOUBLE,
		SH_ANY_BDST,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%RA\n"},

	{ASSIGN,
		SH_ANY_MEM|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_LEFT,
			"\tfstp\t%LA\n"},

	{ASSIGN,
		SH_ANY_MEM|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_BREG,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_LEFT,
			"\tfldl\t%RA\n\tfstpl\t%LA\n"},

	{ASSIGN,
		SH_ANY_MEM|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmovl\t%RA,%1A\n\tmovl\t%1A,%LA\n\tmovl\t4+%RA,%1A\n\tmovl\t%1A,4+%LA\n"},

	{ASSIGN,
		SH_ANY_MEM|SH_FOREFF,
		SH_ANY_BDST,		TY_DOUBLE,
		SH_ANY_BDST,		TY_DOUBLE,
		NEED1(N_AREG),
		RC_LEFT,
			"\tfldl\t%RA\n\tfstpl\t%LA\n"},

	NOMORE
};

static OPCODE regOp[] = {
	{REG,
		SH_TAREG,
		SH_ANY,			TY_ANY,
		SH_ANY_CON,		TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%RA,%1A\n"},

	{REG,
		SH_TAREG|SH_FOREFF,
		SH_ANY,			TY_ANY,
		SH_TAREG,		TY_ANY,
		NEED0,
		RC_RIGHT,
			"\t# load to areg (already ours)\n"},

	{REG,
		SH_TAREG,
		SH_ANY,			TY_ANY,
		SH_ANY_AREG,		TY_CHAR|TY_SHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovs%RWl\t%RA,%1A\n"},

	{REG,
		SH_AREG,
		SH_ANY,			TY_ANY,
		SH_ANY_AREG,		TY_UCHAR|TY_USHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%RWl\t%RA,%1A\n"},

	{REG,
		SH_AREG,
		SH_ANY,			TY_ANY,
		SH_ANY_AREG,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%RA,%1A\n"},

	{REG,
		SH_TBREG,
		SH_ANY,			TY_ANY,
		SH_TBREG,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\t# load to breg (already ours)\n"},

	{REG,
		SH_TBREG,
		SH_ANY,			TY_ANY,
		SH_ANY_BDST,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfld\t%RA\n"},

	NOMORE
};

static OPCODE notOp[] = {
	{NOT,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_LEFT,
			"\tcmp%LW\t$0,%LA\n\tsete\t%LA\n"},

	{NOT,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_FIXED,
		SH_NONE,		TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tcmp%LW\t$0,%LA\n\tsete\t%1A\n"},

	{NOT,
		SH_BREG,
		SH_BREG,		TY_DOUBLE,
		SH_NONE,		TY_ANY,
		NEED1(N_BREG),
		RC_KEEP1,
			"#ZZZ push 1, reverse subtract?\n"},

	NOMORE
};

static OPCODE uminusOp[] = {
#if defined(CMATH)
	{UNARY MINUS,
		SH_ANY_CON|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_NONE,		TY_ANY,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\tLC%1U,[0-%LA]%L&\n"},
#endif

	{UNARY MINUS,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_SIGNED,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_LEFT,
			"\tnegl\t%LA\n"},

	{UNARY MINUS,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_UNSIGNED,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_LEFT,
			"\tneg%LW\t%LA\n"},

	{UNARY MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_CHAR|TY_SHORT,
		SH_NONE,		TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovs%LWl\t%LA,%1A\n\tnegl\t%1A\n"},

	{UNARY MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_INT|TY_LONG,
		SH_NONE,		TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tnegl\t%1A\n"},

	{UNARY MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_UCHAR|TY_USHORT,
		SH_NONE,		TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%LWl\t%LA,%1A\n\tneg%LW\t%1A\n"},

	{UNARY MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_UNSIGNED|TY_ULONG,
		SH_NONE,		TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tneg%LW\t%1A\n"},

	{UNARY MINUS,
		SH_TBREG,
		SH_TBREG,		TY_DOUBLE,
		SH_NONE,		TY_ANY,
		NEED0,
		RC_LEFT,
			"\tfchs\n"},

	{UNARY MINUS,
		SH_TBREG,
		SH_ANY_BDST,		TY_DOUBLE,
		SH_NONE,		TY_ANY,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n\tfchs\n"},

	NOMORE
};

static OPCODE complOp[] = {
#if defined(CMATH)
	{COMPL,
		SH_ANY_CON|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY,			TY_ANY,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\tLC%1U,[!%LA]%L&\n"},
#endif

	{COMPL,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_UNSIGNED|TY_INT|TY_LONG,
		SH_ANY,			TY_ANY,
		NEED0,
		RC_LEFT,
			"\tnot%LW\t%LA\n"},

	{COMPL,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_CHAR|TY_SHORT,
		SH_ANY,			TY_ANY,
		NEED0,
		RC_LEFT,
			"\tnot%LW\t%LA\n\tmovs%LWl\t%LA,%LA\n"},

	{COMPL,
		SH_TAREG,
		SH_ANY_ADST,		TY_UCHAR|TY_USHORT,
		SH_ANY,			TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%LWl\t%LA,%1A\n\tnot%LW\t%1A\n"},

	{COMPL,
		SH_TAREG,
		SH_ANY_ADST,		TY_UNSIGNED|TY_ULONG,
		SH_ANY,			TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tnotl\t%1A\n"},

	{COMPL,
		SH_TAREG,
		SH_ANY_ADST,		TY_CHAR|TY_SHORT,
		SH_ANY,			TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tnot%LW\t%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{COMPL,
		SH_TAREG,
		SH_ANY_ADST,		TY_INT|TY_LONG,
		SH_ANY,			TY_ANY,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tnotl\t%1A\n"},

	NOMORE
};

static OPCODE orOp[] = {
#if defined(CMATH)
	{OR,
		SH_ANY_CON|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\t%1A,[%LA|%RA]\n"},
#endif

	{OR,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_TAREG,		TY_ANY_FIXED,
		NEED0,
		RC_RIGHT,
			"\tor%RW\t$%LA,%RA\n"},

	{OR,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tor%LW\t$%RA,%LA\n"},

	{OR,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%RWl\t%RA,%1A\n\torl\t$%LA,%1A\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%LWl\t%LA,%1A\n\torl\t$%RA,%1A\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%RA,%1A\n\torl\t$%LA,%1A\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\torl\t$%RA,%1A\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\torb\t%RA,%LA\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_TAREG,		TY_ANY_BYTE,
		NEED0,
		RC_RIGHT,
			"\torb\t%LA,%RA\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_TAREG,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED0,
		RC_LEFT,
			"\torw\t%RA,%LA\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_TAREG,		TY_ANY_WORD,
		NEED0,
		RC_RIGHT,
			"\torw\t%LA,%RA\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_TAREG,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\torl\t%RA,%LA\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_TAREG,		TY_ANY_LONG,
		NEED0,
		RC_RIGHT,
			"\torl\t%LA,%RA\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovzbl\t%LA,%1A\n\torb\t%RA,%1A\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovzwl\t%LA,%1A\n\torw\t%RA,%1A\n"},

	{OR,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\torl\t%RA,%1A\n"},

	NOMORE
};

static OPCODE andOp[] = {
#if defined(CMATH)
	{AND,
		SH_ANY_CON|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\t%1A,[%LA&%RA]\n"},
#endif

	{AND,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_TAREG,		TY_ANY_FIXED,
		NEED0,
		RC_RIGHT,
			"\tand%RW\t$%LA,%RA\n"},

	{AND,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tand%LW\t$%RA,%LA\n"},

	{AND,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%RWl\t%RA,%1A\n\tandl\t$%LA,%1A\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovz%LWl\t%LA,%1A\n\tandl\t$%RA,%1A\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%RA,%1A\n\tandl\t$%LA,%1A\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tandl\t$%RA,%1A\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tandb\t%RA,%LA\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_TAREG,		TY_ANY_BYTE,
		NEED0,
		RC_RIGHT,
			"\tandb\t%LA,%RA\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_TAREG,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED0,
		RC_LEFT,
			"\tandw\t%RA,%LA\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_TAREG,		TY_ANY_WORD,
		NEED0,
		RC_RIGHT,
			"\tandw\t%LA,%RA\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_TAREG,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tandl\t%RA,%LA\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_TAREG,		TY_ANY_LONG,
		NEED0,
		RC_RIGHT,
			"\tandl\t%LA,%RA\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovzbl\t%LA,%1A\n\tandb\t%RA,%1A\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovzwl\t%LA,%1A\n\tandw\t%RA,%1A\n"},

	{AND,
		SH_ANY_A|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tandl\t%RA,%1A\n"},

	NOMORE
};

static OPCODE plusOp[] = {
#if defined(CMATH)
	{PLUS,
		SH_ANY_A|SH_ANY_CON,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_LABEL),
		RC_KEEP1,
			"%E\t.set\tLC%1U,[%LA+%RA]\n"},
#endif

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_TAREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED0,
		RC_RIGHT,
			"\tadd%RW\t$%LA%R&,%RA\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t$%RA%L&,%LA\n"},

	{PLUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_TAREG,		TY_SHORT,
		NEED0,
		RC_RIGHT,
			"\tadd%RW\t$%LA%R&,%RA\n\tmovs%RWl\t%RA,%RA\n"},

	{PLUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t$%RA%L&,%LA\n\tmovs%LWl\t%LA,%LA\n"},

	{PLUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_LONG,
		SH_TAREG,		TY_CHAR,
		NEED0,
		RC_RIGHT,
			"\tadd%RW\t$%LA%R&,%RA\n\tmovs%RWl\t%RA,%RA\n"},

	{PLUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_CHAR,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t$%RA%L&,%LA\n\tmovs%LWl\t%LA,%LA\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_AREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tleal\t%LA(%RA),%1A\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_AREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tleal\t%RA(%LA),%1A\n"},

	{PLUS,
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA%R&,%1A\n\tadd%RW\t%RA,%1A\n"},

	{PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%RA%L&,%1A\n\tadd%LW\t%LA,%1A\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_SHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA%R&,%1A\n\tadd%RW\t%RA,%1A\n\tmovs%RWl\t%1A,%1A\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_ADST,		TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%RA%L&,%1A\n\tadd%LW\t%LA,%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_CHAR,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA%R&,%1A\n\tadd%RW\t%RA,%1A\n\tmovs%RWl\t%1A,%1A\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_ADST,		TY_CHAR,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%RA%L&,%1A\n\tadd%LW\t%LA,%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t%RA,%LA\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_TAREG,		TY_ANY_FIXED,
		NEED0,
		RC_RIGHT,
			"\tadd%LW\t%LA,%RA\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE|TY_ANY_WORD,
		SH_ANY_ADST,		TY_SHORT,
		NEED0,
		RC_LEFT,
			"\tadd%RW\t%RA,%LA\n\tmovs%RWl\t%LA,%LA\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_SHORT,
		SH_TAREG,		TY_ANY_BYTE|TY_ANY_WORD,
		NEED0,
		RC_RIGHT,
			"\tadd%LW\t%LA,%RA\n\tmovs%LWl\t%RA,%RA\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_CHAR,
		NEED0,
		RC_LEFT,
			"\tadd%RW\t%RA,%LA\n\tmovs%RWl\t%LA,%LA\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_CHAR,
		SH_TAREG,		TY_ANY_BYTE,
		NEED0,
		RC_RIGHT,
			"\tadd%LW\t%LA,%RA\n\tmovs%LWl\t%RA,%RA\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_USHORT,
		SH_ANY_ADST,		TY_USHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_SHORT,
		SH_ANY_ADST,		TY_SHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{PLUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_UCHAR,
		SH_ANY_ADST,		TY_UCHAR,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n"},

	{PLUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_CHAR,
		SH_ANY_ADST,		TY_CHAR,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{PLUS,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_POP,
			"\tfaddp\n"},

	{PLUS,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED0,
		RC_LEFT,
			"\tfaddl\t%RA\n"},

	{PLUS,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_RIGHT,
			"\tfaddl\t%LA\n"},

	{PLUS,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n\tfaddl\t%RA\n"},

#if 0
	{PLUS,
		SH_TAREG,
		SH_TAREG,		TY_INT,
		SH_ANY_CON,		TY_POINT,
		NEED0,
		RC_LEFT,
			"\tleal\t%RA(%LA),%LA\n"},
#endif

	NOMORE
};

static OPCODE minusOp[] = {
#if defined(CMATH)
	{MINUS,
		SH_ANY_A|SH_ANY_CON,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\tLC%1U,[%LA-%RA]\n"},
#endif

	{MINUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_TAREG,		TY_CHAR,
		NEED0,
		RC_RIGHT,
			"\tnegl\t%RA\n\tadd%RW\t$%LA%R&,%RA\n\tmovs%RWl\t%RA,%RA\n"},

	{MINUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_CHAR,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t$%RA%L&,%LA\n\tmovs%LWl\t%LA,%LA\n"},

	{MINUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_TAREG,		TY_SHORT,
		NEED0,
		RC_RIGHT,
			"\tnegl\t%RA\n\tadd%RW\t$%LA%R&,%RA\n\tmovs%RWl\t%RA,%RA\n"},

	{MINUS,	/* force sign extension	*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t$%RAL&,%LA%\n\tmovs%LWl\t%LA,%LA\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_TAREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED0,
		RC_RIGHT,
			"\tnegl\t%RA\n\tadd%RW\t$%LA%R&,%RA\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t$%RA%L&,%LA\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_AREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tleal\t-%LA(%RA),%1A\n\tnegl\t%1A\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_AREG,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tleal\t-%RA(%LA),%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_CHAR,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA%R&,%1A\n\tsub%RW\t%RA,%1A\n\tmovs%RWl\t%1A,%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_ADST,		TY_CHAR,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tsub%LW\t$%RA%L&,%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_SHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA%R&,%1A\n\tsub%RW\t%RA,%1A\n\tmovs%RWl\t%1A,%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG,
		SH_ANY_ADST,		TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tsub%LW\t$%RA%L&,%1A\n\tmovs%LWl\t%1A,%1A\n"},

	{MINUS,
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t$%LA%R&,%1A\n\tsub%RW\t%RA,%1A\n"},

	{MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tsub%LW\t$%RA%L&,%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_CHAR,
		NEED0,
		RC_LEFT,
			"\tsub%RW\t%RA,%LA\n\tmovs%RWl\t%LA,%LA\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_CHAR,
		SH_TAREG,		TY_ANY_BYTE,
		NEED0,
		RC_RIGHT,
			"\tnegl\t%RA\n\tadd%LW\t%LA,%RA\n\tmovs%LWl\t%RA,%RA\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_BYTE|TY_ANY_WORD,
		SH_ANY_ADST,		TY_SHORT,
		NEED0,
		RC_LEFT,
			"\tsub%RW\t%RA,%LA\n\tmovs%RWl\t%LA,%LA\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_SHORT,
		SH_TAREG,		TY_ANY_BYTE|TY_ANY_WORD,
		NEED0,
		RC_RIGHT,
			"\tnegl\t%RA\n\tadd%LW\t%LA,%RA\n\tmovs%LWl\t%RA,%RA\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t%RA,%LA\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_TAREG,		TY_ANY_FIXED,
		NEED0,
		RC_RIGHT,
			"\tnegl\t%RA\n\tadd%LW\t%LA,%RA\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_CHAR,
		SH_ANY_ADST,		TY_CHAR,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n\t%LP\t%1A,%1A\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_UCHAR,
		SH_ANY_ADST,		TY_UCHAR,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_SHORT,
		SH_ANY_ADST,		TY_SHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n\t%LP\t%1A,%1A\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_USHORT,
		SH_ANY_ADST,		TY_USHORT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n\t%LP\t%1A,%1A\n"},

	{MINUS,	/* force sign extension		*/
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_INT,
		SH_ANY_ADST,		TY_INT,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n"},

	{MINUS,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_UNSIGNED,
		SH_ANY_ADST,		TY_UNSIGNED,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n"},

	{MINUS,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_POP,
			"\tfsub%Xp\n"},

	{MINUS,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED0,
		RC_LEFT,
			"\tfsubl\t%RA\n"},

	{MINUS,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_RIGHT,
			"\tfsubrl\t%LA\n"},

	{MINUS,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n\tfsubl\t%RA\n"},

	NOMORE
};

static OPCODE pluseqOp[] = {
	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t$%RA%L&,%LA\n"},

	{ASG PLUS,	/* force sign extension	*/
		SH_ANY_AREG,
		SH_ANY_AREG,		TY_CHAR|TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t$%RA%L&,%LA\n\tmovs%LWl\t%LA,%LA\n"},

	{ASG PLUS,
		SH_ANY_AREG,
		SH_ANY_MEM,		TY_CHAR|TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t$%RA%L&,%LA\n"},

	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_AREG,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t%RA,%LA\n"},

	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_ANY_AREG,		TY_ANY_WORD,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t%RA,%LA\n"},

	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_ANY_AREG,		TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tadd%LW\t%RA,%LA\n"},

	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n\tmov%LW\t%1A,%LA"},

	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n\tmov%LW\t%1A,%LA"},

	{ASG PLUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmov%LW\t%LA,%1A\n\tadd%LW\t%RA,%1A\n\tmov%LW\t%1A,%LA"},

	{ASG PLUS,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_LEFT,
			"\tfldl\t%LA\n\tfaddl\t%RA\n\tfstpl\n"},

	NOMORE
};

static OPCODE minuseqOp[] = {
	{ASG MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_UNSIGNED|TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t$%RA%L&,%LA\n"},

	{ASG MINUS,	/* force sign extension	*/
		SH_ANY_AREG,
		SH_ANY_AREG,		TY_CHAR|TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t$%RA%L&,%LA\n\tmovs%LWl\t%LA,%LA\n"},

	{ASG MINUS,
		SH_ANY_AREG,
		SH_ANY_MEM,		TY_CHAR|TY_SHORT,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t$%RA%L&,%LA\n"},

	{ASG MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_AREG,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t%RA,%LA\n"},

	{ASG MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_ANY_AREG,		TY_ANY_WORD,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t%RA,%LA\n"},

	{ASG MINUS,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_ANY_AREG,		TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tsub%LW\t%RA,%LA\n"},

	{ASG MINUS,
		SH_TAREG,
		SH_ANY_MEM,		TY_ANY_LONG,
		SH_ANY_MEM,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n\tmov%LW\t%1A,%LA"},

	{ASG MINUS,
		SH_TAREG,
		SH_ANY_MEM,		TY_ANY_WORD,
		SH_ANY_MEM,		TY_ANY_WORD,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n\tmov%LW\t%1A,%LA"},

	{ASG MINUS,
		SH_TAREG,
		SH_ANY_MEM,		TY_ANY_BYTE,
		SH_ANY_MEM,		TY_ANY_BYTE,
		NEED1(N_AREG),
		RC_LEFT,
			"\tmov%LW\t%LA,%1A\n\tsub%LW\t%RA,%1A\n\tmov%LW\t%1A,%LA"},

	{ASG MINUS,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_LEFT,
			"\tfldl\t%LA\n\tfsubl\t%RA\n\tfstpl\n"},

	NOMORE
};

/*
 * we are never asked to multiply things that are not long
 */
static OPCODE mulOp[] = {
#if defined(CMATH)
	{MUL,
		SH_ANY_A|SH_ANY_CON|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\tLC%1U,[%LA*%RA]\n"},
#endif

	{MUL,
		SH_TAREG|SH_FOREFF,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_TAREG,		TY_ANY_LONG,
		NEED0,
		RC_RIGHT,
			"\timull\t$%LA%R&,%RA\n"},

	{MUL,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\timull\t$%RA%L&,%LA\n"},

	{MUL,
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\timull\t$%LA%R&,%RA,%1A\n"},

	{MUL,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_BYTE|TY_ANY_WORD|TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\timull\t$%RA%L&,%LA,%1A\n"},

	{MUL,
		SH_TAREG|SH_FOREFF,
		SH_TAREG,		TY_ANY_FIXED,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\timull\t%RA,%LA\n"},

	{MUL,
		SH_TAREG|SH_FOREFF,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_TAREG,		TY_ANY_FIXED,
		NEED0,
		RC_RIGHT,
			"\timull\t%LA,%RA\n"},

	{MUL,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\timull\t%RA,%1A\n"},

	{MUL,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_POP,
			"\tfmulp\n"},

	{MUL,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED0,
		RC_LEFT,
			"\tfmull\t%RA\n"},

	{MUL,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_RIGHT,
			"\tfmull\t%LA\n"},

	{MUL,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n\tfmull\t%RA\n"},

	NOMORE
};

static OPCODE divOp[] = {
#if defined(CMATH)
	{DIV,
		SH_ANY_A|SH_ANY_CON,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\tLC%1U,%LA/%RA\n"},
#endif

	{DIV,
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED2(N_REG(EAX),N_REG(EDX)),
		RC_KEEP1,
			"\tmovl\t$%LA,%%eax\n\tcltd\n\tidivl\t%RA\n"},

	{DIV,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED3(N_REG(EAX),N_REG(EDX), N_AREG),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tmovl\t$%RA,%3A\n\tcltd\n\tidivl\t%3A\n"},

	{DIV,
		SH_TAREG,
		SH_EAX,			TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_REG(EDX)),
		RC_LEFT,
			"\tcltd\n\tidivl\t%RA\n"},

	{DIV,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED2(N_REG(EAX),N_REG(EDX)),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tcltd\n\tidivl\t%RA\n"},

	{DIV,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_POP,
			"\tfdiv%Xp\n"},

	{DIV,
		SH_TBREG|SH_FOREFF,
		SH_TBREG,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED0,
		RC_LEFT,
			"\tfdivl\t%RA\n"},

	{DIV,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED0,
		RC_RIGHT,
			"\tfdivrl\t%LA\n"},

	{DIV,
		SH_TBREG|SH_FOREFF,
		SH_ANY_MEM,		TY_DOUBLE,
		SH_ANY_MEM,		TY_DOUBLE,
		NEED1(N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n\tfdivl\t%RA\n"},

	NOMORE
};

static OPCODE modOp[] = {
#if defined(CMATH)
	{MOD,
		SH_ANY_A|SH_ANY_CON,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_LABEL),
		RC_KEEP1,
			"\t.set\tLC%1U,%LA%%%RA\n"},
#endif

	{MOD,
		SH_TAREG,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED2(N_REG(EAX),N_REG(EDX)),
		RC_KEEP2,
			"\tmovl\t$%LA,%%eax\n\tcltd\n\tidivl\t%RA\n"},

	{MOD,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED3(N_REG(EAX),N_REG(EDX), N_AREG),
		RC_KEEP2,
			"\tmovl\t%LA,%1A\n\tmovl\t$%RA,%3A\n\tcltd\n\tidivl\t%3A\n"},

	{MOD,
		SH_TAREG,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED2(N_REG(EAX),N_REG(EDX)),
		RC_KEEP2,
			"\tmovl\t%LA,%1A\n\tcltd\n\tidivl\t%RA\n"},

	NOMORE
};

/*
 * all the comparson operators
 */
static OPCODE eqOp[] = {
	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_COND),
		RC_KEEP1,
			"\tmovl\t$%LA,%1A\n\tcmpl\t$%RA,%1A\n\tset%K\t%1A\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_COND,		TY_ANY_FIXED,
		NEED0,
		RC_RIGHT,
			"\tcmp%RW\t$%LA%R&,%RA\n\tset%J\t%RA\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_COND,		TY_ANY_FIXED,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tcmp%LW\t$%RA%L&,%LA\n\tset%K\t%LA\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_FIXED,
		NEED1(N_COND),
		RC_KEEP1,
			"\tcmp%RW\t$%LA%R&,%RA\n\tset%J\t%1A\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_ADST,		TY_ANY_FIXED,
		SH_ANY_CON,		TY_ANY_LONG|TY_ANY_WORD|TY_ANY_BYTE,
		NEED1(N_COND),
		RC_KEEP1,
			"\tcmp%LW\t$%RA%L&,%LA\n\tset%K\t%1A\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_COND,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED0,
		RC_LEFT,
			"\tcmp%LW\t%LA,%RA\n\tset%J\t%LA\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_COND,		TY_ANY_BYTE,
		NEED0,
		RC_RIGHT,
			"\tcmp%RW\t%RA,%LA\n\tset%K\t%RA\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_COND,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED0,
		RC_LEFT,
			"\tcmp%LW\t%LA,%RA\n\tset%J\t%LA\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_COND,		TY_ANY_WORD,
		NEED0,
		RC_RIGHT,
			"\tcmp%RW\t%RA,%LA\n\tset%K\t%RA\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_COND,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED0,
		RC_LEFT,
			"\tcmp%LW\t%LA,%RA\n\tset%J\t%LA\n"},

	{EQ,
		SH_TAREG|SH_FOREFF|SH_COND,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_COND,		TY_ANY_LONG,
		NEED0,
		RC_RIGHT,
			"\tcmp%RW\t%RA,%LA\n\tset%K\t%RA\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_ADST,		TY_ANY_BYTE,
		SH_ANY_ADST,		TY_ANY_BYTE,
		NEED1(N_COND),
		RC_KEEP1,
			"\tmovb\t%LA,%1A\n\tcmpb\t%RA,%1A\n\tset%K\t%1A\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_ADST,		TY_ANY_WORD,
		SH_ANY_ADST,		TY_ANY_WORD,
		NEED1(N_COND),
		RC_KEEP1,
			"\tmovw\t%LA,%1A\n\tcmpw\t%RA,%1A\n\tset%K\t%1A\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_ADST,		TY_ANY_LONG,
		SH_ANY_ADST,		TY_ANY_LONG,
		NEED1(N_COND),
		RC_KEEP1,
			"\tmovl\t%LA,%1A\n\tcmpl\t%RA,%1A\n\tset%K\t%1A\n"},
	{EQ,
		SH_TAREG|SH_COND,
		SH_TBREG,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"%B\tfcompp\n\tfstsw\t%%ax\n\tsahf\n\tset%K\t%%eax\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_TBREG,		TY_DOUBLE,
		SH_ANY_BDST,		TY_DOUBLE,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tfcompl\t%RA\n\tfstsw\t%%ax\n\tsahf\n\tset%K\t%%eax\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_BDST,		TY_DOUBLE,
		SH_TBREG,		TY_DOUBLE,
		NEED1(N_REG(EAX)),
		RC_KEEP1,
			"\tfcompl\t%LA\n\tfstsw\t%%ax\n\tsahf\n\tset%J\t%%eax\n"},

	{EQ,
		SH_TAREG|SH_COND,
		SH_ANY_BDST,		TY_DOUBLE,
		SH_ANY_BDST,		TY_DOUBLE,
		NEED2(N_REG(EAX),N_BREG),
		RC_KEEP1,
			"\tfldl\t%LA\n\tfcompl\t%RA\n\tfstsw\t%%ax\n\tsahf\n\tset%K\t%%eax\n"},

	NOMORE
};

OPCODE *table[DSIZE];	/* look up opcode list		*/

void
opinit()
{
	table[PCONV] = pconvOp;
	table[INIT] = initOp;
	table[GOTO] = gotoOp;
	table[UNARY CALL] = ucallOp;
	table[CALL] = callOp;
	table[FORCE] = forceOp;
	table[REG] = regOp;
	table[ASSIGN] = assignOp;
	table[UNARY MINUS] = uminusOp;
	table[NOT] = notOp;
	table[COMPL] = complOp;
	table[OR] = orOp;
	table[AND] = andOp;
	table[MOD] = modOp;
	table[PLUS] = plusOp;
	table[MINUS] = minusOp;
	table[ASG PLUS] = pluseqOp;
	table[ASG MINUS] = minuseqOp;
	table[MUL] = mulOp;
	table[DIV] = divOp;
	table[EQ] = eqOp;
}
