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

/* $Header: ssm_scsi.h 1.5 90/11/08 $ */

/*
 * ssm_scsi.h
 *      SCSI SSM definitions
 */

/* $Log:	ssm_scsi.h,v $
 */

/*
 * Each SSM SCSI device has a table of command blocks
 * (CBs) associated with it through which the host and
 * SSM communicate.  For each SCSI device, the host 
 * informs the SSM where a device's CBs are (via slic).
 * The host simultaneously communicates the SCSI address 
 * (target adapter and logical unit number) and the SCSI
 * interface number on that SSM through an init_cb located
 * at that address.  The SSM then returns an id number
 * for that device in the init_cb.  
 *
 * The id number returned by the SSM must is used to
 * generate appropriate interrupt vectors to the SSM
 * for that device in the future.
 */

/*
 * Limit definitions.
 */
#define	NCBPERSCSI	2		/* CBs per logical unit */
#define	NCBSCSISHFT	1		/* log2(NCBPERSCSI) */
#define NDEVSCSI	16		/* Max devices per SCSI bus */
#define SC_MAXSCSI	1		/* Max SCSI bus's per SSM */
#define SC_MAXID	(SC_MAXSCSI * NDEVSCSI - 1)
					/* Max SSM SCSI device id number */
#define SC_MAXLUN	7		/* Max Lun per target */

/*
 * Given a device's SSM id and CB number, compute its
 * SSM SLIC interrupt vector.
 */
#define	SCVEC(id,cb)	((id) << NCBSCSISHFT | (cb))

/*
 * Given a base vector for a device's CBs and a CB 
 * number, compute its host SLIC interrupt vector.
 */
#define	SC_HOSTVEC(basevec,cb)	((basevec) + (cb))

/*
 * Given an SSM SLIC interrupt vector, extract a device 
 * id number and CB number from it.
 */
#define	SCVEC_ID(vec)	((vec) >> NCBSCSISHFT & SC_MAXID)
#define	SCVEC_CB(vec)	((vec) & NCBPERSCSI - 1)

/*
 * SLIC interrupt bin for sending SCSI commands to SSM.
 */
#define	SCSI_BIN	0x06		/* SLIC bin for SCSI CBs */

/*
 * Special SSM SLIC interrupt vectors for communicating
 * the location of a device's CBs, the first of which
 * is also used as an initialization CB.
 *
 * SCSI_PROBEVEC is used during driver probe time so the
 * SSM can re-use its device id, etc., conserving memory.
 * SCSI_BOOTVEC is used once per device at boot time to
 * inform the SSM to generate a unique device id.
 */
#define	SCB_PROBEVEC	0xFE		/* Vector sent w/init CB during probe */
#define	SCB_BOOTVEC	0xFF		/* Vector sent w/init CB during boot */

/*
 * Misc. alignment and size definitions.
 */
#define	SCSI_CMDSIZ	12		/* 12 bytes per SCSI cmd max */
#define SCSI_CMDMIN	 6		/* 6 bytes min for SCSI cmd */
/*
 * SCSI bus device probe routine response flags.
 */
#define SCP_NOTFOUND		0x1	/* No good unit here */
#define SCP_NOTARGET		0x2	/* No good target here */
#define SCP_FOUND		0x4	/* Multi-unit target found */
#define SCP_ONELUN		0x8	/* Single-unit target found */

struct shpart {
	/*
	 * These variables are read by the FW for 
	 * every command.  They are not updated by 
	 * the FW and therefore do not need to be 
	 * copied back when the command completes.
	 */
	u_long	cb_sense;		/* SCSI request sense buffer address */
	u_char	cb_slen;		/* SCSI Request Sense buffer length */
	u_char	cb_cmd;			/* SSM command for this CB */
	u_char	cb_reserved0[1];	/* Reserved for future use; MBZ */
	u_char	cb_clen;		/* SCSI command length (6, 10, 12) */
	u_char	cb_scmd[SCSI_CMDSIZ];	/* SCSI command */
	u_char	cb_reserved1[4];	/* Reserved for future use; MBZ */
	/*
	 * These variables are initialized by SW, 
	 * used and updated by FW. After the FW is 
	 * instructed to execute the CB, the host 
	 * must not use that CB until cb_compcode 
	 * is non-zero.
	 */
	u_long	cb_addr;		/* Virtual addr if cb_iovec non-zero
					 * else physical address */
	u_long	*cb_iovec;		/* If non-zero, physical address of
					 * list of iovectors */
	u_long	cb_count;		/* Transfer count */
	u_char	cb_status;		/* SCSI status from cb_scmd */
	u_char	cb_reserved2[2];	/* Reserved for future use; MBZ */
	u_char	cb_compcode;		/* SSM completion code */
};

/*
 * Software only part of the SCSI CB. 
 * Not read by the firmware.
 */
struct softw {
	struct buf	*cb_bp;		/* Pointer to buf header */
	u_long	*cb_iovstart;		/* Start of cb_iovec buffer */
	u_short	cb_errcnt;		/* Number of retries */
	u_char	cb_unit_index;		/* SSM SLIC vector for this CB */
	u_char	cb_scsi_device;		/* This device's SCSI address */
	u_long	cb_state;		/* Current job state */
	/* 
	 * The following field must not be greater than 
	 * 8 bytes to preserve the desired cb size.  
	 * Driver dependent declarations may be inserted.
	 */
	union {
		u_char	cbs_reserved3[8]; /* Reserved for future use */
		struct {		/* For scsi disk driver */
			u_long	cbd_data;
			u_short cbd_iovnum;
			u_char  cbd_slic;
		} cbs_wd;
	} sw_un;
};

/* 
 * Abreviations for union entries.
 */
#define	cb_reserved3		sw_un.cbs_reserved3
#define cb_data			sw_un.cbs_wd.cbd_data
#define cb_iovnum		sw_un.cbs_wd.cbd_iovnum
#define cb_slic			sw_un.cbs_wd.cbd_slic

/*
 * If you change the structure of a SCSI CB make sure the constant
 * definitions below are correct.
 * The relative adrress of the SSM command field must be the same
 * in the structure scsi_cb as in the structure scsi_init_cb
 */
struct scsi_cb {
	struct shpart sh;		/* FW/SW shared part */
	struct softw sw;		/* Software only part */
};

/*
 *
 * Clearing of a SCSI CB:
 * 	bzero(SWBZERO(&SequentScsiCb), SWBZERO_SIZE);
 *
 * Firmware write to Sequent memory:
 *
 *	bcopy( FWWRITE(&SsmScsiCb), FWWRITE(&SequentScsiCb),
 *		FWWRITE_SIZE );
 *	ssmPic.flush();
 *
 * Since the write operation is a 16-byte multiple, 
 * the PIC flush should not have to wait.
 */

#define	SCB_SHSIZ	40		/* Size of shared portion */
#define	SCB_SWSIZ	24		/* Size of S/W-only portion */
#define SCB_PAGSIZMAX	65535		/* Max and Min icb_pagesize ... */
#define SCB_PAGSIZMIN	16		/* ...must be also be a power of 2 */

#define FWWRITE_SIZE	((u_int) &((struct shpart *) 0)->cb_compcode + \
	sizeof(u_char)) - (u_int) &((struct shpart *) 0)->cb_addr
#define FWREAD_ONLY	sizeof(struct shpart) - FWWRITE_SIZE
#define FWWRITE(p)	((char *) ((u_int) (p) + FWREAD_ONLY))
#define SWBZERO_SIZE	((u_int) &((struct shpart *) 0)->cb_compcode + \
	sizeof(u_char)) - (u_int) &((struct shpart *) 0)->cb_cmd
#define SWBZERO(p)	((char *) ((u_int) (p) + 0x05))

struct scsi_init_cb {
	/* Start of shared part */
	u_long	icb_pagesize;		/* Page size in bytes */
	u_char	icb_flags;		/* Flags describing the device */
	u_char	icb_cmd;		/* Command */
	u_char	icb_sdest;		/* SLIC dest for intrs */
	u_char	icb_svec;		/* SLIC vector for intrs */
	u_char	icb_scmd;		/* SLIC cmd for intrs */
	u_char	icb_scsi;		/* SCSI interface number on the SSM */
	u_char	icb_target;		/* Target adapter number on the SCSI */
	u_char	icb_lunit;		/* Logical unit number on the SCSI */
	u_char	icb_control;		/* Control byte for request-sense 
					 * commands generated by the SSM */
	u_char	icb_reserved1[3];	/* Reserved */
	long	icb_id;			/* i.d. number from SSM upon completion.
					 * Error occurred if less than zero. */ 
	u_long	icb_reserved2[4];	/* Reserved */
	u_char	icb_reserved3[3];	/* Reserved */
	u_char	icb_compcode;		/* SSM completion code */

	/* Start of sw-only part */
	u_long	icb_swreserved[SCB_SWSIZ/sizeof(u_long)];	/* Reserved */
};

/*
 * cb_flags 
 *
 * These flags define information passed from the device 
 * driver to the SSM firmware.  The flags will be information
 * the firmware may want to know about this device.
 */
#define SCBF_ASYNCH	0x01		/* Device only transfers data with */
					/* asynchronous SCSI protocol.	   */

/*
 * cb_cmd command codes
 *
 * Commands are composed of flag bits in the high nibble
 * and command bits in the low nibble.
 */
#define	SCB_IENABLE	0x80		/* Enable interrupts */
#define	SCB_CMD_MASK	0x0F		/* Command bits */
#define	SCB_INITDEV	0x00		/* Init SCSI device's state */
#define	SCB_NOP		0x01		/* Do nothing */
#define	SCB_READ	0x02		/* Do read from the SCSI bus */
#define	SCB_WRITE	0x03		/* Do write to SCSI bus */
#define	SCB_MAX_CMD	0x03		/* Max command number */

/* 
 * Extract SCSI command number.
 */
#define	SCB_CMD(cmd)	((cmd) & SCB_CMD_MASK)

/* 
 * SSM SCSI CB completion codes.
 */
#define	SCB_BUSY	0x00		/* No completion code yet */
#define	SCB_OK		0x01		/* Completed OK */
#define	SCB_BAD_CB	0x02		/* CB invalid */
#define	SCB_NO_TARGET	0x03		/* No target adapter response */
#define	SCB_SCSI_ERR	0x04		/* SCSI protocol err */

/* 
 * The following structure is used for returning 
 * initialization information about a SCSI device
 * from the functions init_ssm_scsi_dev() and
 * pinit_ssm_scsi_dev().
 */
struct scb_init {
	u_char	si_mode;		/* Initialization mode for device */
	u_char	si_ssm_slic;		/* SLIC id of the SSM board */
	u_char	si_scsi;		/* SCSI interface on the SSM board */
	u_char	si_target;		/* Target adapter number of device */
	u_char	si_lunit;		/* Logical unit number of device */
	u_char	si_host_bin;		/* SLIC bin to interrupt device at */
	u_char	si_host_basevec;	/* Base of sequential SLIC vectors for 
					 * device; one per scsi_cb. */
	u_char	si_control;		/* Control byte for request-sense 
					 * commands generated by the SSM */
	long	si_id;			/* I.D. number returned by SSM */
	struct	scsi_cb	*si_cbs;	/* Address of CBs for this device */
	u_char	si_flags;		/* Value for init cb icb_flags */
};

/* Values for icb->si_mode. */
#define SCB_PROBE	0		/* Currently probing devices; may
					 * reuse CBs and the device id */
#define SCB_BOOT	1		/* Booting the device; allocate
					 * unique device id and CBs */

/*
 * This record is filled in by the device driver.
 * and passed to the SCSI adapter code to execute 
 * a SCSI command.  
 * 
 * The address of the 'scmd' field should be passed to 
 * SCSI library functions, which will fill in its fields.
 */
struct scsi_adapter_cmd {
	scsicmd_t scmd;			/* Generic SCSI command information */
	u_long dlen;			/* #bytes of data the adapter xfers */
	u_long  data;			/* Physical address of data to xfer */
	u_long iov;			/* Physical address of indirect address
					 * table, if applicable */
	u_long sense;  		/* Physical address of request-sense
					 * buffer, for check conditions. */
	u_char slen;			/* Byte length of sense */
};

typedef struct scsi_adapter_cmd scac_t;

