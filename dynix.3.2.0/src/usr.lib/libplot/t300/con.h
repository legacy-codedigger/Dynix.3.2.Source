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

/* $Header: con.h 2.0 86/01/28 $ */

#
#include <sgtty.h>
/* gsi plotting output routines */
# define DOWN 012
# define UP 013
# define LEFT 010
# define RIGHT 040
# define BEL 007
# define ESC 033
# define ACK 006
# define CR 015
# define FF 014
# define VERTRESP 48
# define HORZRESP 60.
# define VERTRES 8.
# define HORZRES 6.
/* down is line feed, up is reverse line feed,
   left is backspace, right is space.  48 points per inch
   vertically, 60 horizontally */

extern int xnow, ynow;
extern int OUTF;
extern struct sgttyb ITTY, PTTY;
extern float HEIGHT, WIDTH, OFFSET;
extern int xscale, xoffset, yscale;
extern float botx, boty, obotx, oboty, scalex,scaley;

