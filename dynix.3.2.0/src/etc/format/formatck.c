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
 * ident	"$Header: formatck.c 1.2 90/03/17 $
 * formatck.c	
 *	This program determines if a disk is in an appropriate
 *	state to format.  It prints the information needed by
 *	the Admin/psx menu system.  It should not print anything
 *	else, since the menu system expects a particular output.
 *	On failure, it should exit silently. Formatck
 *	checks if the device can be opened exclusively, if
 *	there's a pseudo driver pushed on top, if there's a
 *	valid vtoc, etc.
 */

/* $Log:	formatck.c,v $
 */

#define	NULL	0
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#ifdef BSD
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include "format.h"

extern int open();
extern char *strcpy(), *strcat();

static char devname[MAXNAMLEN+1];
static char path[MAXNAMLEN+1];
static char boot_name[80];

int trashes = 0;
main(argc, argv)
	int argc;
	char **argv;
{
	int i, len;
	int val_vtoc = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: %s diskname\n", *argv);
		exit(1);
	}

	fd = disk = -1;

	strcpy(devname, *++argv);
	strcpy(path, DEVPATH);
	strcat(path, devname); 

	/*
	 * Make sure device can be opened exclusively (not
	 * in use), and that nothing is pushed on top of it.
	 */
	if ((fd = open(path, O_RDWR|O_EXCL)) < 0
	    || (ioctl(fd, RIODRIVER, (char *)NULL) < 0)) {
		exit(1);
	}

	/*
	 * Check for valid vtoc on this device
	 */
	val_vtoc = validvtoc(fd);

	for (i = 0, len = 0; i < nformat_types; i++) {
		if (!strncmp(devname, types[i].disk_name, 
			     strlen(types[i].disk_name))) 
			if (strlen(types[i].disk_name) > len) {
				len = strlen(types[i].disk_name);
				disk = i;
			}
	}
	if (disk < 0)
		exit(1);
	
	if (bootstr(boot_name))
		exit(1);

	printf("%s %s %s\n", devname, (val_vtoc) ? "yes" : "no",
		boot_name);
	exit(0);
}
