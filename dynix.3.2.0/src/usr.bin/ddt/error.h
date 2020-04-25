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
 * error.h: version 1.3 of 3/1/83
 * 
 *
 * @(#)error.h	1.3	(National Semiconductor)	3/1/83
 */



/* these are the error msgs returned by a longjmp when a
   ddt error occurs
*/

extern	jmp_buf	resetbuf;	/* long run on error to here */

#define	NO_ERRORS	0
#define	SCAN_ABORT	1000	/* setjmp return code for aborted command */
#define	SCAN_ERROR	1001	/* return code saying an error occurred */
#define	SCAN_EDIT	1002	/* return code saying editing occurred */
#define	USER_INT	1003	/* user signals del character */
#define	PARSE_ERROR	1004	/* parse sees bad syntax */
#define	MEM_ERROR	1005	/* memory error */
#define	REG_ERROR	1006	/* register error */
#define	PROC_ERROR	1007	/* process handling error */
#define	BPT_ERROR	1008	/* bpt error */
#define	MAC_ERROR	1009	/* machine error */
#define	STP_ERROR	1010	/* stop string eof */
