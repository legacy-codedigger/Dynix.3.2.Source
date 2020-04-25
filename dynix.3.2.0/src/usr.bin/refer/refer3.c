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
static char rcsid[] = "$Header: refer3.c 2.0 86/01/28 $";
#endif

#include "refer..c"
#define move(x, y) close(y); dup(x); close(x);

corout(in, out, rprog, arg, outlen)
char *in, *out, *rprog;
{
	int pipev[2], fr1, fr2, fw1, fw2, n;
	int status;

	pipe(pipev); 
	fr1 = pipev[0]; 
	fw1 = pipev[1];
	pipe(pipev); 
	fr2 = pipev[0]; 
	fw2 = pipev[1];
	if (fork() == 0)
	{
		close(fw1); 
		close(fr2);
		move(fr1, 0);
		move(fw2, 1);
		execl(rprog, "deliv", arg, 0);
		err("Can't run %s", rprog);
	}
	close(fw2); 
	close(fr1);
	write(fw1, in , strlen(in));
	close(fw1);
	wait(&status);
	n = read(fr2, out, outlen);
	out[n] = 0;
	close(fr2);
	return(n);
}
