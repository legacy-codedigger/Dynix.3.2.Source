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

/* $Header: sec.h 2.1 90/10/05 $
 *
 * This file contains the definitions of the interfaces
 * to the SCSI/Ether (SEC) firmware.
 *
 *
 * Below are the device numbers that the firmware expects for the 
 * unit channel id when a message such as STARTIO is sent. The SCSI
 * devices have a range from 0x20 (32) to 0x5f (95) with each target
 * adapter having a maximum of 8 logical units. 
 *
 *	adapter#	lun	sed_chan#
 *	0		0-7	0x20-0x27
 *	1		0-7	0x28-0x2f
 *	2		0-7	0x30-0x37
 *	3		0-7	0x38-0x3f
 *	4		0-7	0x40-0x47
 *	5		0-7	0x48-0x4f
 *	6		0-7	0x50-0x57
 *	host(7)		0-7	0x58-0x5f
 *
 * The host adapter number by default is adapter number 7 but should
 * there be more than one SEC on a single scsi bus then the adapter
 * number should be changed as the board won't work if there are more
 * than one adapter with the same bus number.
 */

/*
 * $Log:	sec.h,v $
 */


#define SDEV_SCSIBOARD		0	/* The SEC itself..... */
#define SDEV_ETHERREAD		1	/* ether input channel */
#define SDEV_ETHERWRITE		2	/* ether output channel */
#define SDEV_CONSOLE0IN		3	/* console port channel input */
#define SDEV_CONSOLE0OUT	4	/* console port channel output */
#define SDEV_CONSOLE1IN		5	/* diag port channel input */
#define SDEV_CONSOLE1OUT	6	/* diag port channel output */
#define SDEV_TOD		7	/* time of day clock channel */
#define SDEV_MEM		8	/* memory channel */
#define SDEV_WATCHDOG		9	/* watch dog timer channel */
#define SDEV_SCSISTART		0x20	/* scsi unit number start (32) */
#define SDEV_SCSIEND		0x5f	/* scsi unit number end (95) */
#define SDEV_NUM_DEVICES	96

/*
 * indirect address table
 */

#define SEC_IAT_FLAG		0x80000000	/* indirect bit to point to iat */
#define	SEC_IATIFY(addr)	((struct sec_iat *)(((int)addr) | SEC_IAT_FLAG))

struct sec_iat {
	u_char	*iat_data;		/* pointer to data */
	int	iat_count;		/* number of bytes to put there */
};

/*
 * device program for all devices except ether read and clocks
 */

#define SCSI_CMD_SIZE	16		/* Maximum number of bytes in a scsi command */

struct sec_dev_prog {
	u_char	dp_status1;		/* byte 1 of status */
	u_char	dp_status2;		/* byte 2 of status */
	short	dp_reserved;
	int	dp_count;		/* number of bytes transferred */
	union {
		u_char	*dp_data;	/* ptr to data */
		struct sec_iat *dp_iat;	/* ptr to indirect address table */
	} dp_un;
	struct sec_dev_prog *dp_next;	/* ptr to next dev program if linked */
	int dp_data_len;		/* total number of bytes to transfer */
	int dp_cmd_len;			/* real size of next field */
	u_char dp_cmd[SCSI_CMD_SIZE];	/* SCSI Device command */
};

/*
 * channel instruction block
 */
struct sec_cib {
	int cib_inst;			/* instruction */
	int *cib_status;		/* ptr to status or other structs */
};

/* Error Flags */
#define SEC_ERR_NONE		0
#define SEC_ERR_INVALID_INS	1
#define SEC_ERR_INVALID_DEV	2
#define SEC_ERR_NO_MORE_IO	3
#define SEC_ERR_NO_SENSE	4
#define SEC_ERR_COUNT_TOO_BIG	128
#define SEC_ERR_BAD_MODE	129

/*
 * Instructions SCSI/Ether controller.
 */
#define SINST_INSDONE		0x80000000	/* instruction complete bit */
#define SINST_INIT		0		/* Initialize instruction */
#define SINST_SETMODE		1		/* set modes instruction */
#define SINST_STARTIO		2		/* start io instruction */
#define SINST_GETMODE		3		/* get modes instruction */
#define SINST_FLUSHQUEUE	4		/* flush queue instruction */
#define SINST_RESTARTCURRENTIO	5		/* restart current inst */
#define SINST_RESTARTIO		6		/* restart instruction */
#define SINST_REQUESTSENSE	7		/* request sense inst */
#define SINST_STOPIO		8		/* stop io instruction */
#define SINST_RETTODIAG		9		/* return to diagnostics inst */

#define SCSI_ETHER_WRITE	0xA		/* SCSI first byte for write */
#define SCSI_ETHER_STATION	0x0		/* SCSI 2nd byte for station */
#define	SCSI_ETHER_MULTICAST	0x1		/* SCSI 2nd byte for multicast */

/*
 * device program queue
 */

#define SEC_POWERUP_QUEUE_SIZE	3	/* Queue size at power-up (cib, doq... etc) */

struct sec_progq {
	u_int pq_head;			/* head of list */
	u_int pq_tail;			/* tail of list */
	union {
		struct sec_dev_prog *pq_progs[SEC_POWERUP_QUEUE_SIZE];
		struct sec_edev_prog {
			int	edp_iat_count;		/* number of iat entries */
			struct	sec_iat *edp_iat;	/* pointer to list of entries */
		} *pq_eprogs[SEC_POWERUP_QUEUE_SIZE];
	} pq_un;
};

/*
 * ether read output queue
 */
struct sec_eprogq {
	u_int	epq_head;	/* head of list */
	u_int	epq_tail;	/* tail of list */
	struct sec_ether_status {
		u_char	es_status1;	/* byte 1 of status */
		u_char	es_status2;	/* byte 2 of status */
		short	es_reserved;
		int 	es_count;	/* number of bytes received */
		u_char	*es_data;	/* pointer to the first byte received */
	} epq_status[SEC_POWERUP_QUEUE_SIZE]; /* ether status blocks */
};

/*
 * SCSI Queues at power-up.
 * The address of this is passed by power-up code to the kernel
 * for auto-config of the SCSI/Ether controller.
 */
struct sec_powerup {
	struct sec_cib	pu_cib;			/* for all devices */
	struct sec_progq	pu_requestq;	/* for all devices but ether read */
	struct sec_progq	pu_doneq;	/* for all devices but ether read */
	struct sec_progq	pu_erequestq;	/* for ether read */
	struct sec_eprogq	pu_edoneq;	/* for ether read */
};

/*
 * init channel instruction data structure (ptr to it is passed in cib status
 * pointer)
 */
struct sec_init_chan_data {
	int	sic_status;		/* status of INIT instruction */
	struct	sec_cib	*sic_cib;	/* pointer to 96 cib's (1/device) */
	struct sec_chan_descr {
		struct sec_progq *scd_requestq;	/* pointer to input queue */
		struct sec_progq *scd_doneq;	/* pointer to output queue */
		u_char	scd_bin;		/* bin to interrupt Unix on */
		u_char	scd_vector;		/* interrupt vector to return */
	} sic_chans[96];		/* channel descriptors (1/device) */
};

/*
 * structure for set modes command.  Ptr to this goes in channel status ptr
 */
struct sec_smode {
	int	sm_status;			/* pointer to status from this cmd */
	union {
		struct sec_cons_modes {
			short	cm_baud;	/* baud rate */
			short	cm_flags;	/* flags for stop bits, dtr etc */
		} sm_cons;
		struct sec_ether_smodes {
			u_char	esm_addr[6];	/* ether address */
			short	esm_size;	/* constant iat chunk size */
			short	esm_flags;	/* receive mode flag */
		} sm_ether;
		struct sec_scsi_smodes {
			short	ssm_timeout;	/* bus timeout */
			short	ssm_flags;	/* used Single ended or diff connect */
		} sm_scsi;
		struct sec_tod_modes {
			int	tod_freq;	/* interrupt frequency */
			int	tod_newtime;	/* new time for TOD clock */
		} sm_tod;
		int	sm_wdt_mode;		/* wd timer mode */
 		struct sec_board_modes {
 			struct sec_powerup *sec_powerup;
 			short	sec_dopoll;
			short	sec_errlight;		/* error light */
 			struct reboot	*sec_reboot;	/* see cfg.h */
 		} sm_board;
		struct sec_mem {
			char *mm_buffer;	/* address of log buffer */
			char *mm_nextchar;	/* next free char in buffer */
			short mm_size;		/* buffer size */
			short mm_nchar;		/* number valid chars in buf */
		} sm_mem;
	} sm_un;
};

/* Console Flags */
#define SCONS_STOP1		0
#define SCONS_STOP1P5		0x1
#define SCONS_STOP2		0x2
#define SCONS_DATA8		0
#define SCONS_DATA7		0x4
#define SCONS_EVEN_PARITY	0x8
#define SCONS_ODD_PARITY	0x10
#define SCONS_SEND_CARRIER	0
#define SCONS_IGN_CARRIER	0x20
#define SCONS_SET_DTR		0
#define SCONS_CLEAR_DTR		0x40
#define SCONS_SEND_BREAK	0
#define SCONS_DIAG_BREAK	0x80
#define SCONS_IGNORE_BREAK	0x100
#define SCONS_DHMODE		0
#define SCONS_DZMODE		0x200
#define SCONS_CARRIER_SET	0
#define SCONS_CARRIER_CLEAR	0x400
#define SCONS_SET_RTS		0
#define SCONS_CLEAR_RTS		0x800
#define SCONS_CLEAR_BREAK	0
#define SCONS_SET_BREAK		0x1000

/* Console info error bits */
#define SCONS_TIMEOUT		0x1
#define SCONS_BREAK_DET		0x2
#define SCONS_CARR_DET		0x4
#define SCONS_OVRFLOW		0x8
#define SCONS_FLUSHED		0x10
#define SCONS_PARITY_ERR	0x20

/* Ether mode flags */
#define SETHER_DISABLE		0
#define SETHER_PROMISCUOUS	1
#define SETHER_S_AND_B		2
#define SETHER_MULTICAST	3
#define SETHER_LOOPBACK		4

/* Front panel error light */
#define	SERR_LIGHT_ON		 1
#define	SERR_LIGHT_SAME		 0
#define SERR_LIGHT_OFF		-1

/* value of the cib_status pointer upon SINST_RETTODIAG command */
#define SRD_BREAK	(int *)0	/* returning from BREAK, halts */
#define SRD_POWERUP	(int *)1	/* use powerup defaults */
#define SRD_REBOOT	(int *)2	/* use setmode data */

/*
 * structure for get modes command.  Ptr to this goes in channel status ptr
 */
struct sec_gmode {
	int	gm_status;		/* status from this instruction */
	union {
		struct sec_cons_modes gm_cons;		/* console data */
		struct sec_ether_gmodes {
			struct sec_ether_smodes egm_sm;	/* same as set modes */
			int egm_rx_ovfl;	/* number of dma overflows */
			int egm_rx_crc;		/* number of crc errors */
			int egm_rx_dribbles;	/* number of dribbles */
			int egm_rx_short;	/* number of short packets */
			int egm_rx_good;	/* number of good packets */
			int egm_tx_unfl;	/* number of dma underflows */
			int egm_tx_coll;	/* number of collisions */
			int egm_tx_16x_coll;	/* number of 16x collisions */
			int egm_tx_good;	/* number of good packets sent */
		} gm_ether;
		struct sec_wdt_gmodes {
			int gwdt_time;		/* time between expirations */
			int gwdt_expired;	/* number of times light has expired */
		} gm_wdt;
		struct sec_tod_modes	gm_tod;	/* time of day data */
		struct sec_scsi_gmodes {
			struct	sec_scsi_smodes sgm_sm;	/* same as scsi set mode */
			int	sgm_bus_parity;	/* number of bus parity errors seen */
		} gm_scsi;
 		struct sec_board_modes	gm_board; /* same as board set mode */
		struct sec_mem gm_mem;
	} gm_un;
};

/*
 * request sense structure.  ptr goes in channel status ptr.
 */
struct sec_req_sense {
	int	rs_status;
	struct sec_dev_prog rs_dev_prog;	/* device program to run for sense command */
};

/* SCSI Sense Info */
#define SSENSE_NOSENSE		0
#define SSENSE_RECOVERABLE	1
#define SSENSE_NOT_READY	2
#define SSENSE_MEDIA_ERR	3
#define SSENSE_HARD_ERR		4
#define SSENSE_ILL_REQ		5
#define SSENSE_UNIT_ATN		6
#define SSENSE_DATA_PROT	7
#define SSENSE_BLANK		8
#define SSENSE_EQUAL		9
#define SSENSE_COPY_ABORT	0xA
#define SSENSE_ABORT		0xB
#define SSENSE_VOL_OVER		0xD
#define SSENSE_CHECK		2
#define SSENSE_INTSTAT		16
#define SSENSE_MOREINFO		0x80
#define SSENSE_HOST_ERR		1
#define SSENSE_NOTREADY		1

/* SCED SCSI_DEVNO macro */
#define SEC_SCSI_DEVNO(x)       (SDEV_SCSISTART + SCSI_DEVNO((x)))

/*
 * Local sec.c defines.
 */
#define SEC_HARDERR	4	/* acts like a hard error sense key */

