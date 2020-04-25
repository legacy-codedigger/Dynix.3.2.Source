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
 * $Header: dbsupport.h 1.1 90/07/31 $
 *
 * These are the numbers for the sub-functions of the SYS_dbsupport
 * system call (intirim database performance enhancement support).
 */
#define DB_DIRECTREAD	0
#define DB_DIRECTWRITE	1
#define DB_BMAPCACHE	2
#define DB_SHFORK	3
#define DB_SHVFORK	4
#define DB_AFFINITYPID	5
