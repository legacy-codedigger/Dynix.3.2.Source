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

/* $Header: dis.c 1.19 90/02/26 $ */

/*LINTLIBRARY*/
#include <stdio.h>
#include "dis.h"

/*
 * Array for holding disassembled instruction
 */
char _386i_str[132];

/*
 * The following are setup by dis386_init()
 */
unsigned char (*_386iread)(); /* function to read a byte from instr stream */
char *(*_386xlate_name)(); /* function to xlate name given absolute addr */
int _386show_addrmodes;	/* nonzero if also real addr modes */

unsigned long _386i_addr;/* internal address of next instruction stream byte */
unsigned long _386s_addr;/* saved address of start of this instruction */
int _386und;		/* Undefined flag, if nonzero on return from format */
			/* then undefined instruction */

#define		RM	0	/* in case RMMR, is the value of the	*/
				/* 'd' bit that indicates an RM rather	*/
				/* than MR instruction			*/
#define		TWOlw	1	/* is value in 'l' and 'w' bits that	*/
				/* indicates a 2 byte immediate operand	*/
#define		TWOw	1	/* value of 'w' bit that indicates a	*/
				/* 2 byte operand			*/
#define		REGv	1	/* value of 'v' bit that indicates a	*/
				/* register needs to be included as an	*/
				/* operand				*/
#define		TYPEv	0	/* in case Iv for an interrupt;value of	*/
				/* 'v' bit which indicates the type of	*/
				/* interrupt				*/
#define		TYPE3	3	/* indicates a type of interrupt	*/
#define		DISP_0	0	/* indicates a 0 byte displacement	*/
#define		DISP_1	8	/* indicates a 1 byte displacement	*/
#define		DISP_2	16	/* indicates a 2 byte displacement	*/
#define		DISP_4	32	/* indicates a 4 byte displacement	*/
#define		REG_ONLY 3	/* indicates a single register with	*/
				/* no displacement is an operand	*/
#define		MAXERRS	5	/* maximum # of errors allowed before	*/
				/* abandoning this disassembly as a	*/
				/* hopeless case			*/
#define		OPLEN	256	/* maximum length of a single operand	*/
static	char	gname[OPLEN];
static	char	operand[4][OPLEN];	/* to store operands as they	*/
					/* are encountered		*/
static	int	opindex;	/* will index into the 'operand' array	*/
static int override;	/* to save the fact that an override segment	*/
			/* register request is outstanding.		*/
static char *overreg;	/* to save the override register		*/

static	unsigned short	curbyte;
static	unsigned short	cur2bytes;
static	unsigned long	cur4bytes;

	int	g_D_mode = 32;	/* default to 32-bit data */
	int	g_A_mode = 32;	/* default to 32-bit addressing */
	int	D_mode, A_mode;
	int	w_default = 1;	/* default W bit */

#define MASKw(x)	(x & 0x1)		/* to get w bit	*/
#define MASK3w(x)	( (x >> 3) & 0x1)	/* to get w bit from middle */
						/* where form is  'xxxxwxxx' */
#define MASKlw(x)	(x & 0x3)		/* to get 'lw' bits */
#define MASKreg(x)	(x & 0x7)		/* to get 3 bit register */
#define MASKv(x)	( (x >> 1) & 0x1)	/* to get 'v' bit */
#define MASKd(x)	( (x >> 1) & 0x1)	/* to get 'd' bit */
#define MASKseg(x)	( (x >> 3) & 0x3)	/* get seg reg from op code*/
#define MASKlseg(x)	( (x >> 3) & 0x7)	/* get long seg reg from op code*/

#define R_M(x)		(x & 0x7)		/* get r/m field from byte */
						/* after op code */
#define REG(x)		( (x >> 3) & 0x7)	/* get register field from */
						/* byte after op code      */
#define MODE(x)		( (x >> 6) & 0x3)	/* get mode field from byte */
						/* after op code */
#define	LOW4(x)		( x & 0xf)		/* ----xxxx low 4 bits */
#define	BITS7_4(x)	( (x >> 4) & 0xf)	/* xxxx---- bits 7 to 4 */
#define MASKret(x)	( (x >> 2) & 0x1)	/* return result bit
						for 287 instructions	*/
#define ESP		(unsigned) 0x4		/* three-bit code for %esp register */

/*
 * dis_init() initializes some externally supplied
 * entities necessary for disassembly
 */
dis386_init(ignored, iread, xlate_name, flag)
	unsigned char (*iread)();
	char *(*xlate_name)();
	int flag;
{

	_386iread = iread;
	_386xlate_name = xlate_name;
	_386show_addrmodes = (flag != 0);
}

/*
 * main disassembly function:   returns ptr to decoded instruction
 * 				sets length of instruction decoded
 */

char *
dis386(addr, length)
	unsigned addr;
	int *length;
{
	struct instable *dp, *submode;
	enum byte_count no_bytes;
	int dbit, wbit, vbit, lwbits;
	unsigned key1, key2, key3, mode, reg, r_m, temporary;
	unsigned key4, key5;
	unsigned short tmpshort;
	char *reg_name;
	int opcode;
	int i486_xchg;
	extern char *decode_1167();
	void emit_386_or_1167();
	/* the following arrays are contained in _tbls.c	*/
	extern	struct	instable	distable[16][16];
	extern	struct	instable	op0F[12][16];
	extern	struct	instable	opfp1n2[8][8];
	extern	struct	instable	opfp3[8][8];
	extern	struct	instable	opfp4[4][8];
	extern	struct	instable	opfp5[8];
	extern	struct	addr		addrmod_16[8][4];
	extern	struct	addr		addrmod_32[8][4];
	extern	char	*regster_16[8][2];
	extern	char	*regster_32[8][2];
	extern	char	*segreg[6];
	char	temp[NCPS+1];
	short	sect_num;
	long	lngval;
	void get_decode(),
	     ck_prt_override(),
	     displacement(),
	     prtaddress(),
	     saving_disp(),
	     imm_data(),
	     get_opcode(),
	     bad_opcode();
	extern	void	getbyte();
	extern	void	convert();
	extern	void	compoff();
	extern	void	convert();
	extern	void	lconvert();

	/*
	 * Initialize instruction length and ascii representation
	 */
	*length = 0;
	_386i_str[0] = '\0';
	_386und = 0;
	_386i_addr = addr;
	_386s_addr = addr;
	D_mode = g_D_mode;	/* pick up global defaults */
	A_mode = g_A_mode;	/* pick up global defaults */
	override = FALSE;
	opindex = 0;
	(void) sprintf(operand[0],"");
	(void) sprintf(operand[1],"");
	(void) sprintf(operand[2],"");
	(void) sprintf(operand[3],"");
	gname[0] = '\0';

	/*
	 * Fetch a byte from the appropriate address
	 * (interface specific?)
	 */
again:
	i486_xchg = 0;
	get_opcode(&key1, &key2);
	dp = &distable[key1][key2];
	/* 286 instructions have 2 bytes of opcode before the mod_r/m */
	/* byte so we need to perform a table indirection.	      */
	if (dp->indirect == (struct instable *)op0F) {
		get_opcode(&key4, &key5);

		if ((key4==1) || ((key4>2) && (key4<8)) || (key4>11))  {
			if (key4==12 && key5>=8) {	/* bswap */
				(void) sprintf(_386i_str,"%-8s%s", "bswap",
						regster_32[MASKreg(key5)][1]);
				goto out;
			}
			if (curbyte == 0xc0 || curbyte == 0xc1)
				i486_xchg = 1;	/* i486 xadd */
			else {
				_386und++;
				goto out;
			}
		} else if (curbyte == 0xa5 || curbyte == 0xa6)
			i486_xchg = 1;	/* i486 cmpxchg */
		dp = &op0F[key4][key5];
	}
	submode = dp -> indirect;
	if (dp -> indirect != TERM) {
		/* This must have been an opcode for which several
		 * instructions exist.  The key3 field (from the
		 * next byte) determines exactly which instruction
		 * this is.
		 */
		get_decode(&mode, &key3, &r_m);
		/* figuring out the names for 287 instruction isn't
		 * as easy as just looking at the opcode byte and
		 * key3.  Let's simply say it's a mix of this and that.
		 *
		 * 287 instructions fall between D8 and DF */
		if (key1 == 0xD && key2 >= 0x8) {
			if (key2 == 0xB && mode == 0x3 && key3 == 4)
			/* instruction form 5 */
				dp = &opfp5[r_m];
			else if (key2 == 0xB && mode == 0x3 && key3 > 4) {
				_386und++;
				goto out;
			} else if (key2==0x9 && mode==0x3 && key3 >= 4)
			/* instruction form 4 */
				dp = &opfp4[key3-4][r_m];
			else if (key2 >= 8 && mode == 0x3)
			/* instruction form 3 */
				dp = &opfp3[key2-8][key3];
			else
			/* instruction form 1 and 2 */
				dp = &opfp1n2[key2-8][key3];
			} else {
				dp = dp -> indirect + key3;
				/* now dp points the proper subdecode
				/* table entry */
		}
	}
	/* print the mnemonic (which may exceed 7 chars) */
	(void) strcpy(_386i_str, dp->name);
	if (dp->suffix)
		(void) strcat(_386i_str, D_mode == 16 ? "w" : "l");
	do (void) strcat(_386i_str, " ");
	while (strlen(_386i_str) < 8);
 
	/*
	 * Each instruction has a particular instruction syntax format
	 * stored in the disassembly tables.  The assignment of formats
	 * to instructins was made by the author.  Individual formats
	 * are explained as they are encountered in the following
	 * switch construct.
	 */

	switch(dp -> adr_mode){

	/* default segment register in next instruction will	*/
	/* be overridden.  This fact, along with the segment	*/
	/* register to be used, must be saved and recognized	*/
	/* later in the 'prtaddress' routine			*/
	case OVERRIDE:
		overreg = dp -> name;
		override = TRUE;
		goto again;		/* please forgive me */
	/* register to or from a memory or register operand,	*/
	/* based on the 'd' bit					*/
	case RMMR:
		dbit = MASKd(key2);
		wbit = MASKw(key2);
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		key3 = curbyte;
		prtaddress(mode, r_m, wbit);
		if (key1 == 0 && key2 == 0xF) {
			switch((key4<<4)|key5) {
			case 0xa3:	/* bt   */
			case 0xa5:	/* shld */
			case 0xa7:	/* ibts */
			case 0xab:	/* bts  */
			case 0xad:	/* shrd */
			case 0xb3:	/* btr  */
			case 0xbb:	/* btc  */
			case 0xbc:	/* bsf  */
			case 0xbd:	/* bsr  */
				dbit = RM;
				break;
			}
		}
		reg_name = decode_1167(key1<<4|key2, key3, cur4bytes);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		emit_386_or_1167 (reg_name, reg, D_mode, dbit, wbit);
		break;
	/* mov[sz][bw][wl] instructions */
	case RMMRe:
		wbit = MASKw(key5);
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress(mode, r_m, wbit);
		if (D_mode==16)
			reg_name= regster_16[reg][i486_xchg ? wbit : w_default];
		else
			reg_name= regster_32[reg][i486_xchg ? wbit : w_default];
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],reg_name);
		break;

	/* 3-operand instructions */
	case RMMRI:
		lwbits = MASKlw(key2);
		if( (opcode = key1<<4|key2) == 0x69 )
			no_bytes = (D_mode==32) ? FOUR : TWO;
		else
			no_bytes = ONE;
		dbit = MASKd(key2);
		wbit = MASKw(key2);
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress( mode, r_m, wbit);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		opindex = 1;
		imm_data(no_bytes);
		opindex = 0;
		if (D_mode==16)
			reg_name = regster_16[reg][wbit];
		else
			reg_name = regster_32[reg][wbit];
		/* imul format is imm,r/m,reg, but sh[lr]d format is imm,reg,r/m */
		if (opcode == 0x69 || opcode == 0x6b)
			(void) sprintf(_386i_str,"%s%s,%s,%s",_386i_str,operand[1],operand[0],reg_name);
		else
			(void) sprintf(_386i_str,"%s%s,%s,%s",_386i_str,operand[1],reg_name,operand[0]);
		break;
	/* memory or register operand to register, with 'w' bit	*/
	case MRw:
		wbit = MASKw(key2);
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress( mode, r_m, wbit);
		if (D_mode==16)
			reg_name = regster_16[reg][wbit];
		else
			reg_name = regster_32[reg][wbit];
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],reg_name);
		break;
	/* register to memory or register operand, with 'w' bit	*/
	case RMw:
		wbit = MASKw(key2);
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress( mode, r_m, wbit);
		if (D_mode==16)
			reg_name = regster_16[reg][wbit];
		else
			reg_name = regster_32[reg][wbit];
		(void) sprintf(_386i_str,"%s%s,%s", _386i_str,reg_name,operand[0]);
		break;

	/*
	 *	For the following two cases, one must read the
	 *	displacement (if present) and have it saved for
	 *	printing after the immediate operand.  This is
	 *	done by calling the 'saving_disp' routine.
	 *	For further documention see this routine.
	 */

	/* immediate to memory or register operand with both	*/
	/* 'l' and 'w' bits present				*/

	case IMlw:
		lwbits = MASKlw(key2);
		wbit = MASKw(key2);
		opindex = 2;
		saving_disp(mode, r_m, wbit);
		if (D_mode==16)
			no_bytes = (lwbits == TWOlw) ? TWO : ONE;
		else
			no_bytes = (lwbits == TWOlw) ? FOUR : ONE;
		opindex = 0;
		imm_data(no_bytes);
		opindex = 1;
		ck_prt_override();
		(void) sprintf(_386i_str, "%s%s,%s%s",
			_386i_str,operand[0],operand[1], operand[2]);
		break;
	/* immediate to memory or register operand with the	*/
	/* 'w' bit present					*/
	case IMw:
		wbit = MASKw(key2);
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		opindex = 2;
		saving_disp(mode, r_m, wbit);
		if (D_mode==16)
			no_bytes = (wbit == TWOw) ? TWO : ONE;
		else
			no_bytes = (wbit == TWOw) ? FOUR : ONE;
		/* 
		 * If this is a Weitek instruction, we need to redo the
		 * op code and first operand.
		 */
		(void) decode_1167 (key1<<4|key2, mode<<6|reg<<3|r_m, cur4bytes);

		opindex = 0;
		imm_data(no_bytes);
		opindex = 1;
		ck_prt_override();
		if (strncmp(_386i_str, "wldctx", 6) == 0)
			(void) sprintf(_386i_str, "%s%s",
				_386i_str,operand[0]);
		else
			(void) sprintf(_386i_str, "%s%s,%s%s",
				_386i_str,operand[0],operand[1], operand[2]);
		break;

	/* immediate to register with register in low 3 bits	*/
	/* of op code						*/
	case IR:
		wbit = (key2 >> 3) & 0x1;
		reg = MASKreg(key2);
		if (D_mode==16)
			no_bytes = (wbit == TWOw) ? TWO : ONE;
		else
			no_bytes = (wbit == TWOw) ? FOUR : ONE;
		imm_data(no_bytes);
		if (D_mode==16)
			reg_name = regster_16[reg][wbit];
		else
			reg_name = regster_32[reg][wbit];
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],reg_name);
		break;

	/* memory operand to accumulator			*/
	case OA:
		dbit = MR;
		goto OAandAO;

	/* accumulator to memory operand			*/
	case AO:
		dbit = RM;
OAandAO:
		wbit = MASKw(key2);
		no_bytes = (A_mode == 16) ? TWO : FOUR;
		displacement(no_bytes);
		reg_name = decode_1167 (key1<<4|key2, 0x05, cur4bytes);
		emit_386_or_1167 (reg_name, 0, A_mode, dbit, wbit);
		break;
	/* memory or register operand to segment register	*/
	case MS:
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress(mode, r_m, w_default);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],segreg[reg]);
		break;

	/* segment register to memory or register operand	*/
	case SM:
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress(mode, r_m, w_default);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,segreg[reg],operand[0]);
		break;

	/* memory or register operand, which may or may not	*/
	/* go to the cl register, dependent on the 'v' bit	*/
	case Mv:
		vbit = MASKv(key2);
		wbit = MASKw(key2);
		prtaddress( mode, r_m, wbit);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s%s%s",_386i_str,
			(key1 == 0xd && vbit != REGv) ? "$1,":"",
			(vbit == REGv) ? "%cl,":"",
			operand[0]);
		break;
	/* added for 186 support; March 84; jws */
	case MvI:
		vbit = MASKv(key2);
		wbit = MASKw(key2);
		prtaddress( mode, r_m, wbit);
		no_bytes = ONE;
		opindex = 1;
		imm_data(no_bytes);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s%s",_386i_str,
			operand[1],
			(vbit == REGv) ? "%cl,":"",
			operand[0]);
		opindex = 0;
		break;

	/* single memory or register operand with 'w' bit present*/
	case Mw:
		wbit = MASKw(key2);
		prtaddress( mode, r_m, wbit);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s",_386i_str,operand[0]);
		break;

	/* single memory or register operand			*/
	case M:
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		/*
		 * The setcc instructions (op codes 0F90-0F9F),
		 * write 8-bit registers.
		 */
		prtaddress(mode, r_m,
			(key1==0 && key2==0xF && key4==9) ? 0 : w_default);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		/*
		 * Decode pushl and popl weitek opcodes
		 */
		if ( (mode == 0 && r_m == 0x5)
		 &&  ( (key1 == 0xf && key2 == 0xf && key3 == 0x6)
		   ||  (key1 == 0x8 && key2 == 0xf && reg == 0x0)
		     )
		 &&  ( (cur4bytes & 0xffff0000) == 0xffc00000  ||
		       (cur4bytes & 0xffff0000) == 0xc0000000 ) )
			
			sprintf(operand[0], "%%fp%d", (cur4bytes >> 2) & 0x1f);
		(void) sprintf(_386i_str,"%s%s",_386i_str,operand[0]);
		break;

	/* control register (386 extension) */
	case Mc:
		dbit = MASKd(key5);
		wbit = MASKw(key2);	/* always 1 */
		get_decode(&mode, &reg, &r_m);
		/* mode is ignored (treated as 11) */
		if (reg != 0 && reg != 2 && reg != 3) {
			_386und++;
			goto out;
		}
		reg_name = regster_32[r_m][wbit];
		if (dbit == RM)
			(void) sprintf(_386i_str,"%s%%cr%c,%s",_386i_str, reg+'0', reg_name);
		else
			(void) sprintf(_386i_str,"%s%s,%%cr%c",_386i_str, reg_name, reg+'0');
		break;

	/* debug register (386 extension) */
	case Md:
		dbit = MASKd(key5);
		wbit = MASKw(key2);
		get_decode(&mode, &reg, &r_m);
		/* mode is ignored (treated as 11) */
		if (reg == 4 || reg == 5) {
			/* 386 aliases these to DR6 and DR7 but ... */
			_386und++;
			goto out;
		}
		reg_name = regster_32[r_m][wbit];
		if (dbit == RM)
			(void) sprintf(_386i_str,"%s%%db%c,%s",_386i_str, reg+'0', reg_name);
		else
			(void) sprintf(_386i_str,"%s%s,%%db%c",_386i_str, reg_name, reg+'0');
		break;
		
	/* test register (386 extension) */
	case Mt:
		dbit = MASKd(key5);
		wbit = MASKw(key2);
		get_decode(&mode, &reg, &r_m);
		/* mode is ignored (treated as 11) */
		if (reg < 3 || reg > 7) {
			/* i386:
			 *     TR0 .. TR3 generate undefined opcode traps
			 *     TR4 and TR5 store and load to nowhere
			 * i486:
			 *     TR0 .. TR2 store and load to nowhere
			 */
			_386und++;
			goto out;
		}
		reg_name = regster_32[r_m][wbit];
		if (dbit == RM)
			(void) sprintf(_386i_str,"%s%%tr%c,%s",_386i_str, reg+'0', reg_name);
		else
			(void) sprintf(_386i_str,"%s%s,%%tr%c",_386i_str, reg_name, reg+'0');
		break;
		
	/* single register operand with register in the low 3	*/
	/* bits of op code					*/
	case R:
		reg = MASKreg(key2);
		if (A_mode==16 || D_mode==16)
			reg_name = regster_16[reg][w_default];
		else
			reg_name = regster_32[reg][w_default];
		(void) sprintf(_386i_str,"%s%s",_386i_str,reg_name);
		break;
	/* register to accumulator with register in the low 3	*/
	/* bits of op code					*/
	case RA:
		reg = MASKreg(key2);
		/*
		 * Pick off ``nop'' which is 
		 * technically "movl   %eax,%eax"
		 */
		if (key1 == 9 && key2 == 0)
			(void) sprintf(_386i_str,"nop");
		else if (A_mode==16)
			(void) sprintf(_386i_str,"%s%s,%%ax",_386i_str,regster_16[reg][w_default]);
		else
			(void) sprintf(_386i_str,"%s%s,%%eax",_386i_str,regster_32[reg][w_default]);
		break;

	/* single segment register operand, with register in	*/
	/* bits 3-4 of op code					*/
	case SEG:
		reg = MASKseg(curbyte);
		(void) sprintf(_386i_str,"%s%s",_386i_str,segreg[reg]);
		break;

	/* single segment register operand, with register in	*/
	/* bits 3-5 of op code					*/
	case LSEG:
		reg = MASKlseg(curbyte);
		(void) sprintf(_386i_str,"%s%s",_386i_str,segreg[reg]);
		break;

	/* memory or register operand to register		*/
	case MR:
		if (submode == NULL)
			get_decode(&mode, &reg, &r_m);
		prtaddress(mode, r_m, w_default);
		if (D_mode==16)
			reg_name = regster_16[reg][w_default];
		else
			reg_name = regster_32[reg][w_default];
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],reg_name);
		break;

	/* immediate operand to accumulator			*/
	case IA:
		wbit = MASKw(key2);
		if (D_mode==16)
			no_bytes = (wbit == TWOw) ? TWO : ONE;
		else
			no_bytes = (wbit == TWOw) ? FOUR : ONE;
		imm_data(no_bytes);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],
			(no_bytes == ONE) ? "%al" : ((no_bytes == TWO) ? "%ax" : "%eax"));
		break;

	/* memory or register operand to accumulator		*/
	case MA:
		wbit = MASKw(key2);
		prtaddress( mode, r_m, wbit);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s",_386i_str, operand[0]);
		break;

	/* si register to di register				*/
	case SD:
		ck_prt_override();
		(void) sprintf(_386i_str,"%s%s(%%si),(%%di)",_386i_str,operand[0]);
		break;

	/* accumulator to di register				*/
	case AD:
		wbit = MASKw(key2);
		ck_prt_override();
		(void) sprintf(_386i_str,"%s%s,%s(%%di)",_386i_str, (A_mode == 16) ?
			regster_16[0][wbit] : regster_32[0][wbit],operand[0]);
		break;

	/* single operand, a 16 bit displacement		*/
	/* added to current offset by 'compoff'			*/
	case D:
		if (A_mode==16)
			no_bytes = TWO;
		else
			no_bytes = FOUR;
		displacement(no_bytes);
		if (A_mode==16)
			lngval = (cur2bytes & 0x8000) ?
				cur2bytes | ~0xffffL :
				cur2bytes & 0xffffL;
		else
			lngval = cur4bytes;
		compoff(lngval, operand[1]);
		if (A_mode==16)
			(void) sprintf(_386i_str,"%s%s",_386i_str,operand[1]);
		else
			(void) sprintf(_386i_str,"%s%s",_386i_str,operand[1]);
		break;

	/* indirect to memory or register operand		*/
	case INM:
		prtaddress(mode, r_m, w_default);
		(void) sprintf(_386i_str,"%s*%s",_386i_str,operand[0]);
		break;
	/* for long jumps and long calls -- a new code segment	*/
	/* register and an offset in IP -- stored in object	*/
	/* code in reverse order				*/
	case SO:
		no_bytes = TWO;
		opindex = 1;
		displacement(no_bytes);
		opindex = 0;
		/* will now get segment operand*/
		displacement(no_bytes);
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],operand[1]);
		break;

	/* single operand, 8 bit displacement			*/
	/* added to current offset in 'compoff'			*/
	case BD:
		no_bytes = ONE;
		displacement(no_bytes);
		lngval = (curbyte & 0x80) ? curbyte | ~0xffL :
					    curbyte & 0xffL;
		compoff(lngval, operand[1]);
		(void) sprintf(_386i_str,"%s%s",_386i_str, operand[1]);
		break;

	/* single 16 bit immediate operand			*/
	case I:
		if (D_mode==16)
			no_bytes = TWO;
		else
			no_bytes = FOUR;
		imm_data(no_bytes);
		(void) sprintf(_386i_str,"%s%s",_386i_str,operand[0]);
		break;

	/* single 8 bit immediate operand			*/
	case Ib:
		no_bytes = ONE;
		imm_data(no_bytes);
		(void) sprintf(_386i_str,"%s%s",_386i_str,operand[0]);
		break;

	/* added for 186 support; March 84; jws */
	case II:
		no_bytes = TWO;
		opindex = 0;
		imm_data(no_bytes);
		no_bytes = ONE;
		opindex = 1;
		imm_data(no_bytes);
		opindex = 0;
		(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],operand[1]);
		break;

	/* single 8 bit port operand				*/
	case P:
		no_bytes = ONE;
		displacement(no_bytes);
		(void) sprintf(_386i_str,"%s$%s",_386i_str,operand[0]);
		break;

	/* single operand, dx register (variable port instruction)*/
	case V:
		ck_prt_override();
		(void) sprintf(_386i_str,"%s%s(%%dx)",_386i_str,operand[0]);
		break;
	/* operand is either 3 or else the next 8 bits,		*/
	/* dependent on the 'v' bit (indicates type of interrupt)*/
	case Iv:
		vbit = MASKw(key2);
		if (vbit == TYPEv) {
			temporary = TYPE3;
			convert(temporary, temp, LEAD);
			(void) sprintf(_386i_str,"%s%s",_386i_str,temp);
		}
		else {
			no_bytes = ONE;
			imm_data(no_bytes);
			(void) sprintf(_386i_str,"%s%s",_386i_str,operand[0]);
		}
		break;

	/* bit test with 8 bits of immediate data		*/
	case BTi:
		prtaddress(mode, r_m, w_default);
		opindex = 1;
		no_bytes = ONE;
		imm_data(no_bytes);
		if (gname[0] != '\0' && index(operand[0], '%') == NULL)
			(void) strcpy(operand[0], gname);
		(void) sprintf(_386i_str,"%s%s,%s",
			_386i_str,operand[1],operand[0]);
		break;

	/* an unused byte must be discarded			*/
	case U:
		getbyte();
		break;

	/* no disassembly, the mnemonic was all there was	*/
	/* so go on						*/
	case GO_ON:
		/* handle special case */
		if (key1 == 9 && key2 == 8 && D_mode == 16)
			(void) sprintf(_386i_str,"%-8s", "cbtw");
		else if (key1 == 9 && key2 == 9 && D_mode == 16)
			(void) sprintf(_386i_str,"%-8s", "cwtd");
		break;

	/* Special byte indicating a the beginning of a 	*/
	/* jump table has been seen. The jump table addresses	*/
	/* will be printed until the address 0xffff which	*/
	/* indicates the end of the jump table is read.		*/
	case JTAB:
		(void) sprintf(_386i_str,"***JUMP TABLE BEGINNING***");
#ifdef	notdef
		printline();
		prt_offset();
		lookbyte();
		if (curbyte == FILL) {
			if (Lflag > 0)
				looklabel(loc,(unsigned short)sect_num);
			line_nums(sech);
			(void) sprintf(_386i_str,"FILL BYTE FOR ALIGNMENT");
			(void) sprintf(object,"%s90",object);
			printline();
			prt_offset();
			(void) printf("\t");
			lookbyte();
			tmpshort = curbyte;
			lookbyte();
			(void) sprintf(object,"%s%02x %02x",
				object,curbyte,tmpshort);
		}
		else {
			tmpshort = curbyte;
			lookbyte();
			(void) printf("\t");
			(void) sprintf(object,"%s%02x %02x",
				object,curbyte,tmpshort);
		}
		(void) sprintf(_386i_str,"");
		while ((curbyte != 0x00ff) || (tmpshort != 0x00ff)) {
			printline();
			prt_offset();
			(void) printf("\t");
			lookbyte();
			tmpshort = curbyte;
			lookbyte();
			(void) sprintf(object,"%s%02x %02x",
				object,curbyte,tmpshort);
		}
		(void) sprintf(_386i_str,"***JUMP TABLE END***");
#endif
		break;

	/* float reg */
	case F:
		(void) sprintf(_386i_str,"%s%%st(%1.1d)",_386i_str,r_m);
		break;

	/* float reg to float reg, with ret bit present */
	case FF:
		if ( MASKret(key2) )
			/* st -> st(i) */
			(void) sprintf(_386i_str,"%s%%st,%%st(%1.1d)",_386i_str,r_m);
		else
			/* st(i) -> st */
			(void) sprintf(_386i_str,"%s%%st(%1.1d),%%st",_386i_str,r_m);
		break;

	/* toggle from 32-bit data mode to 16-bit data mode (or back) */
	case DM:
		D_mode = (D_mode == 32) ? 16 : 32;
		goto again;

	/* toggle from 32-bit address mode to 16-bit address mode (or back) */
	case AM:
		A_mode = (A_mode == 32) ? 16 : 32;
		goto again;

	/* an invalid op code (there aren't too many)	*/
	default:
	case UNKNOWN:
		_386und++;
		break;
	} /* end switch */

out:
	if (_386und) {
		*length = 0;
		return(NULL);
	} else {
		*length = _386i_addr - addr;
		return(_386i_str);
	}
}

/*ftg get_decode () */
/*
 *	void get_decode (mode, reg, r_m)
 *
 *	Get the byte following the op code and separate it into the
 *	mode, register, and r/m fields.
 */

void
get_decode(mode, reg, r_m)
unsigned	*mode;
unsigned	*reg;
unsigned	*r_m;

{
	extern	void	getbyte();
	extern	unsigned short curbyte;

	getbyte();

	*r_m = R_M(curbyte);
	*reg = REG(curbyte);
	*mode = MODE(curbyte);

}

/*ftg ck_prt_override () */
/*
 *	void ck_prt_override ()
 *
 *	Check to see if there is a segment override prefix pending.
 *	If so, print it in the current 'operand' location and set
 *	the override flag back to false.
 */

void
ck_prt_override()
{
	if (override == FALSE)
		return;

	(void) sprintf(operand[opindex],"%s",overreg);
	override = FALSE;
}

/*ftg displacement () */
/*
 *	void displacement (no_bytes)
 *
 *	Get and print in the 'operand' array a one, two or four
 *	byte displacement.
 */

void
displacement(no_bytes)
enum byte_count no_bytes;

{
	extern	void		getbyte();
	extern	unsigned short	curbyte;	/* in _extn.c */
	extern	unsigned short	cur2bytes;	/* in _extn.c */
	extern	unsigned long	cur4bytes;	/* in _extn.c */
	extern	void		convert();	/* in _utls.c */
	extern	void		lconvert();	/* in _utls.c */

	unsigned	short	bytehigh;
	unsigned	short	bytelow;
	unsigned	long	lbyte;
	char		templow[NCPS+1];
	char		temphigh[NCPS+1];
	char	temp[(NCPS*2)+1];
	void ck_prt_override();

	getbyte();
	bytelow = curbyte;

	switch (no_bytes) {
	case ONE:
		convert(bytelow, templow, LEAD);
		(void) sprintf(temp,"%s",templow);
		break;

	case TWO:
		getbyte();
		bytehigh = curbyte;
		bytehigh = (bytehigh << 8) | bytelow;
		/* if location if to be computed by 'compoff', the */
		/* displacement will be around in 'cur2bytes'	*/
		cur2bytes = bytehigh;
		convert(bytehigh, temphigh, LEAD);
		(void) sprintf(temp,"%s",temphigh);
		break;

	case FOUR:
		getbyte();
		bytehigh = curbyte;
		lbyte = (bytehigh << 8) | bytelow;

		getbyte();
		bytelow = curbyte;
		getbyte();
		bytehigh = curbyte;
		bytehigh = (bytehigh << 8) | bytelow;
		lbyte = (bytehigh << 16) | lbyte;
		
		/* if location if to be computed by 'compoff', the */
		/* displacement will be around in 'cur4bytes'	*/
		cur4bytes = lbyte;
		lconvert(lbyte, temphigh, LEAD);
		(void) sprintf(temp,"%s",temphigh);
		break;

	}

	ck_prt_override();
	(void) sprintf(operand[opindex],"%s%s",operand[opindex],temp);
}

/*ftg prtaddress () */
/*
 *	void prtaddress (mode, r_m, wbit)
 *
 *	Print the registers used to find the address mode,  checking 
 *	to see if there was a segment override prefix present.
 */

void
prtaddress(mode, r_m, wbit)
unsigned	mode;
unsigned	r_m;
int		wbit;

{
	extern	struct	addr	addrmod_16[8][4]; /* in _tbls.c */
	extern	struct	addr	addrmod_32[8][4]; /* in _tbls.c */
	extern	char 	*regster_16[8][2];	  /* in _tbls.c */
	extern	char 	*regster_32[8][2];	  /* in _tbls.c */
	extern	char	*scale_factor[4];	  /* in _tbls.c */
	extern	char	*index_reg[8];		  /* in _tbls.c */

	enum byte_count no_bytes;
	unsigned ss, index, base;
	char *reg_name;
	int disp,
	    s_i_b;
	void ck_prt_override(),
	     displacement();

	/* check for a segment override prefix 	*/

	ck_prt_override();

	/* check for the presence of the s-i-b byte */
	if ((r_m==ESP) && (mode!=0x3)) {
		s_i_b = TRUE;
		get_decode(&ss, &index, &base);
	} else
		s_i_b = FALSE;

	disp = (A_mode == 16) ? addrmod_16[(s_i_b)?base:r_m][mode].disp :
				addrmod_32[(s_i_b)?base:r_m][mode].disp;

	switch (disp) {
	case DISP_0:
		break;
	case DISP_1:
		no_bytes = ONE;
		displacement(no_bytes);
		break;

	case DISP_2:
		no_bytes = TWO;
		displacement(no_bytes);
		break;

	case DISP_4:
		no_bytes = FOUR;
		displacement(no_bytes);
		break;
	}

	if (s_i_b==FALSE) {
		if (A_mode==16 || (D_mode==16 && mode == REG_ONLY))
			reg_name = (mode == REG_ONLY) ?
				regster_16[r_m][wbit] :
				addrmod_16[r_m][mode].regs;
		else
			reg_name = (mode == REG_ONLY) ?
				regster_32[r_m][wbit] :
				addrmod_32[r_m][mode].regs;

	} else {
		char buffer[16];
		char obuffer[16];

		/* get the base register from addrmod_32;
		 * we may have to strip off leading/tailing parens */
		strcpy(obuffer, addrmod_32[base][mode].regs);

		if (obuffer[0] == '(') {
			reg_name = obuffer+1;
			reg_name[strlen(reg_name)-1] = '\0';
		} else
			reg_name = obuffer;
		if (*index_reg[index] == '\0' && ss == 0)
			(void) sprintf(buffer, "(%s)", reg_name);
		else
			(void) sprintf(buffer, "(%s%s%s)", reg_name,
				index_reg[index], scale_factor[ss]);
		reg_name = buffer;
	}

	(void) sprintf(operand[opindex],"%s%s",operand[opindex], reg_name);
}

/*ftg saving_disp () */
/*
 *	void saving_disp (mode, r_m, wbit)
 *
 *	If a one, two or four byte displacement is needed, call 'displacement'
 *	to both read and save it in the current 'operand' location
 *	for printing in the mnuemonic output.
 */

void
saving_disp(mode, r_m, wbit)
unsigned	mode;
unsigned	r_m;
unsigned	wbit;

{
	extern	struct	addr	addrmod_16[8][4]; /* in _tbls.c */
	extern	struct	addr	addrmod_32[8][4]; /* in _tbls.c */

	enum byte_count no_bytes;
	unsigned ss, index, base;
	int disp,
	    s_i_b;
	void displacement();
	char *reg_name;

	(void) sprintf(operand[opindex],"");
	if ((r_m==ESP) && (mode!=0x3)) {
		s_i_b = TRUE;
		get_decode(&ss, &index, &base);
	} else
		s_i_b = FALSE;

	disp = (A_mode == 16) ? addrmod_16[(s_i_b)?base:r_m][mode].disp :
				addrmod_32[(s_i_b)?base:r_m][mode].disp;

	switch (disp) {
	case DISP_0:
		break;

	case DISP_1:
		no_bytes = ONE;
		displacement(no_bytes);
		break;

	case DISP_2:
		no_bytes = TWO;
		displacement(no_bytes);
		break;

	case DISP_4:
		no_bytes = FOUR;
		displacement(no_bytes);
		break;
	}

	if (s_i_b==FALSE) {
		if (A_mode==16 || (D_mode==16 && mode == REG_ONLY))
			reg_name = (mode == REG_ONLY) ?
				regster_16[r_m][wbit] :
				addrmod_16[r_m][mode].regs;
		else
			reg_name = (mode == REG_ONLY) ?
				regster_32[r_m][wbit] :
				addrmod_32[r_m][mode].regs;
		if (*reg_name == '\0')
			(void) sprintf(operand[opindex], "%s", gname);
		else
			(void) sprintf(operand[opindex],"%s%s",
				operand[opindex], reg_name);
	} else {
		char buffer[16];
		char obuffer[16];

		/* get the base register from addrmod_32;
		 * we may have to strip off leading/tailing parens */
		strcpy(obuffer, addrmod_32[base][mode].regs);

		if (obuffer[0] == '(') {
			reg_name = obuffer+1;
			reg_name[strlen(reg_name)-1] = '\0';
		} else
			reg_name = obuffer;
		if (*index_reg[index] == '\0' && ss == 0)
			(void) sprintf(buffer, "(%s)", reg_name);
		else
			(void) sprintf(buffer, "(%s%s%s)", reg_name,
				index_reg[index], scale_factor[ss]);
		reg_name = buffer;
		(void) sprintf(operand[opindex],"%s%s",
			operand[opindex], reg_name);
	}
}

/*ftg imm_data () */
/*
 *	void imm_data (no_bytes)
 *
 *	Determine if 1, 2 or 4 bytes of immediate data are needed, then
 *	get and print them.  The bytes will be in reverse order
 *	(i.e., 'byte low    byte high ') in the object code.
 */

void
imm_data(no_bytes)
enum byte_count no_bytes;

{
	extern	void	getbyte();	/* from _utls.c */
	extern	void	convert();	/* from _utls.c */
	extern	unsigned short curbyte;	/* from _extn.c */

	unsigned	short	bytelow, bytehigh;
	unsigned	long	lbyte;
	char		temphigh[NCPS+1];
	char		templow[NCPS+1];

	getbyte();
	bytelow = curbyte;

	switch (no_bytes) {
	case ONE:
		convert(bytelow, templow, LEAD);
		(void) sprintf(operand[opindex],"$%s",templow);
		break;

	case TWO:
		getbyte();
		bytehigh = curbyte;
		bytehigh = (bytehigh << 8) | bytelow;
		convert(bytehigh, temphigh, LEAD);
		(void) sprintf(operand[opindex],"$%s",temphigh);
		break;

	case FOUR:
		getbyte();
		bytehigh = curbyte;
		lbyte = (bytehigh << 8) | bytelow;

		getbyte();
		bytelow = curbyte;
		getbyte();
		bytehigh = curbyte;
		bytehigh = (bytehigh << 8) | bytelow;
		lbyte = (bytehigh << 16 ) | lbyte;
		lconvert(lbyte, temphigh, LEAD);
		(void) sprintf(operand[opindex],"$%s",temphigh);
	}
}

/*ftg get_opcode () */
/*
 *	get_opcode (high, low)
 *
 *	Get the next byte and separate the op code into the high and
 *	low nibbles.
 */

void
get_opcode(high, low)
unsigned *high;
unsigned *low;		/* low 4 bits of op code   */

{

	extern	unsigned short curbyte;		/* from _extn.c */
	unsigned short	byte;

	getbyte();
	byte = curbyte;
	*low = LOW4(byte);
	*high = BITS7_4(byte);
}

/*
 *	getbyte ()
 *
 *	read a byte, mask it, then return the result in 'curbyte'.
 *	The getting of all single bytes is done here. 
 */
void
getbyte()

{
	extern	unsigned short curbyte;	/* from _extn.c */
	char	temp[NCPS+1];
	char	byte;
	
	byte = (*_386iread)(_386i_addr++);
	curbyte = byte & 0377;
#ifdef	notdef
	convert(curbyte, temp, NOLEAD);
	(void) sprintf(object,"%s%s ",object,temp);
#endif
}

/*
 *	compoff (lng, temp)
 *
 *	This routine will compute the location to which control is to be
 *	transferred.  'lng' is the number indicating the jump amount
 *	(already in proper form, meaning masked and negated if necessary)
 *	and 'temp' is a character array to store the jump name/offset.
 */

void
compoff(lng, temp)
long	lng;
char	*temp;
{
	char *symn = NULL;

	lng += _386i_addr;
	if (_386xlate_name != NULL)
		symn = (*_386xlate_name)(lng);
	if (symn != NULL)
		(void) sprintf(temp,"%s",symn);
	else
		(void) sprintf(temp,"0x%x",lng);
}

/*
 *	convert (num, temp, flag)
 *
 *	Convert the passed number to hex leaving the result in the 
 *	supplied string array.
 *	If  LEAD  is specified, preceed the number with '0x' to
 *	indicate the base (used for information going to the mnemonic
 *	printout).  NOLEAD  will be used for all other printing (for
 *	printing the offset, object code, and the second byte in two
 *	byte immediates, displacements, etc.) and will assure that
 *	there are leading zeros.
 */

void
convert(num,temp,flag)
unsigned short num;
char	temp[];
int	flag;

{
	/* we assume signed short here ... */
	short t;
	char *sign = "";

#ifdef	notdef
	if (flag == NOLEAD) 
		(void) sprintf(temp,"%02x",num);
#endif
	t = num;
	if (flag == LEAD) {
		if ((t & 0xFF00) == 0 && (t & 0x80) != 0)  {
			/* negative byte */
			sign = "-";
			t |= 0xFF00;
			t = -t;
		}
		if (t & 0x8000) {
			/* negative short */
			sign = "-";
			t = -t;
		}
		if (t > 9)
			(void) sprintf(temp,"%s0x%x", sign, t);
		else
			(void) sprintf(temp,"%s%x", sign, t);
	}
}

/*
 *	lconvert (num, temp, flag)
 *
 *	lconvert is identical to convert, except that it converts
 *	a long instead of a short.
 */

void
lconvert(num,temp,flag)
unsigned long	num;
char	temp[];
int	flag;

{
	/* we assume unsigned int here */
	char *symn = NULL;

	if (flag == LEAD) {
		if (num > 9)
			(void) sprintf(temp,"0x%x", num);
		else
			(void) sprintf(temp,"%x", num);
	}
	if (_386xlate_name != NULL)
		symn = (*_386xlate_name)(num);
	if (symn != NULL)
		(void) sprintf(gname,"%s", symn);
	else
		(void) sprintf(gname,"%s", temp);
}

/* 
 * Decode WEITEK instructions
 * Returns 2nd WEITEK register name or 
 * NULL if a 386 register is used.
 */
char *
decode_1167(op1, op2, addr)
	int op1;
	int op2;
	int addr;
{
	register char *s;
	register int src1, src2;
	int opcode;
	char *reg;

	/* Is this a reference to the WTL 1167? */
	if (
	     ( op1 != 0x88 && op1 != 0x89 && op1 != 0x8b &&
	       op1 != 0xa1 && op1 != 0xa2 && op1 != 0xa3 && op1 != 0xc7 )
	  || ( op2 & 0x07 ) != 0x5			/* r/m = 101 */
	  || ( op2 & 0xc0 ) != 0 			/* mod = 00  */
	  || ( (addr & 0xffff0000) != 0xffc00000 &&
	       (addr & 0xffff0000) != 0xc0000000 )
	   )
	{
		return (NULL);
	}
	/*
	 * Crack address into opcode, src1, and src2
	 */
	src1 = ((addr >> 5) & 0x1c) | (addr & 0x3);
	src2 = (addr >> 2) & 0x1f;
	opcode = (addr >> 10) & 0x3f;
	/* 
	 * Opcode name 
	 */
	switch ( opcode ) {
	case 0x00:	/* add.s */	s = "wadds"; break;
	case 0x20:	/* add.d */	s = "waddl"; break;
	case 0x01:	/* load  */	s = "wloads"; break;
	case 0x21:	/* load  */	s = "wloadl"; break;
	case 0x02:	/* mul.s */	s = "wmuls"; break;
	case 0x22:	/* mul.d */	s = "wmull"; break;
	case 0x03:	/* stor  */	s = "wstors"; break;
	case 0x23:	/* stor  */	s = "wstorl"; break;
	case 0x04:	/* sub.s */	s = "wsubs"; break;
	case 0x24:	/* sub.d */	s = "wsubl"; break;
	case 0x05:	/* div.s */	s = "wdivs"; break;
	case 0x25:	/* div.d */	s = "wdivl"; break;
	case 0x06:	/* muln.s */	s = "wmulns"; break;
	case 0x26:	/* muln.d */	s = "wmulnl"; break;
	case 0x07:	/* float.s */	s = "wfloats"; break;
	case 0x27:	/* float.d */	s = "wfloatl"; break;
	case 0x08:	/* cmpt.s */	s = "wcmpts"; break;
	case 0x28:	/* cmpt.d */	s = "wcmptl"; break;
	case 0x09:	/* tstt.s */	s = "wtstts"; break;
	case 0x29:	/* tstt.d */	s = "wtsttl"; break;
	case 0x0A:	/* neg.s */	s = "wnegs"; break;
	case 0x2A:	/* neg.d */	s = "wnegl"; break;
	case 0x0B:	/* abs.s */	s = "wabss"; break;
	case 0x2B:	/* abs.d */	s = "wabsl"; break;
	case 0x0C:	/* cmp.s */	s = "wcmps"; break;
	case 0x2C:	/* cmp.d */	s = "wcmpl"; break;
	case 0x0D:	/* tst.s */	s = "wtsts"; break;
	case 0x2D:	/* tst.d */	s = "wtstl"; break;
	case 0x0E:	/* amul.s */	s = "wamuls"; break;
	case 0x2E:	/* amul.d */	s = "wamull"; break;
	case 0x0F:	/* fix.s */	s = "wfixs"; break;
	case 0x2F:	/* fix.d */	s = "wfixl"; break;
	case 0x10:	/* cvts.d */	s = "wcvtsl"; break;
	case 0x11:	/* cvtd.s */	s = "wcvtls"; break;
	case 0x30:	/* ldctx */	s = "wldctx"; break;
	case 0x31:	/* stctx */	s = "wstctx"; break;
	case 0x12:	/* mac.s */	s = "wmacs"; break;
	case 0x32:	/* macd.s */	s = "wmacls"; break;
	default:
		return (NULL);
	}
	(void) sprintf(_386i_str, "%-8s", s);
	if ( src1 ) {
		/* weitek register to weitek register operation */
		static char regname[8];
		(void) sprintf(reg = regname, "%%fp%d", src1);
	} else
		reg = NULL;
	/*
	 * If called from IMw code, we need to put the reg name in the
	 * right slot.  The RMMR code, on the other hand, knows how to
	 * shuffle operands as needed.
	 */
	if (op1 == 0xc7)
		(void) sprintf(operand[2], "%%fp%d", src2);
	else
		(void) sprintf(operand[0], "%%fp%d", src2);
	return (reg);
}

/*
 * We have either a plain 386 instruction or mov instruction that
 * contains a WTL 1167 instruction.  The opcode and operands
 * have already been defined; we just stick them together in the
 * right order and check for certain 1167isms.
 */
void
emit_386_or_1167 (reg_name, reg, AD_mode, dbit, wbit)
	char		*reg_name;	/* null unless defined by decode_1167() */
	unsigned	reg;		/* i386 register number */
	int		AD_mode;	/* 16 or 32 -- select %ax or %eax */
	int		dbit;		/* RM or MR -- selects direction of mov */
	int		wbit;		/* Select byte or word/long operation. */
{
	if (reg_name == NULL) {
		if (AD_mode==16)
			reg_name = regster_16[reg][wbit];
		else
			reg_name = regster_32[reg][wbit];
	}
	if ( strncmp(_386i_str, "wtst", 4) == 0 
		||  strncmp(_386i_str, "wldctx", 6) == 0
		||  strncmp(_386i_str, "wstctx", 6) == 0 ) {
		(void) sprintf(_386i_str,"%s%s",_386i_str,reg_name);
	} else {
		if (dbit == RM)
			(void) sprintf(_386i_str,"%s%s,%s",_386i_str,reg_name,operand[0]);
		else
			(void) sprintf(_386i_str,"%s%s,%s",_386i_str,operand[0],reg_name);
	}
}
