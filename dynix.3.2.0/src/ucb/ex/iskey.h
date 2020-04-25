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

/*  ADA support used by permission of Verdix Corporation
 *  Given Sept 25, 1985 by Steve Zeigler
 *  Author: Ben Priest
 */

/* $Header: iskey.h 2.1 86/04/15 $ */

#define ACCEPT 1
#define BODY 2
#define CASE 3
#define DECLARE 4
#define FOR 5
#define FUNCTION 6
#define IF 7
#define IS 8
#define LOOP 9
#define PACKAGE 10
#define PROCEDURE 11
#define RECORD 12
#define SELECT 13
#define TASK 14
#define WHILE 15
#define SPEC 16

#define LC 0
#define UC 1
#define UL 2

int rw_case;
