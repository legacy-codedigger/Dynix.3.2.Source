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

#ifdef RCS
static char rcsid[]= "$Header: conf_xp.c 2.2 90/05/08 $";
#endif

/*
 * Binary configuration information for standalone Xylogics 450 disk controller
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/vtoc.h>
#include <sys/fs.h>
#include <mbad/dkbad.h>
#include "xp.h"
#include "saio.h"
#include "mbad.h"

struct	xpdevice *xpaddrs[] = {		/* controller addresses */
	(struct xpdevice *) (MB_IOSPACE+0x100),
	(struct xpdevice *) (MB_IOSPACE+0x110),
	(struct xpdevice *) (MB_IOSPACE+0x120),
};

#define	XPCTLRS	(sizeof(xpaddrs) / sizeof(xpaddrs[0]))

int	xpctlrs	= XPCTLRS;

/*
 * N.B.:  These offsets must be the same as in conf/xp_conf.c
 */

daddr_t	eagle_off[8]	= {  0,  18,   0, 408, 426, 760, 408,  91};

struct	st	xpst[]	= {
	{ 46,	20,	46*20,	842,	eagle_off },	/* Fujitsu Eagle */
};

#define	NST	(sizeof(xpst) / sizeof(xpst[0]))
#define XP_DRIVES	4

short	xptype[NST] = {			/* drive types */
	XP_TEAGLE << 6,
};

int	xpnst	= NST;
int	xpctlrunits = XP_DRIVES;
char	xphavebst[XP_DRIVES*XPCTLRS];
struct	dkbad	xpbad[XP_DRIVES*DK_NBADMAX*XPCTLRS];

#ifndef BOOTXX
/*
 * When creating a VTOC from the formatter, a single partition is made
 * available to load the miniroot on.  These are suggestions for
 * what that partition should be, indexed by drive type.  Note that
 * this will be used to create partition "1".
 */

struct partition xp_proto[] = {
	{	385480,	192280, V_RAW, 8192, 1024 },	/* 0 - Eagle */
	{	433504,	 66880, V_RAW, 8192, 1024 },	/* 1 - cdc9766 */
	{	 64480,  67200, V_RAW, 8192, 1024 },	/* 2 - cdc9762 */
};
#endif
