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

/* $Id: pc.h,v 1.1 88/09/02 11:48:15 ksb Exp $ */

#include <setjmp.h>

    /*
     *	random constants for pc
     */
    
    /*
     *	the name of the display.
     */
#define	DISPLAYNAME	"__disply"
typedef char *savedfp_t;
typedef savedfp_t display[DSPLYSZ];

    /*
     *	the structure below describes the locals used by the run time system.
     *	at function entry, at least this much space is allocated,
     *	and the following information is filled in:
     *	the address of a routine to close the current frame for unwinding,
     *	a pointer to the display entry for the current static level and
     *	the previous contents of the display for this static level.
     *	the curfile location is used to point to the currently active file,
     *	and is filled in as io is initiated.
     *	one of these structures is allocated on the (negatively growing) stack.
     *	at function entry, fp is set to point to the last field of the struct,
     *	thus the offsets of the fields are as indicated below.
     */
struct rtlocals {
    jmp_buf		gotoenv;
    struct iorec	*curfile;
    savedfp_t		savedfp;
} rtlocs;
#define	GOTOENVOFFSET	( -sizeof rtlocs )
#define	CURFILEOFFSET	( GOTOENVOFFSET + sizeof rtlocs.gotoenv )
#define	DSAVEOFFSET	( CURFILEOFFSET + sizeof rtlocs.curfile )

    /*
     *	this is a cookie used to communicate between the
     *	routine entry code and the routine exit code.
     *	mostly it's for labels shared between the two.
     */
#if defined(i386)
struct entry_exit_cookie {
    struct nl	*nlp;
    char	extname[BUFSIZ];
    int		toplabel;
    int		savlabel;
};
#define	FRAME_SIZE_LABEL	"LF"
#define	SAVE_MASK_LABEL		".L"
#endif /* i386 */

    /*
     *	formats for various names
     *	    NAMEFORMAT		arbitrary length strings.
     *	    EXTFORMAT		for externals, a preceding underscore.
     *	    LABELFORMAT		for label names, a preceding dollar-sign.
     *	    PREFIXFORMAT	used to print made up names with prefixes.
     *	    LABELPREFIX		with getlab() makes up label names.
     *	    LLABELPREFIX	with getlab() makes up sdb labels.
     *	    FORMALPREFIX	prefix for EXTFORMAT for formal entry points.
     *	a typical use might be to print out a name with a preceeding underscore
     *	with putprintf( EXTFORMAT , 0 , name );
     */
#define	NAMEFORMAT	"%s"
#define	EXTFORMAT	"_%s"
#define	LABELFORMAT	"$%s"
#define	PREFIXFORMAT	"%s%d"
#define	LABELPREFIX	"L"
#define	LLABELPREFIX	"LL"
#define	FORMALPREFIX	"__"

    /*
     *	the name of the statement counter
     */
#define	STMTCOUNT	"__stcnt"

    /*
     *	the name of the pcp counters
     */
#define	PCPCOUNT	"__pcpcount"

    /*
     *	a vector of pointer to enclosing functions for fully qualified names.
     */
char	*enclosing[ DSPLYSZ ];

#if defined(i386)
    /*
     *	the runtime framepointer and argumentpointer registers
     */
#define	P2FP		6
#define	P2FPNAME	"%ebp"
#define	P2AP		6
#define	P2APNAME	"%ebp"

/*
 *  Maximum register number
 */
#define	MAXREGNUM	23		/* ZZZ */

/*
 *	runtime check mask for divide check and integer overflow
 * ZZZZ
 */
#define	RUNCHECK	( ( 1 << 15 ) | ( 1 << 14 ) )

/*
 *	and of course ...
 */
#define	BITSPERBYTE	8
#endif /* i386 */
