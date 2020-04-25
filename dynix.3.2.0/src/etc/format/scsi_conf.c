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
 * ident	"$Header: scsi_conf.c 1.4 90/03/17 $
 * scsi_conf.c
 *	The tables pertaining to the scsi subsystem
 *	of the online formatter
 */

/* $Log:	scsi_conf.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stdio.h>
#include "format.h"
#include "scsiformat.h"

struct usage_check scsi_funcs[] = {
	{FORMAT, 
		B_VERBOSE|B_NOWRITEDIAG|B_NOVERIFY|B_FULLPASS
			|B_TYPE|B_OVERWRITE,
		0,
		WRITEDIAG|VERIFY,	
		O_RDWR|O_EXCL,
		"format -f [-vown] [-p passes] [-t PG| -t P] diskname"
	},
	{ADDBAD,
		B_VERBOSE|B_NOWRITEDIAG|B_OVERWRITE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -a file|logical sector [-vw] diskname"
	},
	{WRITEDIAG,
		B_VERBOSE|B_OVERWRITE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -q [-v] diskname"
	},
	{INFO,
		B_VERBOSE,
		0,
		0,
		O_RDONLY,
		"format -i [-v] diskname"
	},
	{USAGE,
		B_VERBOSE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -u diskname"
	}
};

int scsi_numfuncs = sizeof(scsi_funcs)/sizeof(struct usage_check);

struct scsi_ftype scsi_ftype[] = {
	{ "PG", 0},
	{ "P",	1}
};

int num_ftypes = sizeof(scsi_ftype)/sizeof(struct scsi_ftype);
