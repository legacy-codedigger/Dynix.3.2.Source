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
static char rcsid[]= "$Header: ts.c 2.4 90/10/05 $";
#endif RCS

/*
 * SCSI disk device driver (stand alone)
 *
 * This device driver will ONLY work for the ADSI and Emulex
 * tape target adapter and only with wangtek or archive drives.
 * Cipher drives are not supported because of the vendor unique
 * command required to support it's operation.
 *
 * TODO:
 *	Tests:
 *		Need to test the scsi_tran(slate) command.
 *		
 *	Unimplemented:
 *		case SCSI_SEEK:
 *		case SCSI_INQUIRY:
 *		case SCSI_WRITEB:
 *		case SCSI_READB:
 *		case SCSI_RESRV:
 *		case SCSI_RELSE:
 *		case SCSI_MSENSE:
 *		case SCSI_STARTOP:
 *		case SCSI_RDIAG:
 *		case SCSI_TSIAG:
 *			printf("ts: ioctl not supported yet\n");
 *		
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <machine/cfg.h>
#include "sec.h"
#include "sec_ctl.h"
#include "saio.h"
#include "scsi.h"
#include "tsreg.h"

#define TSTIMEOUT	50000000
#define TSNOTIMEOUT	-1

#define TS_CMD6SZ	6		/* scsi command length */

/*
 * Data structures
 */
#define i_debug i_cyloff		/* unused parameter */
					/* tables for partition mapping */

#ifndef DISCONNECT
	struct	sec_smode md;
#endif

/*
 * This should be binary configurable below
 */
u_char	tsdefault[] = { SCSI_MODES, 0, 0, 0, 0x0c, 0, 0, 0, 0x10, 8, 0,
			0, 0, 0, 0, 0, 2, 0 };
u_char	ts5_25st[] = { SCSI_MODES, 0, 0, 0, 0xc0, 0, 0, 0, 0x10, 8, 5,
			0, 0, 0, 0, 0, 2, 0 };

struct tstypes tsmodes[] = { tsdefault, ts5_25st };	/* Issued on open */
int	tsmaxmodes = sizeof(tsmodes) / sizeof(struct tstypes);
int	TsMode;		/* Used to differenciate open's from ioctls */


/* End of binary configureable stuff */

static	struct sec_pup_info tsinfo;
static	struct sec_dev_prog tsprog;
static	struct sec_iat	tsiat[2];

#define TSERR	16
static	u_char tserrbuf[TSERR];

/*
 * Macro to find the index into a flag byte array for
 * bit manipulation.
 */
#define TS_INDEX(unit)	(SCSI_BOARD((unit))*8+SCSI_TARGET((unit)))
#define TSSANDT		32		/* 4 sec's x 8 ta's */

/*
 * Flags to insure eof handled properly for writing
 * in that the tape can be only written when at BOT
 * or at end of valid data(logical EOM).
 */
static	u_char tswrite_space_eof[TSSANDT];
static	u_char tswritten[TSSANDT];

/*
 * Flags to notify the next request to the strategy routine
 * that the EOF or EOM has been reached.
 */
static	u_char tseof[TSSANDT];

/*
 * tsopen - Driver open procedure.
 *
 * This procedure opens the unit by varifing that a partition
 * for which the open refers is valid.
 */
tsopen(io)
	register struct iob *io;
{
	struct	ts_cmd	tc;
	int	offset;
	static	char	setmode = 1;
	int	err;
	int	i;

	
#if !defined(BOOTXX)
	if((int)SCSI_TYPE(io->i_unit) > 0) {
		printf("ts%d: drive type not supported\n", io->i_unit);
		io->i_error = ENXIO;
		return;
	}
#endif

#ifndef DISCONNECT
	setmode = 0;
	
	if(SEC_set_dev(SCSI_BOARD(io->i_unit), 
	   SEC_SCSI_DEVNO(io->i_unit), &tsinfo) == -1) {
		printf("ts: no such sec board\n");
		io->i_error = ENXIO;
		return;
	}
	md.sm_status = 0;
	md.sm_un.sm_scsi.ssm_timeout = 0;
	md.sm_un.sm_scsi.ssm_flags = 2;
	SEC_startio(	SINST_SETMODE,
			&md.sm_status,
			6,		/* bin # */
			tsinfo.sec_target,
			(struct sec_cib *)tsinfo.sec_pup_q,
			tsinfo.sec_pup_desc.sec_slicaddr
	);
#endif

	/*
	 * Install the device program into the tsinfo struct
	 * and initialize all other pertinent data.
	 */
	tsinfo.sec_pup_dev_prog = &tsprog;

#if !defined(BOOTXX)
	if(SEC_set_dev(SCSI_BOARD(io->i_unit), 
	   SEC_SCSI_DEVNO(io->i_unit), &tsinfo) == -1) {
		printf("ts: no such sec board\n");
		io->i_error = ENXIO;
		return;
	}
#endif
	
	
	/*
	 * NOTE: not zeroing buffer first since only using first byte
	 * 	 SCIS_TEST and SCSI_REWIND commands.
	 */
	tc.ts_command = SCSI_TEST;
	if((err=ts_docmd(io, &tc)) == -1) {
		printf("ts: tape not ready\n");		/* 1 */
		io->i_error = EIO;
		return;
	}

	/*
	 * Set buffered mode on.
	 * (Only possible at BOT)
	 * NOTE: not zeroing buffer first since only using first byte
	 * 	 SCIS_TEST and SCSI_REWIND commands.
	 */
	tc.ts_command = SCSI_REWIND;
	if((err = ts_docmd(io, &tc)) == -1) {
		printf("ts: tape not ready\n");		/* 1 */
		io->i_error = EIO;
		return;
	}

	/*
	 * Handle case where the target adapter ignores the standard 
	 * mode select command and issue the first alternate which is
	 * to try to select using a 5.25" streamer density mode.
	 */
	for(i=0; i<tsmaxmodes; i++) {
		TsMode = 1;
		if((err = ts_docmd(io, tsmodes[i])) == 0x5)	/* operator error */
			continue;
		TsMode = 0;
		break;
	}

#if !defined(BOOTXX)
	/*
	 * If writing and to the front of the
	 * tape, erase entire tape first
	 */
	if (io->i_boff == 0 && io->i_howto != 0) {
		printf("..Erasing tape, please wait (takes 2-3 minutes).. ");
		bzero(&tc, sizeof(tc));		/* must zero */
		tc.ts_command = SCSI_ERASE;
		tc.ts_unit = 0x01;		/* entire media bit */
		err = ts_docmd(io, &tc);

		bzero(&tc, sizeof(tc));		/* must zero */
		tc.ts_command = SCSI_REWIND;
		err |= ts_docmd(io, &tc);
		printf("\n");
		if(err == -1) {
			io->i_error = EIO;
			return;
		}
	}
#endif BOOTXX

	/*
	 * Space to the proper file
	 * if needed.
	 */
	if (offset = io->i_cc = io->i_boff) {
		int	loffset;

		while(offset > 0) {

			if(offset >= 0x7f)
				loffset = 0x7f;		/* maximum forward */
			else
				loffset = offset;
			tswrite_space_eof[TS_INDEX(io->i_unit)] |= 1<<SCSI_UNIT(io->i_unit);
			bzero(&tc, sizeof(tc));
			tc.ts_command = SCSI_SPACE;
			tc.ts_unit = 0x01;		/* Files forward */
			tc.ts_bytes[2] = (u_char)loffset;
			if((err = ts_docmd(io, &tc)) == -1) {
				io->i_error = EIO;
				return;
			}
			offset -= loffset;
		}
	}
	/* no return value */
}

#if !defined(BOOTXX)
tsclose(io)
	register struct iob *io;
{
	u_char	tcmd[6];
	int	err;

	/* 
	 * if data was written, write a file mark at the end.
	 * to insure that one gets written.
	 */
							/* 1 v */
	if(tswritten[TS_INDEX(io->i_unit)] & (1<<SCSI_UNIT(io->i_unit))) {
		bzero(tcmd, 6);
		tcmd[0] = SCSI_WFM;
		tcmd[4] = 1;
		err = ts_docmd(io, tcmd);
	}

	bzero(tcmd, 6);		/* clean cmd */		/* 1 */
 	tcmd[0] = SCSI_REWIND;				/* 1 */
	tswrite_space_eof[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
	tseof[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
	err = ts_docmd(io, tcmd);
}
#endif BOOTXX

/*
 * tsstrategy - standard read/write routine.
 *
 * This procedure executes a request for a read or a write to
 * the device.
 */
tsstrategy(io, func)
	struct iob *io;
{
	struct	ts_cmd	tc;
	u_char c;

	if(tseof[TS_INDEX(io->i_unit)] & 1<<SCSI_UNIT(io->i_unit))
		return(0);		/* EOM or EOF */

	switch( func ) {
		case READ:
			c = SCSI_READ;
			tswrite_space_eof[TS_INDEX(io->i_unit)] |= 1<<SCSI_UNIT(io->i_unit);
			break;
#if !defined(BOOTXX)
		case WRITE:
			c = SCSI_WRITE;
			if(tswrite_space_eof[TS_INDEX(io->i_unit)] & 1<<SCSI_UNIT(io->i_unit)) {
				bzero(&tc, sizeof(tc));
				tc.ts_command = SCSI_SPACE;
				tc.ts_unit = ((SCSI_UNIT(io->i_unit))<<5) | 0x03;	/* All the way to the end */
				tc.ts_bytes[2] = 0;				/* Only forward */
				ts_docmd(io, &tc);
				tswrite_space_eof[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
			}
			tswritten[TS_INDEX(io->i_unit)] |= (1<<SCSI_UNIT(io->i_unit));
			break;
		default:
			io->i_error = ECMD;
			return -1;
#endif BOOTXX
	}
	return(ts_docmd(io, &c));
}

/*
 * ts_docmd - execute a command to the scsi device.
 * 
 * This procedure does all the prepitory work needed to
 * execute a specific command. The actual talking to the device
 * isn't done here since this only fills in the command.
 * SEC_cmd() is used to execute the commands.
 *
 * NOTE: All data contained in the struct iob can not
 * be modified by the device driver because the upper level
 * calling code assumes (tisk-tisk...) that the driver 
 * won't modify them. It turns out to be cheaper size-wise
 * to make local copy's of these variables.
 */
ts_docmd(io, arg)
	register struct iob *io;
	register caddr_t arg;
{
	register struct ts_cmd  *cmd;
	register struct sec_pup_info *hostinfo;
	register unit, status;
	u_int	 err;
	char	 savearg;
	int	 xfercnt, thiscount;
	u_int	 blocks, memory;

#ifdef DEBUG
	int debug = io->i_debug;
	extern	int sec_debug;
#endif

	unit = io->i_unit;
	hostinfo = (struct sec_pup_info *)&tsinfo;
	if(SEC_set_dev(SCSI_BOARD(unit), SEC_SCSI_DEVNO(unit), hostinfo) == -1) {
		printf("ts_docmd: no such sec board\n");
		io->i_error = ENXIO;
		return(-1);
	}

	cmd  = (struct ts_cmd *)(hostinfo->sec_pup_dev_prog->dp_cmd);
	io->i_errcnt = 0;
	io->i_error = 0;
	thiscount = xfercnt = ((io->i_cc+511) & ~511); /* force 512 byte aligned */
	memory = (u_int)io->i_ma;
domoredata:
	bzero(cmd, SCSI_CMD_SIZE);
	cmd->ts_command = *arg;
	cmd->ts_unit |= ((SCSI_UNIT(unit))<<5);

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
		 * Must break into <4096 byte chunks for firmware.
		 */
#ifdef DEBUG
		if(debug > 0)
			putchar(*arg == SCSI_READ ? 'r' : 'w');
#endif
		blocks = (u_char)(((xfercnt>>9) >= 7) ? 7 : (xfercnt>>9));	/* <=7 */
		SETBC(cmd, blocks);
		thiscount = ((xfercnt>=3584) ? 3584 : xfercnt);

		/*
		 * The following code fills out an iat entry(s)
		 * based on whether the <4k transfer will cross
		 * a hardware imposed dma address boundary of 64K bytes.
		 *
		 * Requires a pointer to a two entry sec_iat array.
		 */
		SETMA(memory, thiscount, tsiat);
		
		/*
		 * Kick in head to do the command.
		 */
		err = SEC_cmd(TS_CMD6SZ, (u_char *)SEC_IATIFY(tsiat), thiscount, hostinfo, TSTIMEOUT);
		if((xfercnt>3584) && ((err&~SSENSE_RECOVERABLE) == 0)) { /* 1 */
				memory += 3584;
				xfercnt -= 3584;
				goto domoredata;
		}
		break;

#if !defined(BOOTXX)
	case SCSI_ERASE:
	case SCSI_STARTOP:
	case SCSI_WFM:
#endif BOOTXX
	case SCSI_SPACE:
		bcopy(arg, cmd, TS_CMD6SZ);
		cmd->ts_unit |= ((SCSI_UNIT(unit))<<5);
		err = SEC_cmd(TS_CMD6SZ, (u_char *)0, 0, hostinfo, TSNOTIMEOUT);
		tswritten[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
		break;

	case SCSI_TEST:
		err = SEC_cmd(TS_CMD6SZ, (u_char *)0, 0, hostinfo, TSTIMEOUT);
		break;

	case SCSI_REWIND:
		err = SEC_cmd(TS_CMD6SZ, (u_char *)0, 0, hostinfo, TSNOTIMEOUT);
		break;

	case SCSI_MODES: 
		thiscount = xfercnt=(u_char)arg[4];	/* Extent list length */
		bcopy(arg, cmd, TS_CMD6SZ);
		cmd->ts_unit |= ((SCSI_UNIT(unit))<<5);
		err = SEC_cmd(TS_CMD6SZ, (u_char *)(arg+TS_CMD6SZ), 
			xfercnt, hostinfo, TSNOTIMEOUT);

		/*
		 * Next lines handle an error on mode select on open.
		 * All others hard error.
		 */
		if(TsMode && ((err & TSKEY) == SSENSE_ILL_REQ))	
			/* 5 == command sequence error from ta */
			return(err & TSKEY);
		break;

#if !defined(BOOTXX)
	default:
		printf("ts: bad func %x\n", *arg);
		io->i_error = ECMD;
		return(-1);
#endif BOOTXX
	}
	/*
	 * Operation complete, check for errors and retry 
	 * (fall through) if needed.
	 */

	if(err & (TSFM|TSEOM)) {
		tseof[TS_INDEX(io->i_unit)] |= 1<<SCSI_UNIT(io->i_unit);
		if(((err & TSKEY) == SSENSE_MEDIA_ERR) && (err & TSEOM))
			err = 0; 	/* Zap to prevent h/w error on eom (adsi)*/
		if(((err & TSKEY) == SSENSE_ABORT) && (err & TSEOM))		/* 1 */
			err = 0; 	/* Emulex error on eom  1 */
		/* 
		 * Handle blank tape on eom Emulex error on eom  2 
		 */
		if((err & TSKEY) == SSENSE_BLANK)	/* 2 */
			err = 0; 
	}

	if((err & TSKEY) == SSENSE_UNIT_ATN)
		return(io->i_cc);	/* ok, just media change */
	
	if((err & TSKEY) == SSENSE_DATA_PROT) {	/* Write protect */
		printf("Write protected, no xfer\n");
		io->i_error = EWCK;
		return(-1);
	}

	if((err & TSKEY) == SSENSE_VOL_OVER 
	|| (err & TSKEY) == SSENSE_NOSENSE 
	|| (err & TSKEY) == SSENSE_RECOVERABLE) {
		/* Partial request completed */
		extern int	scsi_info_bytes;
	
#if !defined(BOOTXX)
		if((err & TSKEY) == SSENSE_RECOVERABLE) {
			printf("ts%d: soft error 0x%x (cc=%d,bn=%d)\n", unit, err, io->i_cc, io->i_bn);
			SEC_print_sense();
		}
#endif BOOTXX
		xfercnt = xfercnt - thiscount + scsi_info_bytes * 512;
#if !defined(BOOTXX)
		if(xfercnt<0)
			printf("Warning: transfer lost more than requested %d lost %d\n", thiscount, xfercnt);
#endif BOOTXX
		return(io->i_cc-xfercnt);
	}


#if !defined(BOOTXX)
	/*
	 * Hard error time..
	 * Print out the request sense information as part of the
	 * error message.
	 */
	printf("ts%d: hard error 0x%x (cc=%d,bn=%d)\n", unit, err, io->i_cc, io->i_bn);
	SEC_print_sense();
	{
		int	i;
		/*
		 * printout error'd command
		 */
		printf("First six bytes of Error'd cmd:");
		for(i=0;i<6;i++)
			printf(" %x",(hostinfo->sec_pup_dev_prog->dp_cmd[i]));
		printf("\n");
	}
#endif

	io->i_error = EHER;

	return(-1);
}

/*ARGSUSED*/

#if !defined(BOOTXX)
tsioctl(io, cmd, arg)
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
		if(flag > 0) {
#ifdef DEBUG
			sec_debug |= flag;
#endif
			io->i_debug |= flag;
		}else{
#ifdef DEBUG
			sec_debug &= ~flag;
#endif
			io->i_debug  &= ~flag;
		}
		break;

	/*
	 * Get device data
	 */
	case SAIODEVDATA:
		io->i_error = ECMD;
		return -1;
	/*
	 * SCSI commands
	 */
	case SAIOSCSICMD:
		switch(*(u_char *)arg) {
		/* 
		 * Always get's to BOT.
		 */
		case SCSI_STARTOP:
		case SCSI_REWIND:
		case SCSI_ERASE:
			tseof[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
			tswrite_space_eof[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
			return(ts_docmd(io, arg));
		/*
		 * Always gets away from BOT.
		 * Except for going all of the way out to NDT area which
		 * will be done twice on writes and is redundant.
		 */
		case SCSI_SPACE:
			tseof[TS_INDEX(io->i_unit)] &= ~(1<<SCSI_UNIT(io->i_unit));
			tswrite_space_eof[TS_INDEX(io->i_unit)] |= 1<<SCSI_UNIT(io->i_unit);
		case SCSI_MODES:
		case SCSI_TEST:
		case SCSI_RSENSE:
		case SCSI_WFM:
			return(ts_docmd(io, arg));
		/*
		 * not yet
		 */
		case SCSI_TRAN:
		case SCSI_FORMAT:
		case SCSI_INQUIRY:
		case SCSI_WRITEB:
		case SCSI_READB:
		case SCSI_RESRV:
		case SCSI_RELSE:
		case SCSI_MSENSE:
		case SCSI_RDIAG:
			printf("ts: ioctl not supported yet\n");
		default:
			printf("ts: bad ioctl type (0x%x)\n",*(u_char *)arg);
			io->i_error = ECMD;
			return -1;
		}

	default:
		printf("ts: bad ioctl (('%c'<<8)|%d)\n",
			(u_char)(cmd>>8),(u_char)cmd);
		io->i_error = ECMD;
		return -1;
	}
	return 0;
}
#endif BOOTXX
