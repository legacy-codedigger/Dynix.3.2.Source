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

/* $Header: exec.h 2.4 87/06/30 $
 */

/* $Log:	exec.h,v $
 */

/*
 * Header prepended to each a.out file.
 * Kernel only needs a subset.
 */

struct exec {
	long	      a_magic;		/* magic number */
	unsigned long a_text;		/* size of text segment */
	unsigned long a_data;		/* size of initialized data */
	unsigned long a_bss;		/* size of uninitialized data */
	unsigned long a_syms;		/* size of symbol table */
	unsigned long a_entry;		/* entry point */
#ifndef	KERNEL
	unsigned long a_trsize;		/* size of text relocation */
	unsigned long a_drsize;		/* size of data relocation */
	struct gdtbl {			/* Global Descriptor Table */
		unsigned long g_code[2];
		unsigned long g_data[2];
		unsigned long g_desc[2];
	} a_gdtbl;
	/*
	 * Shared data fields.  Currently ignored by exec, supported
	 * by assembler, linker, and libraries.
	 */
	unsigned long a_shdata;		/* size of initialized shared data */
	unsigned long a_shbss;		/* size of uninitialized shared data */
	unsigned long a_shdrsize;	/* size of shared data relocation */

	unsigned long a_bootstrap[11];	/* bootstrap for standalone */
	unsigned long a_reserved[3];	/* reserved for future use */
	unsigned long a_version;	/* object version */
#endif	KERNEL
};

#define	OMAGIC		0x12eb		/* impure format - for .o's */
#define	ZMAGIC		0x22eb		/* demand load format - zero at zero */
#define	XMAGIC		0x32eb		/* demand load format - invalid zero */
#define	SMAGIC		0x42eb		/* demand load format - standalone */

#ifdef	KERNEL
#define	SHSIZE	32
#endif	KERNEL
