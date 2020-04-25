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

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * #ident	"$Header: cmn_err.h 1.1 90/06/03 $"
 */

#ifndef	_SYS_CMN_ERR_H_

/* $Log:	cmn_err.h,v $
 */


/* Common error handling severity levels */

#define CE_TO_USER	0	/* send message to user's teriminal */
#define CE_CONT		1	/* continuation			*/
#define CE_NOTE		2	/* notice			*/
#define CE_WARN		3	/* warning			*/
#define CE_PANIC	4	/* panic			*/

/*
 * define a macro to replace printf calls for device driver
 * initialization console printouts.  This is to keep 
 * klint happy.
 */

#define CPRINTF	printf

#define	_SYS_CMN_ERR_H_
#endif	/* _SYS_CMN_ERR_H_ */
