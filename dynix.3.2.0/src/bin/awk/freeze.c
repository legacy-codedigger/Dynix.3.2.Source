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
static char rcsid[] = "$Header: freeze.c 2.0 86/01/28 $";
#endif

#include "stdio.h"
freeze(s) char *s;
{	int fd;
	unsigned int *len;
	len = (unsigned int *)sbrk(0);
	if((fd = creat(s, 0666)) < 0) {
		perror(s);
		return(1);
	}
	write(fd, &len, sizeof(len));
	write(fd, (char *)0, len);
	close(fd);
	return(0);
}

thaw(s) char *s;
{	int fd;
	unsigned int *len;
	if(*s == 0) {
		fprintf(stderr, "empty restore file\n");
		return(1);
	}
	if((fd = open(s, 0)) < 0) {
		perror(s);
		return(1);
	}
	read(fd, &len, sizeof(len));
	(void) brk(len);
	read(fd, (char *)0, len);
	close(fd);
	return(0);
}
