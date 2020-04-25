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

/* $Header: conf_wd.c 1.4 90/11/02 $ */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vtoc.h>
#include "saio.h"
#include "scsi.h"
#include "ccs.h"
#include "scsidisk.h"
#include "wd.h"

#define	A_PART	15884
#define	B_PART	33440
#define	D_PART	15884
#define	F_PART	300575

/* tables for partition mapping */
static	daddr_t wd_scsi_part[8] = { 
/*	offset 	*/
	0, 			/* 0 - partition 'a' */
	A_PART,			/* 1 - partition 'b' */
	0, 			/* 2 - partition 'c' */
	A_PART+B_PART, 		/* 3 - partition 'd' */
	A_PART+B_PART+D_PART,	/* 4 - partition 'e' */
	A_PART+B_PART, 		/* 3 - partition 'f' */
	A_PART+B_PART,		/* 3 - partition 'g' */
	A_PART+B_PART+F_PART,	/* 4 - partition 'h' */
};


/*
 * drive descriptions for supported SCSI disks.  All device types share the
 * same partition table.  Note: 8K bootstrap (BOOTXX) knows about only
 * one kind of SCSI disk.
 */

struct drive_type wddrive_table[] = {

	{ "fujitsu",
		"",
		"",
		/* last 3 cyls reserved: 2 for diags, 1 for bad blocks */
		{ 17, 11, 17*11, 751+3, wd_scsi_part  },
		{ 49181, 35156, V_RAW, 8192, 1024 },
		0,
	},

#ifndef BOOTXX
	{ "hp97544",
		"HP      ",
		"97544S          ",
		{ 56, 8, 448, 1445+2 },
		{ 53760, 153664, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "hp97548",
		"HP      ",
		"97548S          ",
		{ 56, 16, 896, 1445+2 },
		{ 570752, 205184, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},
	{ "wren3",
		"CDC     ",
		"94161-9         ",
		{ 35, 9, 9*35, 965+2, wd_scsi_part },
		{ 49140, 75915, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "wren4",
		"CDC     ",
		"94171-9         ",
		{ 54, 9, (9*54)-3, 1324+2, wd_scsi_part },
		{ 49140, 75915, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "microp1375",
		"MICROP  ",
		"1375            ",
		{ 35, 8, 8*35, 1016+2, wd_scsi_part },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "m2246s",
		"FUJITSU ",
		"M224XS          ",
		{ 33, 10, 33*10, 819+2, wd_scsi_part },
		{ 53594, 171754, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

  	{ "m2249sa",
  		"FUJITSU ",
  		"M2249SA         ",
  		{ 35, 15, 35*15, 1239+2, wd_scsi_part },
		{ 53594, 171754, V_RAW, 8192, 1024 },
  		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
  	},

	{ "xt8380s",
		"MAXTOR  ",
		"XT-8380S        ",
		{ 53, 8, 422, 1326 },
		{ 53594, 171754, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},

	{ "xt8380sm",
		"MBF-DISK",
		"XT-8380S        ",
		{ 53, 8, 422, 1326 },
		{ 53594, 171754, V_RAW, 8192, 1024 },
		1, 4, SDF_FORMPG_ND, SDM_ERROR, SDM_PF, SIZE_BDESC, SDE_PER
	},
	{	(char*)0,		/* drive_table[] terminator */
	}
#endif BOOTXX
};

