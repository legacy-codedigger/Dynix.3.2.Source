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
static char rcsid[] = "$Header: drtest.c 2.1 86/05/05 $";
#endif

/*
 * Standalone program to test a disk and driver
 * by reading the disk a track at a time.
 */
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"

#define SECTSIZ	512

caddr_t	calloc();
char	*prompt();

main()
{
	char *cp, *bp;
	int fd, tracksize, debug;
	register int sector, lastsector;
	struct st st;

	printf("Testprogram for stand-alone driver\n\n");
again:
	cp = prompt("Enable debugging (0=exit 1=bse, 2=ecc, 3=bse+ecc)? ");
	debug = atoi(cp);
	if (debug == 0) 
		exit(0);
	if (debug < 0)
		debug = 0;
	fd = getdevice();
	ioctl(fd, SAIODEVDATA, (char *)&st);
	printf("Device data: #cylinders=%d, #tracks=%d, #sectors=%d\n",
		st.ncyl, st.ntrak, st.nsect);
	ioctl(fd, SAIODEBUG, (char *)debug);
	tracksize = st.nsect * SECTSIZ;
	callocrnd(DEV_BSIZE);
	bp = calloc(tracksize);
	printf("Reading in %d byte records\n", tracksize);
	printf("Start ...make sure drive is on-line\n");
	lseek(fd, 0, 0);
	lastsector = st.ncyl * st.nspc;
	for (sector = 0; sector < lastsector; sector += st.nsect) {
		if (sector && (sector % (st.nspc * 10)) == 0)
			printf("cylinder %d\n", sector/st.nspc);
		read(fd, bp, tracksize);
	}
	goto again;
}

/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	register char *cp;
	register struct devsw *dp;
	int fd;

top:
	cp = prompt("Device to read? ");
	if ((fd = open(cp, 2)) < 0) {
		printf("Known devices are: ");
		for (dp = devsw; dp->dv_name; dp++)
			printf("%s ",dp->dv_name);
		printf("\n");
		goto top;
	}
	return (fd);
}
