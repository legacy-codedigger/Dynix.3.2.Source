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

/* $Header: ex_tune.h 2.6 90/06/05 $ */

/*	ex_tune.h	7.5	83/07/02	*/
/* Copyright (c) 1981 Regents of the University of California */
/*
 * Definitions of editor parameters and limits
 */

/*
 * Pathnames.
 *
 * Only exstrings is looked at "+4", i.e. if you give
 * "/usr/lib/..." here, "/lib" will be tried only for strings.
 */
#define libpath(file) "/usr/lib/file"
#define loclibpath(file) "/usr/local/lib/file"
#define binpath(file) "/usr/ucb/file"
#define usrpath(file) "/usr/file"
#define E_TERMCAP	"/etc/termcap"
#define B_CSH		"/bin/csh"
#define	EXRECOVER	libpath(ex3.7recover)
#define	EXPRESERVE	libpath(ex3.7preserve)
#ifndef VMUNIX
#define	EXSTRINGS	libpath(ex3.7strings)
#endif

/*
 * If your system believes that tabs expand to a width other than
 * 8 then your makefile should cc with -DTABS=whatever, otherwise we use 8.
 */
#ifndef TABS
#define	TABS	8
#endif

/*
 * Maximums
 *
 * The definition of LBSIZE should be the same as BUFSIZ (512 usually).
 * Most other definitions are quite generous.
 */
/* FNSIZE is also defined in expreserve.c */
#define	FNSIZE		MAXPATHLEN	/* File name size */
#ifdef VMUNIX
#define	LBSIZE		1024
#define	ESIZE		512
#define CRSIZE		1024
#else
#ifdef u370
#define LBSIZE		4096
#define ESIZE		512
#define CRSIZE		4096
#else
#define	LBSIZE		512		/* Line length */
#define	ESIZE		128		/* Size of compiled re */
#define CRSIZE		512
#endif
#endif
#define	RHSSIZE		256		/* Size of rhs of substitute */
#define	NBRA		9		/* Number of re \( \) pairs */
#define	TAGSIZE		128		/* Tag length */
#define	ONMSZ		512		/* Option name size */
#define	GBSIZE		256		/* Buffer size */
#define	UXBSIZE		128		/* Unix command buffer size */
#define	VBSIZE		128		/* Partial line max size in visual */

/*
 * Why a VIBSIZE?  Originally, vi was set up with buffers capable of holding
 * the largest "desirable" I/O--MAXBSIZE.  It would then adapt to smaller
 * values by looking at the st_blksize field of various files.  This worked
 * fine until (1) it looked at files over NFS, and (2) when it first hit
 * the 64K st_blksize of CD/ROM file systems.  So now it protects itself
 * against values larger than VIBSIZE, and makes the BSIZE more generous.
 * Since this value only effects the size of genbuf[], its impact on overall
 * memory usage should be small.
 */
#define VIBSIZE		(64*1024)	/* Max I/O buffer size for vi/ex */

/* LBLKS is also defined in expreserve.c */
#ifndef VMUNIX
#define	LBLKS		125		/* Line pointer blocks in temp file */
#define	HBLKS		1		/* struct header fits in BUFSIZ*HBLKS */
#else
#define	LBLKS		900
#define	HBLKS		3
#endif
#define	MAXDIRT		12		/* Max dirtcnt before sync tfile */
#define TCBUFSIZE	1024		/* Max entry size in termcap, see
					   also termlib and termcap */

/*
 * Except on VMUNIX, these are a ridiculously small due to the
 * lousy arglist processing implementation which fixes core
 * proportional to them.  Argv (and hence NARGS) is really unnecessary,
 * and argument character space not needed except when
 * arguments exist.  Argument lists should be saved before the "zero"
 * of the incore line information and could then
 * be reasonably large.
 */
#undef NCARGS
#ifndef VMUNIX
#define	NARGS	100		/* Maximum number of names in "next" */
#define	NCARGS	LBSIZE		/* Maximum arglist chars in "next" */
#else
#define	NCARGS	5120
#define	NARGS	(NCARGS/6)
#endif

/*
 * Note: because the routine "alloca" is not portable, TUBESIZE
 * bytes are allocated on the stack each time you go into visual
 * and then never freed by the system.  Thus if you have no terminals
 * which are larger than 24 * 80 you may well want to make TUBESIZE
 * smaller.  TUBECOLS should stay at 160 since this defines the maximum
 * length of opening on hardcopies and allows two lines of open on
 * terminals like adm3's (glass tty's) where it switches to pseudo
 * hardcopy mode when a line gets longer than 80 characters.
 * (TUBECOLS has been increased to support vi running on large windows on
 * bitmapped displays.  Maintaining compatability on harcopies was not seen
 * as being vital.	-davest@sequent).
 */
#ifndef VMUNIX
#define	TUBELINES	60	/* Number of screen lines for visual */
#define	TUBECOLS	160	/* Number of screen columns for visual */
#define	TUBESIZE	5000	/* Maximum screen size for visual */
#else
#define	TUBELINES	106
#define	TUBECOLS	210
#define	TUBESIZE	16960	/* 106 * 160 for X */
#endif

/*
 * Output column (and line) are set to this value on cursor addressible
 * terminals when we lose track of the cursor to force cursor
 * addressing to occur.
 */
#define	UKCOL		-20	/* Prototype unknown column */

/*
 * Attention is the interrupt character (normally 0177 -- delete).
 * Quit is the quit signal (normally FS -- control-\) and quits open/visual.
 */
#define	ATTN	(-2)	/* mjm: (char) ??  */
#define	QUIT	('\\' & 037)
