/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
 * ident	"$Header: zd_conf.c 1.10 91/03/26 $"
 * Configuration informat used for zd subsystem of online formatter.
 */
#include <sys/types.h>
#ifdef BSD
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#else
#include <sys/zdc.h>
#include <sys/zdbad.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include "format.h"
#include "zdformat.h"

struct usage_check zd_funcs[] = {
	{FORMAT, 
		B_VERBOSE|B_NOWRITEDIAG|B_NOVERIFY|B_FULLPASS|B_HDRPASS
			|B_BADFILE|B_DEFECTPASS|B_TYPE|B_CHECKDATA|B_OVERWRITE,
		B_BADFILE|B_TYPE,
		WRITEDIAG|VERIFY,
		O_RDWR|O_EXCL,
		"format -f -b file -t disktype [-vownc] [-p passes] [-d defectpasses] [-h headerpasses] diskname"
	},
	{VFORMAT, 
		B_VERBOSE|B_NOWRITEDIAG|B_NOVERIFY|B_FULLPASS|B_HDRPASS
			|B_BADFILE|B_DEFECTPASS|B_TYPE|B_CHECKDATA|B_OVERWRITE,
		B_BADFILE|B_TYPE,
		WRITEDIAG|VERIFY,
		O_RDWR|O_EXCL,
		"format -F -b file -t disktype [-vownc] [-p passes] [-d defectpasses] [-h headerpasses] diskname"
	},
	{REFORMAT,
		B_VERBOSE|B_NOWRITEDIAG|B_NOVERIFY|B_FULLPASS|B_HDRPASS
			|B_DEFECTPASS|B_CHECKDATA|B_OVERWRITE,
		0,	
		WRITEDIAG|VERIFY,
		O_RDWR|O_EXCL,
		"format -r [-vownc] [-p passes] [-d defectpasses] [-h headerpasses] diskname"
	},
	{REFORMAT_MFG,
		B_VERBOSE|B_NOWRITEDIAG|B_NOVERIFY|B_FULLPASS|B_HDRPASS
			|B_DEFECTPASS|B_CHECKDATA|B_OVERWRITE,
		0,	
		WRITEDIAG|VERIFY,
		O_RDWR|O_EXCL,
		"format -m [-vownc] [-p passes] [-d defectpasses] [-h headerpasses] diskname"
	},
	{ADDBAD,
		B_VERBOSE|B_NOWRITEDIAG|B_OVERWRITE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -a file [-vw] diskname OR\n  format -a \"{cyl head sect type}\" [-vw] diskname"
	},
	{REPBAD,
		B_VERBOSE|B_NOWRITEDIAG|B_OVERWRITE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -A file [-vw] diskname OR\n  format -A \"{cyl head sect type}\" [-vw] diskname"
	},
	{ADD_MFG,
		B_VERBOSE|B_NOWRITEDIAG|B_BADFILE|B_OVERWRITE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -M -b defects [-vw] diskname"
	},
	{VERIFY,
		B_VERBOSE|B_NOWRITEDIAG|B_START|B_END|B_FULLPASS|B_HDRPASS
			|B_DEFECTPASS|B_CHECKDATA|B_OVERWRITE,
		0,
		WRITEDIAG,
		O_RDWR|O_EXCL,
		"format -y [-vowc] [-s startcyl] [-e endcyl] [-p fullpasses] [-d defectpasses] [-h headerpasses] diskname"
	},
	{WRITEDIAG,
		B_VERBOSE|B_OVERWRITE,
		0,
		0,
		O_RDWR|O_EXCL,
		"format -q [-v] diskname"
	},
	{DISPLAY,
		B_VERBOSE|B_OVERWRITE|B_TYPE,
		0,
		0,
		O_RDONLY,
		"format -l [-v] diskname OR\nformat -lo [-v] [-t type] diskname"
	},
	{INFO,
		B_VERBOSE|B_TYPE,
		0,
		0,
		O_RDONLY,
		"format -i [-v] diskname"
	},
	{SHOW,
		B_OVERWRITE|B_VERBOSE|B_TYPE,
		0,
		0,
		O_RDONLY,
		"format -S \"cyl head [sects]\" [-v] diskname"
	},
	{USAGE,
		B_VERBOSE,
		0,
		0,
		O_RDWR,
		"format -u diskname"
	}
};

int zd_numfuncs = sizeof(zd_funcs)/sizeof(struct usage_check);
