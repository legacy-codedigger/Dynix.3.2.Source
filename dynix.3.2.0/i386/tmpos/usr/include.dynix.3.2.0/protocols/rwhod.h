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

/* $Header: rwhod.h 2.0 86/01/28 $ */

/*
 * rwho protocol packet format.
 */
struct	outmp {
	char	out_line[8];		/* tty name */
	char	out_name[8];		/* user id */
	long	out_time;		/* time on */
};

struct	whod {
	char	wd_vers;		/* protocol version # */
	char	wd_type;		/* packet type, see below */
	char	wd_pad[2];
	int	wd_sendtime;		/* time stamp by sender */
	int	wd_recvtime;		/* time stamp applied by receiver */
	char	wd_hostname[32];	/* hosts's name */
	int	wd_loadav[3];		/* load average as in uptime */
	int	wd_boottime;		/* time system booted */
	struct	whoent {
		struct	outmp we_utmp;	/* active tty info */
		int	we_idle;	/* tty idle time */
	} wd_we[1024 / sizeof (struct whoent)];
};

#define	WHODVERSION	1
#define	WHODTYPE_STATUS	1		/* host status */
