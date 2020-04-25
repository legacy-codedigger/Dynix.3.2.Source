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

/* $Header: dis.h 1.4 86/08/19 $ */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)ccs-s5:dis/i286/dis.h	1.1"
/*
 */

/*	Modified March 84 to add 186 instructions; jws */

/*
 *	This is the header file for the iapx disassembler.
 *	The information contained in the first part of the file
 *	is common to each version, while the last part is dependent
 *	on the particular machine architecture being used.
 */

#define		NCPS	8	/* number of chars per symbol	*/
#define		NHEX	40	/* max # chars in object per line	*/
#define		NLINE	36	/* max # chars in mnemonic per line	*/
#define		FAIL	0
#define		TRUE	1
#define		FALSE	0
#define		LEAD	1
#define		NOLEAD	0
#define		TERM 0		/* used in _tbls.c to indicate		*/
				/* that the 'indirect' field of the	*/
				/* 'instable' terminates - no pointer.	*/
				/* Is also checked in 'dis_text()' in	*/
				/* _bits.c.				*/

#ifdef AR32W
#define	LNNOBLKSZ	1024	/* size of blocks of line numbers		*/
#define	SYMBLKSZ	1024	/* size if blocks of symbol table entries	*/
#else
#define	LNNOBLKSZ	512	/* size of blocks of line numbers		*/
#define	SYMBLKSZ	512	/* size of blocks of symbol table entries	*/
#endif
#define		STRNGEQ 0	/* used in string compare operation	*/

/*
 *	This is the structure that will be used for storing all the
 *	op code information.  The structure values themselves are
 *	in '_tbls.c'.
 */

struct	instable {
	char		name[NCPS];
	struct instable *indirect;	/* for decode op codes */
	unsigned	adr_mode;
	int		suffix;		/* for instructions which may
					   have a 'w' or 'l' suffix */
};

/*	NOTE:	the following information in this file must be changed
 *		between the different versions of the disassembler.
 *
 *	This structure is used to determine the displacements and registers
 *	used in the addressing modes.  The values are in 'tables.c'.
 */
struct addr {
	int	disp;
	char	regs[9];
};
/*
 *	These are the instruction formats as they appear in
 *	'tables.c'.  Here they are given numerical values
 *	for use in the actual disassembly of an object file.
 */
#define UNKNOWN	0
#define RMMR	1
#define MRw	2
#define IMlw	3
#define IMw	4
#define IR	5
#define OA	6
#define AO	7
#define MS	8
#define SM	9
#define Mv	10
#define Mw	11
#define M	12
#define R	13
#define RA	14
#define SEG	15
#define MR	16
#define IA	17
#define MA	18
#define SD	19
#define AD	20
#define SA	21	/* no longer used */
#define D	22
#define INM	23
#define SO	24
#define BD	25
#define I	26
#define P	27
#define V	28
#define Iv	29
#define U	30
#define OVERRIDE 31
#define GO_ON	32
#define	O	33	/* for call	*/
#define JTAB	34	/* jump table 	*/
#define RMMRI	35	/* for 186 iimul instr  */
#define MvI	37	/* for 186 logicals */
#define	II	38	/* for 186 enter instr  */
#define RMw	39	/* for 286 arpl instr */
#define Ib	40	/* for push immediate byte */
#define	F	41	/* for 287 instructions */
#define	FF	42	/* for 287 instructions */
#define	FILL	0x90	/* Fill byte used for alignment (nop)	*/
#define DM	43	/* 16-bit data */
#define AM	44	/* 16-bit addr */
#define LSEG	45	/* for 3-bit seg reg encoding */
#define	Mc	47	/* Control registers */
#define	Md	48	/* Debug registers */
#define	Mt	49	/* Test registers */
#define	BTi	50	/* bit test with 8 bits of immediate data */
#define RMMRe	51

/*
 *	This is an enumeration of the possible values for byte counts.
 *	Data and addresses may be 1, 2, or 4 bytes in length.
 */

enum byte_count {
	ONE,
	TWO,
	FOUR
};
