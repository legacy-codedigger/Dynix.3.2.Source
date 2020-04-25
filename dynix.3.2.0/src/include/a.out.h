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

/* $Header: a.out.h 2.2 86/05/09 $
 *
 * a.out.h
 *	a.out header and format definitions.
 *
 * A.out file contains (in order):
 *	header			; see exec.h
 *	text			; a_text bytes
 *	init'd private data	; a_data bytes
 *	init'd shared data	; a_shdata bytes
 *	text reloc info		; a_trsize bytes	
 *	priv data reloc info	; a_drsize bytes	
 *	shared data reloc info	; a_shdrsize bytes	
 *	symbol table		; a_syms bytes
 *	string table		; self defining
 */

#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/exec.h>

/*
 * Macros to determine validity of object file and offsets in a file.
 *
 * In an executable a.out, a_magic determines how position in the file
 * relates to execution address: ZMAGIC and XMAGIC place position 0 in the
 * file at address EXECPGRND in memory; SMAGIC uses same file and execution
 * addresses.  a_text encodes the virtual (execution) size of the text,
 * not necessarily the size in the file.  Header is included in the
 * text.  N_ADDRADJ() encodes this.
 *
 * In a non-executable a.out (OMAGIC), text starts after header.
 *
 * N_MINSIZ(x) gives minimum size for legal executable.
 */

#define	EXECPGRND	(LOWPAGES*NBPG)

#define	N_BADMAG(x) \
    (((x).a_magic)!=OMAGIC && ((x).a_magic)!=ZMAGIC && \
     ((x).a_magic)!=XMAGIC && ((x).a_magic)!=SMAGIC)

#define N_ADDRADJ(x) \
	(((x).a_magic == ZMAGIC || (x).a_magic == XMAGIC) ? EXECPGRND : 0)

#define	N_TXTOFF(x)	((x).a_magic == OMAGIC ? sizeof (struct exec) : 0)
#define N_DATAOFF(x)	(N_TXTOFF(x) + (x).a_text - N_ADDRADJ(x))
#define N_SHDATAOFF(x)	(N_DATAOFF(x) + (x).a_data)
#define N_TROFF(x)	(N_SHDATAOFF(x) + (x).a_shdata)
#define N_DROFF(x)	(N_TROFF(x) + (x).a_trsize)
#define N_SHDROFF(x)	(N_DROFF(x) + (x).a_drsize)
#define N_SYMOFF(x)	(N_SHDROFF(x) + (x).a_shdrsize)
#define	N_STROFF(x)	(N_SYMOFF(x) + (x).a_syms)

#define N_MINSIZ(x)	N_TROFF(x)

/*
 * Format of a relocation datum.
 */

struct relocation_info {
	int	     r_address;		/* address which is relocated */
	unsigned int r_symbolnum:24, 	/* local symbol ordinal */
		     r_pcrel:1, 	/* was relocated pc relative already */
		     r_length:2, 	/* 0=byte, 1=word, 2=long */
		     r_extern:1, 	/* doesn't include value of sym ref'd */
		     r_bsr:1,	 	/* this is an entry for a bsr dest. */
		     r_disp:1,	 	/* the value is a displacemnt */
			  :2;		/* nothing, yet */
};

/*
 * Format of a symbol table entry.
 */

struct nlist {
	union {
		char *n_name;		/* for use when in-core */
		long n_strx;		/* index into file string table */
	} n_un;
	unsigned char n_type;		/* type flag, N_TEXT etc; see below */
	char	n_other;		/* unused */
	short	n_desc;			/* see <stab.h> */
	unsigned long	n_value;	/* value of symbol (or sdb offset) */
};
#define	n_hash		n_desc		/* used internally by ld */

/*
 * Simple values for n_type.
 * Careful if change these definitions; N_SHARED is used as an "attribute"
 * for efficiency in some tools (eg, ld).
 */

#define	N_UNDF		0x00		/* undefined */
#define	N_ABS		0x02		/* absolute */
#define	N_TEXT		0x04		/* text (implicitly shared) */
#define	N_DATA		0x06		/* private data */
#define	N_BSS		0x08		/* private bss */
#define	N_COMM		0x0a		/* common (internal to ld) */
#define	N_FN		0x0c		/* file-name */

#define	N_SHARED	0x10		/* shared N_UNDF, N_DATA, N_BSS */
#define	N_SHUNDF	(N_SHARED|N_UNDF)
#define	N_SHDATA	(N_SHARED|N_DATA)
#define	N_SHBSS		(N_SHARED|N_BSS)
#define	N_SHCOMM	(N_SHARED|N_COMM)

#define	N_EXT		0x01		/* external bit, or'ed in */
#define	N_TYPE		0x1e		/* mask for all the type bits */

/*
 * Sdb entries have some of the N_STAB bits set.
 * These are given in <stab.h>
 */

#define	N_STAB		0xe0		/* if any of these bits set, a SDB entry */

/*
 * Format for namelist values.
 */

#define	N_FORMAT	"%08x"
