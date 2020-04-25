/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
 * $Header: tty.h 2.4 91/03/23 $
 */

/* $Log:	tty.h,v $
 */

#ifdef KERNEL
#include "../h/ttychars.h"
#include "../h/ttydev.h"
#else
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#endif

/*
 * A clist structure is the head of a linked list queue
 * of characters.  The characters are stored in blocks
 * containing a link and CBSIZE (param.h) characters. 
 * The routines in tty_subr.c manipulate these structures.
 */
struct clist {
	int	c_cc;		/* character count */
	char	*c_cf;		/* pointer to first char */
	char	*c_cl;		/* pointer to last char */
};

/*
 * Per-tty structure.
 *
 * Should be split in two, into device and tty drivers.
 * Glue could be masks of what to echo and circular buffer
 * (low, high, timeout).
 * union t_nu removed since tablets/bk stuff unused.
 */
struct tty {
	lock_t 	t_ttylock;		/* tty structure lock */
	struct	clist t_rawq;		/* raw characters or partial line */
	struct	clist t_canq;		/* raw characters or partial line */
	struct	clist t_outq;		/* device */
	sema_t	t_rawqwait;		/* semaphore for rawq sleeps */
	sema_t	t_outqwait;		/* semaphore for outq sleeps */
	int	(*t_oproc)();		/* device */
	struct	proc *t_rsel;		/* tty */
	struct	proc *t_wsel;
	caddr_t	t_addr;			/* ??? */
	dev_t	t_dev;			/* device */
	short	t_nopen;		/* number concurrent opens */
	int	t_flags;		/* some of both */
	int	t_state;		/* some of both */
	int	t_cmask;		/* carrier mask, driver specific */
	short	t_pgrp;			/* tty */
	char	t_line;			/* glue */
	char	t_col;			/* tty */
	char	t_ispeed, t_ospeed;	/* device */
	char	t_rocount, t_rocol;	/* tty */
	struct	ttychars t_chars;	/* tty */
/* be careful of tchars & co. */
#define	t_erase		t_chars.tc_erase
#define	t_kill		t_chars.tc_kill
#define	t_intrc		t_chars.tc_intrc
#define	t_quitc		t_chars.tc_quitc
#define	t_startc	t_chars.tc_startc
#define	t_stopc		t_chars.tc_stopc
#define	t_eofc		t_chars.tc_eofc
#define	t_brkc		t_chars.tc_brkc
#define	t_suspc		t_chars.tc_suspc
#define	t_dsuspc	t_chars.tc_dsuspc
#define	t_rprntc	t_chars.tc_rprntc
#define	t_flushc	t_chars.tc_flushc
#define	t_werasc	t_chars.tc_werasc
#define	t_lnextc	t_chars.tc_lnextc
	struct	winsize t_winsize;
	struct	vnode	*t_vp;		/* tty monitor file */
	char	t_softcarr;		/* soft flags, driver specific */
	char	t_rlitcount;		/* count of raw characters to be ignored */
	char	t_clitcount;		/* count of canonical characters to be ignored */
};

#define	TTIPRI	28
#define	TTOPRI	29

/* limits */
#define	NSPEEDS	16
#define	TTMASK	15
#define	OBUFSIZ	100
#define	IBUFSIZ	100
#define	TTYHOG	255
#ifdef KERNEL
extern	short	tthiwat[], ttlowat[];
#define	TTHIWAT(tp)	tthiwat[(tp)->t_ospeed&TTMASK]
#define	TTLOWAT(tp)	ttlowat[(tp)->t_ospeed&TTMASK]
extern	struct ttychars ttydefaults;
#define	ptsioctl	ptyioctl
#define	ptcioctl	ptyioctl
#endif

/* internal state bits */
#define	TS_TIMEOUT	0x000001	/* delay timeout in progress */
#define	TS_WOPEN	0x000002	/* waiting for open to complete */
#define	TS_ISOPEN	0x000004	/* device is open */
#define	TS_FLUSH	0x000008	/* outq has been flushed during DMA */
#define	TS_CARR_ON	0x000010	/* software copy of carrier-present */
#define	TS_BUSY		0x000020	/* output in progress */
#define	TS_ASLEEP	0x000040	/* wakeup when output done */
#define	TS_XCLUDE	0x000080	/* exclusive-use flag against open */
#define	TS_TTSTOP	0x000100	/* output stopped by ctl-s */
#define	TS_HUPCLS	0x000200	/* hang up upon last close */
#define	TS_TBLOCK	0x000400	/* tandem queue blocked */
#define	TS_RCOLL	0x000800	/* collision in read select */
#define	TS_WCOLL	0x001000	/* collision in write select */
#define	TS_NBIO		0x002000	/* tty in non-blocking mode */
#define	TS_ASYNC	0x004000	/* tty in async i/o mode */
#define	TS_LCLOSE	0x008000	/* last close in progress */
/* state for intra-line fancy editing work */
#define	TS_BKSL		0x010000	/* state for lowercase \ work */
#define	TS_QUOT		0x020000	/* last character input was \ */
#define	TS_ERASE	0x040000	/* within a \.../ for PRTRUB */
#define	TS_LNCH		0x080000	/* next character is literal */
#define	TS_TYPEN	0x100000	/* retyping suspended input (PENDIN) */
#define	TS_CNTTB	0x200000	/* counting tab width; leave FLUSHO alone */
#define TS_LITCHR	0x400000	/* current char is literal */

#define	TS_LOCAL	(TS_BKSL|TS_QUOT|TS_ERASE|TS_LNCH|TS_TYPEN|TS_CNTTB)

/* define partab character types */
#define	ORDINARY	0
#define	CONTROL		1
#define	BACKSPACE	2
#define	NEWLINE		3
#define	TAB		4
#define	VTAB		5
#define	RETURN		6

/* defines for t_softcarr byte field */
#define SOFT_CARR	0x01	/* Ignore h/w carrier state info */
