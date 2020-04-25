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

#ifndef	_SYS_SCSIIOCTL_H_

/*
 * $Header: scsiioctl.h 1.2 89/08/25 $
 *
 * $Log:	scsiioctl.h,v $
 */

/*
 * form psx scsi.h 2.1 87/04/28 $
 * SCSI Ioctl commands
 * NOTE <sys/ioctl.h> needs to be include.
 */
#define SIOC		'S'
#define SIOCSCSICMD	_IOW(S, 0, struct scsiioctl)	/* do SCSI command */
#define SIOCDEVDATA	_IOR(S, 1, struct scsidev)	/* get device specific info */

/*
 * structure returned for SIOCDEVDATA ioctl 
 */
struct scsidev {
	u_char scsi_devno;		/* logical SCSI_DEVNO(targ, lun) */
	u_char scsi_ctlr;		/* SCSI controller number */
};


/*
 * This section defines the scsi disk ioctl interface.
 */

#define b_sioctl	   b_resid
#define SCSI_IOCTLLEN(bp)  (((struct scsiioctl *)(bp)->b_sioctl)->sio_cmdlen)

/*
 * format of a 6 byte command descriptor block (CDB)
 */

struct cmd6 {
	u_char cmd_opcode;
	u_char cmd_lun;
	u_char cmd_lba[2];
	u_char cmd_length;
	u_char cmd_cb;
};

/*
 * format of a 10 byte command descriptor block (CDB)
 */

struct cmd10 {
	u_char cmd_opcode;
	u_char cmd_lun;
	u_char cmd_lba[4];
	u_char cmd_pad;
	u_char cmd_length[2];
	u_char cmd_cb;
};

/*
 * arguments to sdioctl().  This interface requires a packet which
 * contains the CDB in the first bytes and the data for the command
 * in the remaining bytes.
 */

struct scsiioctl {
	u_long sio_datalength;
	u_long sio_addr;
	u_long sio_cmdlen;
	union {
		struct cmd6 cmd6;
		struct cmd10 cmd10;
	} sio_cmd;
};

#define sio_cmd6	sio_cmd.cmd6
#define sio_cmd10	sio_cmd.cmd10

struct reassarg {		/* used for REASSIGN BLOCKS */
	short  pad;
	u_char length[2];
	u_char defect[4];
};

struct defect_hdr {		/* read defect data header */
	u_char pad;
	u_char type;
	ushort length;
};

#define	_SYS_SCSIIOCTL_H_
#endif	/* _SYS_SCSIIOCTL_H_ */
