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
 * $Header: acct.h 2.6 87/04/30 $
 *
 * acct.h
 *	Accounting records.
 */

/* $Log:	acct.h,v $
 */

/*
 * Merged SysV/4.2 accounting structures;
 * these use a comp_t type which is a 3 bits base 8
 * exponent, 13 bit fraction ``floating point'' number.
 * The ac_io field is a special case. If 4.2 accounting is
 * enabled, it holds the number of disk IO blocks. If SysV
 * accounting is enabled, it holds the total number of bytes
 * transferred.
 */
typedef	ushort comp_t;

struct	acct
{
	char	ac_comm[8];		/* Accounting command name */
	comp_t	ac_utime;		/* Accounting user time */
	comp_t	ac_stime;		/* Accounting system time */
	comp_t	ac_etime;		/* Accounting elapsed time */
	time_t	ac_btime;		/* Beginning time */
	short	ac_uid;			/* Accounting user ID */
	short	ac_gid;			/* Accounting group ID */
	short	ac_mem;			/* average memory usage */
	comp_t	ac_io;			/* disk IO blocks or bytes xferred */
	dev_t	ac_tty;			/* control typewriter */
	char	ac_flag;		/* Accounting flag */
	char	ac_scgacct;		/* account identifier */
	comp_t	ac_rw;			/* number of disk IO blocks */
	char	ac_stat;		/* Exit status */
};

#define	AFORK	0001		/* has executed fork, but no exec */
#define	ASU	0002		/* used super-user privileges */
#define	ACORE	0010		/* dumped core */
#define	AXSIG	0020		/* killed by a signal */
#define	ACCTF	0300		/* record type: 00 = acct */
