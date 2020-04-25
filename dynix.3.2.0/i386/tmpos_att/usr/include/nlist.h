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

/*
 * $Header: nlist.h 2.1 86/03/11 $
 */

/* $Log:	nlist.h,v $
 */

/*	nlist.h	4.1	83/05/03	*/

/*
 * Format of a symbol table entry; this file is included by <a.out.h>
 * and should be used if you aren't interested the a.out header
 * or relocation information.
 */
struct	nlist {
	char	*n_name;	/* for use when in-core */
	unsigned char n_type;	/* type flag, i.e. N_TEXT etc; see below */
	char	n_other;	/* unused */
	short	n_desc;		/* see <stab.h> */
	unsigned long n_value;	/* value of this symbol (or sdb offset) */
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
