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
static char rcsid[]= "$Header: xt.c 2.2 87/04/14 $";
#endif

/*
 * xt.c:  Standalone driver for Xylogics 472 tape controller
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "xt.h"
#include "saio.h"
#include "mbad.h"

#define	SETIOPBADDR(x, a) \
	(x)->xtlba  = (a),       \
	(x)->xtmba  = (a) >>  8, \
	(x)->xtlrel = (a) >> 16, \
	(x)->xtmrel = (a) >> 24

/*
 * xt(unit, file)
 *
 * unit:  0x03	drive number on controller
 *	  0x04	density select
 *	  0x38	controller number
 *	  0x40	high speed
 *	  0xE0	multibus adaptor
 *
 * file:  file on tape
 */

#define	XT_UNIT(x)	( x       & 0x03)
#define	XT_HIDEN(x)	( x       & 0x04)
#define	XT_CTLR(x)	((x >> 3) & 0x07)
#define	XT_HISPEED(x)	( x       & 0x40)
#define	XT_MBAD(x)	((x >> 9) & 0x07)

/* defines for mbad mapping registers */
#define IOPBMAPS	2	/* nbr of mbad maps to map iopbs */
#define IOPBBASEMAP	0	/* first map to use to map iopbs */
#define DATAMAPS	254	/* nbr of maps to map data */
#define DATABASEMAP	2	/* first maps to use to map data */

struct	xt_iopb	iopb;			/* I/O parameter block */
int	lastwaswrite;			/* flag: last operation was write */

extern	struct	xtdevice  *xtaddrs[];	/* controller addresses */
extern	int	xtctlrs;		/* number of controllers */
extern	int	xtdensel;		/* density selection allowed */

xtopen(io)
	register struct iob *io;
{
	static int firstopen = 1;

	if(XT_CTLR(io->i_unit) >= xtctlrs)
		_stop("xt: undefined controller");
	mbadinit(XT_MBAD(io->i_unit));
	xtstrategy(io, XT_SEEK|XT_REW);
	
	/*
	 * xtdensel is a binary config option
	 * that allows the use of the set param
	 * device command. This command doesn't work
	 * if the device is a 1600 bpi *only* cipher
	 * tape drive.
	 */
	if(xtdensel)
		xtstrategy(io, XT_HIDEN(io->i_unit) ? XT_GCR : XT_PE);
	xtstrategy(io, XT_PE);
	if(XT_HISPEED(io->i_unit)) {
		printf("HIGH SPEED\n");
		xtstrategy(io, XT_HIGH);
	}
	else {
		printf("LOW SPEED\n");
		xtstrategy(io, XT_LOW);
	}
	if(io->i_cc = io->i_boff)
		xtstrategy(io, XT_SEEK|XT_FILE);
	
}

xtclose(io)
	register struct iob *io;
{
	if(lastwaswrite) {
		xtstrategy(io, XT_FMARK);
		xtstrategy(io, XT_FMARK);
	}
	xtstrategy(io, XT_SEEK|XT_REW);
}

xtstrategy(io, func)
	struct iob *io;
{
	register struct xtdevice *xtaddr;
	register struct xt_iopb *ip;
	register unit;
	int errcount;
	int compcode;
	int errcode;
	int timeout;
	long mbaddr, daddr;
	register i;
	extern int	cpuspeed;

	ip = &iopb;
	unit = io->i_unit;
	xtaddr = (struct xtdevice *)((char *)xtaddrs[XT_CTLR(unit)] +
					(MB_IODELTA * XT_MBAD(unit)));

	if(io->i_cc >= 64*1024)
		_stop("xt: xfer block size ( >= 64k) too large");
	io->i_errcnt = 0;
	io->i_error = 0;
	bzero(ip, sizeof (struct xt_iopb));
	mbaddr = mbad_physmap(XT_MBAD(unit), IOPBBASEMAP,
		(caddr_t)ip, sizeof(struct xt_iopb), IOPBMAPS);
	SETIOPBADDR(xtaddr, (unsigned)mbaddr);
	lastwaswrite = 0;
	switch(func) {

	case XT_SEEK|XT_REW:
	case XT_SEEK|XT_FILE:
		timeout = 0;
		break;

	case READ:
		func = XT_READ | XT_RETY;
		goto rwcom;

	case WRITE:
		func = XT_WRITE | XT_RETY;
		lastwaswrite = 1;
rwcom:
		if((daddr = mbad_physmap(XT_MBAD(unit),
			     DATABASEMAP, io->i_ma, io->i_cc, DATAMAPS)) == -1)
			return(-1);
		SETBADDR(ip, daddr);
		timeout = cpuspeed * 3000000;
		break;

	case XT_FMARK:
		func |= XT_RETY;
		timeout = cpuspeed * 3000000;
		break;

	case XT_PE:
	case XT_GCR:
	case XT_LOW:
	case XT_HIGH:
		func |= XT_SPARAM;
		timeout = cpuspeed * 3000000;
		break;

	default:
		_stop("xt: bad command");
	}
	ip->io_scomm = func | XT_RELO | XT_AUD;
	ip->io_status = 0;
	ip->io_compcode = 0;
	ip->io_throt = XTT_T64;
	ip->io_drive = unit;
	ip->io_cnt = (u_short)io->i_cc;
	xtaddr->xtcsr = XT_GO;
	i = cpuspeed * 300000;
	while(--i)
		/* spin */;
	while(xtaddr->xtcsr & XT_BUSY)
		if(timeout && --timeout == 0)
			_stop("xt timeout");
	compcode = ip->io_compcode;
	if((xtaddr->xtcsr & (XT_ERR|XT_DERR)) == 0 && compcode == 0)
		return(io->i_cc);
	
	switch(compcode) {

	case XTC_WPE:
		printf("xt%d: write locked\n", unit);
		io->i_error = EHER;
		return(-1);

	case XTC_CORR:
		printf("xt%d: recovered data\n", unit);
		return(io->i_cc);

	case XTC_SHORT:
		return((int)ip->io_acnt);

	case XTC_TMREAD:
		printf("xt%d: eof detected\n", unit);
		return(0);

	default:
		printf("xt%d hard error 0x%x\n", unit, compcode);
		io->i_error = EHER;
		return(-1);
	}
}
