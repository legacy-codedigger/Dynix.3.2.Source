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

/* $Header: setjmp.h 2.0 86/01/28 $
 *
 * C structure useful for looking a jmp_buf. (11 ints)
 *	struct jmp_buf {
 *		int reg3, reg4, reg5, reg6, reg7;
 * 		int fp, sp;
 * 		int pc;
 * 		int signal_mask;
 * 		int onstack;
 *		int magic;
 *	}
 */
typedef int jmp_buf[11];
