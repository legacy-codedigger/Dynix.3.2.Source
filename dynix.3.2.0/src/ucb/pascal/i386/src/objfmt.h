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

/* $Id: objfmt.h,v 1.1 88/09/02 11:48:12 ksb Exp $ */

/*
 * The size of the display.
 */
#define DSPLYSZ 20

/*
 *	The structure of the runtime display
 */
#if defined(PC)
#if defined(i386)

#define	AP_OFFSET	(0)
#define	FP_OFFSET	(0)	/* %ebp */
#endif i386

    /*
     *	the structure below describes the block mark used by the architecture.
     *	this is the space used by the machine between the arguments and the
     *	whatever is used to point to the arguments.
     */
#if defined(i386)
	/*
	 *	there's the saved pc (from the call)
	 *	and the saved fp (from the %epb).
	 */
struct blockmark {
	char	*savedfp;
	char	*savedpc;
};
#endif /* i386 */
#endif PC

#if !defined(PC)
struct formalrtn	frtn;
#endif

#define	FENTRYOFFSET	0
#define FBNOFFSET	( FENTRYOFFSET + sizeof frtn.fentryaddr )
#define	FDISPOFFSET	( FBNOFFSET + sizeof frtn.fbn )


	    /*
	     *	these are because of varying sizes of pointers
	     */
#if defined(ADDR16)
#define PTR_AS O_AS2
#define PTR_RV O_RV2
#define PTR_IND O_IND2
#define PTR_CON O_CON2
#define PTR_DUP O_SDUP2
#define CON_INT O_CON2
#define INT_TYP (nl + T2INT)
#define PTR_DCL char *
#define TOOMUCH 50000
#define SHORTADDR 65536
#define MAXSET 65536		/* maximum set size */
#endif ADDR16
#if defined(ADDR32)
#define PTR_AS O_AS4
#define PTR_RV O_RV4
#define PTR_IND O_IND4
#define PTR_CON O_CON4
#define PTR_DUP O_SDUP4
#define CON_INT O_CON24
#define INT_TYP (nl + T4INT)
#define PTR_DCL unsigned long		/* for pointer variables */
#define SHORTADDR 32768			/* maximum short address */
#define TOOMUCH 65536			/* maximum variable size */
#define MAXSET 65536			/* maximum set size */
#endif ADDR32
	/*
	 * Offsets due to the structure of the runtime stack.
	 * DPOFF1	is the amount of fixed storage in each block allocated
	 * 		as local variables for the runtime system.
	 *		since locals are allocated negative offsets,
	 *		-DPOFF1 is the last used implicit local offset.
	 * DPOFF2	is the size of the block mark.
	 *		since arguments are allocated positive offsets,
	 *		DPOFF2 is the end of the implicit arguments.
	 *		for obj, the first argument has the highest offset
	 *		from the stackpointer.  and the block mark is an
	 *		implicit last parameter.
	 *		for pc, the first argument has the lowest offset
	 *		from the argumentpointer.  and the block mark is an
	 *		implicit first parameter.
	 */
#if defined(PC)
#define DPOFF1	( sizeof (struct rtlocals) )
#define DPOFF2	( sizeof (struct blockmark) )
#define INPUT_OFF	0
#define OUTPUT_OFF	0
#endif PC
