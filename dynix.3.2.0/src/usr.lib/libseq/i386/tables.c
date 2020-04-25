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

#ifndef	lint
static char rcsid[] = "$Header: tables.c 1.10 90/05/29 $";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)ccs-s5:dis/i286/tables.c	1.2"
/*
 */
/*
*	Modified March 84 to add 186 instructions
*/
#include	"dis.h"

#define		NONE	0	/* to indicate register only in addrmod array*/
#define		INVALID	{"",TERM,UNKNOWN,0}

/*
 *	In 16-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '16' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

char	*regster_16[16] = {

/* w bit		0		1		*/

/* reg bits */
/* 000	*/		"%al",		"%ax",
/* 001  */		"%cl",		"%cx",
/* 010  */		"%dl",		"%dx",
/* 011	*/		"%bl",		"%bx",
/* 100	*/		"%ah",		"%sp",
/* 101	*/		"%ch",		"%bp",
/* 110	*/		"%dh",		"%si",
/* 111	*/		"%bh",		"%di",

};

/*
 *	In 32-bit addressing mode:
 *	Register operands may be indicated by a distinguished field.
 *	An '8' bit register is selected if the 'w' bit is equal to 0,
 *	and a '32' bit register is selected if the 'w' bit is equal to
 *	1 and also if there is no 'w' bit.
 */

char	*regster_32[16] = {

/* w bit		0		1		*/

/* reg bits */
/* 000	*/		"%al",		"%eax",
/* 001  */		"%cl",		"%ecx",
/* 010  */		"%dl",		"%edx",
/* 011	*/		"%bl",		"%ebx",
/* 100	*/		"%ah",		"%esp",
/* 101	*/		"%ch",		"%ebp",
/* 110	*/		"%dh",		"%esi",
/* 111	*/		"%bh",		"%edi",

};


/*
 *	In 16-bit mode:
 *	This initialized array will be indexed by the 'r/m' and 'mod'
 *	fields, to determine the addressing mode, and will also provide
 *	strings for printing.
 */

struct addr	addrmod_16 [32] = {

/* mod		00			01			10			11 */
/* r/m */
/* 000 */	{0,"(%bx,%si)"},	{8,"(%bx,%si)"},	{16,"(%bx,%si)"},	{NONE,"%ax"},
/* 001 */	{0,"(%bx,%di)"},	{8,"(%bx,%di)"},	{16,"(%bx,%di)"},	{NONE,"%cx"},
/* 010 */	{0,"(%bp,%si)"},	{8,"(%bp,%si)"},	{16,"(%bp,%si)"},	{NONE,"%dx"},
/* 011 */	{0,"(%bp,%di)"},	{8,"(%bp,%di)"},	{16,"(%bp,%di)"},	{NONE,"%bx"},
/* 100 */	{0,"(%si)"},		{8,"(%si)"},		{16,"(%si)"},		{NONE,"%sp"},
/* 101 */	{0,"(%di)"},		{8,"(%di)"},		{16,"(%di)"},		{NONE,"%bp"},
/* 110 */	{16,""},		{8,"(%bp)"},		{16,"(%bp)"},		{NONE,"%si"},
/* 111 */	{0,"(%bx)"},		{8,"(%bx)"},		{16,"(%bx)"},		{NONE,"%di"},
};

/*
 *	In 32-bit mode:
 *	This initialized array will be indexed by the 'r/m' and 'mod'
 *	fields, to determine the addressing mode, and will also provide
 *	strings for printing.
 */

struct addr	addrmod_32 [32] = {

/* mod		00			01			10			11 */
/* r/m */
/* 000 */	{0,"(%eax)"},		{8,"(%eax)"},		{32,"(%eax)"},		{NONE,"%eax"},
/* 001 */	{0,"(%ecx)"},		{8,"(%ecx)"},		{32,"(%ecx)"},		{NONE,"%ecx"},
/* 010 */	{0,"(%edx)"},		{8,"(%edx)"},		{32,"(%edx)"},		{NONE,"%edx"},
/* 011 */	{0,"(%ebx)"},		{8,"(%ebx)"},		{32,"(%ebx)"},		{NONE,"%ebx"},
/* 100 */	{0,"(%esp)"},		{8,"(%esp)"},		{32,"(%esp)"},		{NONE,"%esp"},
/* 101 */	{32,""},		{8,"(%ebp)"},		{32,"(%ebp)"},		{NONE,"%ebp"},
/* 110 */	{0,"(%esi)"},		{8,"(%esi)"},		{32,"(%esi)"},		{NONE,"%esi"},
/* 111 */	{0,"(%edi)"},		{8,"(%edi)"},		{32,"(%edi)"},		{NONE,"%edi"},
};

/*
 *	If r/m==100 then the following byte (the s-i-b byte) must be decoded
 */

char *scale_factor[4] = {
	",1",
	",2",
	",4",
	",8"
};

char *index_reg[8] = {
	",%eax",
	",%ecx",
	",%edx",
	",%ebx",
	"",
	",%ebp",
	",%esi",
	",%edi"
};
/*
 *	Segment registers are selected by a two or three bit field.
 */

char	*segreg[6] = {

/* 000 */	"%es",
/* 001 */	"%cs",
/* 010 */	"%ss",
/* 011 */	"%ds",
/* 100 */	"%fs",
/* 101 */	"%gs",

};


/*
 *	Decode table for 0x0F00 opcodes
 */

struct instable op0F00[8] = {

/*  [0]  */	{"sldt",TERM,M,0},	{"str",TERM,M,0},	{"lldt",TERM,M,0},	{"ltr",TERM,M,0},
/*  [4]  */	{"verr",TERM,M,0},	{"verw",TERM,M,0},	INVALID,		INVALID,
};


/*
 *	Decode table for 0x0F01 opcodes
 */

struct instable op0F01[8] = {

/*  [0]  */	{"sgdt",TERM,M,0},	{"sidt",TERM,M,0},	{"lgdt",TERM,M,0},	{"lidt",TERM,M,0},
/*  [4]  */	{"smsw",TERM,M,0},	INVALID,		{"lmsw",TERM,M,0},	{"invlpg",TERM,M,0}
};


/*
 *	Decode table for 0x0FBA opcodes
 */

struct instable op0FBA[8] = {

/*  [0]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [4]  */	{"bt",TERM,BTi,1},	{"bts",TERM,BTi,1},	{"btr",TERM,BTi,1},	{"btc",TERM,BTi,1},
};


/*
 *	Decode table for 0x0F opcodes
 */

struct instable op0F[194] = {

/*  [00]  */	{"",op0F00,TERM,0},	{"",op0F01,TERM,0},	{"lar",TERM,MR,0},	{"lsl",TERM,MR,0},
/*  [04]  */	INVALID,		INVALID,		{"clts",TERM,GO_ON,0},	INVALID,
/*  [08]  */	{"invd",TERM,GO_ON,0},	{"wbinvd",TERM,GO_ON,0}, INVALID,		INVALID,
/*  [0C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [10]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [14]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [18]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [1C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [20]  */	{"mov",TERM,Mc,1},	{"mov",TERM,Md,1},	{"mov",TERM,Mc,1},	{"mov",TERM,Md,1},
/*  [24]  */	{"mov",TERM,Mt,1},	INVALID,		{"mov",TERM,Mt,1},	INVALID,
/*  [28]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [30]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [34]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [38]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [3C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [40]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [44]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [48]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [4C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [50]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [54]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [58]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [5C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [60]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [64]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [68]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [6C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [70]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [74]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [78]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [7C]  */	INVALID,		INVALID,		INVALID,		INVALID,

/*  [80]  */	{"jo",TERM,D,0},	{"jno",TERM,D,0},	{"jb",TERM,D,0},	{"jae",TERM,D,0},
/*  [84]  */	{"je",TERM,D,0},	{"jne",TERM,D,0},	{"jbe",TERM,D,0},	{"ja",TERM,D,0},
/*  [88]  */	{"js",TERM,D,0},	{"jns",TERM,D,0},	{"jp",TERM,D,0},	{"jnp",TERM,D,0},
/*  [8C]  */	{"jl",TERM,D,0},	{"jge",TERM,D,0},	{"jle",TERM,D,0},	{"jg",TERM,D,0},

/*  [90]  */	{"seto",TERM,M,0},	{"setno",TERM,M,0},	{"setb",TERM,M,0},	{"setae",TERM,M,0},
/*  [94]  */	{"sete",TERM,M,0},	{"setne",TERM,M,0},	{"setbe",TERM,M,0},	{"seta",TERM,M,0},
/*  [98]  */	{"sets",TERM,M,0},	{"setns",TERM,M,0},	{"setp",TERM,M,0},	{"setnp",TERM,M,0},
/*  [9C]  */	{"setl",TERM,M,0},	{"setge",TERM,M,0},	{"setle",TERM,M,0},	{"setg",TERM,M,0},

/*  [A0]  */	{"push",TERM,LSEG,1},	{"pop",TERM,LSEG,1},	INVALID,		{"bt",TERM,RMMR,1},
/*  [A4]  */	{"shld",TERM,RMMRI,1},	{"shld",TERM,RMMR,1},	{"xbts",TERM,RMMR,1}, {"ibts",TERM,RMMR,1},
/*  [A8]  */	{"push",TERM,LSEG,1},	{"pop",TERM,LSEG,1},	INVALID,		{"bts",TERM,RMMR,1},
/*  [AC]  */	{"shrd",TERM,RMMRI,1},	{"shrd",TERM,RMMR,1},	INVALID,		{"imul",TERM,RMMR,1},

/*  [B0]  */	{"cmpxchgb",TERM,RMMRe,0},{"cmpxchg",TERM,RMMRe,1},{"lss",TERM,MR,0},	{"btr",TERM,RMMR,1},
/*  [B4]  */	{"lfs",TERM,MR,0},	{"lgs",TERM,MR,0},	{"movzb",TERM,RMMRe,1},	{"movzwl",TERM,RMMRe,0},
/*  [B8]  */	INVALID,		INVALID,		{"",op0FBA,TERM,0},	{"btc",TERM,RMMR,1},
/*  [BC]  */	{"bsf",TERM,RMMR,1},	{"bsr",TERM,RMMR,1},	{"movsb",TERM,RMMRe,1},	{"movswl",TERM,RMMRe,0},
/*  [C0]  */	{"xaddb",TERM,RMMRe,0},	{"xadd",TERM,RMMRe,1},
};


/*
 *	Decode table for 0x80 opcodes
 */

struct instable op80[8] = {

/*  [0]  */	{"addb",TERM,IMlw,0},	{"orb",TERM,IMw,0},	{"adcb",TERM,IMlw,0},	{"sbbb",TERM,IMlw,0},
/*  [4]  */	{"andb",TERM,IMw,0},	{"subb",TERM,IMlw,0},	{"xorb",TERM,IMw,0},	{"cmpb",TERM,IMlw,0},
};


/*
 *	Decode table for 0x81 opcodes.
 */

struct instable op81[8] = {

/*  [0]  */	{"add",TERM,IMlw,1},	{"or",TERM,IMw,1},	{"adc",TERM,IMlw,1},	{"sbb",TERM,IMlw,1},
/*  [4]  */	{"and",TERM,IMw,1},	{"sub",TERM,IMlw,1},	{"xor",TERM,IMw,1},	{"cmp",TERM,IMlw,1},
};


/*
 *	Decode table for 0x82 opcodes.
 */

struct instable op82[8] = {

/*  [0]  */	{"addb",TERM,IMlw,0},	INVALID,		{"adcb",TERM,IMlw,0},	{"sbbb",TERM,IMlw,0},
/*  [4]  */	INVALID,		{"subb",TERM,IMlw,0},	INVALID,		{"cmpb",TERM,IMlw,0},
};
/*
 *	Decode table for 0x83 opcodes.
 */

struct instable op83[8] = {

/*  [0]  */	{"add",TERM,IMlw,1},	INVALID,		{"adc",TERM,IMlw,1},	{"sbb",TERM,IMlw,1},
/*  [4]  */	INVALID,		{"sub",TERM,IMlw,1},	INVALID,		{"cmp",TERM,IMlw,1},
};

/*
 *	Decode table for 0xC0 opcodes.
 *	186 instruction set
 */

struct instable opC0[8] = {

/*  [0]  */	{"rolb",TERM,MvI,0},	{"rorb",TERM,MvI,0},	{"rclb",TERM,MvI,0},	{"rcrb",TERM,MvI,0},
/*  [4]  */	{"shlb",TERM,MvI,0},	{"shrb",TERM,MvI,0},	INVALID,		{"sarb",TERM,MvI,0},
};

/*
 *	Decode table for 0xD0 opcodes.
 */

struct instable opD0[8] = {

/*  [0]  */	{"rolb",TERM,Mv,0},	{"rorb",TERM,Mv,0},	{"rclb",TERM,Mv,0},	{"rcrb",TERM,Mv,0},
/*  [4]  */	{"shlb",TERM,Mv,0},	{"shrb",TERM,Mv,0},	INVALID,		{"sarb",TERM,Mv,0},
};

/*
 *	Decode table for 0xC1 opcodes.
 *	186 instruction set
 */

struct instable opC1[8] = {

/*  [0]  */	{"rol",TERM,MvI,1},	{"ror",TERM,MvI,1},	{"rcl",TERM,MvI,1},	{"rcr",TERM,MvI,1},
/*  [4]  */	{"shl",TERM,MvI,1},	{"shr",TERM,MvI,1},	INVALID,		{"sar",TERM,MvI,1},
};

/*
 *	Decode table for 0xD1 opcodes.
 */

struct instable opD1[8] = {

/*  [0]  */	{"rol",TERM,Mv,1},	{"ror",TERM,Mv,1},	{"rcl",TERM,Mv,1},	{"rcr",TERM,Mv,1},
/*  [4]  */	{"shl",TERM,Mv,1},	{"shr",TERM,Mv,1},	INVALID,		{"sar",TERM,Mv,1},
};


/*
 *	Decode table for 0xD2 opcodes.
 */

struct instable opD2[8] = {

/*  [0]  */	{"rolb",TERM,Mv,0},	{"rorb",TERM,Mv,0},	{"rclb",TERM,Mv,0},	{"rcrb",TERM,Mv,0},
/*  [4]  */	{"shlb",TERM,Mv,0},	{"shrb",TERM,Mv,0},	INVALID,		{"sarb",TERM,Mv,0},
};
/*
 *	Decode table for 0xD3 opcodes.
 */

struct instable opD3[8] = {

/*  [0]  */	{"rol",TERM,Mv,1},	{"ror",TERM,Mv,1},	{"rcl",TERM,Mv,1},	{"rcr",TERM,Mv,1},
/*  [4]  */	{"shl",TERM,Mv,1},	{"shr",TERM,Mv,1},	INVALID,		{"sar",TERM,Mv,1},
};


/*
 *	Decode table for 0xF6 opcodes.
 */

struct instable opF6[8] = {

/*  [0]  */	{"testb",TERM,IMw,0},	INVALID,		{"notb",TERM,Mw,0},	{"negb",TERM,Mw,0},
/*  [4]  */	{"mulb",TERM,MA,0},	{"imulb",TERM,MA,0},	{"divb",TERM,MA,0},	{"idivb",TERM,MA,0},
};


/*
 *	Decode table for 0xF7 opcodes.
 */

struct instable opF7[8] = {

/*  [0]  */	{"test",TERM,IMw,1},	INVALID,		{"not",TERM,Mw,1},	{"neg",TERM,Mw,1},
/*  [4]  */	{"mul",TERM,MA,1},	{"imul",TERM,MA,1},	{"div",TERM,MA,1},	{"idiv",TERM,MA,1},
};


/*
 *	Decode table for 0xFE opcodes.
 */

struct instable opFE[8] = {

/*  [0]  */	{"incb",TERM,Mw,0},	{"decb",TERM,Mw,0},	INVALID,		INVALID,
/*  [4]  */	INVALID,		INVALID,		INVALID,		INVALID,
};
/*
 *	Decode table for 0xFF opcodes.
 */

struct instable opFF[8] = {

/*  [0]  */	{"inc",TERM,Mw,1},	{"dec",TERM,Mw,1},	{"call",TERM,INM,0},	{"lcall",TERM,INM,0},
/*  [4]  */	{"jmp",TERM,INM,0},	{"ljmp",TERM,INM,0},	{"push",TERM,M,1},	INVALID,
};

/* for 287 instructions, which are a mess to decode */

struct instable opfp1n2[64] = {
/* bit pattern:	1101 1xxx MODxx xR/M */
/*  [0,0] */	{"fadds",TERM,M,0},	{"fmuls",TERM,M,0},	{"fcoms",TERM,M,0},	{"fcomps",TERM,M,0},
/*  [0,4] */	{"fsubs",TERM,M,0},	{"fsubrs",TERM,M,0},	{"fdivs",TERM,M,0},	{"fdivrs",TERM,M,0},
/*  [1,0]  */	{"flds",TERM,M,0},	INVALID,		{"fsts",TERM,M,0},	{"fstps",TERM,M,0},
/*  [1,4]  */	{"fldenv",TERM,M,0},	{"fldcw",TERM,M,0},	{"fnstenv",TERM,M,0},	{"fnstcw",TERM,M,0},
/*  [2,0]  */	{"fiaddl",TERM,M,0},	{"fimull",TERM,M,0},	{"ficoml",TERM,M,0},	{"ficompl",TERM,M,0},
/*  [2,4]  */	{"fisubl",TERM,M,0},	{"fisubrl",TERM,M,0},	{"fidivl",TERM,M,0},	{"fidivrl",TERM,M,0},
/*  [3,0]  */	{"fildl",TERM,M,0},	INVALID,		{"fistl",TERM,M,0},	{"fistpl",TERM,M,0},
/*  [3,4]  */	INVALID,		{"fldt",TERM,M,0},	INVALID,		{"fstpt",TERM,M,0},
/*  [4,0]  */	{"faddl",TERM,M,0},	{"fmull",TERM,M,0},	{"fcoml",TERM,M,0},	{"fcompl",TERM,M,0},
/*  [4,1]  */	{"fsubl",TERM,M,0},	{"fsubrl",TERM,M,0},	{"fdivl",TERM,M,0},	{"fdivrl",TERM,M,0},
/*  [5,0]  */	{"fldl",TERM,M,0},	INVALID,		{"fstl",TERM,M,0},	{"fstpl",TERM,M,0},
/*  [5,4]  */	{"frstor",TERM,M,0},	INVALID,		{"fnsave",TERM,M,0},	{"fnstsw",TERM,M,0},
/*  [6,0]  */	{"fiadd",TERM,M,0},	{"fimul",TERM,M,0},	{"ficom",TERM,M,0},	{"ficomp",TERM,M,0},
/*  [6,4]  */	{"fisub",TERM,M,0},	{"fisubr",TERM,M,0},	{"fidiv",TERM,M,0},	{"fidivr",TERM,M,0},
/*  [7,0]  */	{"fild",TERM,M,0},	INVALID,		{"fist",TERM,M,0},	{"fistp",TERM,M,0},
/*  [7,4]  */	{"fbld",TERM,M,0},	{"fildll",TERM,M,0},	{"fbstp",TERM,M,0},	{"fistpll",TERM,M,0},
};

struct instable opfp3[64] = {
/* bit  pattern:	1101 1xxx 11xx xREG */
/*  [0,0]  */	{"fadd",TERM,FF,0},	{"fmul",TERM,FF,0},	{"fcom",TERM,F,0},	{"fcomp",TERM,F,0},
/*  [0,4]  */	{"fsub",TERM,FF,0},	{"fsubr",TERM,FF,0},	{"fdiv",TERM,FF,0},	{"fdivr",TERM,FF,0},
/*  [1,0]  */	{"fld",TERM,F,0},	{"fxch",TERM,F,0},	{"fnop",TERM,GO_ON,0},	{"fstp",TERM,F,0},
/*  [1,4]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2,0]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [2,4]  */	INVALID,		{"fucompp",TERM,GO_ON,0},INVALID,		INVALID,
/*  [3,0]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [3,4]  */	INVALID,		INVALID,		INVALID,		INVALID,
/*  [4,0]  */	{"fadd",TERM,FF,0},	{"fmul",TERM,FF,0},	{"fcom",TERM,F,0},	{"fcomp",TERM,F,0},
/*  [4,4]  */	{"fsub",TERM,FF,0},	{"fsubr",TERM,FF,0},	{"fdiv",TERM,FF,0},	{"fdivr",TERM,FF,0},
/*  [5,0]  */	{"ffree",TERM,F,0},	{"fxch",TERM,F,0},	{"fst",TERM,F,0},	{"fstp",TERM,F,0},
/*  [5,4]  */	{"fucom",TERM,F,0},	{"fucomp",TERM,F,0},	INVALID,		INVALID,
/*  [6,0]  */	{"faddp",TERM,FF,0},	{"fmulp",TERM,FF,0},	{"fcomp",TERM,F,0},	{"fcompp",TERM,GO_ON,0},
/*  [6,4]  */	{"fsubp",TERM,FF,0},	{"fsubrp",TERM,FF,0},	{"fdivp",TERM,FF,0},	{"fdivrp",TERM,FF,0},
/*  [7,0]  */	{"ffree",TERM,F,0},	{"fxch",TERM,F,0},	{"fstp",TERM,F,0},	{"fstp",TERM,F,0},
/*  [7,4]  */	{"fstsw",TERM,M,0},	INVALID,		INVALID,		INVALID,
};

struct instable opfp4[32] = {
/* bit pattern:	1101 1001 111x xxxx */
/*  [0,0]  */	{"fchs",TERM,GO_ON,0},	{"fabs",TERM,GO_ON,0},	INVALID,		INVALID,
/*  [0,4]  */	{"ftst",TERM,GO_ON,0},	{"fxam",TERM,GO_ON,0},	INVALID,		INVALID,
/*  [1,0]  */	{"fld1",TERM,GO_ON,0},	{"fldl2t",TERM,GO_ON,0},{"fldl2e",TERM,GO_ON,0},{"fldpi",TERM,GO_ON,0},
/*  [1,4]  */	{"fldlg2",TERM,GO_ON,0},{"fldln2",TERM,GO_ON,0},{"fldz",TERM,GO_ON,0},	INVALID,
/*  [2,0]  */	{"f2xm1",TERM,GO_ON,0},	{"fyl2x",TERM,GO_ON,0},	{"fptan",TERM,GO_ON,0},	{"fpatan",TERM,GO_ON,0},
/*  [2,4]  */	{"fxtract",TERM,GO_ON,0},{"fprem1",TERM,GO_ON,0},{"fdecstp",TERM,GO_ON,0},{"fincstp",TERM,GO_ON,0},
/*  [3,0]  */	{"fprem",TERM,GO_ON,0},	{"fyl2xp1",TERM,GO_ON,0},{"fsqrt",TERM,GO_ON,0},{"fsincos",TERM,GO_ON,0},
/*  [3,4]  */	{"frndint",TERM,GO_ON,0},{"fscale",TERM,GO_ON,0},{"fsin",TERM,GO_ON,0},{"fcos",TERM,GO_ON,0},
};

struct instable opfp5[8] = {
/* bit pattern:	1101 1011 1110 0xxx */
/*  [0]  */	INVALID,		INVALID,		{"fnclex",TERM,GO_ON,0},{"fninit",TERM,GO_ON,0},
/*  [4]  */	{"fsetpm",TERM,GO_ON,0},INVALID,		INVALID,		INVALID,
};

/*
 *	Main decode table for the op codes.  The first two nibbles
 *	will be used as an index into the table.  If there is a
 *	a need to further decode an instruction, the array to be
 *	referenced is indicated with the other two entries being
 *	empty.
 */

struct instable distable[256] = {

/* [0,0] */	{"addb",TERM,RMMR,0},	{"add",TERM,RMMR,1},	{"addb",TERM,RMMR,0},	{"add",TERM,RMMR,1},
/* [0,4] */	{"addb",TERM,IA,0},	{"add",TERM,IA,1},	{"push",TERM,SEG,1},	{"pop",TERM,SEG,1},
/* [0,8] */	{"orb",TERM,RMMR,0},	{"or",TERM,RMMR,1},	{"orb",TERM,RMMR,0},	{"or",TERM,RMMR,1},
/* [0,C] */	{"orb",TERM,IA,0},	{"or",TERM,IA,1},	{"push",TERM,SEG,1},	{"",op0F,TERM,0},

/* [1,0] */	{"adcb",TERM,RMMR,0},	{"adc",TERM,RMMR,1},	{"adcb",TERM,RMMR,0},	{"adc",TERM,RMMR,1},
/* [1,4] */	{"adcb",TERM,IA,0},	{"adc",TERM,IA,1},	{"push",TERM,SEG,1},	{"pop",TERM,SEG,1},
/* [1,8] */	{"sbbb",TERM,RMMR,0},	{"sbb",TERM,RMMR,1},	{"sbbb",TERM,RMMR,0},	{"sbb",TERM,RMMR,1},
/* [1,C] */	{"sbbb",TERM,IA,0},	{"sbb",TERM,IA,1},	{"push",TERM,SEG,1},	{"pop",TERM,SEG,1},

/* [2,0] */	{"andb",TERM,RMMR,0},	{"and",TERM,RMMR,1},	{"andb",TERM,RMMR,0},	{"and",TERM,RMMR,1},
/* [2,4] */	{"andb",TERM,IA,0},	{"and",TERM,IA,1},	{"%es:",TERM,OVERRIDE,0},{"daa",TERM,GO_ON,0},
/* [2,8] */	{"subb",TERM,RMMR,0},	{"sub",TERM,RMMR,1},	{"subb",TERM,RMMR,0},	{"sub",TERM,RMMR,1},
/* [2,C] */	{"subb",TERM,IA,0},	{"sub",TERM,IA,1},	{"%cs:",TERM,OVERRIDE,0},{"das",TERM,GO_ON,0},

/* [3,0] */	{"xorb",TERM,RMMR,0},	{"xor",TERM,RMMR,1},	{"xorb",TERM,RMMR,0},	{"xor",TERM,RMMR,1},
/* [3,4] */	{"xorb",TERM,IA,0},	{"xor",TERM,IA,1},	{"%ss:",TERM,OVERRIDE,0},{"aaa",TERM,GO_ON,0},
/* [3,8] */	{"cmpb",TERM,RMMR,0},	{"cmp",TERM,RMMR,1},	{"cmpb",TERM,RMMR,0},	{"cmp",TERM,RMMR,1},
/* [3,C] */	{"cmpb",TERM,IA,0},	{"cmp",TERM,IA,1},	{"%ds:",TERM,OVERRIDE,0},{"aas",TERM,GO_ON,0},

/* [4,0] */	{"inc",TERM,R,1},	{"inc",TERM,R,1},	{"inc",TERM,R,1},	{"inc",TERM,R,1},
/* [4,4] */	{"inc",TERM,R,1},	{"inc",TERM,R,1},	{"inc",TERM,R,1},	{"inc",TERM,R,1},
/* [4,8] */	{"dec",TERM,R,1},	{"dec",TERM,R,1},	{"dec",TERM,R,1},	{"dec",TERM,R,1},
/* [4,C] */	{"dec",TERM,R,1},	{"dec",TERM,R,1},	{"dec",TERM,R,1},	{"dec",TERM,R,1},

/* [5,0] */	{"push",TERM,R,1},	{"push",TERM,R,1},	{"push",TERM,R,1},	{"push",TERM,R,1},
/* [5,4] */	{"push",TERM,R,1},	{"push",TERM,R,1},	{"push",TERM,R,1},	{"push",TERM,R,1},
/* [5,8] */	{"pop",TERM,R,1},	{"pop",TERM,R,1},	{"pop",TERM,R,1},	{"pop",TERM,R,1},
/* [5,C] */	{"pop",TERM,R,1},	{"pop",TERM,R,1},	{"pop",TERM,R,1},	{"pop",TERM,R,1},

/* [6,0] */	{"pusha",TERM,GO_ON,1},	{"popa",TERM,GO_ON,1},	{"bound",TERM,MR,1},	{"arpl",TERM,RMw,0},
/* [6,4] */	{"%fs:",TERM,OVERRIDE,0},{"%gs:",TERM,OVERRIDE,0},{"data16",TERM,DM,0},	{"addr16",TERM,AM,0},
/* [6,8] */	{"push",TERM,I,1},	{"imul",TERM,RMMRI,1},	{"push",TERM,Ib,1},	{"imul",TERM,RMMRI,1},
/* [6,C] */	{"insb",TERM,GO_ON,0},	{"ins",TERM,GO_ON,1},	{"outsb",TERM,GO_ON,0},	{"outs",TERM,GO_ON,1},

/* [7,0] */	{"jo",TERM,BD,0},	{"jno",TERM,BD,0},	{"jc",TERM,BD,0},	{"jae",TERM,BD,0},
/* [7,4] */	{"je",TERM,BD,0},	{"jne",TERM,BD,0},	{"jbe",TERM,BD,0},	{"ja",TERM,BD,0},
/* [7,8] */	{"js",TERM,BD,0},	{"jns",TERM,BD,0},	{"jp",TERM,BD,0},	{"jnp",TERM,BD,0},
/* [7,C] */	{"jl",TERM,BD,0},	{"jge",TERM,BD,0},	{"jle",TERM,BD,0},	{"jg",TERM,BD,0},
/* [8,0] */	{"",op80,TERM,0},	{"",op81,TERM,0},	{"",op82,TERM,0},	{"",op83,TERM,0},
/* [8,4] */	{"testb",TERM,MRw,0},	{"test",TERM,MRw,1},	{"xchgb",TERM,MRw,0},	{"xchg",TERM,MRw,1},
/* [8,8] */	{"movb",TERM,RMMR,0},	{"mov",TERM,RMMR,1},	{"movb",TERM,RMMR,0},	{"mov",TERM,RMMR,1},
/* [8,C] */	{"mov",TERM,SM,1},	{"leal",TERM,MR,0},	{"mov",TERM,MS,1},	{"pop",TERM,M,1},

/* [9,0] */	{"xchg",TERM,RA,1},	{"xchg",TERM,RA,1},	{"xchg",TERM,RA,1},	{"xchg",TERM,RA,1},
/* [9,4] */	{"xchg",TERM,RA,1},	{"xchg",TERM,RA,1},	{"xchg",TERM,RA,1},	{"xchg",TERM,RA,1},
/* [9,8] */	{"cwtl",TERM,GO_ON,0},	{"cltd",TERM,GO_ON,0},	{"lcall",TERM,SO,0},	{"wait",TERM,GO_ON,0},
/* [9,C] */	{"pushf",TERM,GO_ON,1},	{"popf",TERM,GO_ON,1},	{"sahf",TERM,GO_ON,0},	{"lahf",TERM,GO_ON,0},

/* [A,0] */	{"movb",TERM,OA,0},	{"mov",TERM,OA,1},	{"movb",TERM,AO,0},	{"mov",TERM,AO,1},
/* [A,4] */	{"smovb",TERM,GO_ON,0},{"smov",TERM,GO_ON,1},	{"scmpb",TERM,GO_ON,0},	{"scmp",TERM,GO_ON,1},
/* [A,8] */	{"testb",TERM,IA,0},	{"test",TERM,IA,1},	{"sstob",TERM,GO_ON,0},	{"ssto",TERM,GO_ON,1},
/* [A,C] */	{"slodb",TERM,GO_ON,0},	{"slod",TERM,GO_ON,1},	{"sscab",TERM,GO_ON,0},	{"ssca",TERM,GO_ON,1},

/* [B,0] */	{"movb",TERM,IR,0},	{"movb",TERM,IR,0},	{"movb",TERM,IR,0},	{"movb",TERM,IR,0},
/* [B,4] */	{"movb",TERM,IR,0},	{"movb",TERM,IR,0},	{"movb",TERM,IR,0},	{"movb",TERM,IR,0},
/* [B,8] */	{"mov",TERM,IR,1},	{"mov",TERM,IR,1},	{"mov",TERM,IR,1},	{"mov",TERM,IR,1},
/* [B,C] */	{"mov",TERM,IR,1},	{"mov",TERM,IR,1},	{"mov",TERM,IR,1},	{"mov",TERM,IR,1},

/* [C,0] */	{"",opC0,TERM,0},	{"",opC1,TERM,0},	{"ret",TERM,I,0},	{"ret",TERM,GO_ON,0},
/* [C,4] */	{"les",TERM,MR,0},	{"lds",TERM,MR,1},	{"movb",TERM,IMw,0},	{"mov",TERM,IMw,1},
/* [C,8] */	{"enter",TERM,II,0},	{"leave",TERM,GO_ON,0},	{"lret",TERM,I,0},	{"lret",TERM,GO_ON,0},
/* [C,C] */	{"int",TERM,Iv,0},	{"int",TERM,Iv,0},	{"into",TERM,GO_ON,0},	{"iret",TERM,GO_ON,0},

/* [D,0] */	{"",opD0,TERM,0},	{"",opD1,TERM,0},	{"",opD2,TERM,0},	{"",opD3,TERM,0},
/* [D,4] */	{"aam",TERM,U,0},	{"aad",TERM,U,0},	{"falc",TERM,GO_ON,0},	{"xlat",TERM,GO_ON,0},

/* 287 instructions.  Note that although the indirect field		*/
/* indicates opfp1n2 for further decoding, this is not necessarily	*/
/* the case since the opfp arrays are not partitioned according to key1	*/
/* and key2.  opfp1n2 is given only to indicate that we haven't		*/
/* finished decoding the instruction.					*/
/* [D,8] */	{"",opfp1n2,TERM,0},	{"",opfp1n2,TERM,0},	{"",opfp1n2,TERM,0},	{"",opfp1n2,TERM,0},
/* [D,C] */	{"",opfp1n2,TERM,0},	{"",opfp1n2,TERM,0},	{"",opfp1n2,TERM,0},	{"",opfp1n2,TERM,0},

/* [E,0] */	{"loopnz",TERM,BD,0},	{"loopz",TERM,BD,0},	{"loop",TERM,BD,0},	{"jcxz",TERM,BD,0},
/* [E,4] */	{"inb",TERM,P,0},	{"in",TERM,P,1},	{"outb",TERM,P,0},	{"out",TERM,P,1},
/* [E,8] */	{"call",TERM,D,0},	{"jmp",TERM,D,0},	{"ljmp",TERM,SO,0},	{"jmp",TERM,BD,0},
/* [E,C] */	{"inb",TERM,V,0},	{"in",TERM,V,1},	{"outb",TERM,V,0},	{"out",TERM,V,1},

/* [F,0] */	{"lock",TERM,GO_ON,0},	{"",TERM,JTAB,0},	{"repne",TERM,GO_ON,0},	{"repe",TERM,GO_ON,0},
/* [F,4] */	{"hlt",TERM,GO_ON,0},	{"cmc",TERM,GO_ON,0},	{"",opF6,TERM,0},	{"",opF7,TERM,0},
/* [F,8] */	{"clc",TERM,GO_ON,0},	{"stc",TERM,GO_ON,0},	{"cli",TERM,GO_ON,0},	{"sti",TERM,GO_ON,0},
/* [F,C] */	{"cld",TERM,GO_ON,0},	{"std",TERM,GO_ON,0},	{"",opFE,TERM,0},	{"",opFF,TERM,0},
};
