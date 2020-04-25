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

#ident	"$Header: wd.c 1.4 90/10/05 $"

/* $Log:	wd.c,v $
 */

/*
 * SSM SCSI disk device driver (stand alone).
 *
 * This device driver assumes that it will run single user
 * single processor. There is *no* Mutual exclusion
 * done at all.
 *
 *	Unimplemented commands:
 *		 SCSI_SEEK:
 *		 SCSI_WRITEB:
 *		 SCSI_READB:
 *		 SCSI_RESRV:
 *		 SCSI_RELSE:
 *		 SCSI_STARTOP:
 *		 SCSI_RDIAG:
 *		 SCSI_SDIAG:
 * 		 SCSI_TRAN:
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/vm.h>
#include <sys/reboot.h>
#include <machine/cfg.h>
#include <sys/vtoc.h>
#include "saio.h"
#include "ssm.h"
#include "scsi.h"
#include "ccs.h"
#include "ssm_scsi.h"
#include "scsiioctl.h"
#include "scsidisk.h"
#include "wd.h"

extern	struct drive_type wddrive_table[];
#ifdef	SSMFW
extern	void action();
extern	void relinquish();
#endif	/* SSMFW */

void	wd_sprintcb();
void	wd_sprintsense();
void	wd_error();

static	struct ssm_sinfo wdinfo;
static	struct st *geom;		/* geometry of opened drive */

/*
 * wdopen - Driver open procedure.
 *
 * This procedure opens the unit by verifying that a partition
 * for which the open refers is valid.
 */
wdopen(io)
	register struct iob *io;
{

	int	boardno;
	int	devno;
	struct cmd6 test_dat;
	register daddr_t boff;
	register struct drive_type *dp;
	struct inqarg	inq_dat;

	boardno = SCSI_BOARD(io->i_unit);
	devno = SCSI_DEVNO(io->i_unit);

	if (!ssm_get_devinfo(boardno, devno, &wdinfo, 0)) {
		printf("wd%d: No such ssm board\n", io->i_unit);
		io->i_error = ENXIO;
		return(-1);
	}
	
	/*
	 * Verify that the disk exists and is spun up.
	 */
	bzero((char *) &test_dat, SCSI_CMD6SZ);
	test_dat.cmd_opcode = SCSI_TEST;
	test_dat.cmd_lun = wdinfo.si_unit << WD_LUN_SHIFT;
	if (wd_scmd(io, SCSI_TEST, (caddr_t) &test_dat) != SCB_OK) {
		printf("wd%d: TEST UNIT READY command failed\n", io->i_unit);
		io->i_error = EIO;
		return(-1);
	}

	/*
	 * Do an INQUIRY command to determine what kind of a disk
	 * we have.
	 */
	bzero((char *) &inq_dat, sizeof(struct inqarg));
	inq_dat.cmd.cmd_opcode = SCSI_INQUIRY;
	inq_dat.cmd.cmd_lun = wdinfo.si_unit << WD_LUN_SHIFT;
	inq_dat.cmd.cmd_length = sizeof(struct sdinq);
	if (wd_scmd(io, SCSI_INQUIRY, (caddr_t) &inq_dat) != SCB_OK) {
		printf("wd%d: INQURIY command failed\n", io->i_unit);
		io->i_error = EIO;
		return(-1);
	}
#ifdef	DEBUG1
	printf("wdopen: INQUIRY data: ");
	wd_dump(sizeof(struct inqarg), (char *) &inq_dat);
#endif	/* DEBUG1 */
	
	/*
	 * valid INQUIRY data
	 */
	if (inq_dat.inq.sdq_format != CCS_FORMAT) {
		printf("wd%d: INQUIRY command output in wrong format\n", 
			io->i_unit);
		io->i_error = EDEV;
		return(-1);
	}

#ifdef	DEBUG1
	printf("wdopen: Vendor from inq_dat: ");
	wd_dump(SDQ_VEND, (char *) inq_dat.inq.sdq_vendor);
	printf("wdopen: Product from inq_dat: ");
	wd_dump(SDQ_PROD, (char *) inq_dat.inq.sdq_product);
#endif	/* DEBUG1 */

	for (dp = wddrive_table; dp->dt_vendor; dp++) {
	     if (strncmp(inq_dat.inq.sdq_vendor, dp->dt_vendor, SDQ_VEND) == 0
 	     && strncmp(inq_dat.inq.sdq_product, dp->dt_product, SDQ_PROD) == 0)
			break;
#ifdef	DEBUG1
		printf("wdopen: Vendor from table: ");
		wd_dump(SDQ_VEND, (char *) dp->dt_vendor);
		printf("wdopen: Product from table: ");
		wd_dump(SDQ_PROD, (char *) dp->dt_product);
#endif	/* DEBUG1 */
	}

	if (!dp->dt_vendor) {
		int i;
		char *bp = inq_dat.inq.sdq_vendor;

		printf("wd%d: vendor: ", io->i_unit);
		for (i = 0; i < SDQ_VEND; i++)
			printf("%c", *bp++);
		printf("\n");

		bp = inq_dat.inq.sdq_product;
		printf("wd%d: product: ", io->i_unit);
		for (i = 0; i < SDQ_PROD; i++)
			printf("%c", *bp++);
		printf("\n");
	
		printf("wd%d: could not find drive in wddrive_table\n", 
			io->i_unit);
		io->i_error = EDEV;
		return(-1);
	}

	geom = &dp->dt_st; 	/* Save away a pointer to this disks geometry */
	
	/*
	 * Read the partition data.
	 */
	if (vtoc_setboff(io, 0)) {
		io->i_error = 0;
		boff = io->i_boff;
		if((unsigned)boff < 8)	{
			if(geom->off[boff] == -1) {
				printf("bad partition\n");
				io->i_error = EDEV;
			} else
				io->i_boff = geom->off[boff];
		}
	}

	/* return value indicated by io->i_error */
}

/*
 * wdstrategy - standard read/write routine.
 *
 * This procedure executes a request for a read or a write to
 * the device.
 */
wdstrategy(io, cmd)
	struct iob *io;
	int	cmd;
{
	static int boardno;
	static int devno;
	register u_char scsi_cmd;

	boardno = SCSI_BOARD(io->i_unit);
	devno = SCSI_DEVNO(io->i_unit);

	/*
	 * all I/O must have its in-core buffer aligned to an 8-byte
	 * boundary.
	 */
#ifndef SSMFW
	if (((int)io->i_ma & (WD_ADDRALIGN - 1)) != 0) {
		io->i_error = EIO;
		return(-1);
	}
#endif	

	if (io->i_cc > WD_MAX_XFER * DEV_BSIZE) {
		io->i_error = EIO;
		return(-1);
	}

#ifdef	DEBUG
	printf("wdstrategy: io structure\n");
	wd_dump(sizeof(struct iob), (char *) io);
#endif	/* DEBUG */

	ssm_get_devinfo(boardno, devno, &wdinfo, 0);

#ifdef	DEBUG
	printf("wdstrategy: wdinfo structure\n");
	wd_dump(sizeof(struct ssm_sinfo), (char *) &wdinfo);
#endif	/* DEBUG */

	scsi_cmd = (cmd == READ) ? SCSI_READ_EXTENDED : SCSI_WRITE_EXTENDED;
	if (wd_scmd(io, scsi_cmd, (caddr_t) NULL) != SCB_OK)
		return (-1);
	else
		return io->i_cc;
}


#ifndef SSMFW
/*ARGSUSED*/
wdioctl(io, cmd, arg)
	struct iob *io;
	int cmd;
	caddr_t arg;
{

	register flag;
	register boardno = SCSI_BOARD(io->i_unit);
	register devno = SCSI_DEVNO(io->i_unit);
	register u_char scsi_cmd;

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
	 * Return first sector of usable space.
	 */
	case SAIOFIRSTSECT:
		*(int *)arg = 0;
		break;

	/*
	 * Get device data
	 */
	case SAIODEVDATA:
		*(struct st *)arg = *geom;
		break;
	/*
	 * SCSI commands
	 */
	case SAIOSCSICMD:
		switch(scsi_cmd = *(u_char *)arg) {
		case SCSI_FORMAT:
			ssm_get_devinfo(boardno, devno, &wdinfo, 0);
			if (wd_sgo(io, scsi_cmd, arg) != SCB_OK) {
				io->i_error = EIO;
				return (-1);
			} else
				return io->i_error = 0;
		case SCSI_MODES:
		case SCSI_READC:
		case SCSI_REASS:
		case SCSI_REZERO:
		case SCSI_TEST:
		case SCSI_INQUIRY:
		case SCSI_MSENSE:
		case SCSI_RSENSE:
			ssm_get_devinfo(boardno, devno, &wdinfo, 0);
			if (wd_scmd(io, scsi_cmd, arg) != SCB_OK) {
				io->i_error = EIO;
				return (-1);
			} else
				return io->i_error = 0;
		/*
		 * not yet
		 */
		case SCSI_SEEK:
		case SCSI_WRITEB:
		case SCSI_READB:
		case SCSI_RESRV:
		case SCSI_RELSE:
		case SCSI_STARTOP:
		case SCSI_RDIAG:
		case SCSI_SDIAG:
			printf("wd%d: ioctl not supported yet\n", io->i_unit);
		default:
			printf("wd%d: bad ioctl type (0x%x)\n",*(u_char *)arg, io->i_unit);
			io->i_error = ECMD;
			return -1;
		}
	default:
		printf("wd%d: bad ioctl (('%c'<<8)|%d)\n", io->i_unit, 
			(u_char)(cmd>>8),(u_char)cmd);
		io->i_error = ECMD;
		return -1;
	}
	return 0;
}
#endif /* SSMFW */

/*
 * wd_sgo -  start a ssm scsi command
 *	poll for completion and return completion status.
 */
static
wd_sgo(io, command, arg)
	struct iob *io;
	u_char	command;
	caddr_t arg;

{
	register struct scsi_cb *cb = wdinfo.si_cb;
	

	bzero((caddr_t) SWBZERO(cb), SWBZERO_SIZE);
	bzero((caddr_t) cb->sh.cb_sense, SCB_RSENSE_SZ);


	switch (command) {
	case SCSI_RSENSE:
		/* request sense data is handled by ssm firmware */
		return (SCB_OK);

	case SCSI_INQUIRY:
		bcopy(arg, (char *) cb->sh.cb_scmd, SCSI_CMD6SZ);
		cb->sh.cb_addr = (u_long) &(((struct inqarg *) arg)->inq);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_count = cb->sh.cb_scmd[4]; 
		cb->sh.cb_clen  = SCSI_CMD6SZ;
		break;

	case SCSI_READC:
		/*
 		 * PMI 0
		 * Returns capacity of entire disk
		 */
		bcopy(arg, (char *) cb->sh.cb_scmd, SCSI_CMD10SZ);
		cb->sh.cb_count = ((u_long)cb->sh.cb_scmd[7]) << 8; 
		cb->sh.cb_count |= (u_long)cb->sh.cb_scmd[8];
		cb->sh.cb_addr = (u_long) (((struct readcarg *) arg)->nblocks);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen  = SCSI_CMD10SZ;
		break;

	case SCSI_MODES:
	case SCSI_MSENSE:
		bcopy(arg, (char *) cb->sh.cb_scmd, SCSI_CMD6SZ);
		cb->sh.cb_addr = (u_long) &(((struct modearg *) arg)->mode);
		cb->sh.cb_cmd = (command == SCSI_MSENSE) ? SCB_READ : SCB_WRITE;
		cb->sh.cb_count = cb->sh.cb_scmd[4]; 
		cb->sh.cb_clen  = SCSI_CMD6SZ;
		break;

	case SCSI_FORMAT:
		bcopy(arg, (char *) cb->sh.cb_scmd, SCSI_CMD6SZ);
		cb->sh.cb_addr = (u_long) (((struct formarg *) arg)->pad);
		cb->sh.cb_cmd =  SCB_READ;
		cb->sh.cb_count = cb->sh.cb_scmd[4]; 
		cb->sh.cb_clen  = SCSI_CMD6SZ;
		break;

	case SCSI_REASS:
		bcopy(arg, (char *) cb->sh.cb_scmd, SCSI_CMD6SZ);
		cb->sh.cb_addr = (u_long) &(((struct reassarg *) arg)->pad);
		cb->sh.cb_cmd = SCB_WRITE;
		cb->sh.cb_count =4 +(((struct reassarg *) arg)->length[0] << 8 |
		     ((struct reassarg *) arg)->length[1]);
		cb->sh.cb_clen  = SCSI_CMD6SZ;
		break;
	case SCSI_STARTOP:
	case SCSI_TEST:
	case SCSI_REZERO:
		bcopy(arg, (char *) cb->sh.cb_scmd, SCSI_CMD6SZ);
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_clen  = SCSI_CMD6SZ;
		break;

	case SCSI_READ_EXTENDED:
	case SCSI_WRITE_EXTENDED:
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= wdinfo.si_unit << WD_LUN_SHIFT; 
		cb->sh.cb_iovec = 0;
		cb->sh.cb_addr = (u_long) io->i_ma;
		cb->sh.cb_count = roundup(io->i_cc, (u_long) DEV_BSIZE);
		cb->sh.cb_cmd = (command == SCSI_READ_EXTENDED) 
			? SCB_READ : SCB_WRITE;
		cb->sh.cb_scmd[2] = (u_char) ((int) io->i_bn >> 24);
		cb->sh.cb_scmd[3] = (u_char) ((int) io->i_bn >> 16);
		cb->sh.cb_scmd[4] = (u_char) ((int) io->i_bn >> 8);
		cb->sh.cb_scmd[5] = (u_char) io->i_bn;
		cb->sh.cb_scmd[7] = (u_char) ((int) howmany(io->i_cc, DEV_BSIZE) >> 8);
		cb->sh.cb_scmd[8] = (u_char) howmany(io->i_cc, DEV_BSIZE);
		cb->sh.cb_clen  = SCSI_CMD10SZ;
		break;
	}

	/*
	 * start the ssm scsi command by interrupting the ssm
 	 * with the scsi lun || cb num
	 */
	cb->sh.cb_compcode  = SCB_BUSY;
#ifndef SSMFW
#ifdef	DEBUG
	printf("wd_sgo: cb going out ");
	wd_sprintcb(cb);
#endif	/* DEBUG */
	mIntr(wdinfo.si_slic, SCSI_BIN, SCVEC(wdinfo.si_id, 0));
#else	/* SSMFW */

	/*
	 *	SCSI disk transactions should take no longer then 30 seconds.
	 *	the longest is STARTOP.
	 */
	action(SCVEC(wdinfo.si_id, 0),35000);
#endif	/* SSMFW */

	return wd_scbfin(cb);
}

/*
 * wd_scmd - start scsi disk commands
 * 	checks for errors; report hard and soft errors,
 * 	retry the command if appropriate.
 * 
 * All commands that fail are tried four times with a soft error being
 * reported. If after four tries a command still fails a hard error 
 * is reported and failure status is returned. 
 * If a drive recovers internally a soft error is reported 
 * and SCB_OK status is returned.
 */
static
wd_scmd(io, command, arg)
	struct iob *io;
	u_char	command;
	caddr_t arg;
{
	struct scrsense *rsense;
	register u_char compcode;
	struct scsi_cb *cb = wdinfo.si_cb;
	
	io->i_errcnt = 0;

	for (;;) {
		compcode = wd_sgo(io, command, arg);
		switch(compcode) {
		case SCB_BAD_CB:
		case SCB_NO_TARGET:
		case SCB_SCSI_ERR:
			wd_error(cb, io, "Hard Error: ");
			return (-1);
		
		case SCB_OK:
			if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
				rsense = (struct scrsense *) cb->sh.cb_sense;
				if ((rsense->rs_error & RS_ERRCODE) != RS_CLASS_EXTEND) {
					wd_error(cb, io, "Hard Error, bad sense data: ");
					return (-1);
				} 
         	 		switch (rsense->rs_key & RS_KEY) {
				case RS_RECERR:
					wd_error(cb, io, "Soft Error: ");
					return (compcode);

				case RS_UNITATTN:
					/*
					 * If this occurs on while testing
					 * the unit, don't print a message
					 * about it here - let the caller.
					 */
					if (io->i_errcnt >= 4) {
					    if (cb->sh.cb_scmd[0] != SCSI_TEST) 
						wd_error(cb,io,"Hard Error: ");
					    return (-1);
					} 

					if (cb->sh.cb_scmd[0] != SCSI_TEST) 
						wd_error(cb,io,"Soft Error: ");
					io->i_errcnt += 1;
					break;
	
				default:
					if (io->i_errcnt >= 4) {
						wd_error(cb, io, "Hard Error: ");
						return (-1);
					}

					wd_error(cb, io, "Soft Error: ");
					io->i_errcnt += 1;
					break;
				}
			} else
				return (compcode);
		}
	}	
}

/* 
 * wd_scbfin 
 * polls for ssm command completion and returns completion status.
 */
static
wd_scbfin(cb)
	struct scsi_cb	*cb;
{
	while (cb->sh.cb_compcode == SCB_BUSY)
		continue;

#ifdef	DEBUG
	printf("wd_cbfin: count = %x\n", cb->sh.cb_count);
#endif	/* DEBUG */
	return (cb->sh.cb_compcode);
}

/*
 * wd_sprintcb
 *
 * prints out the fields of a command block
 */
static
void
wd_sprintcb(cb)
	struct scsi_cb *cb;
{
	int x;

	printf("command: ");
#ifdef	DEBUG
	printf("0x%x 0x%x 0x%x \n", cb->sh.cb_cmd,
		cb->sh.cb_reserved0[0],
		cb->sh.cb_clen); 
#endif	/* DEBUG */
	for (x = 0; x < cb->sh.cb_clen; x++) 
		printf("0x%x ", cb->sh.cb_scmd[x]);
	printf("\n");
#ifdef DEBUG
	printf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		*(u_long*) &cb->sh.cb_reserved1[0],	 
		cb->sh.cb_addr,		
		(u_long) cb->sh.cb_iovec,
		cb->sh.cb_count,
		cb->sh.cb_status, 
		*(ushort*) &cb->sh.cb_reserved2[0],
		cb->sh.cb_compcode);
	printf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		(u_long) cb->sw.cb_bp, 
		(u_long) cb->sw.cb_iovstart,
		cb->sw.cb_errcnt,
		cb->sw.cb_unit_index,
		cb->sw.cb_scsi_device,
		cb->sw.cb_state,
		cb->sw.cb_data,
		cb->sw.cb_iovnum,
		cb->sw.cb_slic); 
#endif /* DEBUG */
}

/*
 * wd_sprintsense - prints sense information returned from a
 * 	scsi request sense command
 */
static
void
wd_sprintsense(rsense)
	struct scrsense *rsense;
{
	int more, x;
	u_char *moredat;

	printf("sense: ");
	if ((rsense->rs_error & RS_ERRCODE) != RS_CLASS_EXTEND) 
		printf("Don't understand request sense data\n"); 
	
	printf("Error 0x%x, Seg Num 0x%x, Key 0x%x, Info 0x%x, Additional 0x%x, ", 
		rsense->rs_error,
		rsense->rs_seg,
		rsense->rs_key, 
		*(u_long *) &rsense->rs_info[0],
		rsense->rs_addlen);
	
	more = (int) rsense->rs_addlen;
	if (more > SCB_RSENSE_SZ - sizeof (struct scrsense))
		more = SCB_RSENSE_SZ - sizeof (struct scrsense);
	moredat = (u_char *) ((int) (&rsense->rs_addlen) + 1);
	for (x = 0; x < more; x++) 
		printf("0x%x ", moredat[x]);
	printf("\n\n");
}

/*
 * wd_error
 * Error reporting for dirver.
 */
static
void
wd_error(cb, io, str)
	struct scsi_cb *cb;
	struct iob *io;
	char *str;
{
	struct scrsense *rsense;

	rsense = (struct scrsense *) cb->sh.cb_sense;	

	if (rsense->rs_key & RS_KEY)
		printf("wd%d: %s command=0x%x, sensekey=0x%x, sensecode=0x%x, lba=0x%x, compcode=0x%x\n",
			io->i_unit,
			str,
			cb->sh.cb_scmd[0],
			rsense->rs_key,
			rsense[12],
			io->i_bn, 
			cb->sh.cb_compcode
		);

	else
		printf("wd%d: %s command=0x%x, lba=0x%x, compcode=0x%x\n",
			io->i_unit,
			str,
			cb->sh.cb_scmd[0],
			io->i_bn, 
			cb->sh.cb_compcode
		);
		
	
	printf("wd%d: ", io->i_unit);
	wd_sprintcb(cb);

#ifndef DEBUG
	if (rsense->rs_key & RS_KEY) {
#endif	/* DEBUG */
		printf("wd%d: ", io->i_unit);
		wd_sprintsense(rsense);
#ifndef DEBUG
	}
#endif	/* DEBUG */
}

#ifdef	DEBUG
wd_dump(len, ptr)
	int len;
	char * ptr;
{
	int	x;
	
	for (x = 0; x < len; x++) {
		printf("%x ", (u_char) ptr[x]);
		if ((x % 25 == 0) && (x != 0))
			printf("\n");
	}
	printf("\n");
}

wd_fill(len, ptr)
	int len;
	char * ptr;
{
	int	x;
	
	for (x = 0; x < len; x++) 
		ptr[x] = 55;
}
#endif	/* DEBUG */
