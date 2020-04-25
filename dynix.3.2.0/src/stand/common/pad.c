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

#ifdef RCS
static char rcsid[] = "$Header: pad.c 1.1 87/02/06 $";
#endif

/*
 * Pad data space on BOOTXX programs so BSS lives above
 * new firmware cfg tables.  This pad is thrown away to
 * create the 8*1024 actual file.
 */
static char pad[16 * 1024] = 0;
