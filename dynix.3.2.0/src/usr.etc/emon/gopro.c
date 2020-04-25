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

#ifndef	lint
static	char	rcsid[] = "$Header: gopro.c 2.3 87/04/10 $";
#endif

/*
 * $Log:	gopro.c,v $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifndef SIOCIFPRMON	/* temp until <sys/ioctl.h> updated XXX */
#define	SIOCIFPRMON	_IOW(i,25, struct ifreq)	/* MONITOR NET */
#define	SIOCIFPRMOFF	_IOW(i,26, struct ifreq)	/* MONITOR OFF */
#endif

/*
 * This is a utility program to turn Ethernet promiscuous mode on/off
 *
 *	USAGE: gopro [-on] [-off] [-mon] [-moff] [interface] -nnn
 *	where:
 *		-on/-off turns promiscuous mode on or off
 *		-mon/moff turns monitoring on/off independent of promisc
 *		interface is name of Ether interface - default is "se0"
 *		-nnn is a count of the number of entries to keep
 *
 *	-on also increments monitoring while -moff does not take
 *	 out of promiscuous mode -> -on followed by -moff results
 *	 in an unmonitored promiscuous interface.
 *
 * Promiscuous mode collects all packets from the Ether and places
 * them into a wrap around buffer in kernel virtual space.
 *
 *        USE WITH CAUTION:  Connections can be lost when the Ether
 *		goes from promiscuous ON to promiscuous OFF.
 */

main(argc, argv)
	int argc;
	char **argv;
{
	int s, cc;
	int onoroff = SIOCIFPRON;
	struct ifreq ifr;

	argc--; argv++;

	/*
	 * "se0" is default interface name - will take whatever last
	 * unrecognized argv string is.  20 is default size.
	 */

	ifr.ifr_wbsize = 20;
	strncpy(ifr.ifr_name, "se0", sizeof("se0"));

	while(argc) {
		if(strcmp(argv[0], "-off") == 0)
			onoroff = SIOCIFPROFF;
		else if(strcmp(argv[0], "-on") == 0)
			onoroff = SIOCIFPRON;
		else if(strcmp(argv[0], "-mon") == 0)
			onoroff = SIOCIFPRMON;
		else if(strcmp(argv[0], "-moff") == 0)
			onoroff = SIOCIFPRMOFF;
		else if(atoi(argv[0]) > 0)
			ifr.ifr_wbsize = atoi(argv[0]);
		else
			strncpy(ifr.ifr_name, argv[0], sizeof (ifr.ifr_name));
		argc--; argv++;
	}

	s = socket(AF_INET, SOCK_DGRAM, 0);

	if(s < 0) {
		perror("socket");
		printf("YOU can't do this!\n");
		exit(1);
	}

	cc = ioctl(s, onoroff, (caddr_t)&ifr);

	if(cc < 0) {
		perror("BAD");
	printf("USAGE: gopro [-on] [-off] [-mon] [-moff] [interface] -nnn\n");
	}
	else
	switch(onoroff) {
	case SIOCIFPROFF:
		printf("promiscuous mode OFF\n");
		break;
	case SIOCIFPRON:
		printf("promiscuous ON\n");
		break;
	case SIOCIFPRMON:
		printf("monitoring ON\n");
		break;
	case SIOCIFPRMOFF:
		printf("monitoring OFF\n");
		break;
	}
}
