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
 * $Header: jioctl.h 2.1 86/02/05 $
 */

/* $Log:	jioctl.h,v $
 */

/*
 *	Unix to Blit I/O control codes
 */

#ifndef _JIOCTL_
#define	_JIOCTL_
#ifdef KERNEL
#include "ioctl.h"
#else
#include <sys/ioctl.h>
#endif

#define JSMPX		TIOCUCNTL
#define	JMPX		_IO(u, 0)
#define	JBOOT		_IO(u, 1)
#define	JTERM		_IO(u, 2)
#define	JTIMO		_IO(u, 4)	/* Timeouts in seconds */
#define	JTIMOM		_IO(u, 6)	/* Timeouts in milliseconds */
#define	JZOMBOOT	_IO(u, 7)
#define JWINSIZE	TIOCGWINSZ
#define JSWINSIZE	TIOCSWINSZ

/* Channel 0 control message format */

struct jerqmesg {
	char	jm_cmd;		/* A control code above */
	char	jm_chan;	/* Channel it refers to */
};

/*
 * Character-driven state machine information for Blit to Unix communication.
 */

#define	C_SENDCHAR	1	/* Send character to layer process */
#define	C_NEW		2	/* Create new layer process group */
#define	C_UNBLK		3	/* Unblock layer process */
#define	C_DELETE	4	/* Delete layer process group */
#define	C_EXIT		5	/* Exit */
#define	C_BRAINDEATH	6	/* Send terminate signal to proc. group */
#define	C_SENDNCHARS	7	/* Send several characters to layer proc. */
#define	C_RESHAPE	8	/* Layer has been reshaped */
#define	C_JAGENT	9	/* Jagent return (what do they mean?) */
#define	C_EMPTY		10	/* Create a shell-less window */

/*
 * Use the new window structure
 */
#define	jwinsize	winsize
#define	bitsx		ws_xpixel
#define	bitsy		ws_ypixel
#define	bytesx		ws_col
#define	bytesy		ws_row

/*
 *	Usual format is: [command][data]
 */
#endif _JIOCTL_
