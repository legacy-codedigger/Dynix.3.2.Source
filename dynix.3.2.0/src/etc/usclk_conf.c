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
static char rcsid[]= "$Header: usclk_conf.c 1.3 87/03/23 $";
#endif
/*
 *	usclk_conf -  Test data path to device, print on stdout current
 *		    value of microsecond clock, sleep for a while and
 *		    print new current values.
 */

/*$Log:	usclk_conf.c,v $
 */
#include <stdio.h>
#include <signal.h>
#include <usclkc.h>
#include <sys/time.h>

void quit();

#define RATE		1000000	/* ticks/second = 1 usec resolution*/
#define MAXSEC		4294.967296  /* secs to usclk rollover */
			             /*   = 1h:11m:34.967296secs */
#if defined(ns32000) || defined(KXX)
#define USCLKVER	3		/* Boards Firmware version */
#define	TSTCELL		4		/* Data Path test cell */
#define NOTCELL		5		/*  "			*/
#define TSTPAT		0xbabe		/* pattern stored by device */

typedef unsigned short	usclk16_t;
extern usclk16_t	*usclk_p;
#endif defined(ns32000) || defined(KXX)

main(argc, argv)
int	argc;
char	*argv[];
{
#if defined(ns32000) || defined(KXX)
	unsigned short tst, tstnot;
#endif defined(ns32000) || defined(KXX)
	struct timeval tod, tod2;
	struct timezone tz, tz2;
	unsigned long sync_tod, del_tod;
	usclk_t t32, sync_t32, del_t32;

	signal(SIGBUS,quit);

	usclk_init();

#if defined(ns32000) || defined(KXX)
	/* Check data path to device */
	tst = usclk_p[TSTCELL];
	tstnot = usclk_p[NOTCELL];
	if ((tst != TSTPAT) || (tst != ~tstnot)) {
		printf("usclk_conf:  Data path error to /dev/usclk.\n");
		exit(1);
	}
	printf("usclk_conf:  Data path to usclk device passes diagnostic.\n");
	printf("             Firmware version = %04x\n", usclk_p[USCLKVER]);
#endif defined(ns32000) || defined(KXX)

	/* The usclk is read synchronous with the UNIX 'gettimeofday(2)'
	 * service, and values compared.
	 */

	gettimeofday(&tod,&tz);
	t32 = getusclk();
	gettimeofday(&tod2,&tz2);

	/* While loop ensures we have synchronous time values
	 * (independent of context switch) and tod.tv_usec is 0.
	 */
	while ((tod.tv_usec != 0) || timercmp(&tod,&tod2,!=)) {
		gettimeofday(&tod,&tz);
		t32 = getusclk();
		gettimeofday(&tod2,&tz2);
	}
	sync_t32 = t32;
	sync_tod = tod.tv_sec;

	/* Print out current information. */
	printf("Gettimeofday(2)   :  %lu.%06lu seconds\n", tod.tv_sec, 0);
	printf("Microsecond  Clock:       %4lu.%06lu seconds\n", t32 / RATE,
		t32 % RATE);

	printf("usclk_conf:  Sleeping for about 60 secs to verify accuracy.\n");
	sleep(59);

	/* The usclk is read synchronous with the UNIX 'gettimeofday(2)'
	 * service, and values compared.
	 */

	gettimeofday(&tod,&tz);
	t32 = getusclk();
	gettimeofday(&tod2,&tz2);

	/* While loop ensures we have synchronous time values
	 * (independent of context switch) and tod.tv_usec is 0.
	 */
	while ((tod.tv_usec != 0) || timercmp(&tod,&tod2,!=)) {
		gettimeofday(&tod,&tz);
		t32 = getusclk();
		gettimeofday(&tod2,&tz2);
	}

	/* Print out current information. */
	printf("Gettimeofday(2)   :  %lu.%06lu seconds\n", tod.tv_sec, 0);
	printf("Microsecond  Clock:       %4lu.%06lu seconds\n", t32 / RATE,
		t32 % RATE);

	del_tod = tod.tv_sec - sync_tod;
	del_t32 = t32 - sync_t32;

	printf("Delta Gettimeofday      = %lu seconds\n", del_tod);
	printf("Delta Microsecond Clock = %4lu.%06lu seconds\n", del_t32 / RATE,
		del_t32 % RATE);
}

void quit()
{
	fprintf(stderr, "usclk_conf: access error reading /dev/usclk.\n");
	exit(1);
}
