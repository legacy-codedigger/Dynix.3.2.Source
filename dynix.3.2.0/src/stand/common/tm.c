/* $Copyright: $
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

#ifdef RCS
static char rcsid[]= "$Header: tm.c 1.4 1991/07/01 22:55:56 $";
#endif RCS

/*
 * SCSI tape device driver for the SSM (stand alone).
 *
 * This device driver will ONLY work for the ADSI and Emulex
 * tape target adapter and only with wangtek or archive drives.
 * Cipher drives are not supported because of the vendor unique
 * command required to support it's operation.
 *
 */

/* $Log: tm.c,v $
 *
 */

#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <machine/cfg.h>
#include "ssm.h"
#include "saio.h"
#include "scsi.h"
#include "ssm_scsi.h"
#include "tm.h"

#ifdef	SSMFW
extern	void action();
#endif	SSMFW


/*
 * tmopen() uses the following data for mode selection.  
 * An attempt is first made to have the drive use its 
 * default modes.  If that fails, modes are selected 
 * explicitly.
 */
static u_char	tm_default[TM_MODESEL_LEN] = { 
					0, 0, 0x10, 8, 0, 0, 0, 0, 0, 0, 2, 0 
}; 
static u_char	tm_5_25st[TM_MODESEL_LEN] = {  
					0, 0, 0x10, 8, 5, 0, 0, 0, 0, 0, 2, 0 
};

/*
 * One information structure is shared by all SSM/SCSI 
 * tape drives.  Each time a particular drive wants to
 * perform an operation it invokes ssm_get_devinfo() to
 * provide information about the device from the diagnostic 
 * tables.  Sharing works since standalone operations are 
 * synchronous and automic.
 */
static	struct ssm_sinfo tm_info;
static  struct scrsense *sensestrp = NULL;

#ifndef BOOTXX
/*
 * Flags to indicate when a space to the end of data 
 * is needed prior to writing, since the tape can only 
 * be written when at BOT or at end of valid data.
 */
static	u_char tm_write_space_eof[TM_NUM_FLAG_BYTES];

/*
 * Flags to indicate that the tape has been written
 * upon and needs a filemark appended.
 * Also inhibits read operations until closed.
 */
static	u_char tm_written[TM_NUM_FLAG_BYTES];

/* 
 * Flags to prevent operations past EOF/EOM and to
 * ensure that a filemark is written when closing
 * a file that has been written.
 */
static	u_char tm_eof[TM_NUM_FLAG_BYTES];
#endif BOOTXX

/*
 * tmopen()
 * 	Opens the unit by validating presense 
 * 	of the unit being opened.
 */
tmopen(io)
	register struct iob *io;
{
	int board = SCSI_BOARD(io->i_unit);
	int devno = SCSI_DEVNO(io->i_unit);
	int retry = 5;
	
#ifndef BOOTXX
	/*
 	 * Verify that its a SCSI type device.
	 */
	if ((int)SCSI_TYPE(io->i_unit) > 0) {
		printf("tm(%d): drive type not supported\n", io->i_unit);
		io->i_error = ENXIO;
		return;
	}
#endif

	/*
 	 * ssm_get_devinfo() will fill in the information 
	 * that describes this device and make certain
	 * the SSM knows about it.
	 *
	 * Then issue a SCSI test-unit-ready command 
	 * to ensure the device is ready for operation.
	 * Next rewind the media and select execution
	 * modes for this device.
	 *
	 * During mode selection attempt to use default
	 * settings.  If that fails, issue explicit 
	 * settings for a 5.25" streamer density.
	 */
#ifdef DEBUG
	printf("tmopen: Get device information.\n");
#endif
	if (!ssm_get_devinfo(board, devno, &tm_info, TM_CONTROL)) {
		printf("tm: no such SSM board or SCSI device.\n");
		io->i_error = ENXIO;
		return;
	} 

#ifdef DEBUG
	printf("tmopen: Issue Test-unit-ready.\n"); 
#endif
	/* Retry Test-unit-ready a few times to clear unit attention. */
	while (tm_docmd(io, SCSI_TEST, 0, 0, 0) == TM_CMD_FAIL) {
		if (!retry--) {
			printf("tm: tape unit not ready\n"); 
			io->i_error = EIO;
			return;
		}
	}

#ifdef DEBUG
	printf("tmopen: rewind the media.\n"); 
#endif
	if (tm_docmd(io, SCSI_REWIND, 0, 0, 0) == TM_CMD_FAIL) {
		printf("tm: rewind failed; media not ready\n"); 
		io->i_error = EIO;
		return;
	}
	
#ifdef DEBUG
	printf("tmopen: select the default modes.\n"); 
#endif
	if (tm_docmd(io, SCSI_MODES, 0, TM_MODESEL_LEN, tm_default) 
		   == TM_CMD_FAIL) {
#ifdef DEBUG
		printf("tmopen: select explicit modes (defaults failed).\n"); 
#endif
		if (tm_docmd(io, SCSI_MODES, 0, TM_MODESEL_LEN, tm_5_25st) 
		   	== TM_CMD_FAIL) {
			printf("tm: mode selection failed\n"); 
			io->i_error = EIO;
			return;
		}
	}
#ifdef DEBUG
	printf("tmopen: mode selection complete.\n"); 
#endif

#ifndef BOOTXX
	/*
	 * If writing to the front of the tape, 
	 * first erase entire tape and rewind.
	 */
	if (io->i_boff == 0 && io->i_howto != 0) {
		printf("Erasing tape, please wait (takes 2-3 minutes)...\n ");
		if (tm_docmd(io, SCSI_ERASE, SCSI_ERASE_LONG, 0, 0) 
		    == TM_CMD_FAIL 
		||  tm_docmd(io, SCSI_REWIND, 0, 0, 0) == TM_CMD_FAIL) {
			io->i_error = EIO;
			return;
		}
	}
#endif BOOTXX

	/*
	 * Locate the specified file.
	 */
	if (io->i_cc = io->i_boff) {
#ifndef BOOTXX
		tm_write_space_eof[TM_INDEX(io->i_unit)] |= 
			TM_FLAG(io->i_unit);
#endif BOOTXX
#ifdef DEBUG
		printf("tmopen: position at file #%d.\n", io->i_boff);
#endif
		if (tm_docmd(io, SCSI_SPACE, SCSI_SPACE_FILEMARKS, 
			      io->i_boff, 0) == TM_CMD_FAIL) {
			io->i_error = EIO;
#ifdef DEBUG
			printf("tmopen: positioning failed.\n");
#endif
			return;
		}
	}
#ifdef DEBUG
	printf("tmopen: media now position at file #%d. tmopen complete.\n", 
	io->i_boff);
#endif
}

#ifndef BOOTXX
/*
 * tmclose()
 *	Write a file mark at the end of the tape 
 *	if one has not been written and data has
 *	been.  Rewind the tape.
 *
 *	Note:  If a write was followed by a read, 
 *	read should error out.  This may also result
 *	in a bit for the unit being set in both the
 *	tm_written and tm_write_space_eof vectors.
 */
tmclose(io)
	register struct iob *io;
{
#ifdef DEBUG
	printf("tmclose: entered.\n");
#endif
	if ((tm_written[TM_INDEX(io->i_unit)] & TM_FLAG(io->i_unit))
	&&  !(tm_write_space_eof[TM_INDEX(io->i_unit)] & TM_FLAG(io->i_unit))
	&&  tm_docmd(io, SCSI_WFM, 0, 1, 0) == TM_CMD_FAIL) {
		io->i_error = EIO;
		printf("tm: could not write file mark.\n");
	} 

	tm_written[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);
	tm_write_space_eof[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);
	tm_eof[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);

#ifdef DEBUG
	printf("tmclose: rewind the media.\n");
#endif
	if (tm_docmd(io, SCSI_REWIND, 0, 0, 0) == TM_CMD_FAIL) {
		io->i_error = EIO;
		printf("tm: Rewind of media failed.\n");
	}
#ifdef DEBUG
	printf("tmclose: completed successfully.\n");
#endif
}
#endif BOOTXX

/*
 * tmstrategy() 
 * 	Executes a requested read or write.
 *	Must be at begining of tape or the 
 *	logical end of data to write.
 *
 *	Returns TM_CMD_FAILED upon failure
 *	and bytes transferred otherwise.
 */
tmstrategy(io, func)
	struct iob *io;
	int func;
{
	int xferbytes = roundup(io->i_cc, DEV_BSIZE);
	int n;

#ifdef BOOTXX
	if (func == READ) {
		n = (tm_docmd(io, SCSI_READ, 0, xferbytes, io->i_ma));
		return ((n == TM_CMD_FAIL) ? n : ((!n) ? 
			io->i_cc : xferbytes - n));
	} else {
		io->i_error = ECMD;
		return (TM_CMD_FAIL);
	}
#else BOOTXX
	if(tm_eof[TM_INDEX(io->i_unit)] & TM_FLAG(io->i_unit))
		return (0);		/* EOM or EOF encountered */

	switch (func) {			/* Base action on the function */
	case READ:
		tm_write_space_eof[TM_INDEX(io->i_unit)] |= 
			TM_FLAG(io->i_unit);
		n = (tm_docmd(io, SCSI_READ, 0, xferbytes, io->i_ma));
		break;
	case WRITE:
		if (tm_write_space_eof[TM_INDEX(io->i_unit)] 
	    	    & TM_FLAG(io->i_unit)) {
			n = tm_docmd(io, SCSI_SPACE, 
				      SCSI_SPACE_ENDOFDATA, 0, 0);
			if (n == TM_CMD_FAIL)
				return (TM_CMD_FAIL);

			tm_write_space_eof[TM_INDEX(io->i_unit)] &= 
				~TM_FLAG(io->i_unit);
		}
		tm_written[TM_INDEX(io->i_unit)] |= TM_FLAG(io->i_unit);
			TM_FLAG(io->i_unit);
		n = (tm_docmd(io, SCSI_WRITE, 0, xferbytes, io->i_ma));
		break;
	default:
		io->i_error = ECMD;
		return (TM_CMD_FAIL);
	}
	return ((n == TM_CMD_FAIL) ? n : ((!n) ? io->i_cc : xferbytes - n));
#endif BOOTXX
}

/*
 * tm_docmd() 
 * 	Execute SCSI command to the device.
 *
 *	Returns TM_CMD_FAILED upon total 
 *	failure, zero for complete success,
 *	and the number of bytes or operations
 *	not completed otherwise.
 *	
 *	If the caller is tmioctl() then the
 *	cmd argument will be TM_IOCTL and
 *	the SCSI command block and related 
 *	data is addressed by the data argument. 
 *	The related data follows immediately 
 *	after the command block. 'count' is
 *	the total number of bytes addressed
 *	by 'data'.  tmioctl verfies that
 *	the command is valid.
 */
tm_docmd(io, cmd, modifiers, count, data)
	register struct iob *io;
	int cmd;
	u_int modifiers;
	int count;
	char *data;
{
	register struct scsi_cb	*cb;
	int board = SCSI_BOARD(io->i_unit); 
	int devno = SCSI_DEVNO(io->i_unit);
	int blocks;
#ifndef BOOTXX
	u_char *cp;
	int not_xferred;
	struct scrsense *rs;
#endif BOOTXX

	/* Allocate the sense structure if it has not yet been done */
	if (sensestrp == NULL) {
#ifdef	DEBUG
		printf("tm_docmd: allocating sense struct\n");
#endif	/* DEBUG */
		sensestrp = (struct scrsense *)ssm_alloc(sizeof(struct scrsense),
			SSM_ALIGN_XFER, SSM_BAD_BOUND);
		tm_info.si_cb->sh.cb_sense = (ulong) sensestrp;
		tm_info.si_cb->sh.cb_slen = sizeof(struct scrsense);
	}
#ifndef	SSMFW
	if (!ssm_get_devinfo(board, devno, &tm_info, TM_CONTROL)) {
		printf("tm_docmd: no such SSM board or device\n");
		io->i_error = ENXIO;
		return(TM_CMD_FAIL);
	}
#else	SSMFW
#endif	SSMFW

	io->i_errcnt = 0;
	io->i_error = 0;

	/*
	 * Build an I/O CB for the command.
	 */
	cb = tm_info.si_cb; 			/* Command independent stuff */
	bzero (SWBZERO(cb), SWBZERO_SIZE); 
	cb->sh.cb_scmd[1] = (SCSI_UNIT(io->i_unit)) << 5;
	cb->sh.cb_clen = SCSI_CMD6SZ;
	switch (cmd) {	     			/* Command specific stuff */
	case SCSI_READ:
#ifndef BOOTXX
	case SCSI_WRITE:
#endif BOOTXX
		/*
	 	 * For errors uncorrected after retries 
		 * rezero the unit once and retry once more.
		 */
		blocks = btodb(count);
		cb->sh.cb_cmd = (cmd == SCSI_READ) ? SCB_READ : SCB_WRITE; 
		cb->sh.cb_scmd[0] = cmd;
		cb->sh.cb_scmd[1] |= SCSI_FIXED_BLOCKS;
		cb->sh.cb_scmd[2] = (u_char)(blocks >> 16); 
		cb->sh.cb_scmd[3] = (u_char)(blocks >> 8); 
		cb->sh.cb_scmd[4] = (u_char)blocks;
		cb->sh.cb_addr = (u_long)data;
		cb->sh.cb_count = count;
#ifdef DEBUG
		if (cmd == SCSI_READ) 
			printf("tm_docmd: reading %d bytes (%d blocks).\n",
				count, blocks);
		else 
			printf("tm_docmd: writing %d bytes (%d blocks).\n",
				count, blocks);
#endif DEBUG
		break;
#ifndef BOOTXX
	case SCSI_ERASE:
	case SCSI_WFM:
#endif BOOTXX
	case SCSI_SPACE:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[0] = cmd;
		cb->sh.cb_scmd[1] |= (u_char)modifiers;
		cb->sh.cb_scmd[2] = (u_char)(count >> 16); 
		cb->sh.cb_scmd[3] = (u_char)(count >> 8); 
		cb->sh.cb_scmd[4] = (u_char)count;
#ifndef BOOTXX
		tm_written[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);
#endif BOOTXX
#ifdef DEBUG
		switch (cmd) {
		case SCSI_ERASE:
			printf("tm_docmd: erasing the tape (modifier %d).\n",
				modifiers); 
			break;
		case SCSI_WFM:
			printf("tm_docmd: writing %d filemarks.\n", count); 
			break;
		case SCSI_SPACE:
			if (modifiers == SCSI_SPACE_FILEMARKS) {
				printf("tm_docmd: positioning media ");
				printf("%d filemarks.\n", count); 
			} else {
				printf("tm_docmd: positioning media to ");
				printf("the end-of-data.\n");
			}
			break;
		}
#endif DEBUG
		break;
	case SCSI_TEST:
	case SCSI_REWIND:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[0] = cmd;
		break;
	case SCSI_MODES: 
		cb->sh.cb_cmd = SCB_WRITE;
		cb->sh.cb_scmd[0] = cmd;
		cb->sh.cb_scmd[4] = (u_char)count;
		cb->sh.cb_addr = (u_long)data;
		cb->sh.cb_count = (u_long)count;
		break;
#ifndef BOOTXX
	case TM_IOCTL:
		/*
		 * This is a special case where the
		 * caller provides the SCSI command 
 		 * block and any other data.  
		 *
		 * Supported functions are 6-byte SCSI
		 * commands, which are validated by
		 * tmioctl prior to comming here. The
		 * byte count of the data passed will
		 * be validated here, then the SCSI
		 * command portion is copied into the
		 * CB.  If any data remains its size
		 * and address are stored in the CB.
		 *
		 * The logical unit number will be 'or'ed
		 * into the SCSI command block here.
		 */
		if (count < SCSI_CMD6SZ) {
			printf("tm: Illegal operation rejected %x\n", cmd);
			io->i_error = ECMD;
			return(TM_CMD_FAIL);
		} else {
			bcopy(data, cb->sh.cb_scmd, SCSI_CMD6SZ);
			cb->sh.cb_scmd[1] |= (SCSI_UNIT(io->i_unit)) << 5;
			if (count -= SCSI_CMD6SZ) {
				cb->sh.cb_cmd = SCB_WRITE;
				cb->sh.cb_addr = (u_long)data + SCSI_CMD6SZ;
				cb->sh.cb_count = count;
			} else
				cb->sh.cb_cmd = SCB_READ;
		}
		break;
	default:
		printf("tm: Illegal operation rejected %x\n", cmd);
		io->i_error = ECMD;
		return(TM_CMD_FAIL);
#endif BOOTXX
	}

	/*
	 * Start the CB executing by notifying 
	 * the SSM of it, then await its completion.
	 */
	cb->sh.cb_compcode = SCB_BUSY;

#ifndef	SSMFW
	mIntr(tm_info.si_slic, SCSI_BIN, SCVEC(tm_info.si_id, 0));
#else	SSMFW

	/*
	 *	longest tape transaction is re-tension, approx. 90 seconds
	 */
	action(SCVEC(tm_info.si_id, 0),90000);
#endif	SSMFW
	while (cb->sh.cb_compcode == SCB_BUSY) {
		continue;		/* Poll for completion */
	}

	/*
	 * Take action based on termination status.
	 */
	 if (cb->sh.cb_compcode != SCB_OK) {
		/* Interface timeout or error */
#ifdef DEBUG
		printf("tm_docmd: bad compcode = 0x%x.\n", cb->sh.cb_compcode);
#endif DEBUG
#ifndef BOOTXX
	 	/*
	 	 * Print out the failed command.
	  	 */
		printf("tm(%d): interface error occurred (cc=%d,bn=%d)\n", 
			io->i_unit, io->i_cc, io->i_bn);
		printf("The SCSI cmd that created the error:");
		cp = cb->sh.cb_scmd;
		while (cb->sh.cb_clen--)
			printf(" %x", *cp++);
		printf("\n");
#endif BOOTXX
		io->i_error = EHER;
		return (TM_CMD_FAIL);
#ifndef BOOTXX
	 } else if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
		/* Take action based on the request sense data. */
		rs = (struct scrsense *) cb->sh.cb_sense;

		if ((rs->rs_error & RS_VALID) == 0 
		||  (rs->rs_error & RS_CURERR) != RS_CURERR) {
#ifdef DEBUG
			printf("tm_docmd: Invalid or not current Check ");
			printf("Condition (error code 0x%x).\n", rs->rs_error);
#endif DEBUG
			return (TM_CMD_FAIL);	/* Lost the error info */
		} else {
			not_xferred = rs->rs_info[0] << 24;
			not_xferred |= rs->rs_info[1] << 16;
			not_xferred |= rs->rs_info[2] << 8;
			not_xferred |= rs->rs_info[3];
#ifdef DEBUG
			printf("tm_docmd: Check Condition occurred ");
			printf("(key 0x%x, residue %d).\n", 
				rs->rs_key, not_xferred);
#endif DEBUG
			switch (rs->rs_key & RS_KEY) { 
			case RS_PROTECT:
				printf("Write protected, no transfer\n");
				io->i_error = EWCK;
				return (TM_CMD_FAIL);
			case RS_RECERR:
				printf("tm(%d): recovered from error 0x%x ",
					io->i_unit, rs->rs_key);
				printf("(cc=%d,bn=%d)\n", io->i_cc, io->i_bn);
				ssm_print_sense(cb);
				/*
		 		 * Fall through into NOSENSE case.
		 	 	 */
			case RS_NOSENSE:
			case RS_OVFLOW:
			case RS_UNITATTN:
				if (rs->rs_key & (RS_EOM | RS_FILEMARK))
					tm_eof[TM_INDEX(io->i_unit)] |= 
						TM_FLAG(io->i_unit);
				return (dbtob(not_xferred));
			case RS_BLANK:
				if (rs->rs_key & (RS_EOM | RS_FILEMARK)) {
					tm_eof[TM_INDEX(io->i_unit)] |= 
						TM_FLAG(io->i_unit);
					return (dbtob(not_xferred));
				}
				break;	/* go handle the harderror */
			case RS_MEDERR:
			case RS_CPABORT:
				if (rs->rs_key & RS_EOM)
					return (dbtob(not_xferred));
				break;	/* go handle the harderror */
			default:
				break;	/* go handle the harderror */
			}

			/* 
			 * If it came here a hard error 
			 * occurred (all other cases returned).
	 		 * Print out request sense information 
	 		 * and the command itself.
	 		 */
			printf("tm(%d): hard error 0x%x (cc=%d,bn=%d)\n", 
				io->i_unit, rs->rs_key, io->i_cc, io->i_bn);
			ssm_print_sense(cb);
			printf("The SCSI cmd that created the error:");
			cp = cb->sh.cb_scmd;
			while (cb->sh.cb_clen--)
				printf(" %x", *cp++);
			printf("\n");
			io->i_error = EHER;
			return (TM_CMD_FAIL);
		}
#endif BOOTXX
	 } else if (!SCSI_GOOD(cb->sh.cb_status)) {
#ifdef DEBUG
		printf("tm_docmd: unexpected termination status (0x%x).\n",
			cb->sh.cb_status);
#endif DEBUG
		io->i_error = EHER;
		return (TM_CMD_FAIL);	/* Unknown program error */
	 } else
		return (0);		/* Successful */
}

#ifndef BOOTXX
/*
 * tmioctl()
 *	Executes driver dependent commands 
 *	through the IOCTL interface. 
 */
tmioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{
	int n;
	io->i_cc = 0;
	/* 
	 * Determine the type of the IOCTL
	 * and execute its semantics.
	 */
	switch (cmd) {
	case SAIODEBUG:
		/*
	 	 * Set or Clear debug flags
	 	 */
		if ((int)arg > 0) 
			io->i_debug |= (int)arg;
		else
			io->i_debug  &= ~(int)arg;
		n = 0;
		break;
	case SAIOSCSICMD:
		/*
	 	 * SCSI commands. Verify the SCSI command 
		 * type and take action based upon it 
		 * (attempt the command and set/clear state 
		 * flags).  
		 *
		 * tm_docmd() expects the SCSI command type 
		 * to be validated here, and that it is a 
		 * 6-byte type commands block.  The block 
		 * address and associated data must be 
		 * sequential and described by the data 
		 * address and size arguments. 
	 	 */
		switch (*(u_char *)arg) {
		case SCSI_STARTOP:
		case SCSI_REWIND:
		case SCSI_ERASE:
			/* 
			 * Results in tape positioned to BOT.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			if (n != TM_CMD_FAIL) {
				tm_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_written[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_write_space_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
			}
			break;
		case SCSI_SPACE:
			/* 
			 * Always positions tape away from BOT.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			if (n != TM_CMD_FAIL) {
				tm_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_written[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_write_space_eof[TM_INDEX(io->i_unit)] |= 
					TM_FLAG(io->i_unit);
			}
			break;
		case SCSI_TEST:
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			break;
		case SCSI_WFM:
			/* 
			 * Writes a filemark on the tape.
			 * Sets eof flag for close.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			if (n != TM_CMD_FAIL) {
				tm_written[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_write_space_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
			}
			break;
		case SCSI_MODES:
			/*
			 * Sets drive execution modes.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, 
			       SCSI_CMD6SZ + arg[4], arg);
			break;
		default:
			printf("tm: SCSI ioctl command type (0x%x)\n",
				*(u_char *)arg);
			io->i_error = ECMD;
			n = TM_CMD_FAIL;
			break;
		}
		return(n);
	default:
		printf("tm: bad ioctl type (('%c'<<8)|%d)\n",
			(u_char)(cmd>>8),(u_char)cmd);
		io->i_error = ECMD;
		n = TM_CMD_FAIL;
		break;
	}
	return (n);
}
#endif BOOTXX
