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

#ifndef lint
static char rcsid[] = "$Header: byteorder.c 1.1 90/06/08 $";
#endif

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)byteorder.c	2.6 (Berkeley) 6/18/88";
#endif /* not lint */

#include "globals.h"
#include <protocols/timed.h>

/*
 * Two routines to do the necessary byte swapping for timed protocol
 * messages. Protocol is defined in /usr/include/protocols/timed.h
 */

bytenetorder(ptr)
struct tsp *ptr;
{
	ptr->tsp_seq = htons((u_short)ptr->tsp_seq);
	switch (ptr->tsp_type) {

	case TSP_SETTIME:
	case TSP_ADJTIME:
	case TSP_SETDATE:
	case TSP_SETDATEREQ:
		ptr->tsp_time.tv_sec = htonl((u_long)ptr->tsp_time.tv_sec);
		ptr->tsp_time.tv_usec = htonl((u_long)ptr->tsp_time.tv_usec);
		break;
	
	default:
		break;		/* nothing more needed */
	}
}

bytehostorder(ptr)
struct tsp *ptr;
{
	ptr->tsp_seq = ntohs((u_short)ptr->tsp_seq);
	switch (ptr->tsp_type) {

	case TSP_SETTIME:
	case TSP_ADJTIME:
	case TSP_SETDATE:
	case TSP_SETDATEREQ:
		ptr->tsp_time.tv_sec = ntohl((u_long)ptr->tsp_time.tv_sec);
		ptr->tsp_time.tv_usec = ntohl((u_long)ptr->tsp_time.tv_usec);
		break;
	
	default:
		break;		/* nothing more needed */
	}
}
