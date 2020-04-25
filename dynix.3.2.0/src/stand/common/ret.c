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
static	char rcsid[] = "$Header: ret.c 2.2 90/11/06 $";
#endif

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "saio.h"
#include "scsi.h"

/*
 *  Retension the tape one shot ioctl for streamers:
 * 
 *	Uses x9t3.2 rev 14 scsi load command as basis
 */

char	devname[20];
char	*cp;
char	*prompt();
#ifdef	DEBUG
int	 debug = 0;
#endif

main()
{
	int	 dev;
	u_char	 tapecmd[6];

	printf("Standalone retention program\n");
#ifdef	DEBUG
	cp = prompt("Enable debugging (0=nodebug 1-9=level 11=exit, level)? ");
	debug = atoi(cp);
	if (debug == 11) 
		exit(0);
	if ((debug < 0) || (debug>=10))
		debug = 0;
#endif
	
	dev = getdevice();
#ifdef	DEBUG
	ioctl(dev,SAIODEBUG,(caddr_t)debug);
#endif

	tapecmd[0] = SCSI_STARTOP;
	tapecmd[1] = 0;
	tapecmd[2] = 0;
	tapecmd[3] = 0;
	tapecmd[4] = 3;		/* ret, load it! */
	tapecmd[5] = 0;
	printf("..retension..");
	if(ioctl(dev, SAIOSCSICMD, tapecmd) != 0) {
		printf("\n");
		printf("Sorry, retention command failed\n");
		close(dev);
		exit(1);
	}
	printf("\n");
	close(dev);
	exit(0);
}


/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	char buf1[100];
	register struct devsw *dp;
	int fd;

top:
	printf("Device?");
	gets(cp = buf1);
	if ((fd = open(cp, 0)) < 0) {
		printf("Known devices are: ");
		for (dp = devsw; dp->dv_name; dp++)
			printf("%s ",dp->dv_name);
		printf("\n");
		goto top;
	}
	strcpy(devname, buf1);
	cp = devname;
	return (fd);
}
