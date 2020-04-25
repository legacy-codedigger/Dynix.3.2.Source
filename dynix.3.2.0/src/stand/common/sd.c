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
static char rcsid[]= "$Header: sd.c 2.14 90/10/05 $";
#endif RCS

/*
 * SCSI disk device driver (stand alone).
 *
 * This device driver assumes that it will run single user
 * single processor. There is *no* Mutual exclusion
 * done at all.
 *
 *	Unimplemented:
 *		case SCSI_RSENSE:
 *		case SCSI_SEEK:
 *		case SCSI_WRITEB:
 *		case SCSI_READB:
 *		case SCSI_RESRV:
 *		case SCSI_RELSE:
 *		case SCSI_STARTOP:
 *		case SCSI_RDIAG:
 *		case SCSI_SDIAG:
 *			printf("sd: ioctl not supported yet\n");
 *		
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <sys/vtoc.h>
#include <machine/cfg.h>
#include "sec.h"
#include "sec_ctl.h"
#include "saio.h"
#include "scsi.h"
#include "scsidisk.h"
#include "sdreg.h"
#include "ccs.h"

#define SDTIMEOUT	100000
#define SDNOTIMEOUT	-1
#define SD_CMD6SZ	6		/* scsi command length */
#define SD_CMDASZ	10		/* scsi command length */
#define SD_CMDFSZ	16		/* scsi command length */

/*
 * Data structures
 */
#define i_debug i_cyloff		/* unused parameter */
static	struct st *sdst;
static	struct sec_pup_info sdinfo;
static	struct sec_dev_prog sdprog;
static	struct sec_iat	sdiat[2];
extern	struct drive_type drive_table[];

#ifndef BOOTXX
int sd_expecterror = 0;
static u_char buf[SIZE_MAXDATA + SD_CMDFSZ];	/* common I/O area */
#ifdef notdef
extern int xtend_sense;
#endif /* notdef */
#endif

#if defined(DEBUG) && !defined(BOOTXX)
#define	DEBUG_PRINT(x,y)	if(x) { y; }
#else
#define	DEBUG_PRINT(x,y)	;
#endif

/*
 * sdopen - Driver open procedure.
 *
 * This procedure opens the unit by verifying that a partition
 * for which the open refers is valid.
 */
sdopen(io)
	register struct iob *io;
{
	register daddr_t boff;
#ifndef BOOTXX
	struct sdinq *sdinq;
	struct drive_type *dp;
	struct sd_modes *modes;
#endif

	/*
	 * default geometry is entry 0, in case we don't figure it out
	 * (as in BOOTXX).
	 */

	sdst = &drive_table[0].dt_st;

	/*
	 * Install the device program into the sdinfo struct
	 * and initialize all other pertinent data.
	 */

	sdinfo.sec_pup_dev_prog = &sdprog;
#if !defined(BOOTXX)
	if(SEC_set_dev(SCSI_BOARD(io->i_unit), 
	   SEC_SCSI_DEVNO(io->i_unit), &sdinfo) == -1)
		_stop("sd: no such sec board");

	/*
	 * Do a TEST UNIT command to the target.  This is done to absorb
	 * the initial Unit Attention error on CCS disks.
	 */

	bzero((char *)buf, SIZE_MAXDATA);
	sd_expecterror = 1; *buf = SCSI_TEST;
	if (sd_docmd(io, (caddr_t)buf) == -1)
		_stop("sd: TEST UNIT command fails");
	sd_expecterror = 0;

	/*
	 * Do an INQUIRY command to decide what sort of target we have.
	 */

	bzero((char *)buf, SD_CMD6SZ + SIZE_INQ);
	buf[0] = SCSI_INQUIRY;
	buf[4] = SIZE_INQ;

	if (sd_docmd(io, (caddr_t)buf) == -1) {
		printf("sd: INQUIRY command failed - ");
		printf(" assuming an UNFORMATED 72Mb disk\n");
		io->i_error = 0;
		goto genpart;
	}

	sdinq = (struct sdinq *)(buf + SD_CMD6SZ);

	/*
	 * format is 0 if this is an adaptec disk - if so, no further
	 * action required
	 */

	if (sdinq->sdq_format == 0)
		goto genpart;

	/*
	 * discover the disk information by doing an extended INQUIRY
	 * command.
	 */

	bzero((char *)buf, SD_CMD6SZ + SIZE_INQ_XTND);
	buf[0] = SCSI_INQUIRY;
	buf[4] = SIZE_INQ_XTND;
	if (sd_docmd(io, (caddr_t)buf) == -1)
		_stop("sd: extended INQUIRY command fails");

	sdinq = (struct sdinq *) (buf + SD_CMD6SZ);

	for (dp = drive_table; dp->dt_vendor; dp++) {
		if (strncmp(sdinq->sdq_vendor, dp->dt_vendor, SDQ_VEND) == 0
		&&  strncmp(sdinq->sdq_product, dp->dt_product, SDQ_PROD) == 0)
			break;
	}

	if (! dp->dt_vendor) {
		int i;
		char *bp = sdinq->sdq_vendor;

		printf("sd: drive ID: ");
		for (i = 0; i < sdinq->sdq_length; i++)
			printf("%c", bp++);
		printf("\n");
		_stop("sd: could not find drive in drive_table");
	}

	if (sdinq->sdq_format != dp->dt_inqformat)
		_stop("sd: INQUIRY command output in wrong format");

	sdst = &dp->dt_st;
#endif

genpart:

	/*
	 * generate partitions
	 */
	if (vtoc_setboff(io, 0)) {
		io->i_error = 0;
		boff = io->i_boff;
		if((unsigned)boff < 8)	{
			if(sdst->off[boff] == -1) {
				printf("bad partition\n");
				io->i_error = EDEV;
			} else
				io->i_boff = sdst->off[boff];
		}
	}

	/* return value indicated by io->i_error */
}

/*
 * sdstrategy - standard read/write routine.
 *
 * This procedure executes a request for a read or a write to
 * the device.
 */
sdstrategy(io, func)
	struct iob *io;
{
	u_char c;

	/*
	 * all I/O must have its in-core buffer aligned to an 8-byte
	 * boundary.
	 */
	if (((int)io->i_ma & (SD_ADDRALIGN - 1)) != 0) {
		io->i_error = EIO;
		return(-1);
	}

	c = (func == READ) ? SCSI_READ : SCSI_WRITE;
	if(sd_docmd(io, (caddr_t)&c) != io->i_cc)
		return(-1);		/* returns error to user */
	return(io->i_cc);
}

/*
 * sd_docmd - execute a command to the scsi device.
 * 
 * This procedure does all the prepitory work needed to
 * execute a specific command. The actual talking to the device
 * isn't done here since this only fills in the command.
 * SEC_cmd() is used to execute the commands.
 *
 * NOTE: All data contained in the struct iob can not
 * be modified by the device driver because the upper level
 * calling code assumes that the driver
 * won't modify them. It turns out to be cheaper size-wise
 * to make local copy's of these variables.
 */
sd_docmd(io, arg)
	register struct iob *io;
	register caddr_t arg;
{
	register struct sd_cmd  *cmd;
	register struct sec_pup_info *hostinfo;
	register unit, status;
	u_int	error;
	char savearg;
	int	xfercnt, thiscount;
	u_int	blockno, memory;
	int dir = -1;
	int cmdsiz;

#if defined(DEBUG)
	int debug = io->i_debug;
#endif

	unit = io->i_unit;
	hostinfo = (struct sec_pup_info *)&sdinfo;
	if(SEC_set_dev(SCSI_BOARD(unit), SEC_SCSI_DEVNO(unit), hostinfo) == -1)
		_stop("sd_docmd: no such sec board");

	cmd  = (struct sd_cmd *)(hostinfo->sec_pup_dev_prog->dp_cmd);
	io->i_errcnt = 0;
	io->i_error = 0;
	xfercnt = ((io->i_cc+511) & ~511); /* force 512 byte aligned */
	blockno = (u_int)io->i_bn;
	if (*arg == SCSI_READ || *arg == SCSI_WRITE)
		memory = (u_int)io->i_ma;
	else
		memory = (u_int)io->i_buf;

retry:
	bzero((char *)cmd, SCSI_CMD_SIZE);
	cmd->sd_command = *arg;
	cmd->sd_unit = ((SCSI_UNIT(unit))<<5);

	switch (*arg) {

	/*
	 * Standard read write stuff, if there is an error
	 * that can't be corrected by retries then we'll
	 * rezero the unit once and try the command one more
	 * time.
	 */
	case SCSI_READ:
	case SCSI_WRITE:
		/*
		 * Must break into <2048 chunks for firmware.
		 */
		DEBUG_PRINT(debug>0, putchar(*arg == SCSI_READ ? 'R' : 'W'));
		SETBA(cmd, blockno);
		cmd->sd_bytes[2] = (u_char)(((xfercnt>>9) >= 7) ? 7 : (xfercnt>>9));	/* <=7 */
		thiscount = ((xfercnt>=3584) ? 3584 : xfercnt);

		/*
		 * The following code fills out an iat entry(s)
		 * based on whether the <4k transfer will cross
		 * a hardware imposed dma address boundary of 64K bytes.
		 *
		 * Requires a pointer to a two entry sec_iat array.
		 */
		SETMA(memory, thiscount, sdiat);
		
		/*
		 * Kick in head to do the command.
		 */
		error = SEC_cmd(SD_CMD6SZ, (u_char *)SEC_IATIFY(sdiat), thiscount, hostinfo, SDTIMEOUT);
		if((xfercnt>3584) && ((error&0xFF) == 0)) {
				memory += 3584;
				blockno += 7;
				xfercnt -= 3584;
				goto retry;
		}

		break;
#if !defined(BOOTXX)
	case SCSI_REZERO:
		DEBUG_PRINT(debug>0, putchar('Z'));
		error = SEC_cmd(SD_CMD6SZ, (u_char *)0, 0, hostinfo, SDTIMEOUT);
		break;

	case SCSI_TEST:
		DEBUG_PRINT(debug>0, putchar('T'));
		error = SEC_cmd(SD_CMD6SZ, (u_char *)0, 0, hostinfo, SDTIMEOUT);
		break;

	/*
	 * For these commands, arg is assumed to point to an area that is
	 * N + M bytes large, and contains:
	 *
	 *	N byte CDB (N==6 or N==10, based on which command)
	 *	M byte data area (M is usually based on allocation length)
	 */

	case SCSI_MSENSE:
	case SCSI_INQUIRY:
		dir = 0;		/* read */
		xfercnt = (u_char)arg[4];
		goto cmdexec;
	case SCSI_MODES:
		dir = 1;		/* write */
		xfercnt=(u_char)arg[4];	/* Extent list length */
		goto cmdexec;		/* Dirty but effective */

	case SCSI_FORMAT:		/* 4(extent list) + length of defect list */
	case SCSI_REASS:
		xfercnt=4+((u_char)arg[9])+((int)((u_char)arg[8])<<8);
cmdexec:
		thiscount = xfercnt;
		bcopy(arg, cmd, SD_CMD6SZ);
		cmdsiz = SD_CMD6SZ;
		cmd->sd_unit |= ((SCSI_UNIT(unit))<<5) | (cmd->sd_unit & 0x1f);
		if (dir)		/* write */
			bcopy (arg+cmdsiz,(caddr_t)memory,
			       thiscount);
		SETMA(memory, xfercnt, sdiat);
		DEBUG_PRINT(debug>0, putchar(*arg == SCSI_MODES ? 'M' : 'F'));

		error = SEC_cmd(SD_CMD6SZ, (u_char *)SEC_IATIFY(sdiat), thiscount, hostinfo, SDNOTIMEOUT);
		break;

	case SCSI_TRAN:
		DEBUG_PRINT(debug>0, putchar('X'));
		SETBA(cmd,(u_int)io->i_bn);
		error = SEC_cmd(SD_CMD6SZ, (u_char *)arg, io->i_cc, hostinfo, SDTIMEOUT);
		break;

	case SCSI_READC:
		DEBUG_PRINT(debug>0, putchar('X'));
		bcopy(arg, cmd, SD_CMDASZ);
		cmd->sd_unit |= ((SCSI_UNIT(unit))<<5) | (cmd->sd_unit & 0x1f);
		xfercnt = SIZE_CAP;
		thiscount = SIZE_CAP;
		io->i_bn = (((((cmd->sd_bytes[0] << 8) | cmd->sd_bytes[1]) << 8)
			     | cmd->sd_bytes[2]) << 8) | cmd->sd_bytes[3];
		cmdsiz = SD_CMDASZ;
		dir = 0;
		SETMA(memory, SIZE_CAP, sdiat);
		DEBUG_PRINT(debug>1, printf("\nmemory = %x\n", memory));
		error = SEC_cmd(SD_CMDASZ, (u_char *)SEC_IATIFY(sdiat), SIZE_CAP, hostinfo, 10 * SDTIMEOUT);
		break;

#endif
	default:
		_stop("sd: bad func");
	}
	/*
	 * Operation complete, check for errors and retry 
	 * (fall through) if needed.
	 */
	if(error == SEC_ERR_NONE && io->i_errcnt != 5) {
		if (dir == 0)			/* read */
			bcopy ((caddr_t)memory, arg+cmdsiz, thiscount);
		return(io->i_cc);
	}
#if defined(BOOTXX)
	goto retry;
#else
	/*
	 * Error Handling Policy for all commands except format:
	 *  	1. try 5 times		io->errcnt == 1-5
	 *	2. then rezero and	io->errcnt == 6
	 *	3. try 5 more times	io->errcnt == 7-11
	 * A soft error is any that completes successfully
	 * before step 3 is compete.  A hard error one that fails all 3.
	 * Error Handling Policy for the format command:
	 *	1. Hard error any errors becuase a retry takes 4 minutes.
	 */

#ifdef notdef
	/*
	 * A soft error which returned an extended sense key of 
	 * SDRECOVERED actually succeeded (with difficulty).
	 */
	if (xtend_sense && ((error & SDSENSEKEY) == SDRECOVERED)) {
		xtend_sense = 0;
		SEC_print_sense();
		printf("sd%d: soft error #%d err #%d (cc=%d,bn=%d)\n",
			unit, io->i_errcnt, error, io->i_cc, io->i_bn);
		printf("(Drive recovered)\n");
		return(io->i_cc);
	} else
#endif /* notdef */
	if (*arg != SCSI_FORMAT && !sd_expecterror) {
		++io->i_errcnt;
		if(io->i_errcnt <= 5) {
			if(io->i_errcnt == 5) {
				savearg = *arg;
				*arg = SCSI_REZERO;
			}
			SEC_print_sense();
			printf("sd%d: soft error #%d err #%d (cc=%d,bn=%d)\n",
				unit, io->i_errcnt, error, io->i_cc, io->i_bn);
			goto retry;
		}
		if(io->i_errcnt <= 11) {
			if(io->i_errcnt == 6) {
				*arg = savearg;
				goto retry;
			}
			SEC_print_sense();
			printf("sd%d: soft error #%d (cc=%d,bn=%d)\n",
				unit, io->i_errcnt-1, io->i_cc, io->i_bn);
			goto retry;
		}
	} else if (sd_expecterror) {
		sd_expecterror = 0;
		goto retry;
	} else {
		printf("sd%d: Formating error, format probably didn't take\n", unit);
	}

	/* Hard error time.. */
	SEC_print_sense();	/* print out request senes information */
	{	/* print out the command that error'd */
		int	i;
		u_char	*scsicmd;

		scsicmd = (u_char *)arg;
		printf("sd%d: errored command block: ", unit);
		for( i=0; i<6; i++)
			printf(" %x", *scsicmd++);
	}
	printf("sd%d: hard error (cc=%d,bn=%d)\n", unit, io->i_cc, io->i_bn);
	io->i_error = EHER;
	return(-1);
#endif
}

#if !defined(BOOTXX)
/*ARGSUSED*/
sdioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{

	register flag;
	io->i_cc = 0;

	switch(cmd) {
	/*
	 * Set or Clear debug flags
	 */
	 case SAIODEBUG:
		flag = (int)arg;
		if(flag > 0)
			io->i_debug |= flag;
		else
			io->i_debug  &= ~flag;
		break;

	/*
	 * Set offset to sector 0 of disk
	 */
	 case SAIOZSETBASE:
		io->i_boff = (daddr_t)arg;
		break;


	/*
	 * get offset to first sector of disk
	 */
	 case SAIOFIRSTSECT:
		*(int *)arg = 0;
		break;

	/*
	 * Get device data
	 */
	case SAIODEVDATA:
		*(struct st *)arg = *sdst;
		break;
	/*
	 * SCSI commands
	 */
	case SAIOSCSICMD:
		switch(*(u_char *)arg) {
		case SCSI_MODES:
		case SCSI_TRAN:
		case SCSI_READC:
		case SCSI_FORMAT:
		case SCSI_REASS:
		case SCSI_REZERO:
		case SCSI_TEST:
		case SCSI_INQUIRY:
		case SCSI_MSENSE:
			return(sd_docmd(io, arg));
		/*
		 * not yet
		 */
		case SCSI_RSENSE:
		case SCSI_SEEK:
		case SCSI_WRITEB:
		case SCSI_READB:
		case SCSI_RESRV:
		case SCSI_RELSE:
		case SCSI_STARTOP:
		case SCSI_RDIAG:
		case SCSI_SDIAG:
			printf("sd: ioctl not supported yet\n");
		default:
			printf("sd: bad ioctl type (0x%x)\n",*(u_char *)arg);
			io->i_error = ECMD;
			return -1;
		}
	default:
		printf("sd: bad ioctl (('%c'<<8)|%d)\n",
			(u_char)(cmd>>8),(u_char)cmd);
		io->i_error = ECMD;
		return -1;
	}
	return 0;
}
#endif
