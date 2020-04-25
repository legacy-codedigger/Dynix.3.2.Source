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
 * $Header: sweepd.h 1.2 89/08/18 $
 */

/*
 * $Log:	sweepd.h,v $
 */

#define KMEM_FILENAME	"/dev/kmem"
#define DYNIX_FILENAME	"/dynix"
#define IGNORE_FILENAME	"/usr/etc/sweepd.ignore"

#define DRIVES_PER_PROCESS	12
#define SWEEP_OFFSET		(1024 * 1024)

#define FORCEREADSIZE		8192
#define UNITENDSLOP		(64 * 512) /* 64 sectors, expressed in bytes */

struct dcc_unit {		/* one per physical drive */
    int unit_fd;		/* file descriptor for raw partition */
    int unit_pos;		/* position (in bytes) that last sweep put the head */
    int unit_size;		/* partition size in bytes */
};

#define MAX_DCC_UNITS	80	/* allow for 10 DCCs, eight drives each */

caddr_t calloc();
caddr_t malloc();
char*	valloc();
