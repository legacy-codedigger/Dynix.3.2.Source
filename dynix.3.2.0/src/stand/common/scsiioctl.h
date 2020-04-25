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

/* $Header: scsiioctl.h 1.1 90/09/13 $ */

/* $Log $
 */

/**
 ** This section defines the scsi disk ioctl interface.
 **/


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

#define cmd_pgcode cmd_lba[0]		/* page code for MODE SENSE/SELECT */

/*
 * arguments to sdioctl().  This interface requires a packet which
 * contains the CDB in the first bytes and the data for the command
 * in the remaining bytes.
 */

struct modearg {		/* used for MODE SENSE/SELECT */
	struct cmd6 cmd;
	struct sd_modes mode;
};

struct inqarg {			/* used for INQUIRY */
	struct cmd6 cmd;
	struct sdinq inq;
};

struct formarg {		/* used for FORMAT UNIT */
	struct cmd6 cmd;
	u_char pad[4];
};

struct reassarg {		/* used for REASSIGN BLOCKS */
	struct cmd6 cmd;
	short  pad;
	u_char length[2];
	u_char defect[4];
};

struct readcarg {		/* used for READ CAPACITY */
	struct cmd10 cmd;
	u_char nblocks[4];		/* highest addressable block on disk */
	u_char bsize[4];		/* formatted size of disk blocks */
};

