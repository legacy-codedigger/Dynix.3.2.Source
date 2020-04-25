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

#ifdef notyet
static char rcsid[]= "$Header: xp.c 2.6 90/04/18 $";
#endif

/*
 * xp.c:  Standalone driver for Xylogics 450 disk controller
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/vtoc.h>
#include <sys/fs.h>
#include <mbad/dkbad.h>
#include "xp.h"
#include "saio.h"
#include "mbad.h"

#define	BADSCNT			/* controller returns wrong sector count */

#ifndef BOOTXX
#define IOPBMAPS	2	/* nbr of mbad maps to map iopbs */
#define IOPBBASEMAP	0	/* first map to use to map iopbs */
#define DATAMAPS	254	/* nbr of maps to map data */
#define DATABASEMAP	2	/* first maps to use to map data */
#endif

#define	i_debug	i_cyloff
#define	XP_BSEDEBUG	01	/* BSE debugging */
#define	XP_ECCDEBUG	02	/* ECC debugging */

/*
 * xp(unit, partition)
 *
 * unit:  0x007	drive number on controller
 *	  0x038	controller number
 *	  0x1C0	drive type - index into xpst table
 *	  0xE00	multibus adaptor number
 *
 * partition: index (0-7) into partition table
 */

#define	XP_UNIT(x)	( x       & 0x7)
#define	XP_CTLR(x)	((x >> 3) & 0x7)
#define	XP_TYPE(x)	((x >> 6) & 0x7)
#define	XP_MBAD(x)	((x >> 9) & 0x7)

#define	MAXECC		  5		/* max size of correctible ECC for
					 * severe burnin */

struct	xp_iopb	xpiopb;			/* I/O parameter block */

/*
 * Binary configured information
 */

extern	struct	xpdevice *xpaddrs[];	/* controller addresses */
extern	struct	st	xpst[];		/* size tables */
extern	struct	dkbad	xpbad[];	/* bad sector information */
extern		int	xpctlrs;	/* number of controllers */
extern		int	xpnst;		/* number of entries in xpst */
extern		short	xptype[];	/* drive types */
extern		char	xphavebst[];	/* initialized flags */
extern		int	xpctlrunits;	/* max number of drives/controller */

xpopen(io)
	register struct iob *io;
{
	register struct st *st;
	register type;
	register unsigned partition;
	register int ctlrunit;
	int i;

	if(XP_CTLR(io->i_unit) >= xpctlrs)
		_stop("xp: bad ctlr");
	type = XP_TYPE(io->i_unit);
	if(type >= xpnst)
		_stop("xp: bad type");
	st = &xpst[type];
	partition = io->i_boff;
	mbadinit(XP_MBAD(io->i_unit));
	xpstrategy(io, XP_DSIZE);
	ctlrunit = XP_CTLR(io->i_unit) * xpctlrunits + XP_UNIT(io->i_unit);
	if(xphavebst[ctlrunit] == 0) {

		struct iob tio;
		register block;
		register copy;
#ifndef BOOTXX
		register union bt_bad *bt;
#endif

		tio = *io;
		tio.i_cc = sizeof (struct dkbad);
		tio.i_flgs |= F_RDDATA;
		block = 0;
		do {
			tio.i_ma = (caddr_t) &xpbad[ctlrunit*DK_NBADMAX+block];
			for(copy=0; copy<DK_NBADCOPY; copy++) {
				tio.i_bn = (st->nspc * st->ncyl) - st->nsect +
						DK_LOC(block, copy);
				if(xpstrategy(&tio, READ) == sizeof (struct dkbad))
					break;
			}
#ifndef BOOTXX
			if(copy == DK_NBADCOPY) {
				if(block == 0) {
					printf("Can't read bst\n");
					xpbad[ctlrunit*DK_NBADMAX].bt_lastb = 0;
					bt = xpbad[ctlrunit*DK_NBADMAX].bt_bad;
					for(copy=0; copy<DK_MAXBAD; copy++) {
						bt[copy].bt_cyl = DK_END;
						bt[copy].bt_trksec = DK_END;
					}
					break;
				}
				else {
					printf("Can't read block %d of bst\n",
								block);
					bt = (union bt_bad *)&xpbad[ctlrunit*DK_NBADMAX+block];
					for(copy=0; copy<DK_NBAD_N; copy++) {
						bt[copy].bt_cyl = DK_INVAL;
						bt[copy].bt_trksec = DK_INVAL;
					}
				}
			}
#endif
		} while(++block < DK_NBADMAX);
		xphavebst[ctlrunit] = 1;
	}
	if (vtoc_setboff(io, 0) < 0) {
		io->i_error = 0;
		if(partition < 8) {
			if(st->off[partition] == -1)
				_stop("xp: bad partition");
			io->i_boff = st->off[partition] * st->nspc;
		}
	}
}

xpstrategy(io, func)
	struct iob *io;
{
	u_char cmd;

	if(func == READ)
		cmd = XP_READ;
#ifndef	BOOTXX
	else if(func == WRITE)
		cmd = XP_WRITE;
#endif
	else
		cmd = func;
	return(doxpcmd(io, &cmd));
}

doxpcmd(io, cmdp)
	register struct iob *io;
	register caddr_t cmdp;
{
	register struct xpdevice *xpaddr;
	register struct xp_iopb *ip;
	register unit;
	struct st *st;
	int bn;
	int timeout;
	int errcode;
	int scnt;
#ifndef	BOOTXX
	long mbaddr;
	extern int	cpuspeed;
#endif

	ip = &xpiopb;
	unit = io->i_unit;
	xpaddr = (struct xpdevice *)((char *)xpaddrs[XP_CTLR(unit)] +
			(MB_IODELTA * XP_MBAD(unit)));
	st = &xpst[XP_TYPE(unit)];
	/*
	 * don't reset error count on drive reset,
	 * which can only come from within a retry
	 */
	if(*cmdp != XP_DRESET)
		io->i_errcnt = 0;
	io->i_error = 0;
retry:
	bzero(ip, sizeof (struct xp_iopb));
#ifndef	BOOTXX
	mbaddr = mbad_physmap(XP_MBAD(unit), IOPBBASEMAP,
		(caddr_t)ip, sizeof(struct xp_iopb), IOPBMAPS);
	SETIOPBADDR(xpaddr, (unsigned)mbaddr);
#else
	SETIOPBADDR(xpaddr, (unsigned)ip + MB_RAMBASE);
#endif
	switch(*cmdp) {

	case XP_READ:
#ifndef	BOOTXX
	case XP_WRITE:
		if((ip->io_lbaddr = mbad_physmap(XP_MBAD(unit),
			     DATABASEMAP, io->i_ma, io->i_cc, DATAMAPS)) == -1)
			return(-1);
#else
		ip->io_lbaddr = ((unsigned)io->i_ma + MB_RAMBASE);
#endif
		bn = io->i_bn;
		ip->io_cyl = bn / st->nspc;
		bn %= st->nspc;
		ip->io_head = bn / st->nsect;
		ip->io_sect = bn % st->nsect;
		ip->io_scnt = (io->i_cc + DEV_BSIZE-1) >> 9;
#ifdef BOOTXX
		if((unsigned)(io->i_ma + ((io->i_cc+DEV_BSIZE-1) & ~DEV_BSIZE))
								>= 0x40000)
			return(-1);
#endif
		break;

	case XP_DRESET:
		break;

#ifndef	BOOTXX
	case XP_XREAD:
	case XP_XWRITE:
	    {
		register struct x450ioctl *arg = (struct x450ioctl *) cmdp;

		ip->io_cyl  = arg->cyl;
		ip->io_head = arg->head;
		ip->io_sect = arg->sect;
		ip->io_scnt = arg->scnt;
		ip->io_lbaddr = ((u_long)arg->buf) + MB_RAMBASE;
		if((ip->io_lbaddr = mbad_physmap(XP_MBAD(io->i_unit),
			     DATABASEMAP, arg->buf, (512 * arg->scnt + 8
			     		* arg->scnt), DATAMAPS)) == -1) {
			io->i_error = EIO;
			return(-1);
		}
		break;
	    }

	case XP_FORMAT:
	    {
		register struct x450ioctl *arg = (struct x450ioctl *) cmdp;

		ip->io_cyl  = arg->cyl;
		ip->io_head = arg->head;
		ip->io_sect = arg->sect;
		ip->io_scnt = arg->scnt;
		break;
	   }
#endif

	case XP_DSIZE:
	    {
		register struct st *st;

		st = &xpst[XP_TYPE(unit)];
		ip->io_head = st->ntrak - 1;
		ip->io_sect = st->nsect - 1;
		ip->io_cyl  = st->ncyl  - 1;
		break;
	    }

	default:
		_stop("xp:?cmd");
	}
	scnt = ip->io_scnt;
	ip->io_comm = *cmdp | XP_RELO | XP_AUD;
	ip->io_imode |= XPM_ASR | XPM_ECC2;
#ifndef	BOOTXX
#ifdef	F_SEVRE
	/*
	 * set ECC mode 3
	 */
	if(io->i_flgs & F_SEVRE)
		ip->io_imode |= XPM_ECC3;
	else
#endif
	/*
	 * set ECC mode 0
	 */
	if(io->i_flgs & F_ECCLM)
		ip->io_imode &= ~XPM_ECC3;
#endif
	ip->io_throt = XPT_USE_INTLV | XPT_T64;
	ip->io_drive = xptype[XP_TYPE(unit)] | XP_UNIT(unit);
restart:
	ip->io_status = 0;
	ip->io_compcode = 0;
#ifndef	BOOTXX
	timeout = cpuspeed * 3000000;
#endif
#ifdef	EXTRADEBUG
	printf("cmd 0x%x: (%d, %d, %d) - %d sects, addr 0x%x\n", ip->io_comm,
			ip->io_cyl, ip->io_head, ip->io_sect, ip->io_scnt,
			ip->io_lbaddr);
#endif
	ip->io_iscnt = ip->io_scnt;	/* need for revector error */
#ifdef	DEBUG
	dumpiopb(ip);
#endif
	xpaddr->xpcsr = XP_GO;
	while(xpaddr->xpcsr & XP_BUSY)
#ifndef	BOOTXX
		if(*cmdp != XP_FORMAT && --timeout == 0) {
			printf("xp%d timeout: cmd 0x%x\n", unit, *cmdp);
			_stop("");
		}
#else
		;
#endif
#ifdef	BADSCNT
	fixscnt(io, ip);
#endif
#ifdef	EXTRADEBUG
	printf("compcode 0x%x\n", ip->io_compcode);
#endif
	if((xpaddr->xpcsr & (XP_ERR|XP_DERR)) == 0 && ip->io_compcode == XPC_GOOD) {
good:
#ifndef	BOOTXX
		if(io->i_errcnt > 0 && *cmdp != XP_DRESET)
			printf("xp%d: recovered err 0x%x (%d, %d, %d)\n",
				unit, errcode, ip->io_cyl, ip->io_head,
				ip->io_sect);
#endif
		xpaddr->xpcsr = XP_IPND;
		return(io->i_cc);
	}
	xpaddr->xpcsr = XP_IPND | XP_ERR;
#ifndef BOOTXX
	errcode = ip->io_compcode;
#endif
	switch(ip->io_compcode) {

#ifndef	BOOTXX
	/*
	 * give up immediately on write protect error
	 */
	case XPC_WPE:
		printf("xp%d: write locked\n", unit);
		io->i_error = EHER;
		return(-1);

	/*
	 * other unrecoverable hard errors
	 */
	case XPC_IPND:
	case XPC_CONFLCT:
	case XPC_ICA:
	case XPC_ISA:
	case XPC_SMSEC:
	case XPC_BADCMD:
	case XPC_SCZ:
	case XPC_ISS:
	case XPC_STA:
	case XPC_STB:
	case XPC_STC:
	case XPC_BADHEAD:
	case XPC_HERR:
		io->i_error = EHER;
#endif
hard:
#ifdef BOOTXX
		return(-1);
#else
		printf("xp%d hard error 0x%x ", unit, ip->io_compcode);
		printf("(%d, %d, %d)\n", ip->io_cyl, ip->io_head, ip->io_sect);
		io->i_errblk = io->i_bn +
				((((io->i_cc + DEV_BSIZE-1) >> 9) - scnt));
		return(io->i_cc - (scnt << 9));
#endif

	/*
	 * header not found.  If this is during a severe burn in, then
	 * just retry it like any other hard error.  Otherwise, look to
	 * see if it's a forwarded sector and revector it if so.
	 * If neither, then keep retrying until either it
	 * comes out ok of fails all the retries.
	 */
	case XPC_HNF:
#ifdef	EXTRADEBUG
		printf("xp%d hard error 0x%x ", unit, ip->io_compcode);
		printf("(%d, %d, %d) ", ip->io_cyl, ip->io_head, ip->io_sect);
		printf("scnt %d\n", ip->io_scnt);
#endif
#if defined(F_SEVRE) && !defined(BOOTXX)
		if(io->i_flgs & F_SEVRE) {
			io->i_error = EHER;
			goto hard;
		}
#endif
		/* not updated on error */
		ip->io_lbaddr += (ip->io_iscnt - ip->io_scnt) * 512;

		if((io->i_flgs & F_NBSF)==0 && xprevector(io, ip, xpaddr)==0) {
			if(--ip->io_scnt == 0)
				goto good;
			if(++ip->io_sect >= st->nsect) {
				ip->io_sect = 0;
				if(++ip->io_head >= st->ntrak) {
					ip->io_head = 0;
					ip->io_cyl++;
				}
			}
			goto restart;
		}
		io->i_error = EHER;
		break;

	/*
	 * all other hard errors get retried
	 */
	default:
		io->i_error = EHER;
		break;

	/*
	 * hard ECC gets retried
	 */
	case XPC_HECC:
		io->i_error = EECC;
#if defined(F_SEVRE) && !defined(BOOTXX)
		if(io->i_flgs & F_SEVRE)
			goto hard;
#endif
		break;

	/*
	 * recovered seek or ECC errors aren't errors
	 */
	case XPC_ASR:
	case XPC_RECC:
#ifndef	BOOTXX
		printf("xp%d: autoretry OK error 0x%x\n", unit, ip->io_compcode);
#endif
		goto good;

#ifndef	BOOTXX
	/*
	 * soft ECC error:  if it's less than MAXECC bits,
	 * then call it good.  This error can only happen if
	 * SAIOECCLIM has been issued, since otherwise either the
	 * controller will correct them or, if SAIOSEVRE
	 * has been issued, they'll all be reported as hard
	 * errors.
	 */
	case XPC_SECC:
		if(xpecc(io, ip) == 0) {
			/*
			 * continue the transfer
			 */
			if(--ip->io_scnt == 0)
				goto good;
			if(++ip->io_sect >= st->nsect) {
				ip->io_sect = 0;
				if(++ip->io_head >= st->ntrak) {
					ip->io_head = 0;
					ip->io_cyl++;
				}
			}
			goto restart;
		}
		io->i_error = EECC;
		break;
#endif
	}

	/*
	 * abort retry sequence if drive reset failed
	 */
	if(*cmdp == XP_DRESET) {
		io->i_error = EHER;
		return(-1);
	}
	/*
	 * Retry on other errors.  On every eighth retry,
	 * starting with the fourth, reset the drive.  Give
	 * up after 28 retries.
	 * Give up sooner when doing severe testing.
	 */
	if(++io->i_errcnt > 27)
		goto hard;
#if defined(F_SEVRE) && !defined(BOOTXX)
	if((io->i_flgs & F_SEVRE) && io->i_errcnt == 5)
		goto hard;
#endif
	if((io->i_errcnt % 8) == 4) {
		u_char c = XP_DRESET;
		if(doxpcmd(io, &c) < 0) {
#ifndef	BOOTXX
			printf("xp%d: can't reset\n", unit);
#endif
			io->i_error = EHER;
			return(-1);
		}
	}
	goto retry;
}

xpioctl(io, cmd, arg)
	struct iob *io;
	struct x450ioctl *arg;
{
#ifndef	BOOTXX
	register flag;
	register struct st *st;

	io->i_cc = 0;
	st = &xpst[XP_TYPE(io->i_unit)];
	switch(cmd) {

	case SAIODEBUG:
		flag = (int)arg;
		if(flag > 0)
			io->i_debug |= flag;
		else
			io->i_debug &= ~flag;
		break;

	case SAIODEVDATA:
		*(struct st *)arg = *st;
		break;

	/*
	 * get offset to first sector of disk
	 */
	case SAIOFIRSTSECT:
		*(int *)arg = 0;
		break;

	case SAIOX450CMD:
		switch(arg->cmd) {

		case XP_XREAD:
		case XP_XWRITE:
		case XP_FORMAT:
			doxpcmd(io, (caddr_t) arg);
			break;

		default:
			printf("xp%d: bad ioctl 0x%x\n", 
				io->i_unit, arg->cmd);
			io->i_error = ECMD;
			return -1;
		}
		break;

	default:
		printf("xp%d: bad ioctl (('%c'<<8)|%d)\n", io->i_unit,
			(u_char)(cmd >> 8), cmd & 0xFF);
		io->i_error = ECMD;
		return -1;
	}
	return 0;
#endif
}

#ifndef	BOOTXX
/*
 * Correct ECC errors.  If the error is more than MAXECC
 * bits wide, report a hard error anyway.
 */

xpecc(io, ip)
	register struct iob *io;
	register struct xp_iopb *ip;
{
	long eccmask = 0;
	u_short eccbit;
	long mask;
	int nbits = 0;
	u_short tmpmask;

	if(io->i_debug & XP_ECCDEBUG)
		printf("xp%d: soft ECC (%d, %d, %d) (0x%x, 0x%x) ", io->i_unit,
			ip->io_cyl, ip->io_head, ip->io_sect, ip->io_eccm,
			ip->io_ecca);
	mask = 0x8000;
	tmpmask = ip->io_eccm;
	if(tmpmask == 0){
		if(io->i_debug & XP_ECCDEBUG)
			printf(" NO BITS BAD!?!\n");
		return(0);
	}
	while((tmpmask & 01) == 0) {
		tmpmask >>= 1;
		mask >>= 1;
	}
	while(tmpmask != 0) {
		if(tmpmask & 01)
			eccmask |= mask;
		nbits++;
		tmpmask >>= 1;
		mask >>= 1;
	}
	if(nbits > MAXECC && (io->i_flgs & F_ECCLM)) {
		if(io->i_debug & XP_ECCDEBUG)
			printf("declared hard - %d bits wide\n", nbits);
		return(1);
	}
	eccbit = ip->io_ecca - 1;
	eccmask <<= eccbit & 0xF;
	eccbit >>= 3;
	*(long *)((io->i_ma + ((((io->i_cc+DEV_BSIZE-1)>>9)
				- ip->io_scnt) << 9)) + eccbit) ^= eccmask;
	if(io->i_debug & XP_ECCDEBUG)
		printf("recovered\n");
	return(0);
}
#endif

/*
 * Revector bad sectors if possible.
 */

xprevector(io, ip, xpaddr)
	struct iob *io;
	register struct xp_iopb *ip;
	register struct xpdevice *xpaddr;
{
	int cyl, head, sect, scnt, imode;
	int bbn, unit;
	register int sector;
	register int temp;
	register x;
	int status;
	struct st *st;
#ifndef BOOTXX
	int timeout, lastspare;
	long ioaddr;
	extern int	cpuspeed;

	ioaddr = ip->io_lbaddr;
#endif
	cyl  = ip->io_cyl;
	head = ip->io_head;
	sect = ip->io_sect;
	scnt = ip->io_scnt;
	imode = ip->io_imode;
#ifndef	BOOTXX
	if(io->i_debug & XP_BSEDEBUG)
		printf("revectoring (%d, %d, %d)", cyl, head, sect);
#endif
	unit = (XP_CTLR(io->i_unit) * xpctlrunits + XP_UNIT(io->i_unit)) *
								DK_NBADMAX;
	if((bbn = isbad(&xpbad[unit], cyl, head, sect)) < 0) {
#ifndef	BOOTXX
		if(io->i_debug & XP_BSEDEBUG)
			printf(": not in table\n");
#endif
		return(1);
	}
	st = &xpst[XP_TYPE(io->i_unit)];
	x = st->ntrak * st->nsect;
	sector = st->ncyl * x - st->nsect - 1 - bbn;
	ip->io_cyl = sector / x;
	temp = sector % x;
	ip->io_head = temp / st->nsect;
	ip->io_sect = temp % st->nsect;
	ip->io_imode = XPM_ASR | XPM_ECC2;
	ip->io_scnt = 1;

nextspare:

#ifndef	BOOTXX
	if(io->i_debug &XP_BSEDEBUG)
		printf(" to (%d, %d, %d) ", ip->io_cyl, ip->io_head,
								ip->io_sect);
#endif
#ifdef	DEBUG
	dumpiopb(ip);
#endif
#ifndef	BOOTXX
	timeout = cpuspeed * 3000000;
#endif
	xpaddr->xpcsr = XP_GO;
	while(xpaddr->xpcsr & XP_BUSY)
#ifndef	BOOTXX
		if((ip->io_comm & 0xF) != XP_FORMAT && --timeout == 0) {
			printf("xp%d timeout: cmd 0x%x\n", io->i_unit, ip->io_comm & 0xF);
			_stop("");
		}
#else
		;
#endif
	status = ip->io_compcode;
#ifdef BOOTXX
	ip->io_cyl  = cyl;
	ip->io_head = head;
	ip->io_sect = sect;
	ip->io_scnt = scnt;
	ip->io_imode = imode;
	if((xpaddr->xpcsr & (XP_ERR|XP_DERR)) == 0 &&
	   (status == 0 || status == XPC_RECC || status == XPC_ASR))
		return(0);
	return(1);
#else
	if((xpaddr->xpcsr & (XP_ERR|XP_DERR)) == 0 &&
	   (status == 0 || status == XPC_RECC || status == XPC_ASR)) {
		if(io->i_debug & XP_BSEDEBUG)
			printf("succeeded\n");
		ip->io_cyl  = cyl;
		ip->io_head = head;
		ip->io_sect = sect;
		ip->io_scnt = scnt;
		ip->io_imode = imode;
		return(0);
	}
	if(status == XPC_HNF) {
		if((bbn = isbad(&xpbad[unit],
				ip->io_cyl, ip->io_head, ip->io_sect)) < 0) {
			if(io->i_debug & XP_BSEDEBUG)
				printf(": not in table\n");
			goto out;
		}
		lastspare = sector;
		x = st->ntrak * st->nsect;
		sector = st->ncyl * x - st->nsect - 1 - bbn;
		if(sector >= lastspare)
			goto failed;
		ip->io_cyl = sector / x;
		temp = sector % x;
		ip->io_head = temp / st->nsect;
		ip->io_sect = temp % st->nsect;
		ip->io_imode = XPM_ASR | XPM_ECC2;
		ip->io_scnt = 1;
		ip->io_lbaddr = ioaddr;
		ip->io_compcode = 0;
		xpaddr->xpcsr = XP_ERR;
		goto nextspare;
	}

failed:
	if(io->i_debug & XP_BSEDEBUG)
		printf("failed: csr 0x%x, compcode 0x%x\n", xpaddr->xpcsr,
									status);
out:
	ip->io_cyl  = cyl;
	ip->io_head = head;
	ip->io_sect = sect;
	ip->io_scnt = scnt;
	ip->io_imode = imode;
	return(1);
#endif
}

#ifdef	BADSCNT
fixscnt(io, ip)
	register struct iob *io;
	register struct xp_iopb *ip;
{
	register daddr_t endblk;
	register struct st *st;
	int scnt = ip->io_scnt;

	st = &xpst[XP_TYPE(io->i_unit)];
	endblk = ip->io_cyl * st->nspc + ip->io_head * st->nsect + ip->io_sect;
	ip->io_scnt = ((io->i_cc + DEV_BSIZE-1) >> 9) - (endblk - io->i_bn);
#ifndef	BOOTXX
	if(scnt != ip->io_scnt && io->i_debug && ip->io_compcode != XPC_GOOD)
		printf("io_scnt %u reset to %u [%u - (%u - %u)]\n", scnt,
			ip->io_scnt, ((io->i_cc+DEV_BSIZE-1)>>9), endblk,
			io->i_bn);
#endif
}
#endif

#ifdef DEBUG
dumpiopb(ip)
	register struct xp_iopb *ip;
{
	printf("iopb at 0x%x: comm 0x%x imode 0x%x status 0x%x\n",
		ip, ip->io_comm, ip->io_imode, ip->io_status);
	printf("   compcode 0x%x throt 0x%x drive 0x%x\n",
		ip->io_compcode, ip->io_throt, ip->io_drive);
	printf("   head %d sect %d cyl %d scnt %d\n",
		ip->io_head, ip->io_sect, ip->io_cyl, ip->io_scnt);
	printf("   lbaddr 0x%x hdoff 0x%x niop 0x%x\n",
		ip->io_lbaddr, ip->io_hdoff, ip->io_niop);
	printf("   scnt %d, ecca 0x%x, iscnt %d\n",
		ip->io_scnt, ip->io_ecca, ip->io_iscnt);
}
#endif DEBUG
