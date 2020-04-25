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
 * $Header: psl.h 2.1 86/02/13 $
 *
 * psl.h
 *	Definitions of processor status word(s).
 *
 * The name "psl" is historic.  The register is called "flags" on i386.
 */

/* $Log:	psl.h,v $
 */

/*
 * Intel 80386 Flags Register.
 */

#define	FLAGS_CF	0x00000001	/* carry flag */
#define	FLAGS_PF	0x00000004	/* parity even flag */
#define	FLAGS_AF	0x00000010	/* "ascii" carry flag */
#define	FLAGS_ZF	0x00000040	/* zero result flag */
#define	FLAGS_SF	0x00000080	/* signed result flag */
#define	FLAGS_TF	0x00000100	/* single-step trap flag */
#define	FLAGS_IF	0x00000200	/* interrupt enable flag */
#define	FLAGS_DF	0x00000400	/* direction flag */
#define	FLAGS_OF	0x00000800	/* overflow flag */
#define	FLAGS_IOPL	0x00003000	/* IO privelege level mask */
#define	FLAGS_NT	0x00004000	/* nested task flag */
#define	FLAGS_RF	0x00010000	/* retry flag (coord fault, debug) */
#define	FLAGS_VM	0x00020000	/* virtual 8086 mode */
#define	FLAGS_RSVD	0xFFFC802A	/* reserved bits -- don't use */

#define	KERNEL_IOPL	0x00000000	/* kernel runs at IOPL 0 */
#define	USER_IOPL	0x00003000	/* user runs at IOPL 3 */
