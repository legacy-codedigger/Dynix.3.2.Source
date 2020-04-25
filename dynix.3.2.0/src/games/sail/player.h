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

/* $Header: player.h 2.0 86/01/28 $ */

/*
 * sccsid = "@(#)player.h	1.1 3/17/83";
 */
#include <curses.h>
#include "externs.h"

#define ROWSINVIEW 15
#define COLSINVIEW 75

extern WINDOW *view;
extern WINDOW *slot;

