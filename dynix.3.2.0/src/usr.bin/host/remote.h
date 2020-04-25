/* @(#)$Copyright:	$
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

/* @(#)$Header: remote.h 1.6 84/12/18 $ */

#define SOH		'\001'	/* ^A Start of Header */
#define STX		'\002'	/* ^B used for one byte count */
#define ETX		'\003'	/* ^C used for two byte count */
#define DLE		'\020'	/* ^P default interrupt char */
#define DC1		'\021'	/* ^Q */
#define DC3		'\023'	/* ^S */
#define ESC		'\033'	/* ESC - used to escape these */

/* packet types */
#define ACK		'\006'	/* acknowledge: ^F */
#define NAK		'\025'	/* negative acknowledge: ^U */
#define DATA		'D'	/* Data Packet */
#define CLOSE		'C'
#define LSEEK		'L'
#define OPEN		'O'
#define PREAD		'R'
#define PWRITE		'W'
#define SCRIPT		'S'	/* start scripting */
#define NOSCRIPT	'N'	/* stop scripting */

#define BYTE(n)		((n)+32)	/* convert to ascii */
#define UNBYTE(c)	((c)-32)	/* convert from ascii */
#define CTL(x)		((x)^64)	/* toggle control bit */
#define WORD(x,y)	(UNBYTE(x)+94*UNBYTE(y))
#define UNWORD(w,p)	{*p++ =BYTE((w)%94);*p++ =BYTE((w)/94);}

#define MAXPACKET	3000		/* absolute max packet size */
#define PACKETSIZE	1500		/* preferred packet size */
