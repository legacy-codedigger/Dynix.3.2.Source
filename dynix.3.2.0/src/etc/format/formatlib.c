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
 * ident	"$Header: formatlib.c 1.4 90/03/17 $
 * formatlib.c
 *	general routines used by the online formatter and
 *	also by formatck (a program used by the menu system)
 */

/* $Log:	formatlib.c,v $
 */

#include <sys/param.h>
#include <sys/types.h>
#ifndef BSD
#include <sys/sysmacros.h>
#endif
#include <sys/stat.h>
#ifndef BSD
#include <sys/devsw.h>
#endif
#include <sys/ioctl.h>
#include <sys/vtoc.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include "format.h"
	 
extern char *malloc();

int
validvtoc(vfd)
	int vfd;
{
	char *buf;
	struct vtoc *vtoc;
	int valid = 0;
	int offset;

	if ((buf = MALLOC_ALIGN(V_SIZE, 512)) == NULL) {
		fprintf(stderr, "unable to malloc\n");
		exit(1);
	}

	/*
	 * If no response to RIOFIRSTSECT, device is
	 * in a bad state and so can't have a valid VTOC
	 */
	if (ioctl(vfd, RIOFIRSTSECT, &offset) < 0)
		return(0);

	if (lseek(vfd, (long)((offset+V_VTOCSEC) * DEV_BSIZE), 0) < 0) {
		perror("lseek error");
		exit(1);
	}
	if (read(vfd, buf, (unsigned)V_SIZE) != V_SIZE) {
		fprintf(stderr, "Cannot read VTOC\n");
		perror("");
		exit(1);
	}
	vtoc = (struct vtoc *)buf;
	if (vtoc->v_sanity == VTOC_SANE)
		valid = 1;
	return(valid);
}

	
int
bootstr(str)
	char *str;
{
	int bootnum;

	bootnum = (*types[disk].bootnum)();
	if (bootnum < 0) {
		*str = '\0';
		return(-1);
	}
	sprintf(str, "%s(%d,0)", types[disk].disk_name, bootnum);
	return(0);
}

/*
 * internal version of atoi: check to be sure all of the input is
 * numeric first.
 */
int
xatoi(s)
	char *s;
{
	register char *chp;

	for (chp = s; *chp; chp++)
		if (*chp < '0' || *chp > '9')
			return(-1);
	return(atoi(s));
}

/*
 * advance_str(str, count)
 *	return pointer to count'th word in str
 */
char *
advance_str(str, count)
	char *str;
	int count;
{
	int i = 0;

	for (;;) {
		while (*str == ' ' || *str == '\t' || *str == '\n')
			str++;
		if (i++ == count)
			break;
		while (*str != '\0' && *str != ' ' && *str != '\t'
		       && *str != '\n')
			str++;
	}
	return(str);
}


/*
 * trap signals - just set a flag since most routines must decide
 * when to take a signal or not.
 */
int	sigints = 0;

sigint()
{
	if (sigints++ > 10)
		debug = sigints;
	signal(SIGINT, sigint);
	signal(SIGHUP, sigint);
	fflush(stdout);
	fflush(stderr);
}
