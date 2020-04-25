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
static char rcsid[] = "$Header: usclk.c 1.10 90/06/09 $";
#endif lint

/*
 * driver for usclk board on balance systems
 */

/* $Log:	usclk.c,v $
 */
#if defined(ns32000) || defined(KXX)
#define MBAD_CLK
#endif defined(ns32000) || defined(KXX)

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/errno.h"
#include "../machine/intctl.h"
#ifdef MBAD_CLK
#include "../mbad/mbad.h"
#endif MBAD_CLK
#include "../machine/ioconf.h"
#include "../machine/gate.h"
#include "../machine/hwparam.h"
#include "../h/ioctl.h"
#include "../h/user.h"

#ifndef NULL
#define NULL	0
#endif

#define	MAX_USCLKS	1		/* max number of usclk boards allowed */

#ifdef MBAD_CLK
#ifdef DEBUG
static int debug_level = 0;
#endif DEBUG

#define USCLKVER	3		/* Boards Firmware version */
#define TSTCELL		4		/* Data Path test cell */
#define NOTCELL		5		/*  "			*/
#define TSTPAT		0xbabe		/* pattern stored by device */
caddr_t	usclk_base;			/* pointer to beginning of usclk */
#else MBAD_CLK
#define usclk_base	(caddr_t)PHYS_ETC
#endif MBAD_CLK

int usclk_alive = 0;			/* flag, is this device alive? */

#ifdef MBAD_CLK
int usclkprobe(), usclkboot();
struct mbad_driver usclk_driver = {
	"usclk",			/* name */
	(MBD_HASPROBE|MBD_HASBOOT),	/* configuration flags */
	usclkprobe,			/* probe procedure */
	usclkboot,			/* boot procedure */
	NULL,				/* interrupt procedure */
};

usclkprobe(mp)
	register struct mbad_probe *mp;
{
	unsigned short tst, tstnot;
	unsigned short *usclk_p;

	usclk_p = (unsigned short *)(&mp->mp_desc->mb_mem[mp->mp_csr]);
#ifdef DEBUG
	if (debug_level)
		printf("usclk_p %x\n", usclk_p);
#endif DEBUG
	/*
	 * if you get a memory error, there is no board
	 * 	Check data path to device 
	 *	Need the strange "+1"'s to match what usclk_conf does.
	 */
	tst = usclk_p[TSTCELL + 1];
	tstnot = usclk_p[NOTCELL + 1];
#ifdef DEBUG
	if (debug_level)
		printf("tst %x tstnot %x\n", tst, tstnot);
#endif DEBUG
	if ((tst != TSTPAT) || (tst != ~tstnot)) {
#ifdef DEBUG
		if (debug_level)
			printf("usclk:  Data path error to /dev/usclk.\n");
#endif DEBUG
		return(0);
	}
	return(1);
}

usclkboot(n, md)
	int n;
	register struct mbad_dev *md;
{
	int idx;

	if (n > MAX_USCLKS) {
		printf("more than one usclk board was found.\n");
		printf("only the first board will be used\n");
		/*
		 *+ An attempt was made to add more usclk boards.
		 *+ Check the configuration against the value of MAX_USCLKS.
		 */
		n = MAX_USCLKS;
	}

	usclk_alive = (int)md->md_alive;
	if (!usclk_alive)
		return;

	idx = ((u_long)md->md_desc->mb_mem - VA_MBAd(0))/MBAD_ADDR_SPACE;
	usclk_base = (caddr_t)(PA_MBAd(idx)) + md->md_csr;
#ifdef DEBUG
	if (debug_level)
		printf("usclk_base %x\n", usclk_base);
#endif DEBUG
}

#else MBAD_CLK

usclkboot()
{
	usclk_alive = 1;
}
#endif MBAD_CLK

/*
 * open the device
 */

usclkopen(dev)
	dev_t	dev;
{
	int unit = minor(dev);
	int error = 0;

	if ((unit >= MAX_USCLKS) || (usclk_alive == 0))
		return(ENXIO);

	return(error);
}
	

/*
 * usclkmmap()
 *	Perform mapping functions.
 *
 */

/*ARGSUSED*/
usclkmmap(dev, cmd, off, size, prot)
	dev_t	dev;
	int	cmd;
	u_long	off;	/* HW pages */
	int	size;	/* HW pages */
	int	prot;	/* PROT_READ|PROT_WRITE */
{
	int val = 0;

	switch(cmd) {

	case MM_MAP:

		/*
		 * map read-only
		 */
		if (prot & PROT_WRITE) {
			val = EINVAL;
			break;
		}

		/*
		 * Allow only ONE page to be mapped.
		 */
		if (off + size > CLSIZE)
			val = ENOSPC;
		else 
			val = MM_PHYS;
			 
		break;

	case MM_REFPG:

		val = (int)usclk_base;
		break;

	case MM_UNMAP:
	case MM_SWPOUT:
	case MM_SWPIN:
		break;

	default:
		printf("cmd = %d\n", cmd);
		panic("usclkmmap: bad function");
		/*
		 *+ The usclk driver was called through its mmap entry point
		 *+ to perform an unsupported operation.
		 */
	}
	return(val);
}
