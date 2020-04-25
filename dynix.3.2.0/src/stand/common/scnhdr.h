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

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: scnhdr.h 1.1 90/11/06 $"

#ifndef	_SYS_SCNHDR_H_


struct scnhdr {
	char		s_name[8];	/* section name */
	long		s_paddr;	/* physical address, aliased s_nlib */
	long		s_vaddr;	/* virtual address */
	long		s_size;		/* section size */
	long		s_scnptr;	/* file ptr to raw data for section */
	long		s_relptr;	/* file ptr to relocation */
	long		s_lnnoptr;	/* file ptr to line numbers */
	unsigned short	s_nreloc;	/* number of relocation entries */
	unsigned short	s_nlnno;	/* number of line number entries */
	long		s_flags;	/* flags */
	};

/* the number of shared libraries in a .lib section in an absolute output file
 * is put in the s_paddr field of the .lib section header, the following define
 * allows it to be referenced as s_nlib
 */

#define s_nlib	s_paddr
#define	SCNHDR	struct scnhdr
#define	SCNHSZ	sizeof(SCNHDR)




/*
 * Define constants for names of "special" sections
 */

#define _TEXT ".text"
#define _DATA ".data"
#define _BSS  ".bss"
#define _TV  ".tv"



/*
 * The low 2 bytes of s_flags is used as a section "type"
 */

#define STYP_REG	0x00		/* "regular" section:
						allocated, relocated, loaded */
#define STYP_DSECT	0x01		/* "dummy" section:
						not allocated, relocated,
						not loaded */
#define STYP_NOLOAD	0x02		/* "noload" section:
						allocated, relocated,
						 not loaded */
#define STYP_GROUP	0x04		/* "grouped" section:
						formed of input sections */
#define STYP_PAD	0x08		/* "padding" section:
						not allocated, not relocated,
						 loaded */
#define STYP_COPY	0x10		/* "copy" section:
						for decision function used
						by field update;  not
						allocated, not relocated,
						loaded;  reloc & lineno
						entries processed normally */
#define	STYP_TEXT	0x20		/* section contains text only */
#define STYP_DATA	0x40		/* section contains data only */
#define STYP_BSS	0x80		/* section contains bss only */


#define	_SYS_SCNHDR_H_
#endif	/* _SYS_SCNHDR_H_ */
