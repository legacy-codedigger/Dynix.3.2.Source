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
static char rcsid[] = "$Header: sestat.c 2.2 87/08/13 $";
#endif

/*
 *	Read kernel networking stats.
 *	sestat [ -z ] [ /dev/kmem [ /dynix ] ]
 *		print current buffer
 */

#include <stdio.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <sec/sec.h>
#include <netif/if_se.h>

struct	nlist nl[] = {
	{ "_se_max_unit" },
#	define	MAX_UNIT	0
	{ "_se_state" },
#	define	STATE		1
	{ "" }
};

main(argc, argv)
	int argc;
	char **argv;
{
	int unit, max_unit, km, zflg = 0;
	char *unix_file;		/* name of kernel to get names from */
	char *kmem_file;		/* name of kernel data area to use */
	struct se_state *softp;		/* pointerr to soft state */

	if (argc > 1 && strcmp(argv[1], "-z") == 0) {
		++zflg;
		--argc;
		++argv;
	}

	/*
	 * use specific files from command line?
	 */

	unix_file = (argc>1 ? argv[1] : "/dynix" );
	kmem_file = (argc>2 ? argv[2] : "/dev/kmem" );

	km = open(kmem_file, zflg? 2: 0);
	if (km < 0) {
		char buf[100];

		sprintf(buf, "Can't open kmem file %s\n", kmem_file);
		done(buf);
	}
	nlist(unix_file, nl);
	if (nl[MAX_UNIT].n_value == 0) {
		fprintf(stderr, "Can't find se_max_unit in %s\n", unix_file);
		exit(1);
	}
	if (nl[STATE].n_value == 0L) {
		fprintf(stderr, "Can't find se_state in %s\n", unix_file);
		exit(1);
	}
	lseek(km, (long)nl[MAX_UNIT].n_value, 0);
	if (read(km, (char *)&max_unit, sizeof(max_unit)) < 0) {
		fprintf(stderr, "Can't read se_max_unit from %s\n", kmem_file);
		exit(1);
	}
	lseek(km, (long)nl[STATE].n_value, 0);
	if (read(km, (char *)&softp, sizeof(softp)) < 0) {
		fprintf(stderr, "Can't read se_state from %s\n", kmem_file);
		exit(1);
	}
	for (unit = 0; unit <= max_unit; ++unit) {
		print_state(km, unit, (long)(softp+unit));
		if (zflg)
			zero_state(km, unit, (long)(softp+unit));
	}
	close(km);
	done((char *)NULL);
}


zero_state(fd, unit, offset)
	int unit;
	long offset;
{
	struct se_counts counts;

	bzero((char *)&counts, sizeof(counts));
	lseek(fd, (long) &((struct se_state *)offset)->ss_sum, 0);
	write(fd, (char *)&counts, sizeof(counts));
}


print_state(fd, unit, offset)
	int unit;
	long offset;
{
	struct se_state state;

	lseek(fd, offset, 0);
	read(fd, (char *)&state, sizeof(state));

	if (!state.ss_alive)
		return;
	printf("Summary information for se%d:\n", unit);
	printf("%10d receiver overflows\n", state.ss_sum.ec_rx_ovfl);
	printf("%10d receiver CRC errors\n", state.ss_sum.ec_rx_crc);
	printf("%10d receiver dribble errors\n", state.ss_sum.ec_rx_dribbles);
	printf("%10d receiver short packets\n", state.ss_sum.ec_rx_short);
	printf("%10d good packets received\n", state.ss_sum.ec_rx_good);
	printf("%10d transmitter underflows\n", state.ss_sum.ec_tx_unfl);
	printf("%10d transmitter collisions\n", state.ss_sum.ec_tx_coll);
	printf("%10d transmitter excessive collisions\n", state.ss_sum.ec_tx_16xcoll);
	printf("%10d good packets transmitted\n", state.ss_sum.ec_tx_good);
	printf("\n");
}


done(s)
	char *s;
{
	if (s) {
		printf(s);
	}
	exit(s!=NULL);
}
