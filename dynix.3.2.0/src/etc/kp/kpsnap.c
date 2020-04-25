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
static	char	rcsid[] = "$Header: kpsnap.c 1.1 86/10/07 $";
#endif

/* $Log:	kpsnap.c,v $
 */

/*
 *	kpsnap - dump profile data to a log file
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sec/kp.h>
#include <stdio.h>
#include "kp_util.h"

main(argc, argv)
	int	argc;
	char	**argv;
{
	char *buf;
	int  kp, log, rec_size;
	struct kp_status status;
	struct kp_hdr p_hdr;
	extern char *malloc();

	if (argc != 2) {
		fprintf(stderr, "usage: kpsnap  logfile");
		exit(1);
	}

	if ((kp = open("/dev/kp", 0)) < 0) {
		perror("kpsnap: cannot open /dev/kp: ");
		exit(1);
	}

	if ((log = open(argv[1], O_WRONLY|O_CREAT, 0666)) < 0) {
		perror("kpsnap: cannot creat log file: ");
		exit(1);
	}

	if (ioctl(kp, KP_GETSTATE, (char *) &status) < 0) {
		fprintf(stderr, "%s: GETSTATE failed ", argv[0]);
		perror("");
		exit(1);
	}
	if (gettimeofday(&p_hdr.tod, (struct timezone *)NULL) != 0) {
		perror("kpsnap: Gettimeofday error: ");
		exit(1);
	}
	p_hdr.tod_flag = TIMESTAMP;
	p_hdr.engines = status.kps_engines;
	p_hdr.bins = status.kps_bins;
	p_hdr.binshift = status.kps_binshift;
	p_hdr.b_text = status.kps_b_text;
	p_hdr.e_text = status.kps_e_text;
	rec_size = status.kps_bins * sizeof(unsigned);
	buf = malloc((unsigned)rec_size);
	if (read(kp, buf, rec_size) != rec_size) {
		perror("kpsnap: Bad read of profile data");
		exit(1);
	}

	if (write(log, (char *) &p_hdr, sizeof(p_hdr)) != sizeof(p_hdr) ||
	    write(log, buf, rec_size) != rec_size) {
		fprintf(stderr, "kpsnap: write error '%s': ", argv[1]);
		perror("");
		exit(1);
	}
}
