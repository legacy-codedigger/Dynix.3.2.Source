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
static	char	rcsid[] = "$Header: emon_main.c 2.5 87/04/30 $";
#endif

/*
 * $Log:	emon_main.c,v $
 */

#include "emon.h"	/** includes some includes **/
#include <sgtty.h>
#include <sys/ioctl.h>
#include <stdio.h>

/*--------------------------------------------------------------*/

int inited = 0;

int debug = 0;

int	intson;			/* interrupts on flag **/

struct promiscstat *promiscstatp;

/*------------------------------------------------------------------*/

main(argc, argv)
	int argc;
	char **argv;
{
	char cc;
	short quit;

	argc--; argv++;

	while(argc){
		if(strcmp(*argv, "-d") == 0)
			debug = 1;
		else if(atoi(*argv) > 0) {
			wnumbufs = atoi(*argv);	/* number of buffers */
			if(wnumbufs <= 0)
				usage();
		}
		else
			usage();
		argv++; argc--;
	}

	init_wbuf();

	reverse=1;	/* sets original direction to FIFO */

	do_do_stats();

	if_debug = OFF;

	do_do_intson();

	quit = 0;
	while (quit == 0) {
		getandocommands();
		printf("quit (y)");
		cc = getchar();
		printf("\n");
		if(cc == 'y') quit++;
	}

	do_do_quit();
}

/*----------------------------------------------------*/

do_do_debug()	/* flip flop debug flag */
{
	if(if_debug > 0) {
		if_debug = OFF;
		printf("if_debug now OFF\n");
	}else{
		if_debug = ON;
		printf("if_debug now ON\n");
	}
	return(0);
}

/*----------------------------------------------------*/

do_do_intson()	/* flip flop continuing interrupt policy */
{

	if(!intson) {
		intson = ON; 
		spl0();
		printf("intsoff->intson (ioctl here?) ");
	}
	printf("interrupts ENABLED\n");
	return;
}

do_do_intsoff()
{
	if(intson) {
		spl7();
		intson = OFF;
		printf("intson->intsoff ");
	}
	printf("interrupts INHIBITED\n");
}

/*----------------------------------------------------*/

do_do_buck()		/** print out bucket counts **/
{
	short i;
	int total = 0;

	printf("bucket counts ~= histogram\n");

	for (i = 0; i < NUM_BUX; i++) {
		printf("\t\tbuck[%d] = %d\n", i, promiscstatp->buck[i]);
		total += promiscstatp->buck[i];
	}
	printf("total == %d ----------\n", total);
	printf("\ntotal_sequent ETHER =\t%d\n", total_sequentE);
	return;
}

/*----------------------------------------------------*/

do_do_clear()
{
	int i;

	spl7();		/** inhibit interrupts during init ---- **/
	intson = OFF;

	/* initialize counters */

	init_filters();		/* no longer keep anything */

	total_packets = 0;	/* total packets counter */
	total_bytes = 0;	/* total bytes counter */
	total_beelions = 0;	/* total beelions of bytes counter */
	total_broad_packets = 0; /* total broad packets counter */
	total_rwho_packets = 0; /* total rwho packets counter */
	total_ip_packets = 0;	/* total ip packets counter */
	total_udp_packets = 0;	/* total udp packets counter */
	total_tcp_packets = 0;	/* total tcp packets counter */
	total_icmp_packets = 0;	/* total icmp packets counter */
	total_trail_packets = 0; /* total trailer packets counter */
	total_arp_packets = 0;	/* total arp packets counter */
	total_xns_packets = 0;	/* total xns packets counter */

	total_crc_errors = 0;	/* total cint detected crc errors */
	total_align_errors = 0;	/* total cint detected alignment errors */
	total_lost_errors = 0;	/* total cint detected lost packets */
	total_runt_errors = 0;	/* total cint detected runt packets */
	total_toobig_errors = 0; /* total cint detected toobig packets */
	total_unrecognized = 0;	/* total unrecognized packets */

	total_sequentE = 0;

	dumpit = OFF;
	autoprint = OFF;		/** start with no auto printing data **/

	trapx = 0;
	trapping = OFF;

	reverse = 1;		/* begin by autoincrementing */
	savechar = '\n';

	for(i = 0; i < NUM_BUX; i++) promiscstatp->buck[i] = 0;

	promiscstatp->promisc_wbufp = promiscstatp->promisc_wbuf_begin;

	counter = 0;
	bytes_kept = 0;

	return(0);
}

spl0()
{
	printf("simulate enable interrupts - spl0()\n");
}
spl7()
{
	printf("simulate inhibit interrupts - spl7()\n");
}

struct	nlist nl[] = {
	{ "_promiscstatp" },
#define	PROMISC	0
	{ "" }
};

init_wbuf()
{
	int	size;
	int	pgsz = getpagesize();
	char	*vaddr;
	char	*sbrk();
	u_long	pos = 0;

	int  km, zflg;
	char *unix_file;		/* name of kernel to get names from */
	char *kmem_file;		/* name of kernel data area to use */

	/*
	 * use specific files from command line?
	 */

	if(inited)
		return;

	inited++;

	zflg = 1;	/* used to open r/w */

	unix_file = ("/dynix");
	kmem_file = ("/dev/kmem" );

	km = open(kmem_file, zflg ? 2: 0);

	if (km < 0) {
		char buf[100];

		sprintf(buf, "Can't open kmem file %s\n", kmem_file);
		done(buf);
	}

	nlist(unix_file, nl);
	if (nl[PROMISC].n_value == 0) {
		fprintf(stderr, "Can't find promiscstatp in %s\n", unix_file);
		exit(1);
	}
	lseek(km, (long)nl[PROMISC].n_value, 0);
	if (read(km, (char *)&promiscstatp, sizeof(promiscstatp)) < 0) {
		fprintf(stderr, "Can't read promiscstatp from %s\n", kmem_file);
		exit(1);
	}

	if(debug) printf("promiscstatp = 0x%x\n", promiscstatp);

	size = (WNUMBUFS*sizeof(struct wbuf)) + NBPG;

	size = (size + (CLBYTES-1)) &~ (CLBYTES-1);

	malign(pgsz);		/* to ensure size is % page size */
	vaddr = sbrk(size);	/* to put memory there for this process */

	pos = (u_long) promiscstatp;	/* to position mapped file */

	if (mmap(vaddr, size, PROT_READ | PROT_WRITE, MAP_SHARED, km, pos) < 0)
	{
		perror("mmap");
		exit(2);
	}

	promiscstatp = (struct promiscstat *) vaddr;

	wbuf_begin = (struct wbuf *) (vaddr + NBPG);

	return;
}

malign(bound)
	int	bound;
{
	char	*cur = sbrk(0);
	int	delta = bound - ((int)cur & (bound-1));

	if (delta > 0)
		(void) sbrk(delta);
}

done(s)
	char *s;
{
	if (s) {
		printf(s);
	}
	exit(s!=NULL);
}

do_do_quit()
{
	exit(0);
}

usage()
{
	printf("Usage: [-d] [<wnumbufs>]\n");
	printf("       -d  for debug switch\n");
	printf("       <wnumbufs> integer number of circular buffers\n");
	printf("                  usually the same as specified in gopro\n");
	exit(1);
}

/*
 * emon_lint() - resolve some references soz lint doesn't complain
 * NEVER CALLED
 */

struct atpstats atpstats;

emon_lint()
{
	int	x, y;
	struct atpstats* a = &atpstats;
	char	s[2];

	x = 1;
	y = mtohw(x);
	x = htomw(y);
	a->icount++;
	(void) gets(s);
	
/*
signal defined( ???(74) ), but never used
bcast declared( ???(139) ), but never used or defined
fopen defined( ???(54) ), but never used
fdopen defined( ???(55) ), but never used
freopen defined( ???(56) ), but never used
ftell defined( ???(57) ), but never used
fgets defined( ???(59) ), but never used
*/
}
