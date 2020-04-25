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

#ident "$Header: scsi.c 1.2 90/11/06 $"

/*
 * scsi.c
 *	Generic SCSI support functions.
 */

/* $Log:	scsi.c,v $
 */

#include "../h/param.h"
#include "../h/cmn_err.h"
#include "../h/scsi.h"

/*
 * The following arrays are for producing 
 * messages summarizing a SCSI command
 * termination and noting check conditions.
 */
char *scsi_errors[] = {
	"No Sense",
	/*
	 *+ There is no specific sense key information to be reported 
	 *+ for the designated device.  This would be the case for a 
	 *+ command that completed successfully or a command that received a 
	 *+ CHECK CONDITION status because one of the EOM, filemark,
	 *+ or ILI bits is set to 1 in the sense data.
	 */
	"Recovered Error",
	/*
	 *+ The last command completed successfully with some recovery 
	 *+ action performed by the device.  For details, 
	 *+ examine the additional sense and information bytes of the 
	 *+ sense data.
	 */
	"Not Ready",
	/*
	 *+ The device is not ready to be accessed.  Operator
	 *+ intervention might be required to correct this condition.
	 */
	"Medium Error",
	/*
	 *+ The command terminated with a nonrecoverable error 
	 *+ condition.  It was probably caused by a flaw in the
	 *+ medium or by an error in the recorded data.
	 */
	"Hardware Error",
	/*
	 *+ While the device was performing a command or carrying out a self test,
	 *+ it detected a nonrecoverable hardware failure
	 *+ such as a controller failure, device failure, or parity
	 *+ error.
	 */
	"Illegal Request",
	/*
	 *+ There was an illegal parameter in the command descriptor
	 *+ block or in the additional parameters supplied as data
	 *+ for some commands.  If the illegal parameter was 
	 *+ detected in the command descriptor
	 *+ block, the command was terminated without altering the 
	 *+ medium.  The medium might have been altered if an illegal parameter
	 *+ was detected in the data.
	 */
	"Unit Attention",
	/*
	 *+ The removable medium has been changed or the device has
	 *+ been reset.
	 */
	"Data Protect",
	/*
	 *+ A command that reads or writes the medium was attempted
	 *+ on a block that is protected from this operation.  The
	 *+ operation is not performed.
	 */
	"Blank Check",
	/*
	 *+ Either a sequential access device (that is, a tape) or a write-once,
	 *+ read-multiple device encountered a blank block while
	 *+ reading, or a write-once, read-multiple device encountered 
	 *+ a nonblank block while writing.
	 */
	"Vendor Unique",
	/*
	 *+ A vendor-unique condition is being reported.
	 */
	"Copy Aborted",
	/*
	 *+ A COPY, COMPARE, or COPY AND VERIFY command was aborted
	 *+ due to an error condition on either the source device, the
	 *+ destination device, or both.
	 */
	"Aborted Command",
	/*
	 *+ The SCSI device aborted the command.  
	 *+ Corrective action:  reissue the command.
	 */
	"Equal",
	/*
	 *+ A SEARCH DATA command has satisfied an equal comparison.
	 */
	"Volume Overflow",
	/*
	 *+ A buffered peripheral device has reached the end-of-medium.
	 *+ However, data remains in the buffer that has not been written
	 *+ to the medium.
	 */
	"Miscompare",
	/*
	 *+ The source data did not match the data read from the medium.
	 */
	"Reserved Key",
	/*
	 *+ This key is reserved for future use.
	 */
};

int num_scsi_errors = sizeof(scsi_errors) / sizeof(scsi_errors[0]);

char *scsi_commands[] = {
	"Test Unit Ready",
	"Rezero/Rewind Unit",
	"Retension",
	"Request Sense",
	"Format Unit",
	"Read Block Limits",
	"0x06",
	"Reassign Blocks",
	"Read",
	"0x09",
	"Write",
	"Seek/Track Select",
	"0x0c",
	"0x0d",
	"0x0e",
	"Read Reverse",
	"Write Filemarks",
	"Space",
	"Inquiry", 
	"Verify",
	"Recover Buffered Data",
	"Mode Select",
	"Reserve Unit",
	"Release Unit",
	"Copy",
	"Erase", 
	"Mode Sense",
	"Start/Stop or Load/Unload Unit",
	"Recieve Diagnostic Results/Mode Select",
	"Send Diagnostic Results", 
	"Prevent/Allow Medium Removal", 
	"0x1f", 
	"0x20",
	"0x21",
	"0x22", 
	"0x23", 
	"0x24", 
	"Read Capacity", 
	"0x26", 
	"0x27", 
	"Read (extended)", 
	"0x29", 
	"Write (extended)", 
	"Seek (extended)", 
	"0x2c", 
	"0x2d", 
	"Write and Verify", 
	"Verify", 
	"Search Data High",
	"Search Data Equal", 
	"Search Data Low", 
	"Set Limits", 
	"0x34", 
	"0x35", 
	"0x36", 
	"0x37", 
	"0x38", 
	"Compare", 
	"Copy and Verify"
};

int num_scsi_commands = sizeof(scsi_commands) / sizeof(scsi_commands[0]);

char *scsi_status_msg[] = { 
	"Good termination",				  /* SSTAT_OK */
	"No such device at that address",		  /* SSTAT_NODEV */
	"A SCSI bus error occurred",			  /* SSTAT_BUSERR */
	"Target adapter does not respond",		  /* SSTAT_NOTARGET */
	"Target adapter is busy",			  /* SSTAT_BUSYTARGET */
	"Logical unit is busy",				  /* SSTAT_BUSYLUN */
	"Check condition",				  /* SSTAT_CCHECK */
	"Deferred check condition",			  /* SSTAT_DCHECK */
	"Check condition with vendor unique sense data",  /* SSTAT_UCHECK */
	"Reseved SCSI completion status"		  /* SSTAT_RESERVED */
};

/* 
 * The first portion of this module contains functions 
 * which generate specific SCSI commands.  The second
 * portion contains functions that help analyze and 
 * interpret the termination of a SCSI command and 
 * request-sense data associated with check conditions.
 *
 * Functions generating SCSI commands have the address of
 * a "struct scsi_cmd" as their first argument, and the
 * logical unit number of the device as their second
 * argument.  They only fail if an argument is invalid, in 
 * which case the return value is NULL.  When successful, 
 * the return value is the first argument passed to it and 
 * the fields 'cmd', 'clen', and 'dir' are filled in.  
 * 
 * Please note that currently this library only generates
 * SCSI commands with a control byte value of zero.  This
 * means that linked commands are not supported, and may
 * exclude commands that need vendor unique values in the
 * control byte.  Functions may be added to this module
 * as commands are needed that haven't been included.
 */

#ifdef notyet
scsicmd_t *
scsi_inquiry_cmd(scmd, lun, ibuf_len)
	register scsicmd_t *scmd;
	u_char lun;
	u_long ibuf_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_inquiry_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_inquiry_cmd: invalid SCSI logical unit number argument");
	ASSERT(ibuf_len == (u_char)ibuf_len, 
		"scsi_inquiry_cmd: buffer allocation length exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL || ibuf_len != (u_char)ibuf_len)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_INQUIRY;
	cp[1] = lun << SCSI_LUNSHFT;
	cp[4] = (u_char)ibuf_len;
	return(scmd);
}
#endif notyet

scsicmd_t *
scsi_mode_select_cmd(scmd, lun, mode_len)
	scsicmd_t *scmd;
	u_char lun;
	u_long mode_len;
{
	register struct scmode_cmd *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_mode_select_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_mode_select_cmd: invalid SCSI logical unit number argument");
	ASSERT(mode_len == (u_char)mode_len, 
		"scsi_mode_select_cmd: parameter list length exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  mode_len != (u_char)mode_len)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_HTOD;
	bzero((char *)&scmd->cmd[0], scmd->clen = SCSI_CMD6SZ);
	cp = (struct scmode_cmd *) &scmd->cmd[0];
	cp->m_cmd = SCSI_MODES;
	cp->m_unit = lun << SCSI_LUNSHFT;
	cp->m_plen = (u_char)mode_len;
	return(scmd);
}

scsicmd_t *
scsi_mode_sense_cmd(scmd, lun, mode_len)
	scsicmd_t *scmd;
	u_char lun;
	u_long mode_len;
{
	register struct scmode_cmd *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_mode_sense_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_mode_sense_cmd: invalid SCSI logical unit number argument");
	ASSERT(mode_len == (u_char)mode_len, 
		"scsi_mode_sense_cmd: buffer allocation length exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  mode_len != (u_char)mode_len)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	bzero((char *)&scmd->cmd[0], scmd->clen = SCSI_CMD6SZ);
	cp = (struct scmode_cmd  *) &scmd->cmd[0];
	cp->m_cmd = SCSI_MSENSE;
	cp->m_unit = lun << SCSI_LUNSHFT;
	cp->m_plen = (u_char)mode_len;
	return(scmd);
}

#ifdef notyet
scsicmd_t *
scsi_prevent_allow_removal_cmd(scmd, lun, prevent)
	scsicmd_t *scmd;
	u_char lun;
	u_char prevent;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_prevent_allow_removal_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_prevent_allow_removal_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(prevent & ~SCSI_REMOVAL_PREVENT),
		"scsi_prevent_allow_removal_cmd: invalid prevent/allow flag");
#else /* DEBUG */
	if (scmd == NULL  
	||  prevent & ~SCSI_REMOVAL_PREVENT)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_PA_REMOVAL;
	cp[1] = lun << SCSI_LUNSHFT;
	cp[4] = (u_char)prevent;
	return(scmd);
}

scsicmd_t *
scsi_request_sense_cmd(scmd, lun, rsense_len)
	scsicmd_t *scmd;
	u_char lun;
	u_long rsense_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_request_sense_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_request_sense_cmd: invalid SCSI logical unit number argument");
	ASSERT(rsense_len == (u_char)rsense_len, 
		"scsi_request_sense_cmd: buffer allocation length exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL  
	||  rsense_len != (u_char)rsense_len)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_RSENSE;
	cp[1] = lun << SCSI_LUNSHFT;
	cp[4] = (u_char)rsense_len;
	return(scmd);
}
#endif /* notyet */

scsicmd_t *
scsi_test_unit_ready_cmd(scmd, lun)
	scsicmd_t *scmd;
	u_char lun;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_test_unit_ready_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_test_unit_ready_cmd: invalid SCSI logical unit number argument");
#else /* DEBUG */
	if (scmd == NULL )
		return((scsicmd_t *) NULL);
#endif /* DEBUG */
	scmd->dir = SDIR_NONE;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_TEST;
	cp[1] = lun << SCSI_LUNSHFT;
	return(scmd);
}
	
#ifdef notyet
/*
 * SCSI command interfaces for Direct Access devices.
 */
scsicmd_t *
scsi_DA_format_unit_cmd(scmd, lun, misc_flags, vendor, interleave, dlist_len)
	scsicmd_t *scmd;
	u_char lun;
	u_char misc_flags;
	u_char vendor;
	u_long interleave;
	u_long dlist_len;
{
	register struct scfmt_cmd *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_DA_format_unit_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_DA_format_unit_cmd: invalid SCSI logical unit number argument");
	ASSERT(misc_flags < 1 << SCSI_LUNSHFT, 
		"scsi_DA_format_unit_cmd: invalid flags argument");
	ASSERT(interleave <= 0xffff, 
		"scsi_DA_format_unit_cmd: invalid interleave value");
#else /* DEBUG */
	if (scmd == NULL 
	||  misc_flags >= 1 << SCSI_LUNSHFT || interleave > 0xffff) 
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = (dlist_len == 0) ? SDIR_NONE : SDIR_HTOD;
	scmd->clen = SCSI_CMD6SZ;
	cp = (struct scfmt_cmd *)&scmd->cmd[0], 

	cp->f_type = SCSI_FORMAT;
	cp->f_misc = lun << SCSI_LUNSHFT | misc_flags; 
	cp->f_vendor = vendor;
	cp->f_ileave[1] = interleave >> 8;
	cp->f_ileave[2] = interleave;
	cp->f_control = 0;
	return(scmd);
}

scsicmd_t *
scsi_DA_read_capacity_cmd(scmd, lun, lba, pmi)
	scsicmd_t *scmd;
	u_char lun;
	u_long lba;
	u_char pmi;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_DA_read_capacity_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_DA_read_capacity_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(pmi & ~SCSI_PART_CAP),
		"scsi_DA_read_capacity_cmd: invalid partial media indicator");
#else /* DEBUG */
	if (scmd == NULL ||  pmi & ~SCSI_PART_CAP)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	scmd->clen = SCSI_CMD10SZ;
	cp = &scmd->cmd[0];

	*cp++ = SCSI_READC;
	*cp++ = lun << SCSI_LUNSHFT;
	*cp++ = lba >> 24;
	*cp++ = lba >> 16;
	*cp++ = lba >> 8;
	*cp++ = lba;
	*cp++ = 0;
	*cp++ = 0;
	*cp++ = pmi;
	*cp++ = 0;
	return(scmd);
}

scsicmd_t *
scsi_DA_read_cmd(scmd, lun, lba, xfer_len)
	scsicmd_t *scmd;
	u_char lun;
	u_long lba, xfer_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_DA_read_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_DA_read_cmd: invalid SCSI logical unit number argument");
	ASSERT(xfer_len <= 0xffff, 
		"scsi_DA_read_cmd: tranfer length argument exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  xfer_len > 0xffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	cp = &scmd->cmd[0];

	if (lba > 0x1fffff || xfer_len != (u_char)xfer_len) {
		/*
		 * Use the extended command format.
		 */
		scmd->clen = SCSI_CMD10SZ;
		*cp++ = SCSI_READ_EXTENDED;
		*cp++ = lun << SCSI_LUNSHFT;
		*cp++ = lba >> 24;
		*cp++ = lba >> 16;
		*cp++ = lba >> 8;
		*cp++ = lba;
		*cp++ = 0;
		*cp++ = xfer_len >> 8;
	} else {
		/*
		 * Use the normal command format.
		 */
		scmd->clen = SCSI_CMD6SZ;
		*cp++ = SCSI_READ;
		*cp++ = lun << SCSI_LUNSHFT | lba >> 16;
		*cp++ = lba >> 8;
		*cp++ = lba;
	}
	*cp++ = xfer_len;
	*cp = 0;
	return(scmd);
}

scsicmd_t *
scsi_DA_reassign_blocks_cmd(scmd, lun)
	scsicmd_t *scmd;
	u_char lun;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_DA_reassign_blocks_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_DA_reassign_blocks_cmd: invalid SCSI logical unit number argument");
#else /* DEBUG */
	if (scmd == NULL )
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_HTOD;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_REASS;
	cp[1] = lun << SCSI_LUNSHFT;
	return(scmd);
}

scsicmd_t *
scsi_DA_rezero_unit_cmd(scmd, lun)
	scsicmd_t *scmd;
	u_char lun;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_DA_rezero_unit_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_DA_rezero_unit_cmd: invalid SCSI logical unit number argument");
#else /* DEBUG */
	if (scmd == NULL )
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_REZERO;
	cp[1] = lun << SCSI_LUNSHFT;
	return(scmd);
}

scsicmd_t *
scsi_DA_write_cmd(scmd, lun, lba, xfer_len)
	scsicmd_t *scmd;
	u_char lun;
	u_long lba, xfer_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_DA_write_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_DA_write_cmd: invalid SCSI logical unit number argument");
	ASSERT(xfer_len <= 0xffff, 
		"scsi_DA_write_cmd: tranfer length argument exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  xfer_len > 0xffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_HTOD;
	cp = &scmd->cmd[0];

	if (lba > 0x1fffff || xfer_len != (u_char)xfer_len) {
		/*
		 * Use the extended command format.
		 */
		scmd->clen = SCSI_CMD10SZ;
		*cp++ = SCSI_WRITE_EXTENDED;
		*cp++ = lun << SCSI_LUNSHFT;
		*cp++ = lba >> 24;
		*cp++ = lba >> 16;
		*cp++ = lba >> 8;
		*cp++ = lba;
		*cp++ = 0;
		*cp++ = xfer_len >> 8;
	} else {
		/*
		 * Use the normal command format.
		 */
		scmd->clen = SCSI_CMD6SZ;
		*cp++ = SCSI_WRITE;
		*cp++ = lun << SCSI_LUNSHFT | lba >> 16;
		*cp++ = lba >> 8;
		*cp++ = lba;
	}
	*cp++ = xfer_len;
	*cp = 0;
	return(scmd);
}
#endif /* notyet */
	
/*
 * SCSI command interfaces for Sequential Access devices.
 */
scsicmd_t *
scsi_SA_erase_cmd(scmd, lun, long_mode)
	scsicmd_t *scmd;
	u_char lun;
	u_char long_mode;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_erase_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_erase_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(long_mode & ~SCSI_ERASE_LONG),
		"scsi_SA_erase_cmd: invalid erase mode flag argument");
#else /* DEBUG */
	if (scmd == NULL ||  long_mode & ~SCSI_ERASE_LONG)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_ERASE;
	cp[1] = lun << SCSI_LUNSHFT | long_mode;
	return(scmd);
}

scsicmd_t *
scsi_SA_load_unload_cmd(scmd, lun, load_reten)
	scsicmd_t *scmd;
	u_char lun;
	u_char load_reten;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_load_unload_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_load_unload_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(load_reten & ~(SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA)),
		"scsi_SA_load_unload_cmd: invalid load and retension flags argument");
#else /* DEBUG */
	if (scmd == NULL 
	||  load_reten & ~(SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA))
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_LOAD_UNLOAD;
	cp[1] = lun << SCSI_LUNSHFT;
	cp[4] = load_reten;
	return(scmd);
}

scsicmd_t *
scsi_SA_read_cmd(scmd, lun, fixed, xfer_len)
	scsicmd_t *scmd;
	u_char lun;
	u_char fixed;
	u_long xfer_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_read_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_read_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(fixed & ~SCSI_FIXED_BLOCKS),
		"scsi_SA_read_cmd: invalid fixed/variable flag argument");
	ASSERT(xfer_len <= 0xffffff,
		"scsi_SA_read_cmd: transfer length argument exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  fixed & ~SCSI_FIXED_BLOCKS
	||  xfer_len > 0xffffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	scmd->clen = SCSI_CMD6SZ;
	cp = &scmd->cmd[0];
	*cp++ = SCSI_READ;
	*cp++ = lun << SCSI_LUNSHFT | fixed;
	*cp++ = xfer_len >> 16;
	*cp++ = xfer_len >> 8;
	*cp++ = xfer_len;
	*cp++ = 0;
	return(scmd);
}

scsicmd_t *
scsi_SA_rewind_cmd(scmd, lun, immediate)
	scsicmd_t *scmd;
	u_char lun;
	u_char immediate;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_rewind_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_rewind_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(immediate & ~SCSI_IMMEDIATE),
		"scsi_SA_rewind_cmd: invalid immediate flag argument");
#else /* DEBUG */
	if (scmd == NULL ||  immediate & ~SCSI_IMMEDIATE)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_REWIND;
	cp[1] = lun << SCSI_LUNSHFT | immediate;
	return(scmd);
}

scsicmd_t *
scsi_SA_space_cmd(scmd, lun, opcount, function_code)
	scsicmd_t *scmd;
	u_char lun;
	u_long opcount;
	u_char function_code;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_space_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_space_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(function_code & ~SCSI_SPACE_CODE),
		"scsi_SA_space_cmd: invalid function code argument");
#else /* DEBUG */
	if (scmd == NULL 
	||  function_code & ~SCSI_SPACE_CODE)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	scmd->clen = SCSI_CMD6SZ;
	cp = &scmd->cmd[0]; 

	*cp++ = SCSI_SPACE;
	*cp++ = lun << SCSI_LUNSHFT | function_code;
	*cp++ = opcount >> 16;
	*cp++ = opcount >> 8;
	*cp++ = opcount;
	*cp = 0;
	return(scmd);
}

scsicmd_t *
scsi_SA_write_cmd(scmd, lun, fixed, xfer_len)
	scsicmd_t *scmd;
	u_char lun;
	u_char fixed;
	u_long xfer_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_write_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_write_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(fixed & ~SCSI_FIXED_BLOCKS),
		"scsi_SA_write_cmd: invalid fixed/variable flag argument");
	ASSERT(xfer_len <= 0xffffff,
		"scsi_SA_write_cmd: transfer length argument exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  fixed & ~SCSI_FIXED_BLOCKS
	||  xfer_len > 0xffffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_HTOD;
	scmd->clen = SCSI_CMD6SZ;
	cp = &scmd->cmd[0];
	*cp++ = SCSI_WRITE;
	*cp++ = lun << SCSI_LUNSHFT | fixed;
	*cp++ = xfer_len >> 16;
	*cp++ = xfer_len >> 8;
	*cp++ = xfer_len;
	*cp++ = 0;
	return(scmd);
}

scsicmd_t *
scsi_SA_write_filemarks_cmd(scmd, lun, num_fms)
	scsicmd_t *scmd;
	u_char lun;
	u_long num_fms;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_write_filemarks_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_write_filemarks_cmd: invalid SCSI logical unit number argument");
	ASSERT(num_fms <= 0xffffff, 
		"scsi_SA_write_filemarks_cmd: #filemarks arg exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  num_fms > 0xffffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_NONE;
	cp = &scmd->cmd[0];
	scmd->clen = SCSI_CMD6SZ;
	*cp++ = SCSI_WFM;
	*cp++ = lun << SCSI_LUNSHFT;
	*cp++ = num_fms >> 16;
	*cp++ = num_fms >> 8;
	*cp++ = num_fms;
	*cp++ = 0;
	return(scmd);
}

#ifdef notyet
scsicmd_t *
scsi_SA_read_blk_limits_cmd(scmd, lun)
	scsicmd_t *scmd;
	u_char lun;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_read_blk_limits_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_read_blk_limits_cmd: invalid SCSI logical unit number argument");
#else /* DEBUG */
	if (scmd == NULL 
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	bzero((char *)(cp = &scmd->cmd[0]), scmd->clen = SCSI_CMD6SZ);
	cp[0] = SCSI_READ_BLK_LIMITS;
	cp[1] = lun << SCSI_LUNSHFT;
	return(scmd);
}

scsicmd_t *
scsi_SA_read_reverse_cmd(scmd, lun, fixed, xfer_len)
	scsicmd_t *scmd;
	u_char lun;
	u_char fixed;
	u_long xfer_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_read_reverse_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_read_reverse_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(fixed & ~SCSI_FIXED_BLOCKS),
		"scsi_SA_read_reverse_cmd: invalid fixed/variable flag argument");
	ASSERT(xfer_len <= 0xffffff,
		"scsi_SA_read_reverse_cmd: transfer length argument exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  fixed & ~SCSI_FIXED_BLOCKS
	||  xfer_len > 0xffffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = SDIR_DTOH;
	scmd->clen = SCSI_CMD6SZ;
	cp = &scmd->cmd[0];
	*cp++ = SCSI_READ_REVERSE;
	*cp++ = lun << SCSI_LUNSHFT | fixed;
	*cp++ = xfer_len >> 16;
	*cp++ = xfer_len >> 8;
	*cp++ = xfer_len;
	*cp++ = 0;
	return(scmd);
}

scsicmd_t *
scsi_SA_verify_cmd(scmd, lun, byte_cmp, fixed, ver_len)
	scsicmd_t *scmd;
	u_char lun;
	u_char byte_cmp, fixed;
	u_long ver_len;
{
	register u_char *cp;

#ifdef DEBUG
	ASSERT(scmd != NULL, 
		"scsi_SA_verify_cmd: NULL pointer to SCSI command structure");
	ASSERT(lun <= SCSI_MAX_LUN, 
		"scsi_SA_verify_cmd: invalid SCSI logical unit number argument");
	ASSERT(!(byte_cmp & ~SCSI_VERIFY_MEDIA),
		"scsi_SA_verify_cmd: invalid byte compare flag argument");
	ASSERT(!(fixed & ~SCSI_FIXED_BLOCKS),
		"scsi_SA_verify_cmd: invalid fixed/variable flag argument");
	ASSERT(ver_len <= 0xffffff,
		"scsi_SA_read_reverse_cmd: verification length argument exceeds maximum");
#else /* DEBUG */
	if (scmd == NULL ||  byte_cmp & ~SCSI_VERIFY_MEDIA 
	||  fixed & ~SCSI_FIXED_BLOCKS || ver_len > 0xffffff)
		return((scsicmd_t *) NULL);
#endif /* DEBUG */

	scmd->dir = (byte_cmp) ? SDIR_HTOD : SDIR_NONE;
	scmd->clen = SCSI_CMD6SZ;
	cp = &scmd->cmd[0];

	*cp++ = SCSI_VERIFY;
	*cp++ = lun << SCSI_LUNSHFT | byte_cmp | fixed;
	*cp++ = ver_len >> 16;
	*cp++ = ver_len >> 8;
	*cp++ = ver_len;
	*cp++ = 0;
	return(scmd);
}
#endif /* notyet */


/*
 * SCSI common interfaces for command termination.
 */

/* scsi_status()
 * 	Translate the termination status byte from a 
 * 	completed SCSI command into a generic class code.
 * 	In the case of a check condition, determine if it
 * 	is in a recognizable format vs. vendor unique or
 * 	reserved format.
 *
 * 	The generic code suffices in many cases, otherwise
 * 	a driver will need to perform more in-depth analysis,
 * 	possibly invoking other generic SCSI support functions.
 */
int
scsi_status(term_byte, rsense)
	u_char term_byte;
	struct scrsense *rsense;
{
	ASSERT(rsense != NULL, 
		"scsi_status: NULL request-sense buffer address");
	/*
	 *+ scsi_rsense() has been passed a false request-sense
	 *+ buffer address.  The argument should address the
	 *+ request-sense data buffer used while executing the
	 *+ command for which the termination status is being 
	 *+ analyzed.  The calling function has an internal
	 *+ error.
	 */
	switch (STERM_CODE(term_byte)) {
	case STERM_BUSY:
		return(SSTAT_BUSYTARGET); 
	case STERM_RES_CONFLICT: 
		return(SSTAT_BUSYLUN);
	case STERM_GOOD:
	case STERM_CONDITION_MET:
	case STERM_INTERMEDIATE:
	case STERM_INTERMEDIATE_COND:
		return(SSTAT_OK);
	case STERM_CHECK_CONDITION:
		if (SCSI_RS_ERR_CLASS(rsense) == RS_CLASS_EXTEND)
			/* Extended sense data returned */
			switch (SCSI_RS_ERR_CODE(rsense)) {
			case 0:
				return(SSTAT_CCHECK);	/* Current report */
			case 1:
				return(SSTAT_DCHECK);  /* Deferred report */
			}
		return(SSTAT_UCHECK);	/* Vendor Unique format */
	default:
		return(SSTAT_RESERVED);
	}
}

/*
 * scsi_cmd_dump()
 *	This function displays information about a SCSI 
 *	command to be executed or that has been executed.
 *
 *	'scmd' is the address of a generic SCSI command descriptor,
 *	'dlen' is the amount of data the command should transfer,
 *	'data' is the physical address of the data to transfer,
 *	'iov' is the physical address of the i/o vector mapping
 *		for this transfer,
 *	'slen' is the length of the sense data buffer passed,
 *	'sdata' is the physical address of the sense data buffer.
 *	
 *	It is intended primarily for debug, to be invoked 
 *	after the command it has been executed. This can only
 * 	be done if thes SCSI command description, request sense
 *	data, and iovector information is preserved until this
 *	time.  The dumped output appears as:
 *		SCSI command: xx xx xx xx xx xx, direction is <htod,dtoh,none>.
 *		xx data bytes @ 0xXXXXXXXXX, iovec @ 0xXXXXXXXX.
 *		xx request sense bytes @ 0xXXXXXXXX, contents: 
 *			x xx xx xx xx xx xx xx xx xx xx xx xx
 */
void
scsi_cmd_dump(scmd, dlen, data, iov, slen, sdata)
	scsicmd_t *scmd;
	u_long dlen;
	u_long  data, iov;
	u_char slen;
	u_long sdata;
{
	u_char *cp, i;
	
	if (!scmd || !scmd->clen) {
		CPRINTF("SCSI command NULL address or zero length.");
		return;
	}

	CPRINTF( "SCSI command:");

	for (i = scmd->clen, cp = &scmd->cmd[0]; i--; cp++)
		CPRINTF((*cp < 16)? " 0%x" : " %x", *cp);
	if (scmd->dir == SDIR_NONE) {
		CPRINTF( ", no data transferred.\n");
	} else {
		CPRINTF(", direction is %s.\n",
			(scmd->dir == SDIR_HTOD) ? "host-to-device" 
						 : "device_to_host");
		CPRINTF("%d data bytes @ 0x%x, iovec @ 0x%x.\n",
			dlen, data, iov);
	}

	if (sdata && slen) {
		CPRINTF("%d request sense bytes @ 0x%x, contents:\n\t",
		slen, sdata);
		for (cp = (u_char *)sdata; slen--; cp++) 
			CPRINTF( (*cp < 16)? " 0%x" : " %x", *cp);
		CPRINTF(".\n");
	} else {
		CPRINTF("%d request sense bytes @ 0x%x.\n", slen,sdata);
	}
}

/*
 * scsi_rsense_string()
 *	This function generates an English language
 *	summary summarizing a normal SCSI check condition
 *	into the string buffer 'str'.  The check condition 
 *	is assumed to have occurred while a SCSI adapter was 
 *	executing the command described by 'scmd', which must
 *	be preserved to this point by the driver along with its 
 *	associated request sense data.  The sense data is 
 *	described by the 'rsense' and must be in "normal extended 
 *	sense data format", in which case the termination status 
 *	from the SCSI adapter executing it is SSTAT_CCHECK for
 *	current reporting or SSTAT_DCHECK for deferred reporting.
 *	
 *	The argument 'n' is the size of the buffer 'str'.
 *	The return value is zero if the request sense data
 *	is not in normal extended format.  Otherwise it is
 *	the smaller of 'n' and the length of the resulting 
 *	message copied into 'str', including a null string 
 *	termination byte. The message may be truncated if 
 *	'n' is not sufficiently large, but will still be null 
 *	terminated.
 * 
 *	The resulting message will be of the form:
 *	
 *	 	<sensekey> on command <scsi_command>; FILEMARK; EOM; ILI;
 *			Info bytes=0xXXXXXXXX; Additional length is xx bytes.
 *
 *	where "sensekey" is a string from the array "scsi_errors",
 *	"scsi_command" is a string from the array "scsi_commands", 
 *	the value of info bytes is only printed if the valid bit is
 *	set, and the EOF, EOM, and ILI portions are only present if
 *	the corresponding bits for filemark, end of media, and invalid
 *	length indicator are set in the request sense data.
 */
unsigned
scsi_rsense_string(scmd, str, n, rsense)
	scsicmd_t *scmd;
	char *str;
	int n;
	struct scrsense *rsense;
{
	char *kp, *cp;
	u_char addlen;
	unsigned buf_len = 0;
	char buf[160], temp[8];
	char *bp = &buf[0];
	char *tp = &temp[0];
	u_long info;
	
	/*
	 * If a problem is detected return an error.
	 */
#ifdef DEBUG
	ASSERT(scmd != NULL, "scsi_resense_string: NULL scsi command argument");
	ASSERT(str != NULL, "scsi_resense_string: NULL string buffer argument");
	ASSERT(n > 0, "scsi_resense_string: zero length string buffer");
	ASSERT(SCSI_RS_ERR_CLASS(rsense) == RS_CLASS_EXTEND &&
	       SCSI_RS_ERR_CODE(rsense) <= 1,
	       "scsi_resense_string: sense data not in normal extended format");
	ASSERT(scmd->cmd[0] < num_scsi_commands,
	       "scsi_resense_string: unknown SCSI command value");
	ASSERT(SCSI_RS_SENSE_KEY(rsense) < num_scsi_errors,
	       "scsi_resense_string: unknown SCSI sense key");
#else /* DEBUG */
	if (!scmd || !str || !n || SCSI_RS_ERR_CLASS(rsense) != RS_CLASS_EXTEND
	||  SCSI_RS_ERR_CODE(rsense) > 1 || scmd->cmd[0] >= num_scsi_commands
	||  SCSI_RS_SENSE_KEY(rsense) >= num_scsi_errors)
		return(0);
#endif /* DEBUG */

	/*
	 * Build the message in a local buffer.
	 */
	for (kp=scsi_errors[SCSI_RS_SENSE_KEY(rsense)]; *kp != '\0'; buf_len++)
		*bp++ = *kp++;
	bcopy(" on command ", bp, 12);
	bp += 12;
	for (cp = scsi_commands[scmd->cmd[0]]; *cp != '\0'; buf_len++)
		*bp++ = *cp++;
	*bp++ = ';';
	buf_len += 13;
	if (SCSI_RS_FILEMARK_SET(rsense)) {
		bcopy(" FILEMARK;", bp, 10);
		buf_len += 10;
		bp += 10;
	} 
	if (SCSI_RS_EOM_SET(rsense)) {
		bcopy(" EOM;", bp, 5);
		buf_len += 5;
		bp += 5;
	}
	if (SCSI_RS_ILI_SET(rsense)) {
		bcopy(" ILI;",bp, 5);
		buf_len += 5;
		bp += 5;
	}
	if (scsi_rs_info_bytes(rsense, &info)) {
		bcopy("\n\tInfo bytes=0x", bp, 15);
		bp += 15;
		buf_len += 15;
		do
			*tp++ = "0123456789abcdef"[info & 0x0f];
		while (info >>= 4);
		for (; tp != &temp[0]; buf_len++)
			*bp++ = *--tp;
	} else {
		bcopy("\n\tInfo bytes are not valid", bp, 26);
		bp += 26;
		buf_len += 26;
	}
	addlen = rsense->rs_addlen;
	bcopy("; Additional length is ", bp, 23);
	bp += 23;
	do
		*tp++ = "0123456789"[addlen % 10];
	while (addlen /= 10);
	for (; tp != &temp[0]; buf_len++)
		*bp++ = *--tp;
	bcopy(" bytes.\n", bp, 8);
	bp += 8;
	buf_len += 31;

	/*
	 * Truncate the message, if necessary.
	 * Copy the message to the caller's buffer
	 * and terminate it with a null character.
	 */
	if (--n < buf_len)
		buf_len = n;
	bcopy(buf, str, buf_len);
	str[buf_len++] = '\0';
	return(buf_len);
}

/*
 * scsi_rs_info_bytes()
 *	This function extracts from SCSI request sense data
 *	the value of its information bytes, if they are valid.
 *	It assumes that the request sense data is in the 
 *	extended format.
 *	
 *	If the 'valid' bit is not set in the request sense data
 *	the function will return zero and not change the value
 *	addressed by 'value'.  Otherwise it will set it to the
 *	value contained by the info-bytes and return one.
 */
int
scsi_rs_info_bytes(rsense, value)
	struct scrsense *rsense;
	u_long *value;
{
	long temp;
#ifdef DEBUG
	ASSERT(rsense, "scsi_rs_info_bytes: NULL sense data address argument");
	ASSERT(value, "scsi_rs_info_bytes: NULL value address argument");
#else /* DEBUG */
	if (!rsense || !value)
		return(0);
#endif /* DEBUG */

	if (SCSI_RS_INFO_VALID(rsense)) {
		temp = (long)rsense->rs_info[0] << 24;
		temp |= (long)rsense->rs_info[1] << 16;
		temp |= (long)rsense->rs_info[2] << 8;
		temp |= (long)rsense->rs_info[3];
		*value = temp;
		return(1);	/* Successful */
	} else
		return(0);	/* Failure, don't change value */
}

/* 
 * scsi_rs_lba()
 *	 This function extracts from SCSI request sense data
 *	 the value of its logical block address, if it is valid.
 *	 It assumes that the request sense data is in nonextended 
 *	 format.  It is provided to assist with vendor unique formats.
 *	
 *	 If the 'valid' bit is not set in the request sense data
 *	 the function will return zero and not change the value
 *	 addressed by 'value'.  Otherwise it will set it to the
 *	 value contained by the logical block address and return one.
 */
int
scsi_rs_lba(rsense, value)
	u_char *rsense;
	long *value;
{
	register u_char *lba = rsense + 1;
	long temp;

#ifdef DEBUG
	ASSERT(rsense, "scsi_rs_lba: NULL sense data address argument");
	ASSERT(value, "scsi_rs_lba: NULL value address argument");
#else /* DEBUG */
	if (!rsense || !value)
		return(0);
#endif /* DEBUG */

	if (SCSI_RS_INFO_VALID(rsense)) {
		temp = (long) (*lba++ & 0x1f) << 16;
		temp |= (long)*lba++ << 8;
		temp |= (long)*lba;
		*value = temp;
		return(1);	/* Successful */
	} else
		return(0);	/* Failure, don't change value */
}
