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

#if !defined(lint)
static char rcsid[] = "$Id: opc.c,v 1.1 88/09/02 11:48:13 ksb Exp $";
#endif lint

#include "OPnames.h"

main()  {
	register int i;

	for (i = 0;  i < 256;  i++)
		if (otext[i])
			printf("#define O_%s %04o\n", otext[i]+1, i);
	exit(0);
}
